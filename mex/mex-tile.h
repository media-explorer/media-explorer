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

#ifndef __MEX_TILE_H__
#define __MEX_TILE_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_TILE mex_tile_get_type()

#define MEX_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_TILE, MexTile))

#define MEX_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_TILE, MexTileClass))

#define MEX_IS_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_TILE))

#define MEX_IS_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_TILE))

#define MEX_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_TILE, MexTileClass))

typedef struct _MexTile MexTile;
typedef struct _MexTileClass MexTileClass;
typedef struct _MexTilePrivate MexTilePrivate;

struct _MexTile
{
  MxBin parent;

  MexTilePrivate *priv;
};

struct _MexTileClass
{
  MxBinClass parent_class;
};

GType mex_tile_get_type (void) G_GNUC_CONST;

ClutterActor *mex_tile_new (void);
ClutterActor *mex_tile_new_with_label (const gchar *label);

void         mex_tile_set_label (MexTile *tile, const gchar *label);
const gchar *mex_tile_get_label (MexTile *tile);

void          mex_tile_set_primary_icon (MexTile *tile, ClutterActor *icon);
ClutterActor *mex_tile_get_primary_icon (MexTile *tile);

void          mex_tile_set_secondary_icon (MexTile *tile, ClutterActor *icon);
ClutterActor *mex_tile_get_secondary_icon (MexTile *tile);

gboolean mex_tile_get_header_visible (MexTile *tile);
void     mex_tile_set_header_visible (MexTile *tile, gboolean header_visible);

gboolean mex_tile_get_important (MexTile *tile);
void     mex_tile_set_important (MexTile *tile, gboolean important);


const gchar * mex_tile_get_secondary_label (MexTile *tile);
void          mex_tile_set_secondary_label (MexTile *tile, const gchar *label);

G_END_DECLS

#endif /* __MEX_TILE_H__ */
