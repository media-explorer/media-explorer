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

/* mex-twitter-applet-plugin.c */

#include "mex-twitter-applet-plugin.h"
#include <mex/mex-applet-provider.h>
#include <mex/mex-action-provider.h>

#include "mex-twitter-applet.h"

#include <config.h>
#include <glib/gi18n-lib.h>

static void mex_applet_provider_iface_init (MexAppletProviderInterface *iface);
static void mex_action_provider_iface_init (MexActionProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexTwitterAppletPlugin, mex_twitter_applet_plugin, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_APPLET_PROVIDER,
                                                mex_applet_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                mex_action_provider_iface_init))

#define TWITTER_APPLET_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TWITTER_APPLET_PLUGIN, MexTwitterAppletPluginPrivate))

struct _MexTwitterAppletPluginPrivate
{
  MexTwitterApplet *applet;
  GList *applets, *actions;
};

static void
mex_twitter_applet_plugin_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_twitter_applet_plugin_parent_class)->dispose (object);
}

static void
mex_twitter_applet_plugin_finalize (GObject *object)
{
  MexTwitterAppletPluginPrivate *priv = MEX_TWITTER_APPLET_PLUGIN (object)->priv;

  g_list_free (priv->applets);

  G_OBJECT_CLASS (mex_twitter_applet_plugin_parent_class)->finalize (object);
}

static void
mex_twitter_applet_plugin_class_init (MexTwitterAppletPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexTwitterAppletPluginPrivate));

  object_class->dispose = mex_twitter_applet_plugin_dispose;
  object_class->finalize = mex_twitter_applet_plugin_finalize;
}

static const GList *
mex_twitter_applet_plugin_get_applets (MexAppletProvider *self)
{
  MexTwitterAppletPluginPrivate *priv = MEX_TWITTER_APPLET_PLUGIN (self)->priv;
  return priv->applets;
}

static void
mex_applet_provider_iface_init (MexAppletProviderInterface *iface)
{
  iface->get_applets = mex_twitter_applet_plugin_get_applets;
}

static const GList *
mex_twitter_applet_plugin_get_actions (MexActionProvider *self)
{
  MexTwitterAppletPluginPrivate *priv = MEX_TWITTER_APPLET_PLUGIN (self)->priv;
  return priv->actions;
}

static void
mex_action_provider_iface_init (MexActionProviderInterface *iface)
{
  iface->get_actions = mex_twitter_applet_plugin_get_actions;
}

static void
_twitter_applet_share_action_cb (MxAction *action,
                                 gpointer userdata)
{
  MexTwitterAppletPluginPrivate *priv = TWITTER_APPLET_PLUGIN_PRIVATE (userdata);
  MexContent *content;

  content = mex_action_get_content (action);

  mex_twitter_applet_share (priv->applet, content);
}

static const gchar *app_mimetypes[] = { "video", "audio", "x-mex/media", "x-mex-channel", NULL };

static void
mex_twitter_applet_plugin_init (MexTwitterAppletPlugin *self)
{
  MexTwitterAppletPluginPrivate *priv = TWITTER_APPLET_PLUGIN_PRIVATE (self);
  MexActionInfo *action_info;

  self->priv = priv;

  priv->applet = g_object_new (MEX_TYPE_TWITTER_APPLET, NULL);

  priv->applets = g_list_append (priv->applets, priv->applet);

  action_info = g_new0 (MexActionInfo, 1);
  action_info->action = mx_action_new_full ("x-mex-twitter-applet-share",
                                            _("Share"),
                                            (GCallback)_twitter_applet_share_action_cb,
                                            self);
  mx_action_set_icon (action_info->action, "media-share-mex");
  action_info->mime_types = g_strdupv ((gchar **)app_mimetypes);
  priv->actions = g_list_append (priv->actions, action_info);

}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_TWITTER_APPLET_PLUGIN;
}
