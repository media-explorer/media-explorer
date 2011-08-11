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

#ifndef _MEX_DBUS_BACKGROUND_VIDEO_H
#define _MEX_DBUS_BACKGROUND_VIDEO_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_DBUS_BACKGROUND_VIDEO mex_dbus_background_video_get_type()

#define MEX_DBUS_BACKGROUND_VIDEO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_DBUS_BACKGROUND_VIDEO, MexDbusBackgroundVideo))

#define MEX_DBUS_BACKGROUND_VIDEO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_DBUS_BACKGROUND_VIDEO, MexDbusBackgroundVideoClass))

#define MEX_IS_DBUS_BACKGROUND_VIDEO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_DBUS_BACKGROUND_VIDEO))

#define MEX_IS_DBUS_BACKGROUND_VIDEO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_DBUS_BACKGROUND_VIDEO))

#define MEX_DBUS_BACKGROUND_VIDEO_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_DBUS_BACKGROUND_VIDEO, MexDbusBackgroundVideoClass))

typedef struct _MexDbusBackgroundVideo MexDbusBackgroundVideo;
typedef struct _MexDbusBackgroundVideoClass MexDbusBackgroundVideoClass;
typedef struct _MexDbusBackgroundVideoPrivate MexDbusBackgroundVideoPrivate;

struct _MexDbusBackgroundVideo
{
  ClutterActor parent;

  MexDbusBackgroundVideoPrivate *priv;
};

struct _MexDbusBackgroundVideoClass
{
  ClutterActorClass parent_class;
};

GType mex_dbus_background_video_get_type (void) G_GNUC_CONST;

MexDbusBackgroundVideo *mex_dbus_background_video_new (void);

G_END_DECLS

#endif /* _MEX_DBUS_BACKGROUND_VIDEO_H */
