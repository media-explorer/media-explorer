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

#ifndef __MEX_GRID_VIEW_H__
#define __MEX_GRID_VIEW_H__

#include <mx/mx.h>
#include "mex-menu.h"
#include "mex-model.h"
#include "mex-grid.h"

G_BEGIN_DECLS

#define MEX_TYPE_GRID_VIEW mex_grid_view_get_type()

#define MEX_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_GRID_VIEW, MexGridView))

#define MEX_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_GRID_VIEW, MexGridViewClass))

#define MEX_IS_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_GRID_VIEW))

#define MEX_IS_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_GRID_VIEW))

#define MEX_GRID_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_GRID_VIEW, MexGridViewClass))

typedef struct _MexGridView MexGridView;
typedef struct _MexGridViewClass MexGridViewClass;
typedef struct _MexGridViewPrivate MexGridViewPrivate;

struct _MexGridView
{
  MxWidget parent;

  MexGridViewPrivate *priv;
};

struct _MexGridViewClass
{
  MxWidgetClass parent_class;
};

GType mex_grid_view_get_type (void) G_GNUC_CONST;

ClutterActor *mex_grid_view_new (MexModel *model);

MexMenu *mex_grid_view_get_menu (MexGridView *view);
MexGrid *mex_grid_view_get_grid (MexGridView *view);

G_END_DECLS

#endif /* __MEX_GRID_VIEW_H__ */
