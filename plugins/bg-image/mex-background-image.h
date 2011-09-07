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

#ifndef _MEX_BACKGROUND_IMAGE_H
#define _MEX_BACKGROUND_IMAGE_H

#include <glib-object.h>

#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_BACKGROUND_IMAGE mex_background_image_get_type()

#define MEX_BACKGROUND_IMAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_BACKGROUND_IMAGE, MexBackgroundImage))

#define MEX_BACKGROUND_IMAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_BACKGROUND_IMAGE, MexBackgroundImageClass))

#define MEX_IS_BACKGROUND_IMAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_BACKGROUND_IMAGE))

#define MEX_IS_BACKGROUND_IMAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_BACKGROUND_IMAGE))

#define MEX_BACKGROUND_IMAGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_BACKGROUND_IMAGE, MexBackgroundImageClass))

typedef struct _MexBackgroundImage MexBackgroundImage;
typedef struct _MexBackgroundImageClass MexBackgroundImageClass;
typedef struct _MexBackgroundImagePrivate MexBackgroundImagePrivate;

struct _MexBackgroundImage
{
  MxStack parent;

  MexBackgroundImagePrivate *priv;
};

struct _MexBackgroundImageClass
{
  MxStackClass parent_class;
};

GType mex_background_image_get_type (void) G_GNUC_CONST;

MexBackgroundImage *mex_background_image_new (void);

G_END_DECLS

#endif /* _MEX_BACKGROUND_IMAGE_H */
