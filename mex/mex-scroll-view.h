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

#ifndef __MEX_SCROLL_VIEW_H__
#define __MEX_SCROLL_VIEW_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_SCROLL_VIEW mex_scroll_view_get_type()

#define MEX_SCROLL_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_SCROLL_VIEW, MexScrollView))

#define MEX_SCROLL_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_SCROLL_VIEW, MexScrollViewClass))

#define MEX_IS_SCROLL_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_SCROLL_VIEW))

#define MEX_IS_SCROLL_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_SCROLL_VIEW))

#define MEX_SCROLL_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_SCROLL_VIEW, MexScrollViewClass))

typedef struct _MexScrollView MexScrollView;
typedef struct _MexScrollViewClass MexScrollViewClass;
typedef struct _MexScrollViewPrivate MexScrollViewPrivate;

struct _MexScrollView
{
  MxKineticScrollView parent;

  MexScrollViewPrivate *priv;
};

struct _MexScrollViewClass
{
  MxKineticScrollViewClass parent_class;
};

GType mex_scroll_view_get_type (void) G_GNUC_CONST;

ClutterActor *mex_scroll_view_new (void);

void     mex_scroll_view_set_indicators_hidden (MexScrollView *view,
                                                gboolean       hidden);
gboolean mex_scroll_view_get_indicators_hidden (MexScrollView *view);

void     mex_scroll_view_set_follow_recurse (MexScrollView *view,
                                             gboolean       recurse);
gboolean mex_scroll_view_get_follow_recurse (MexScrollView *view);

void mex_scroll_view_ensure_visible (MexScrollView          *scroll,
                                     const ClutterGeometry *geometry);

void mex_scroll_view_set_scroll_delay (MexScrollView *view,
                                       guint          delay);

guint mex_scroll_view_get_scroll_delay (MexScrollView *view);

void mex_scroll_view_set_scroll_gravity (MexScrollView  *view,
                                         ClutterGravity  gravity);

ClutterGravity mex_scroll_view_get_scroll_gravity (MexScrollView *view);

void     mex_scroll_view_set_interpolate (MexScrollView *view,
                                          gboolean       interpolate);
gboolean mex_scroll_view_get_interpolate (MexScrollView *view);

G_END_DECLS

#endif /* __MEX_SCROLL_VIEW_H__ */
