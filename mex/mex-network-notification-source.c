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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-network-notification-source.h"

#include <gio/gio.h>
#include <glib/gi18n-lib.h>

G_DEFINE_TYPE (MexNetworkNotificationSource, mex_network_notification_source, MEX_TYPE_NOTIFICATION_SOURCE)

#define NETWORK_NOTIFICATION_SOURCE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_NETWORK_NOTIFICATION_SOURCE, MexNetworkNotificationSourcePrivate))

struct _MexNetworkNotificationSourcePrivate
{
  GNetworkMonitor *monitor;
  MexNotification *offline_notification;
};

static void _online_notify_cb (GNetworkMonitor *monitor,
                               gboolean available,
                               gpointer user_data);

static void
mex_network_notification_source_dispose (GObject *object)
{
  MexNotificationSource *source = MEX_NOTIFICATION_SOURCE (object);
  MexNetworkNotificationSourcePrivate *priv;

  priv = MEX_NETWORK_NOTIFICATION_SOURCE (source)->priv;

  if (priv->monitor) {
    g_signal_handlers_disconnect_by_func (priv->monitor, _online_notify_cb, source);
    /* Don't unref as we don't own the monitor */
    priv->monitor = NULL;
  }

  G_OBJECT_CLASS (mex_network_notification_source_parent_class)->dispose (object);
}

static void
mex_network_notification_source_class_init (MexNetworkNotificationSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexNetworkNotificationSourcePrivate));

  object_class->dispose = mex_network_notification_source_dispose;
}

static gboolean
_idle_cb (MexNetworkNotificationSource *source)
{
  /* only emit an initial notification if offline */
  if (!g_network_monitor_get_network_available (source->priv->monitor))
    _online_notify_cb (source->priv->monitor, FALSE, source);

  return FALSE;
}

static void
mex_network_notification_source_init (MexNetworkNotificationSource *self)
{
  self->priv = NETWORK_NOTIFICATION_SOURCE_PRIVATE (self);

  self->priv->monitor = g_network_monitor_get_default ();

  g_signal_connect (self->priv->monitor, "network-changed", G_CALLBACK (_online_notify_cb), self);

  /*
   * Must do this in high priority idle because the signal won't be connected
   * until after it was instantiated
   */
  g_idle_add_full (G_PRIORITY_DEFAULT,
                   (GSourceFunc)_idle_cb,
                   self,
                   NULL);
}

static void
_online_notify_cb (GNetworkMonitor *monitor,
                   gboolean online,
                   gpointer user_data)
{
  MexNotificationSource *source = MEX_NOTIFICATION_SOURCE (user_data);
  MexNetworkNotificationSourcePrivate *priv;
  MexNotification *n;

  priv = MEX_NETWORK_NOTIFICATION_SOURCE (source)->priv;

  if (online)
    {
      if (priv->offline_notification)
        {
          /* Remove old notification */
          mex_notification_source_emit_notification_remove (source,
                                                            priv->offline_notification);
          mex_notification_free (priv->offline_notification);
          priv->offline_notification = NULL;
        }

      n = mex_notification_source_new_notification (source,
                                                    _("Network connection established"),
                                                    "icon-notifications", 7);
      mex_notification_source_emit_notification_added (source, n);

      mex_notification_free (n);
    } else {
        if (!priv->offline_notification)
          {
            n = mex_notification_source_new_notification (source,
                                                          _("Network connection lost"),
                                                          "icon-notifications",
                                                          7);

            priv->offline_notification = n;
            mex_notification_source_emit_notification_added (source, n);
          }
    }
}
