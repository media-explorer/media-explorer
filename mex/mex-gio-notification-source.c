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

/* mex-gio-notification-source.c */

#include "mex-gio-notification-source.h"

#include <config.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

G_DEFINE_TYPE (MexGIONotificationSource, mex_gio_notification_source, MEX_TYPE_NOTIFICATION_SOURCE)

#define GIO_NOTIFICATION_SOURCE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GIO_NOTIFICATION_SOURCE, MexGIONotificationSourcePrivate))

#define GET_PRIVATE(o) MEX_GIO_NOTIFICATION_SOURCE(o)->priv

struct _MexGIONotificationSourcePrivate
{
  GVolumeMonitor *volume_monitor;
};

static void _volume_monitor_mount_added_cb (GVolumeMonitor           *volume_monitor,
                                            GMount                   *mount,
                                            MexGIONotificationSource *source);
static void _volume_monitor_mount_removed_cb (GVolumeMonitor           *volume_monitor,
                                              GMount                   *mount,
                                              MexGIONotificationSource *source);

static void
mex_gio_notification_source_dispose (GObject *object)
{
  MexGIONotificationSourcePrivate *priv = GET_PRIVATE (object);

  if (priv->volume_monitor)
    {
      g_signal_handlers_disconnect_by_func (priv->volume_monitor,
                                            _volume_monitor_mount_added_cb,
                                            object);
      g_signal_handlers_disconnect_by_func (priv->volume_monitor,
                                            _volume_monitor_mount_removed_cb,
                                            object);
      g_object_unref (priv->volume_monitor);
      priv->volume_monitor = NULL;
    }

  G_OBJECT_CLASS (mex_gio_notification_source_parent_class)->dispose (object);
}

static void
mex_gio_notification_source_class_init (MexGIONotificationSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexGIONotificationSourcePrivate));

  object_class->dispose = mex_gio_notification_source_dispose;
}

static void
_volume_monitor_mount_added_cb (GVolumeMonitor           *volume_monitor,
                                GMount                   *mount,
                                MexGIONotificationSource *source)
{
  MexNotification *notification;
  gchar *name = NULL, *device_msg;
  GVolume *volume;
  const gchar *icon_name;

  volume = g_mount_get_volume (mount);

  if (volume) {
    name = g_volume_get_identifier (volume, G_VOLUME_IDENTIFIER_KIND_LABEL);
  }

  if (!name) {
    name = g_mount_get_name (mount);
  }

  device_msg = g_strdup_printf (_("Device \"%s\" plugged in"), name);
  /* We could get the icon from GMount/volume/drive */
  icon_name = "icon-notifications";

  notification = mex_notification_source_new_notification (MEX_NOTIFICATION_SOURCE (source),
                                                           device_msg,
                                                           icon_name,
                                                           30);

  mex_notification_source_emit_notification_added (MEX_NOTIFICATION_SOURCE (source),
                                                   notification);

  mex_notification_free (notification);
  g_free (name);
  g_free (device_msg);

  if (volume)
    g_object_unref (volume);
}

static void
_volume_monitor_mount_removed_cb (GVolumeMonitor           *volume_monitor,
                                  GMount                   *mount,
                                  MexGIONotificationSource *source)
{
  MexNotification *notification;
  gchar *name = NULL, *device_msg;
  GVolume *volume;
  const gchar *icon_name;

  volume = g_mount_get_volume (mount);

  if (volume) {
    name = g_volume_get_identifier (volume, G_VOLUME_IDENTIFIER_KIND_LABEL);
  }

  if (!name) {
    name = g_mount_get_name (mount);
  }

  device_msg = g_strdup_printf (_("Device \"%s\" removed"), name);
  /* We could get the icon from GMount/volume/drive */
  icon_name = "icon-notifications";

  notification = mex_notification_source_new_notification (MEX_NOTIFICATION_SOURCE (source),
                                                           device_msg,
                                                           icon_name,
                                                           30);

  mex_notification_source_emit_notification_added (MEX_NOTIFICATION_SOURCE (source),
                                                   notification);

  mex_notification_free (notification);
  g_free (name);
  g_free (device_msg);

  if (volume)
    g_object_unref (volume);
}

static void
mex_gio_notification_source_init (MexGIONotificationSource *self)
{
  self->priv = GIO_NOTIFICATION_SOURCE_PRIVATE (self);

  self->priv->volume_monitor = g_volume_monitor_get ();

  g_signal_connect (self->priv->volume_monitor,
                    "mount-added",
                    (GCallback)_volume_monitor_mount_added_cb,
                    self);
  g_signal_connect (self->priv->volume_monitor,
                    "mount-removed",
                    (GCallback)_volume_monitor_mount_removed_cb,
                    self);
}
