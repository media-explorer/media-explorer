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

/* mex-generic-notification-source.c */

#include "mex-generic-notification-source.h"

#include <config.h>
#include <glib/gi18n-lib.h>

G_DEFINE_TYPE (MexGenericNotificationSource, mex_generic_notification_source, MEX_TYPE_NOTIFICATION_SOURCE)

#define GENERIC_NOTIFICATION_SOURCE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GENERIC_NOTIFICATION_SOURCE, MexGenericNotificationSourcePrivate))

static void
mex_generic_notification_source_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_generic_notification_source_parent_class)->dispose (object);
}

static void
mex_generic_notification_source_class_init (MexGenericNotificationSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = mex_generic_notification_source_dispose;
}

void
mex_generic_notification_new_notification (MexGenericNotificationSource *sourcein,
                                           const gchar *message,
                                           gint timeout)
{
  MexGenericNotificationSource *source =
    MEX_GENERIC_NOTIFICATION_SOURCE (sourcein);

  MexNotification *notification;

  notification =
    mex_notification_source_new_notification (MEX_NOTIFICATION_SOURCE (source),
                                              message,
                                              "icon-notifications",
                                              timeout);

  mex_notification_source_emit_notification_added (MEX_NOTIFICATION_SOURCE (source),
                                                   notification);
  mex_notification_free (notification);
}

static void
mex_generic_notification_source_init (MexGenericNotificationSource *self)
{
}

MexGenericNotificationSource *
mex_generic_notification_source_new (void)
{
  return g_object_new (MEX_TYPE_GENERIC_NOTIFICATION_SOURCE, NULL);
}
