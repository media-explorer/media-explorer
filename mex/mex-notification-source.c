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

/* mex-notification-source.c */

#include "mex-notification-source.h"

G_DEFINE_ABSTRACT_TYPE (MexNotificationSource, mex_notification_source, G_TYPE_OBJECT)

enum
{
  NOTIFICATION_ADDED,
  NOTIFICATION_REMOVED,
  LAST_SIGNAL
};

static MexNotification *mex_notification_dup (MexNotification *notification);

G_DEFINE_BOXED_TYPE (MexNotification,
                     mex_notification,
                     mex_notification_dup,
                     mex_notification_free)

static guint signals[LAST_SIGNAL] = { 0, };

static void
mex_notification_source_class_init (MexNotificationSourceClass *klass)
{
  signals[NOTIFICATION_ADDED] =
    g_signal_new ("notification-added",
                  MEX_TYPE_NOTIFICATION_SOURCE,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MexNotificationSourceClass,
                                   notification_added),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE,
                  1,
                  MEX_TYPE_NOTIFICATION);

  signals[NOTIFICATION_REMOVED] =
    g_signal_new ("notification-removed",
                  MEX_TYPE_NOTIFICATION_SOURCE,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MexNotificationSourceClass,
                                   notification_removed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE,
                  1,
                  MEX_TYPE_NOTIFICATION);
}

static void
mex_notification_source_init (MexNotificationSource *self)
{
}

MexNotificationSource *
mex_notification_source_new (void)
{
  return g_object_new (MEX_TYPE_NOTIFICATION_SOURCE, NULL);
}

static MexNotification *
_mex_notification_new (void)
{
  MexNotification *notification;

  notification = g_slice_new0 (MexNotification);

  return notification;
}

MexNotification *
mex_notification_source_new_notification (MexNotificationSource *source,
                                          const gchar           *message,
                                          const gchar           *icon,
                                          gint                  duration)
{
  MexNotification *notification;

  notification = _mex_notification_new ();
  notification->message = g_strdup (message);
  notification->icon = g_strdup (icon);
  notification->duration = duration;

  notification->source = g_object_ref (source);

  return notification;
}

void
mex_notification_source_emit_notification_added (MexNotificationSource *source,
                                                 MexNotification       *notification)
{
  g_signal_emit (source, signals[NOTIFICATION_ADDED], 0, notification);
}

void
mex_notification_source_emit_notification_remove (MexNotificationSource *source,
                                                  MexNotification       *notification)
{
  g_signal_emit (source, signals[NOTIFICATION_REMOVED], 0, notification);
}

void
mex_notification_free (MexNotification *notification)
{
  g_object_unref (notification->source);
  g_free (notification->message);
  g_free (notification->icon);
  g_slice_free (MexNotification, notification);
}

static MexNotification *
mex_notification_dup (MexNotification *notification)
{
  MexNotification *new_notification;

  new_notification = _mex_notification_new ();

  new_notification->message = g_strdup (notification->message);
  new_notification->icon = g_strdup (notification->icon);
  new_notification->duration = notification->duration;
  new_notification->source = g_object_ref (notification->source);

  return new_notification;
}


