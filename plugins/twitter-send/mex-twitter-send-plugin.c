/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Collabora Ltd.
 *   @author Dario Freddi <dario.freddi@collabora.com>
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

#include "mex-twitter-send-plugin.h"
#include <libsocialweb-client/sw-client.h>
#include <glib/gi18n.h>

#define MEX_LOG_DOMAIN_DEFAULT  twitter_send_log_domain
MEX_LOG_DOMAIN_STATIC(twitter_send_log_domain);

static const gchar *app_mimetypes[] = { "video", "audio", "x-mex/media", "x-mex-channel", NULL };

static void action_provider_iface_init (MexActionProviderInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MexTwitterSendPlugin,
                         mex_twitter_send_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                action_provider_iface_init))

#define TWITTER_SEND_PLUGIN_PRIVATE(o)                         \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                           \
                                MEX_TYPE_TWITTER_SEND_PLUGIN,  \
                                MexTwitterSendPluginPrivate))

struct _MexTwitterSendPluginPrivate
{
  GList                      *actions;

  SwClient                   *client;
  SwClientService            *service;

  ClutterActor               *dialog;
  ClutterActor               *prompt_label;

  const gchar                *last_title;
  const gchar                *content_verb;
};

G_GNUC_UNUSED static void
mex_twitter_send_plugin_dispose (GObject *gobject)
{
  MexTwitterSendPlugin *self = MEX_TWITTER_SEND_PLUGIN (gobject);
  MexTwitterSendPluginPrivate *priv = self->priv;

  while (priv->actions)
    {
      MexActionInfo *info = priv->actions->data;

      g_object_unref (info->action);
      g_strfreev (info->mime_types);
      g_free (info);

      priv->actions = g_list_delete_link (priv->actions, priv->actions);
    }
  if (priv->dialog)
    {
      g_object_unref (priv->dialog);
      priv->dialog = NULL;
    }

  G_OBJECT_CLASS (mex_twitter_send_plugin_parent_class)->dispose (gobject);
}

static void
mex_twitter_send_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_twitter_send_plugin_parent_class)->finalize (object);
}

static void
mex_twitter_send_plugin_class_init (MexTwitterSendPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = mex_twitter_send_plugin_finalize;

  g_type_class_add_private (klass, sizeof (MexTwitterSendPluginPrivate));
}

static void
mex_twitter_send_plugin_hide_prompt_dialog (MexTwitterSendPlugin *self)
{
  /* Hide the dialog. */
  clutter_actor_destroy (self->priv->dialog);
}

static void
mex_twitter_send_plugin_on_status_updated (SwClientService *service,
                                           const GError    *error,
                                           gpointer         user_data)
{
  MexTwitterSendPlugin *self = MEX_TWITTER_SEND_PLUGIN (user_data);

  if (error != NULL)
    {
      mex_info_bar_new_notification (MEX_INFO_BAR (mex_info_bar_get_default ()),
                                     _("There has been an error while posting "
                                       "your tweet!"),
                                     20);
    }
  else
    {
      mex_info_bar_new_notification (MEX_INFO_BAR (mex_info_bar_get_default ()),
                                     _("Your tweet was posted successfully!"),
                                     20);
    }

  mex_twitter_send_plugin_hide_prompt_dialog (self);
}

static void
mex_twitter_send_plugin_on_cancel_tweet (MxAction *action     G_GNUC_UNUSED,
                                         gpointer  user_data)
{
  MexTwitterSendPlugin *self = MEX_TWITTER_SEND_PLUGIN (user_data);

  /* TODO: Should we do anything here? */
  MEX_DEBUG ("No tweet");

  mex_twitter_send_plugin_hide_prompt_dialog (self);
}

static void
mex_twitter_send_plugin_on_tweet (MxButton *button,
                                  gpointer  user_data)
{
  MexTwitterSendPlugin *self = MEX_TWITTER_SEND_PLUGIN (user_data);
  MexTwitterSendPluginPrivate *priv = self->priv;
  gchar *tweet;

  MEX_DEBUG ("accept chosen");

  tweet = g_strdup_printf (_("I %s \"%s\" on media-explorer."
                             " It's %s"),
                           priv->content_verb,
                           priv->last_title,
                           mx_button_get_label (MX_BUTTON (button)));

  sw_client_service_update_status (priv->service,
                                   mex_twitter_send_plugin_on_status_updated,
                                   tweet, self);
  g_free (tweet);
}

