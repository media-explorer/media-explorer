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

#ifndef _MEX_BACKGROUND_VIDEO_H
#define _MEX_BACKGROUND_VIDEO_H

#include <glib-object.h>

#include <clutter-gst/clutter-gst.h>

G_BEGIN_DECLS

#define MEX_TYPE_BACKGROUND_VIDEO mex_background_video_get_type()

#define MEX_BACKGROUND_VIDEO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_BACKGROUND_VIDEO, MexBackgroundVideo))

#define MEX_BACKGROUND_VIDEO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_BACKGROUND_VIDEO, MexBackgroundVideoClass))

#define MEX_IS_BACKGROUND_VIDEO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_BACKGROUND_VIDEO))

#define MEX_IS_BACKGROUND_VIDEO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_BACKGROUND_VIDEO))

#define MEX_BACKGROUND_VIDEO_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_BACKGROUND_VIDEO, MexBackgroundVideoClass))

typedef struct _MexBackgroundVideo MexBackgroundVideo;
typedef struct _MexBackgroundVideoClass MexBackgroundVideoClass;
typedef struct _MexBackgroundVideoPrivate MexBackgroundVideoPrivate;

struct _MexBackgroundVideo
{
  ClutterGstVideoTexture parent;

  MexBackgroundVideoPrivate *priv;
};

struct _MexBackgroundVideoClass
{
  ClutterGstVideoTextureClass parent_class;
};

GType mex_background_video_get_type (void) G_GNUC_CONST;

MexBackgroundVideo *mex_background_video_new (void);

G_END_DECLS

#endif /* _MEX_BACKGROUND_VIDEO_H */
