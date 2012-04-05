/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

/* mex-search-plugin.c */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include "mex-search-plugin.h"
#include <mex/mex-view-model.h>
#include <mex/mex-grilo-feed.h>
#include <mex/mex-grilo-tracker-feed.h>
#include <rest/rest-xml-parser.h>

static void mex_tool_provider_iface_init (MexToolProviderInterface *iface);
static void mex_model_provider_iface_init (MexModelProviderInterface *iface);
static void mex_action_provider_iface_init (MexActionProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexSearchPlugin, mex_search_plugin, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_TOOL_PROVIDER,
                                                mex_tool_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                mex_model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                mex_action_provider_iface_init))

#define SEARCH_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SEARCH_PLUGIN, MexSearchPluginPrivate))

struct _MexSearchPluginPrivate
{
  GList        *models;

  MexFeed      *history_model;
  MexFeed      *suggest_model;

  GList        *actions;
  MexActionInfo action_info;

  ClutterActor *search_page;
  ClutterActor *search_entry;
  ClutterActor *search_shell;
  ClutterActor *suggest_column;
  ClutterActor *spinner;

  guint         suggest_timeout;
  gpointer      suggest_id;

  MexProxy     *search_proxy;
  MexModel     *search_model;
};

static void mex_search_plugin_search_cb (MexSearchPlugin *self);

static void
mex_search_plugin_dispose (GObject *object)
{
  MexSearchPlugin *self = MEX_SEARCH_PLUGIN (object);
  MexSearchPluginPrivate *priv = self->priv;

  mex_model_manager_remove_category (mex_model_manager_get_default (),
                                     "search");

  if (priv->history_model)
    {
      g_object_unref (priv->history_model);
      priv->history_model = NULL;
    }

  if (priv->suggest_model)
    {
      g_object_unref (priv->suggest_model);
      priv->suggest_model = NULL;
    }

  if (priv->suggest_timeout)
    {
      g_source_remove (priv->suggest_timeout);
      priv->suggest_timeout = 0;
    }

  if (priv->suggest_id)
    {
      MexDownloadQueue *dq = mex_download_queue_get_default ();
      mex_download_queue_cancel (dq, priv->suggest_id);
      priv->suggest_id = NULL;
    }

  if (priv->search_proxy)
    {
      g_object_unref (priv->search_proxy);
      priv->search_proxy = NULL;
    }

  if (priv->search_page)
    {
      g_object_unref (priv->search_page);
      priv->search_page = NULL;
    }

  if (priv->action_info.action)
    {
      g_object_unref (priv->action_info.action);
      priv->action_info.action = NULL;
    }

  G_OBJECT_CLASS (mex_search_plugin_parent_class)->dispose (object);
}

static void
mex_search_plugin_finalize (GObject *object)
{
  MexSearchPluginPrivate *priv = MEX_SEARCH_PLUGIN (object)->priv;

  g_list_free (priv->models);
  g_list_free (priv->actions);

  G_OBJECT_CLASS (mex_search_plugin_parent_class)->finalize (object);
}

static void
mex_search_plugin_class_init (MexSearchPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexSearchPluginPrivate));

  object_class->dispose = mex_search_plugin_dispose;
  object_class->finalize = mex_search_plugin_finalize;
}

static void
mex_search_plugin_mimetype_set_cb (MexContent *content)
{
  const gchar *mime = mex_content_get_metadata (content,
                                                MEX_CONTENT_METADATA_MIMETYPE);
  if (!mime || !(*mime) || g_str_equal (mime, "application/x-shockwave-flash"))
    mex_content_set_metadata (content, MEX_CONTENT_METADATA_MIMETYPE,
                              "x-mex/media");
}

static void
mex_search_plugin_model_changed_cb (GController          *controller,
                                    GControllerAction     action,
                                    GControllerReference *ref,
                                    MexModel             *model)
{
  gint i;
  gint n_indices = g_controller_reference_get_n_indices (ref);

  /* If the MIME type is unset, set one so that it's still playable in the
   * main UI. This plugin will only use white-listed Grilo plugins that we
   * know return playable media, so this is OK.
   */
  switch (action)
    {
    case G_CONTROLLER_ADD:
      for (i = 0; i < n_indices; i++)
        {
          gchar *notify;
          MexContent *content;
          const gchar *property_name;

          gint content_index = g_controller_reference_get_index_uint (ref, i);
          content = mex_model_get_content (model, content_index);

          /* Make sure the mime-type remains useful */
          property_name =
            mex_content_get_property_name (content,
                                           MEX_CONTENT_METADATA_MIMETYPE);
          if (property_name)
            {
              notify = g_strconcat ("notify::", property_name, NULL);
              g_signal_connect (content, notify,
                                G_CALLBACK (mex_search_plugin_mimetype_set_cb),
                                NULL);
              g_free (notify);
            }

          mex_search_plugin_mimetype_set_cb (content);
        }
      break;

    default:
      break;
    }
}

