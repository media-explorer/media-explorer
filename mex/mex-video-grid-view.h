/*
 * Mex - a media explorer
 *
 * Copyright © 2012 Intel Corporation.
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

#ifndef __MEX_VIDEO_GRID_VIEW_H__
#define __MEX_VIDEO_GRID_VIEW_H__


#include <mex/mex-grid-view.h>

G_BEGIN_DECLS

#define MEX_TYPE_VIDEO_GRID_VIEW mex_video_grid_view_get_type()

#define MEX_VIDEO_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_VIDEO_GRID_VIEW, MexVideoGridView))

#define MEX_VIDEO_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_VIDEO_GRID_VIEW, MexVideoGridViewClass))

#define MEX_IS_VIDEO_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_VIDEO_GRID_VIEW))

#define MEX_IS_VIDEO_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_VIDEO_GRID_VIEW))

#define MEX_VIDEO_GRID_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_VIDEO_GRID_VIEW, MexVideoGridViewClass))

typedef struct _MexVideoGridView MexVideoGridView;
typedef struct _MexVideoGridViewClass MexVideoGridViewClass;
typedef struct _MexVideoGridViewPrivate MexVideoGridViewPrivate;

struct _MexVideoGridView
{
  MexGridView parent;

  MexVideoGridViewPrivate *priv;
};

struct _MexVideoGridViewClass
{
  MexGridViewClass parent_class;
};

GType mex_video_grid_view_get_type (void) G_GNUC_CONST;

ClutterActor *mex_video_grid_view_new (MexModel *model);

G_END_DECLS

#endif /* __MEX_VIDEO_GRID_VIEW_H__ */
