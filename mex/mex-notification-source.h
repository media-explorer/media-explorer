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

#ifndef __MEX_NOTIFICATION_SOURCE_H__
#define __MEX_NOTIFICATION_SOURCE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_NOTIFICATION_SOURCE mex_notification_source_get_type()

#define MEX_NOTIFICATION_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_NOTIFICATION_SOURCE, MexNotificationSource))

#define MEX_NOTIFICATION_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_NOTIFICATION_SOURCE, MexNotificationSourceClass))

#define MEX_IS_NOTIFICATION_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_NOTIFICATION_SOURCE))

#define MEX_IS_NOTIFICATION_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_NOTIFICATION_SOURCE))

#define MEX_NOTIFICATION_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_NOTIFICATION_SOURCE, MexNotificationSourceClass))

typedef struct _MexNotificationSource MexNotificationSource;
typedef struct _MexNotificationSourceClass MexNotificationSourceClass;

typedef struct
{
  MexNotificationSource *source;
  gchar *message;
  gchar *icon;
  gint duration;
} MexNotification;

struct _MexNotificationSource
{
  GObject parent;
};

struct _MexNotificationSourceClass
{
  GObjectClass parent_class;
  void (*notification_added) (MexNotificationSource *source, MexNotification *notification);
  void (*notification_removed) (MexNotificationSource *source, MexNotification *notification);
};

GType mex_notification_source_get_type (void) G_GNUC_CONST;

MexNotificationSource *mex_notification_source_new (void);

MexNotification *mex_notification_source_new_notification (MexNotificationSource *source,
                                                           const gchar           *message,
                                                           const gchar           *icon,
                                                           gint                   duration);
void mex_notification_source_emit_notification_added (MexNotificationSource *source,
                                                      MexNotification       *notification);
void mex_notification_source_emit_notification_remove (MexNotificationSource *source,
                                                       MexNotification       *notification);

GType mex_notification_get_type (void);
#define MEX_TYPE_NOTIFICATION mex_notification_get_type ()
void mex_notification_free (gpointer boxed_type);


G_END_DECLS

#endif /* __MEX_NOTIFICATION_SOURCE_H__ */
