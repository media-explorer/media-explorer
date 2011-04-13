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


#ifndef _MEX_ASPECT_FRAME_H
#define _MEX_ASPECT_FRAME_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_ASPECT_FRAME mex_aspect_frame_get_type()

#define MEX_ASPECT_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_ASPECT_FRAME, MexAspectFrame))

#define MEX_ASPECT_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_ASPECT_FRAME, MexAspectFrameClass))

#define MEX_IS_ASPECT_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_ASPECT_FRAME))

#define MEX_IS_ASPECT_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_ASPECT_FRAME))

#define MEX_ASPECT_FRAME_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_ASPECT_FRAME, MexAspectFrameClass))

typedef struct _MexAspectFrame MexAspectFrame;
typedef struct _MexAspectFrameClass MexAspectFrameClass;
typedef struct _MexAspectFramePrivate MexAspectFramePrivate;

struct _MexAspectFrame
{
  MxBin parent;

  MexAspectFramePrivate *priv;
};

struct _MexAspectFrameClass
{
  MxBinClass parent_class;
};

GType mex_aspect_frame_get_type (void) G_GNUC_CONST;

ClutterActor *mex_aspect_frame_new (void);

void mex_aspect_frame_set_expand (MexAspectFrame *frame, gboolean expand);
gboolean mex_aspect_frame_get_expand (MexAspectFrame *frame);

void mex_aspect_frame_set_ratio (MexAspectFrame *frame, gfloat ratio);
gfloat mex_aspect_frame_get_ratio (MexAspectFrame *frame);

G_END_DECLS

#endif /* _MEX_ASPECT_FRAME_H */