static void
mex_search_plugin_history_cb (MxAction        *action,
                              MexSearchPlugin *self)
{
  MexSearchPluginPrivate *priv = self->priv;
  MexContent *content = mex_action_get_content (action);
  const gchar *text = mex_content_get_metadata (content,
                                                MEX_CONTENT_METADATA_TITLE);

  mx_entry_set_text (MX_ENTRY (priv->search_entry), text);
  mex_search_plugin_search_cb (self);
}

static void
mex_search_plugin_update_history (MexSearchPlugin *self,
                                  const gchar     *term)
{
  gint i;
  gsize length;
  gchar *contents, *current;

  MexSearchPluginPrivate *priv = self->priv;
  const gchar *base_dir =
    mex_settings_get_config_dir (mex_settings_get_default ());
  gchar *history_file = g_build_filename (base_dir, "search", "history", NULL);

  /* Read the history file contents */
  /* TODO: Make this less rubbish? */
  g_file_get_contents (history_file, &contents, &length, NULL);

  /* Prepend new search-term if appropriate */
  if (term)
    {
      gint terms;
      gchar *path;
      gsize new_length;

      gsize term_len = strlen (term);
      gchar *new_contents = g_malloc (length + term_len + 1);

      memcpy (new_contents, term, term_len);
      new_contents[term_len] = '\n';
      new_length = term_len + 1;

      /* Truncate list to 10 terms and remove duplicates */
      if (contents)
        {
          i = 0;
          terms = 1;
          do
            {
              gint cur_len;
              char *eos = strchr (contents + i, '\n');

              if (!eos)
                cur_len = strlen (contents + i);
              else
                cur_len = eos - (contents + i);

              if ((cur_len != term_len) ||
                  (strncmp (contents + i, term, term_len) != 0))
                {
                  memcpy (new_contents + new_length, contents + i, cur_len + 1);
                  new_length += cur_len + 1;

                  if (++terms >= 10)
                    break;
                }

              if (!eos)
                break;

              i += cur_len + 1;
            } while (i < length);
        }

      new_contents[new_length++] = '\0';

      /* Save new list */
      path = g_path_get_dirname (history_file);
      g_mkdir_with_parents (path, 0755);
      g_free (path);

      g_file_set_contents (history_file, new_contents, new_length, NULL);

      /* Replace content with new content */
      g_free (contents);
      contents = new_contents;
      length = new_length;
    }

  /* Empty current list */
  mex_model_clear (MEX_MODEL (priv->history_model));

  /* Populate with search history */
  if (contents)
    {
      current = contents;
      while (current < contents + length)
        {
          MexContent *content =
            MEX_CONTENT (mex_program_new (priv->history_model));
          gchar *end = g_utf8_strchr (current, -1, '\n');

          if (end)
            *end = '\0';

          if (*current)
            {
              mex_content_set_metadata (content,
                                        MEX_CONTENT_METADATA_TITLE,
                                        current);
              mex_content_set_metadata (content,
                                        MEX_CONTENT_METADATA_MIMETYPE,
                                        "x-mex/search");
              mex_model_add_content (MEX_MODEL (priv->history_model),
                                     content);
            }

          if (end)
            current = end + 1;
          else
            break;
        }

      g_free (contents);
    }
}

