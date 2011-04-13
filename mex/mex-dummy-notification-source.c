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

/* mex-dummy-notification-source.c */

#include "mex-dummy-notification-source.h"

#include <config.h>
#include <glib/gi18n-lib.h>

G_DEFINE_TYPE (MexDummyNotificationSource, mex_dummy_notification_source, MEX_TYPE_NOTIFICATION_SOURCE)

#define DUMMY_NOTIFICATION_SOURCE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_DUMMY_NOTIFICATION_SOURCE, MexDummyNotificationSourcePrivate))

struct _MexDummyNotificationSourcePrivate
{
  guint notification_timeout_cb;
};

static void
mex_dummy_notification_source_dispose (GObject *object)
{
  MexDummyNotificationSourcePrivate *priv = MEX_DUMMY_NOTIFICATION_SOURCE (object)->priv;

  if (priv->notification_timeout_cb)
    {
      g_source_remove (priv->notification_timeout_cb);
      priv->notification_timeout_cb = 0;
    }

  G_OBJECT_CLASS (mex_dummy_notification_source_parent_class)->dispose (object);
}

static void
mex_dummy_notification_source_class_init (MexDummyNotificationSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexDummyNotificationSourcePrivate));

  object_class->dispose = mex_dummy_notification_source_dispose;
}

static gboolean
_notification_timeout_cb (gpointer userdata)
{
  MexDummyNotificationSource *source = MEX_DUMMY_NOTIFICATION_SOURCE (userdata);
  MexNotification *notification;

  notification = mex_notification_source_new_notification (MEX_NOTIFICATION_SOURCE (source),
                                                           _("Snow?"),
                                                           "weather-snow",
                                                           8);

  mex_notification_source_emit_notification_added (MEX_NOTIFICATION_SOURCE (source),
                                                   notification);
  mex_notification_free (notification);

  return TRUE;
}

static void
mex_dummy_notification_source_init (MexDummyNotificationSource *self)
{
  self->priv = DUMMY_NOTIFICATION_SOURCE_PRIVATE (self);

  self->priv->notification_timeout_cb =
    g_timeout_add_seconds (10,
                           _notification_timeout_cb,
                           self);
}