static void
mex_twitter_send_plugin_on_share_action (MxAction *action,
                                         gpointer  user_data)
{
  MexTwitterSendPlugin *self = MEX_TWITTER_SEND_PLUGIN (user_data);
  MexTwitterSendPluginPrivate *priv = self->priv;
  ClutterActor *stack, *layout;
  MxButtonGroup *button_group;
  ClutterActor *awesome_button, *decent_button,
               *pretty_bad_button, *horrible_button, *dontshare_button;
  const gchar *mime;

  gchar *label_text;

  MexContent *current_content = mex_action_get_content (action);
  priv->last_title = mex_content_get_metadata (current_content,
                                               MEX_CONTENT_METADATA_TITLE);


  mime = mex_content_get_metadata (current_content,
                                   MEX_CONTENT_METADATA_MIMETYPE);

  if (g_str_has_prefix (mime, "audio/"))
    priv->content_verb = _("listened to");
  else
    priv->content_verb = _("watched");

  priv->dialog = mx_dialog_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->dialog),
                               "MexShareDialog");
  clutter_actor_set_name (priv->dialog, "twitter-send-prompt-dialog");

  stack = mx_window_get_child (mex_get_main_window ());
  mx_dialog_set_transient_parent (MX_DIALOG (priv->dialog), stack);

  label_text = g_strdup_printf (_("Whatd do you think of: %s?"),
                                priv->last_title);

  priv->prompt_label = mx_label_new_with_text (label_text);
  mx_label_set_line_wrap (MX_LABEL (priv->prompt_label), TRUE);

  MEX_DEBUG ("Label text: %s", label_text);

  awesome_button = mx_button_new_with_label (_("Awesome!"));
  decent_button = mx_button_new_with_label (_("Decent"));
  pretty_bad_button = mx_button_new_with_label (_("Pretty bad"));
  horrible_button = mx_button_new_with_label (_("Horrible!"));
  dontshare_button = mx_button_new_with_label (_("Don't share"));

  button_group = mx_button_group_new ();
  mx_button_group_add (button_group, MX_BUTTON (awesome_button));
  mx_button_group_add (button_group, MX_BUTTON (decent_button));
  mx_button_group_add (button_group, MX_BUTTON (pretty_bad_button));
  mx_button_group_add (button_group, MX_BUTTON (horrible_button));
  mx_button_group_add (button_group, MX_BUTTON (dontshare_button));

  g_signal_connect (awesome_button,
                    "clicked",
                    G_CALLBACK (mex_twitter_send_plugin_on_tweet),
                    self);
  g_signal_connect (decent_button,
                    "clicked",
                    G_CALLBACK (mex_twitter_send_plugin_on_tweet),
                    self);
  g_signal_connect (pretty_bad_button,
                    "clicked",
                    G_CALLBACK (mex_twitter_send_plugin_on_tweet),
                    self);
  g_signal_connect (horrible_button,
                    "clicked",
                    G_CALLBACK (mex_twitter_send_plugin_on_tweet),
                    self);
  g_signal_connect (dontshare_button,
                    "clicked",
                    G_CALLBACK (mex_twitter_send_plugin_on_cancel_tweet),
                    self);

  layout = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (layout),
                                 MX_ORIENTATION_VERTICAL);

  mx_box_layout_insert_actor (MX_BOX_LAYOUT (layout),
                              priv->prompt_label,
                              0);

  mx_box_layout_insert_actor (MX_BOX_LAYOUT (layout),
                              awesome_button,
                              1);
  mx_box_layout_insert_actor (MX_BOX_LAYOUT (layout),
                              decent_button,
                              2);
  mx_box_layout_insert_actor (MX_BOX_LAYOUT (layout),
                              pretty_bad_button,
                              3);
  mx_box_layout_insert_actor (MX_BOX_LAYOUT (layout),
                              horrible_button,
                              4);
  mx_box_layout_insert_actor (MX_BOX_LAYOUT (layout),
                              dontshare_button,
                              5);

  clutter_actor_set_width (layout, 600.0);

  mx_bin_set_child (MX_BIN (priv->dialog), layout);

  clutter_actor_show (priv->dialog);
  mex_push_focus (MX_FOCUSABLE (priv->dialog));

  g_free (label_text);
}

static void
mex_twitter_send_plugin_init (MexTwitterSendPlugin *self)
{
  MexTwitterSendPluginPrivate *priv;
  MexActionInfo *action_info;

  priv = self->priv = TWITTER_SEND_PLUGIN_PRIVATE (self);
  priv->actions = NULL;

  /* lsw init */
  priv->client = sw_client_new ();
  priv->service = sw_client_get_service (priv->client, "twitter");

  /* log domain */
  MEX_LOG_DOMAIN_INIT (twitter_send_log_domain, "TwitterSend");

  /* actions */
  action_info = g_new0 (MexActionInfo, 1);
  action_info->action = mx_action_new_full ("x-mex-twitter-applet-share",
                                            _("Share"),
                                            G_CALLBACK (mex_twitter_send_plugin_on_share_action),
                                            self);
  mx_action_set_icon (action_info->action, "media-share-mex");
  action_info->mime_types = g_strdupv ((gchar **)app_mimetypes);
  priv->actions = g_list_append (priv->actions, action_info);
  /* log domain */
  MEX_LOG_DOMAIN_INIT (twitter_send_log_domain, "TwitterSend");
}

MexTwitterSendPlugin *
mex_twitter_send_plugin_new (void)
{
  return g_object_new (MEX_TYPE_TWITTER_SEND_PLUGIN, NULL);
}

static const GList *
mex_twitter_send_plugin_get_actions (MexActionProvider *action_provider)
{
  MexTwitterSendPlugin *self = MEX_TWITTER_SEND_PLUGIN (action_provider);
  MexTwitterSendPluginPrivate *priv = self->priv;

  return priv->actions;
}

static void
action_provider_iface_init (MexActionProviderInterface *iface)

{
  iface->get_actions = mex_twitter_send_plugin_get_actions;
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_TWITTER_SEND_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Twitter send plugin",
                   "Twitter integration",
                   PACKAGE_VERSION,
                   "LGPLv2.1+",
                   "Rob Bradford <rob.bradford@linux.intel.com>,"
                   "Dario Freddi <dario.freddi@collabora.com>",
                   MEX_API_MAJOR, MEX_API_MINOR,
                   mex_twitter_send_plugin_get_type,
                   MEX_PLUGIN_PRIORITY_NORMAL)