static void
mex_search_plugin_search (MexSearchPlugin *self,
                          const gchar     *search)
{
  GList *l, *list;
  MexSearchPluginPrivate *priv = self->priv;
  MexModelManager *manager = mex_model_manager_get_default ();
  gboolean have_tracker = FALSE;

  if (!priv->search_model)
    {
      /* Create search model */
      priv->search_model = mex_aggregate_model_new ();
      g_object_set (G_OBJECT (priv->search_model),
                    "title", _("Search results"), NULL);
    }

  /* Kill the last search */
  list = (GList *) mex_aggregate_model_get_models (
                       MEX_AGGREGATE_MODEL (priv->search_model));
  for (l = list; l; l = l->next)
    mex_model_manager_remove_model (manager, l->data);
  mex_aggregate_model_clear (MEX_AGGREGATE_MODEL (priv->search_model));

  /* Iterate over searchable Grilo sources */
  list = grl_plugin_registry_get_sources (grl_plugin_registry_get_default (),
                                             FALSE);

  /* find the local files source and place it first */
  for (l = list; l; l = l->next)
    {
      GrlMetadataSource *meta_src = l->data;
      const gchar *name = grl_metadata_source_get_name (meta_src);
      const gchar *source_id;

      if (!GRL_IS_METADATA_SOURCE (meta_src))
        continue;

      source_id = grl_media_plugin_get_id (GRL_MEDIA_PLUGIN (meta_src));

      if (source_id && g_str_equal (source_id, "grl-tracker"))
        have_tracker = TRUE;

      if (name && !strcmp (name, "Local files"))
        {
          list = g_list_remove_link (list, l);
          list = g_list_concat (list, l);
          break;
        }
    }

  /* prefer tracker over the filesystem plugin by removing it from the list if
   * tracker is available */
  if (have_tracker)
    {
      for (l = list; l; l = l->next)
        {
          GrlMetadataSource *meta_src = l->data;
          const gchar *source_id;

          if (!GRL_IS_METADATA_SOURCE (meta_src))
            continue;

          source_id = grl_media_plugin_get_id (GRL_MEDIA_PLUGIN (meta_src));

          if (source_id && g_str_equal (source_id, "grl-filesystem"))
            {
              list = g_list_delete_link (list, l);
              break;
            }
        }
    }


  for (l = list; l; l = l->next)
    {
      const gchar *source_id;
      GrlSupportedOps supported;
      GrlMetadataSource *meta_src = l->data;

      if (!GRL_IS_METADATA_SOURCE (meta_src))
        continue;

      /* only search upnp and tracker sources */
      source_id = grl_media_plugin_get_id (GRL_MEDIA_PLUGIN (meta_src));

      supported = grl_metadata_source_supported_operations (meta_src);
      if ((supported & GRL_OP_SEARCH) || (supported & GRL_OP_QUERY))
        {
          MexFeed *feed;
          GController *controller;

          if (g_str_equal (source_id, "grl-tracker"))
            feed = mex_grilo_tracker_feed_new (GRL_MEDIA_SOURCE (meta_src),
                                               NULL, NULL, NULL, NULL);
          else
            feed =
              mex_grilo_feed_new (GRL_MEDIA_SOURCE (meta_src), NULL, NULL, NULL);
          mex_model_set_sort_func (MEX_MODEL (feed),
                                   mex_model_sort_time_cb,
                                   GINT_TO_POINTER (TRUE));

          g_object_set (G_OBJECT (feed),
                        "category", "search-results",
                        "placeholder-text", _("No videos found"),
                        NULL);
          mex_model_manager_add_model (manager, MEX_MODEL (feed));

          controller = mex_model_get_controller (MEX_MODEL (feed));

          /* Attach to the changed signal so that we can alter the
           * mime-type of content if necessary.
           */
          g_signal_connect (controller, "changed",
                            G_CALLBACK (mex_search_plugin_model_changed_cb),
                            feed);

          mex_aggregate_model_add_model (
            MEX_AGGREGATE_MODEL (priv->search_model), MEX_MODEL (feed));

          /* FIXME: Arbitrary 50 item limit... */
          mex_grilo_feed_search (MEX_GRILO_FEED (feed), search, 0, 50);

          g_object_unref (G_OBJECT (feed));
        }
    }
  g_list_free (list);
}

