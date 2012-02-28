/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011, 2012 Intel Corporation.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include "mex-apps-plugin.h"
#include "webkit/webkit.h"

static void mex_tool_provider_iface_init (MexToolProviderInterface *iface);
static void mex_model_provider_iface_init (MexModelProviderInterface *iface);
static void mex_action_provider_iface_init (MexActionProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexAppsPlugin, mex_apps_plugin, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_TOOL_PROVIDER,
                                                mex_tool_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                mex_model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                mex_action_provider_iface_init))

#define APPS_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_APPS_PLUGIN, MexAppsPluginPrivate))

#define MEX_APPS_MIMETYPE "x-mex/app"

struct _MexAppsPluginPrivate
{
  GList        *models;
  GList        *actions;

  MexActionInfo action_info;
  ClutterActor *view;
};

static void
mex_apps_model_load (MexModel *model)
{
  JsonParser *parser;
  gchar *filename;
  GError *error = NULL;
  JsonNode *root;
  JsonArray *array;
  gint i = 0;

  filename = MEX_DATA_PLUGIN_DIR "/apps.json";

  if (!g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      g_free (filename);

      return;
    }

  parser = json_parser_new ();
  if (!json_parser_load_from_file (parser, filename, &error))
    {
      g_warning (G_STRLOC ": error populating from file: %s",
                 error->message);
      g_clear_error (&error);
      goto out;
    }

  root = json_parser_get_root (parser);

  if (!JSON_NODE_HOLDS_ARRAY (root))
    {
      g_warning (G_STRLOC ": JSON data not of expected format!");

      goto out;
    }

  array = json_node_get_array (root);

  for (i = 0; i < json_array_get_length (array); i++)
    {
      MexContent *content;
      JsonNode *node;

      node = json_array_get_element (array, i);
      content = (MexContent *)json_gobject_deserialize (MEX_TYPE_PROGRAM,
                                                        node);

      mex_model_add_content (MEX_MODEL (model), content);
    }

out:
  g_object_unref (parser);
}
static void
mex_apps_plugin_dispose (GObject *object)
{
  MexAppsPlugin *self = MEX_APPS_PLUGIN (object);
  MexAppsPluginPrivate *priv = self->priv;

  if (priv->models)
    {
      g_object_unref (priv->models->data);
      g_list_free (priv->models);
      priv->models = NULL;
    }

  if (priv->action_info.action)
    {
      g_object_unref (priv->action_info.action);
      priv->action_info.action = NULL;
    }

  G_OBJECT_CLASS (mex_apps_plugin_parent_class)->dispose (object);
}

static void
mex_apps_plugin_finalize (GObject *object)
{
  /* MexAppsPluginPrivate *priv = MEX_APPS_PLUGIN (object)->priv; */

  G_OBJECT_CLASS (mex_apps_plugin_parent_class)->finalize (object);
}

static void
mex_apps_plugin_class_init (MexAppsPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexAppsPluginPrivate));

  object_class->dispose = mex_apps_plugin_dispose;
  object_class->finalize = mex_apps_plugin_finalize;
}


static void
mex_apps_plugin_launch_cb (MxAction        *action,
                           MexAppsPlugin *self)
{
  /* MexAppsPluginPrivate *priv = self->priv; */
  MexContent *content = mex_action_get_content (action);
  const gchar *text = mex_content_get_metadata (content,
                                                MEX_CONTENT_METADATA_URL);

  mex_tool_provider_present_actor (MEX_TOOL_PROVIDER (self), self->priv->view);

  webkit_iweb_view_load_uri(WEBKIT_IWEB_VIEW(self->priv->view), text);
  clutter_actor_grab_key_focus (self->priv->view);
}

static void
mex_tool_provider_iface_init (MexToolProviderInterface *iface)
{
}

static const GList *
mex_apps_plugin_get_models (MexModelProvider *self)
{
  MexAppsPluginPrivate *priv = MEX_APPS_PLUGIN (self)->priv;
  return priv->models;
}

static void
mex_model_provider_iface_init (MexModelProviderInterface *iface)
{
  iface->get_models = mex_apps_plugin_get_models;
}

static const GList *
mex_apps_plugin_get_actions (MexActionProvider *self)
{
  MexAppsPluginPrivate *priv = MEX_APPS_PLUGIN (self)->priv;
  return priv->actions;
}

static void
mex_action_provider_iface_init (MexActionProviderInterface *iface)
{
  iface->get_actions = mex_apps_plugin_get_actions;
}

static void
view_hidden_cb (MxWebView *view)
{
  webkit_iweb_view_load_uri (WEBKIT_IWEB_VIEW (view), "about:blank");
}

static gboolean
view_captured_event_cb (MxWebView *view,
                        ClutterEvent *event,
                        gpointer data)
{

  if (event->type == CLUTTER_KEY_PRESS || event->type == CLUTTER_KEY_RELEASE)
    {
      ClutterKeyEvent *key_event = (ClutterKeyEvent *) event;

      /* ensure that "back" always closes the app */
      if (MEX_KEY_BACK (key_event->keyval))
        {
          mex_tool_provider_remove_actor (data, CLUTTER_ACTOR (view));
          return TRUE;
        }
    }

  return FALSE;
}

static const gchar *apps_mimetypes[] = { MEX_APPS_MIMETYPE, NULL };

static const MexModelCategoryInfo apps = {
    "apps", N_("Apps"), "icon-panelheader-apps", 45
};


static void
mex_apps_plugin_init (MexAppsPlugin *self)
{
  MexAppsPluginPrivate *priv = self->priv = APPS_PLUGIN_PRIVATE (self);
  MexModel *model;
  WebKitWebPage *page;
  WebKitWebSettings *settings;

  MexModelManager *mmanager;

  /* register the "apps" category */
  mmanager = mex_model_manager_get_default ();
  mex_model_manager_add_category (mmanager, &apps);

  model = (MexModel*) mex_feed_new (_("Apps"), _("Apps"));
  g_object_set (G_OBJECT (model), "category", "apps", NULL);

  priv->models = g_list_append (NULL, model);

  /* Create the actions list */
  memset (&priv->action_info, 0, sizeof (MexActionInfo));
  priv->action_info.action =
    mx_action_new_full (MEX_APPS_MIMETYPE,
                        _("Launch"),
                        G_CALLBACK (mex_apps_plugin_launch_cb),
                        self);
  priv->action_info.mime_types = (gchar **)apps_mimetypes;
  priv->actions = g_list_append (NULL, &priv->action_info);

  /* load content */
  mex_apps_model_load (model);

  /* create view */
  webkit_init ();
  priv->view = mx_web_view_new();

  page = webkit_iweb_view_get_page (WEBKIT_IWEB_VIEW (priv->view));
  settings = webkit_web_page_get_settings (page);

  g_object_set (settings, "enable-plugins", FALSE, "user-stylesheet-uri",
                "file://" MEX_DATA_PLUGIN_DIR "/blank.css", NULL);

  g_signal_connect (priv->view, "hide",
                    G_CALLBACK (view_hidden_cb), NULL);
  g_signal_connect (priv->view, "captured-event",
                    G_CALLBACK (view_captured_event_cb), self);
}

MEX_DEFINE_PLUGIN ("Apps",
		   "Provides the Apps column",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_apps_plugin_get_type,
		   MEX_PLUGIN_PRIORITY_CORE)
