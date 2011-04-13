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

/* mex-notification-area.h */

#ifndef _MEX_NOTIFICATION_AREA_H
#define _MEX_NOTIFICATION_AREA_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_NOTIFICATION_AREA mex_notification_area_get_type()

#define MEX_NOTIFICATION_AREA(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_NOTIFICATION_AREA, MexNotificationArea))

#define MEX_NOTIFICATION_AREA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_NOTIFICATION_AREA, MexNotificationAreaClass))

#define MEX_IS_NOTIFICATION_AREA(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_NOTIFICATION_AREA))

#define MEX_IS_NOTIFICATION_AREA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_NOTIFICATION_AREA))

#define MEX_NOTIFICATION_AREA_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_NOTIFICATION_AREA, MexNotificationAreaClass))

typedef struct _MexNotificationArea MexNotificationArea;
typedef struct _MexNotificationAreaClass MexNotificationAreaClass;
typedef struct _MexNotificationAreaPrivate MexNotificationAreaPrivate;

struct _MexNotificationArea
{
  MxStack parent;

  MexNotificationAreaPrivate *priv;
};

struct _MexNotificationAreaClass
{
  MxStackClass parent_class;
};

GType mex_notification_area_get_type (void) G_GNUC_CONST;

ClutterActor *mex_notification_area_new (void);

G_END_DECLS

#endif /* _MEX_NOTIFICATION_AREA_H */
