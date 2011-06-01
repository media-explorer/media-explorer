/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#ifndef _MEX_TRACKER_NOTIFICATIONS_H
#define _MEX_TRACKER_NOTIFICATIONS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_TRACKER_NOTIFICATIONS mex_tracker_notifications_get_type()

#define MEX_TRACKER_NOTIFICATIONS(obj)                          \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               MEX_TYPE_TRACKER_NOTIFICATIONS,  \
                               MexTrackerNotifications))

#define MEX_TRACKER_NOTIFICATIONS_CLASS(klass)                          \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            MEX_TYPE_TRACKER_NOTIFICATIONS,             \
                            MexTrackerNotificationsClass))

#define MEX_IS_TRACKER_NOTIFICATIONS(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               MEX_TYPE_TRACKER_NOTIFICATIONS))

#define MEX_IS_TRACKER_NOTIFICATIONS_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                            \
                            MEX_TYPE_TRACKER_NOTIFICATIONS))

#define MEX_TRACKER_NOTIFICATIONS_GET_CLASS(obj)                        \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              MEX_TYPE_TRACKER_NOTIFICATIONS,           \
                              MexTrackerNotificationsClass))

typedef struct _MexTrackerNotifications MexTrackerNotifications;
typedef struct _MexTrackerNotificationsClass MexTrackerNotificationsClass;
typedef struct _MexTrackerNotificationsPrivate MexTrackerNotificationsPrivate;

struct _MexTrackerNotifications
{
  GObject parent;

  MexTrackerNotificationsPrivate *priv;
};

struct _MexTrackerNotificationsClass
{
  GObjectClass parent_class;
};

GType mex_tracker_notifications_get_type (void) G_GNUC_CONST;

MexTrackerNotifications *mex_tracker_notifications_get (void);

G_END_DECLS

#endif /* _MEX_TRACKER_NOTIFICATIONS_H */