static void
mex_search_plugin_search_cb (MexSearchPlugin *self)
{
  MexSearchPluginPrivate *priv = self->priv;

  const gchar *search = mx_entry_get_text (MX_ENTRY (priv->search_entry));

  /* don't run a search if the search entry was empty */
  if (!search || search[0] == '\0')
    return;

  /* Stop the suggestions search */
  if (priv->suggest_timeout)
    {
      g_source_remove (priv->suggest_timeout);
      priv->suggest_timeout = 0;
    }

  /* Start a new search */
  mex_search_plugin_search (self, search);

  /* Update the history list */
  mex_search_plugin_update_history (self, search);

  /* Present the search model */
  mex_model_provider_present_model (MEX_MODEL_PROVIDER (self),
                                    priv->search_model);

  /* Hide the search page, if it was visible */
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->search_page))
    mex_tool_provider_remove_actor (MEX_TOOL_PROVIDER (self),
                                    priv->search_page);

  /* TODO: Maybe use weak references to stop the search if the application
   *       didn't use it?
   */
}

static void
mex_search_activate_content (MexContent *content)
{
  MxAction *action;

  MexActionManager *manager = mex_action_manager_get_default ();
  GList *actions = mex_action_manager_get_actions_for_content (manager,
                                                               content);

  if (!actions)
    return;

  /* Just execute the highest-priority action for this item. This is
   * always the 'play'/'execute' action.
   */
  action = actions->data;
  mex_action_set_content (action, content);
  g_signal_emit_by_name (action, "activated");
}

static void
mex_suggest_complete_cb (MexDownloadQueue *queue,
                         const gchar      *uri,
                         const gchar      *buffer,
                         gsize             count,
                         const GError     *error,
                         gpointer          userdata)
{
  RestXmlNode *root, *n;
  RestXmlParser *parser;
  MexSearchPlugin *self = userdata;
  MexSearchPluginPrivate *priv = self->priv;

  priv->suggest_id = NULL;

  /* hide spinner */
  mx_spinner_set_animating (MX_SPINNER (priv->spinner), FALSE);
  clutter_actor_hide (priv->spinner);

  if (error)
    {
      g_warning ("Error querying Google suggestions: %s",
                 error->message);
      return;
    }

  parser = rest_xml_parser_new ();
  root = rest_xml_parser_parse_from_data (parser, buffer, count);

  if (!root)
    {
      g_warning ("Unknown error parsing Google suggestions XML");
      g_object_unref (parser);
      return;
    }

  /* Clear model */
  mex_model_clear (MEX_MODEL (priv->suggest_model));

  /* Add new suggestions to model */
  n = rest_xml_node_find (root, "CompleteSuggestion");
  for (; n; n = n->next)
    {
      MexContent *content;
      const gchar *suggestion;

      RestXmlNode *node = rest_xml_node_find (n, "suggestion");

      if (!node)
        continue;

      suggestion = rest_xml_node_get_attr (node, "data");

      if (!suggestion)
        continue;

      content = MEX_CONTENT (mex_program_new (priv->suggest_model));
      mex_content_set_metadata (content, MEX_CONTENT_METADATA_TITLE,
                                suggestion);
      mex_content_set_metadata (content, MEX_CONTENT_METADATA_MIMETYPE,
                                "x-mex/search");
      mex_model_add_content (MEX_MODEL (priv->suggest_model), content);
    }

  /* Unref */
  rest_xml_node_unref (root);
  g_object_unref (parser);
}

static gboolean
mex_suggest_timeout_cb (MexSearchPlugin *self)
{
  gchar *uri;
  const gchar *text;
  MexDownloadQueue *dq;

  MexSearchPluginPrivate *priv = self->priv;

  priv->suggest_timeout = 0;

  text = mx_entry_get_text (MX_ENTRY (priv->search_entry));

  dq = mex_download_queue_get_default ();

  if (priv->suggest_id)
    mex_download_queue_cancel (dq, priv->suggest_id);

  uri =
    g_strdup_printf ("http://google.com/complete/search?output=toolbar&q=%s",
                     text);
  priv->suggest_id =
    mex_download_queue_enqueue (dq, uri, mex_suggest_complete_cb, self);

  return FALSE;
}

static void
mex_search_text_changed_cb (MxEntry         *entry,
                            GParamSpec      *pspec,
                            MexSearchPlugin *self)
{
  MexSearchPluginPrivate *priv = self->priv;

  if (priv->suggest_timeout)
    {
      g_source_remove (priv->suggest_timeout);
      priv->suggest_timeout = 0;
    }

  /* Don't start suggestions unless we have at least 3 characters */
  if (g_utf8_strlen (mx_entry_get_text (entry), -1) < 3)
    {
      /* ensure the spinner is not visible */
      mx_spinner_set_animating (MX_SPINNER (priv->spinner), FALSE);
      clutter_actor_hide (priv->spinner);

      return;
    }

  /* show spinner */
  mx_spinner_set_animating (MX_SPINNER (priv->spinner), TRUE);
  clutter_actor_show (priv->spinner);

  priv->suggest_timeout =
    g_timeout_add_seconds (1, (GSourceFunc)mex_suggest_timeout_cb, self);
}

