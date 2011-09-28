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

#include "mex-twitter-send-plugin.h"

#include <glib/gi18n.h>

#define MEX_LOG_DOMAIN_DEFAULT  twitter_send_log_domain
MEX_LOG_DOMAIN_STATIC(twitter_send_log_domain);

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
  GList *actions;
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
mex_twitter_send_plugin_init (MexTwitterSendPlugin *self)
{
  MexTwitterSendPluginPrivate *priv;

  priv = self->priv = TWITTER_SEND_PLUGIN_PRIVATE (self);
  priv->actions = NULL;

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
