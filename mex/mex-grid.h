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


#ifndef _MEX_GRID_H
#define _MEX_GRID_H

#include <glib-object.h>
#include <mx/mx.h>
#include <mex/mex-utils.h>

G_BEGIN_DECLS

#define MEX_TYPE_GRID mex_grid_get_type()

#define MEX_GRID(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_GRID, MexGrid))

#define MEX_GRID_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_GRID, MexGridClass))

#define MEX_IS_GRID(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_GRID))

#define MEX_IS_GRID_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_GRID))

#define MEX_GRID_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_GRID, MexGridClass))

typedef struct _MexGrid MexGrid;
typedef struct _MexGridClass MexGridClass;
typedef struct _MexGridPrivate MexGridPrivate;

struct _MexGrid
{
  MxWidget parent;

  MexGridPrivate *priv;
};

struct _MexGridClass
{
  MxWidgetClass parent_class;
};

GType mex_grid_get_type (void) G_GNUC_CONST;

ClutterActor * mex_grid_new (void);

gint mex_grid_get_stride (MexGrid *grid);
void mex_grid_set_stride (MexGrid *grid, gint stride);

void mex_grid_get_tile_size (MexGrid *grid,
                             gfloat  *width,
                             gfloat  *height);

void             mex_grid_set_sort_func (MexGrid          *grid,
                                         MexActorSortFunc  func,
                                         gpointer          userdata);
MexActorSortFunc mex_grid_get_sort_func (MexGrid          *grid,
                                         gpointer         *userdata);

void mex_grid_resort     (MexGrid      *grid,
                          ClutterActor *child);
void mex_grid_resort_all (MexGrid      *grid,
                          ClutterActor *child);

G_END_DECLS

#endif /* _MEX_GRID_H */