static void
mex_tool_provider_iface_init (MexToolProviderInterface *iface)
{
}

static const GList *
mex_search_plugin_get_models (MexModelProvider *self)
{
  MexSearchPluginPrivate *priv = MEX_SEARCH_PLUGIN (self)->priv;
  return priv->models;
}

static gboolean
mex_search_plugin_model_activated (MexModelProvider *self,
                                   MexModel         *model)
{
  MexSearchPluginPrivate *priv = MEX_SEARCH_PLUGIN (self)->priv;

  mex_tool_provider_present_actor (MEX_TOOL_PROVIDER (self),
                                   g_object_ref (priv->search_page));

  return TRUE;
}

static void
mex_model_provider_iface_init (MexModelProviderInterface *iface)
{
  iface->get_models = mex_search_plugin_get_models;
  iface->model_activated = mex_search_plugin_model_activated;
}

static const GList *
mex_search_plugin_get_actions (MexActionProvider *self)
{
  MexSearchPluginPrivate *priv = MEX_SEARCH_PLUGIN (self)->priv;
  return priv->actions;
}

static void
mex_action_provider_iface_init (MexActionProviderInterface *iface)
{
  iface->get_actions = mex_search_plugin_get_actions;
}

static const gchar *search_mimetypes[] = { "x-mex/search", NULL };

static void
mex_search_proxy_add_cb (MexProxy         *proxy,
                         MexContent       *content,
                         ClutterActor     *button,
                         ClutterContainer *layout)
{
  clutter_container_add_actor (layout, button);
  mx_bin_set_alignment (MX_BIN (button), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (mex_search_activate_content), content);
}

static void
mex_search_proxy_remove_cb (MexProxy         *proxy,
                            MexContent       *content,
                            ClutterActor     *label,
                            ClutterContainer *layout)
{
  clutter_container_remove_actor (layout, label);
}

static void
mex_search_text_style_changed (MxStylable *text,
                               GParamSpec *pspec,
                               MxStylable *header)
{
  /* ensure the text entry and header pseudo class match for styling purposes */
  mx_stylable_set_style_pseudo_class (header,
                                      mx_stylable_get_style_pseudo_class (text));
}

