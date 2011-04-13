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


#ifndef _MEX_CONTENT_TILE_H
#define _MEX_CONTENT_TILE_H

#include <mx/mx.h>
#include <mex/mex-content-view.h>
#include <mex/mex-tile.h>

G_BEGIN_DECLS

#define MEX_TYPE_CONTENT_TILE mex_content_tile_get_type()

#define MEX_CONTENT_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_CONTENT_TILE, MexContentTile))

#define MEX_CONTENT_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_CONTENT_TILE, MexContentTileClass))

#define MEX_IS_CONTENT_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_CONTENT_TILE))

#define MEX_IS_CONTENT_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_CONTENT_TILE))

#define MEX_CONTENT_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_CONTENT_TILE, MexContentTileClass))

typedef struct _MexContentTile MexContentTile;
typedef struct _MexContentTileClass MexContentTileClass;
typedef struct _MexContentTilePrivate MexContentTilePrivate;

struct _MexContentTile
{
  MexTile parent;

  MexContentTilePrivate *priv;
};

struct _MexContentTileClass
{
  MexTileClass parent_class;
};

GType mex_content_tile_get_type (void) G_GNUC_CONST;

ClutterActor *mex_content_tile_new (void);

G_END_DECLS

#endif /* _MEX_CONTENT_TILE_H */
