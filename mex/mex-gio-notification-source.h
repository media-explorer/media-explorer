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

#ifndef __MEX_GIO_NOTIFICATION_SOURCE_H__
#define __MEX_GIO_NOTIFICATION_SOURCE_H__

#include <glib-object.h>
#include <mex/mex-notification-source.h>

G_BEGIN_DECLS

#define MEX_TYPE_GIO_NOTIFICATION_SOURCE mex_gio_notification_source_get_type()

#define MEX_GIO_NOTIFICATION_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_GIO_NOTIFICATION_SOURCE, MexGIONotificationSource))

#define MEX_GIO_NOTIFICATION_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_GIO_NOTIFICATION_SOURCE, MexGIONotificationSourceClass))

#define MEX_IS_GIO_NOTIFICATION_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_GIO_NOTIFICATION_SOURCE))

#define MEX_IS_GIO_NOTIFICATION_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_GIO_NOTIFICATION_SOURCE))

#define MEX_GIO_NOTIFICATION_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_GIO_NOTIFICATION_SOURCE, MexGIONotificationSourceClass))

typedef struct _MexGIONotificationSource MexGIONotificationSource;
typedef struct _MexGIONotificationSourceClass MexGIONotificationSourceClass;
typedef struct _MexGIONotificationSourcePrivate MexGIONotificationSourcePrivate;

struct _MexGIONotificationSource
{
  MexNotificationSource parent;

  MexGIONotificationSourcePrivate *priv;
};

struct _MexGIONotificationSourceClass
{
  MexNotificationSourceClass parent_class;
};

GType mex_gio_notification_source_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __MEX_GIO_NOTIFICATION_SOURCE_H__ */
