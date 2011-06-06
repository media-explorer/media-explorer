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

/* mex-generic-notification-source.h */

#ifndef _MEX_GENERIC_NOTIFICATION_SOURCE_H
#define _MEX_GENERIC_NOTIFICATION_SOURCE_H

#include <glib-object.h>
#include <mex/mex-notification-source.h>

G_BEGIN_DECLS

#define MEX_TYPE_GENERIC_NOTIFICATION_SOURCE mex_generic_notification_source_get_type()

#define MEX_GENERIC_NOTIFICATION_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_GENERIC_NOTIFICATION_SOURCE, MexGenericNotificationSource))

#define MEX_GENERIC_NOTIFICATION_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_GENERIC_NOTIFICATION_SOURCE, MexGenericNotificationSourceClass))

#define MEX_IS_GENERIC_NOTIFICATION_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_GENERIC_NOTIFICATION_SOURCE))

#define MEX_IS_GENERIC_NOTIFICATION_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_GENERIC_NOTIFICATION_SOURCE))

#define MEX_GENERIC_NOTIFICATION_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_GENERIC_NOTIFICATION_SOURCE, MexGenericNotificationSourceClass))

typedef struct _MexGenericNotificationSource MexGenericNotificationSource;
typedef struct _MexGenericNotificationSourceClass MexGenericNotificationSourceClass;
typedef struct _MexGenericNotificationSourcePrivate MexGenericNotificationSourcePrivate;

struct _MexGenericNotificationSource
{
  MexNotificationSource parent;
};

struct _MexGenericNotificationSourceClass
{
  MexNotificationSourceClass parent_class;
};

GType mex_generic_notification_source_get_type (void) G_GNUC_CONST;

void
mex_generic_notification_new_notification (MexGenericNotificationSource *sourcein,
                                           const gchar *message,
                                           gint timeout);

MexGenericNotificationSource *
mex_generic_notification_source_new (void);

G_END_DECLS

#endif /* _MEX_GENERIC_NOTIFICATION_SOURCE_H */