static void
mex_search_plugin_init (MexSearchPlugin *self)
{
  MexModel *view_model;
  MexProxy *suggest_proxy;
  ClutterActor *icon, *header, *text, *frame, *box, *hbox;
  MexSearchPluginPrivate *priv = self->priv = SEARCH_PLUGIN_PRIVATE (self);
  MexModelCategoryInfo search = { "search", _("Search"), "icon-panelheader-search", 0, "" };
  /* create a category for search results that is not shown on the home screen */
  MexModelCategoryInfo search_results = {
      "search-results", _("Search"), "icon-panelheader-search", -1, "" };
  MexModelManager *manager = mex_model_manager_get_default ();

  manager = mex_model_manager_get_default ();
  mex_model_manager_add_category (manager, &search);
  mex_model_manager_add_category (manager, &search_results);

  /* Create the history model and models list */
  priv->history_model = mex_feed_new (_("Search"), _("Search"));
  g_object_set (G_OBJECT (priv->history_model), "category", "search", NULL);

  priv->models = g_list_append (NULL, priv->history_model);

  /* Create the actions list */
  memset (&priv->action_info, 0, sizeof (MexActionInfo));
  priv->action_info.action =
    mx_action_new_full ("x-mex/search",
                        _("Search"),
                        G_CALLBACK (mex_search_plugin_history_cb),
                        self);
  priv->action_info.mime_types = (gchar **)search_mimetypes;
  priv->actions = g_list_append (NULL, &priv->action_info);

  /* Create the suggestions model */
  priv->suggest_model =
    mex_feed_new (_("Suggestions"), _("Google Suggestions"));

  /* Create the search page */

  /* Create header */
  icon = mx_icon_new ();
  mx_stylable_set_style_class (MX_STYLABLE (icon), "Search");

  header = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (header), 5);
  clutter_actor_set_name (header, "search-header");

  /* Create search entry */
  frame = mx_table_new ();
  clutter_actor_set_name (frame, "search-entry-frame");
  priv->search_entry = mx_entry_new ();
  priv->spinner = mx_spinner_new ();

  mx_table_insert_actor (MX_TABLE (frame), priv->search_entry, 0, 0);
  mx_table_insert_actor (MX_TABLE (frame), priv->spinner, 0, 1);
  mx_table_child_set_x_fill (MX_TABLE (frame), priv->spinner, FALSE);
  mx_table_child_set_x_expand (MX_TABLE (frame), priv->spinner, FALSE);
  mx_table_child_set_y_fill (MX_TABLE (frame), priv->spinner, FALSE);

  mx_spinner_set_animating (MX_SPINNER (priv->spinner), FALSE);
  clutter_actor_hide (priv->spinner);

  clutter_container_add (CLUTTER_CONTAINER (header), icon, frame, NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (header), icon,
                               "x-fill", FALSE, "y-fill", FALSE, NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (header), frame,
                               "expand", TRUE, "x-fill", TRUE, NULL);

  text = mx_entry_get_clutter_text (MX_ENTRY (priv->search_entry));
  g_signal_connect_swapped (text, "activate",
                            G_CALLBACK (mex_search_plugin_search_cb), self);
  g_signal_connect (priv->search_entry, "notify::text",
                    G_CALLBACK (mex_search_text_changed_cb), self);
  g_signal_connect (priv->search_entry, "notify::style-pseudo-class",
                    G_CALLBACK (mex_search_text_style_changed), header);

  /* Create the suggestions column */
  priv->suggest_column = mx_box_layout_new ();
  clutter_actor_set_name (priv->suggest_column, "suggest-column");
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->suggest_column),
                                 MX_ORIENTATION_VERTICAL);

  view_model = mex_view_model_new (MEX_MODEL (priv->suggest_model));
  suggest_proxy = mex_generic_proxy_new (view_model,
                                         MX_TYPE_BUTTON);
  mex_generic_proxy_bind (MEX_GENERIC_PROXY (suggest_proxy),
                          mex_enum_to_string (MEX_TYPE_CONTENT_METADATA,
                                              MEX_CONTENT_METADATA_TITLE),
                          "label");
  g_signal_connect (suggest_proxy, "object-created",
                    G_CALLBACK (mex_search_proxy_add_cb),
                    priv->suggest_column);
  g_signal_connect (suggest_proxy, "object-removed",
                    G_CALLBACK (mex_search_proxy_remove_cb),
                    priv->suggest_column);
  g_object_weak_ref (G_OBJECT (priv->suggest_column),
                     (GWeakNotify)g_object_unref, suggest_proxy);

  /* Pack the search page */
  priv->search_page = mx_frame_new ();
  clutter_actor_set_name (priv->search_page, "search-page");
  mx_bin_set_fill (MX_BIN (priv->search_page), FALSE, TRUE);
  mx_bin_set_alignment (MX_BIN (priv->search_page), MX_ALIGN_START,
                        MX_ALIGN_START);
  hbox = mex_resizing_hbox_new ();
  mex_resizing_hbox_set_resizing_enabled (MEX_RESIZING_HBOX (hbox), FALSE);
  box = mx_box_layout_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->search_page), hbox);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), box);
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box),
                                 MX_ORIENTATION_VERTICAL);
  clutter_container_add (CLUTTER_CONTAINER (box),
                         header, priv->suggest_column, NULL);
  mx_box_layout_child_set_expand (MX_BOX_LAYOUT (box),
                                  priv->suggest_column, TRUE);
  clutter_container_child_set (CLUTTER_CONTAINER (box), header,
                               "x-fill", TRUE,
                               "x-align", MX_ALIGN_START, NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->suggest_column,
                               "x-fill", TRUE,
                               "x-align", MX_ALIGN_START, NULL);
  clutter_actor_set_width (box, 426.0);

  /* Update the history list */
  mex_search_plugin_update_history (self, NULL);
}

static GType
mex_search_get_type (void)
{
  return MEX_TYPE_SEARCH_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Search",
		   "Provides the search column",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Chris Lord <chris@linux.intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_search_get_type,
		   MEX_PLUGIN_PRIORITY_CORE)
