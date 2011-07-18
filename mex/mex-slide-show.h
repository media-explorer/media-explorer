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


#ifndef __MEX_SLIDE_SHOW_H__
#define __MEX_SLIDE_SHOW_H__

#include <glib-object.h>
#include <clutter/clutter.h>

#include <mex/mex-content.h>
#include <mex/mex-model.h>
#include <mex/mex-proxy.h>

#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_SLIDE_SHOW mex_slide_show_get_type()

#define MEX_SLIDE_SHOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_SLIDE_SHOW, MexSlideShow))

#define MEX_SLIDE_SHOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_SLIDE_SHOW, MexSlideShowClass))

#define MEX_IS_SLIDE_SHOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_SLIDE_SHOW))

#define MEX_IS_SLIDE_SHOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_SLIDE_SHOW))

#define MEX_SLIDE_SHOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_SLIDE_SHOW, MexSlideShowClass))

typedef struct _MexSlideShow MexSlideShow;
typedef struct _MexSlideShowPrivate MexSlideShowPrivate;
typedef struct _MexSlideShowClass MexSlideShowClass;


struct _MexSlideShow
{
  MxFrame parent;

  MexSlideShowPrivate *priv;
};

struct _MexSlideShowClass
{
  MxFrameClass parent_class;
};

GType           mex_slide_show_get_type       (void) G_GNUC_CONST;

ClutterActor *  mex_slide_show_new            (void);

G_END_DECLS

#endif /* __MEX_SLIDE_SHOW_H__ */
