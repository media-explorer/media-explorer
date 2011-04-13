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


#ifndef __MEX_EPG_TILE_H__
#define __MEX_EPG_TILE_H__

#include <glib-object.h>
#include <mx/mx.h>

#include <mex/mex-epg-event.h>

G_BEGIN_DECLS

#define MEX_TYPE_EPG_TILE mex_epg_tile_get_type()

#define MEX_EPG_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_EPG_TILE, MexEpgTile))

#define MEX_EPG_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_EPG_TILE, MexEpgTileClass))

#define MEX_IS_EPG_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_EPG_TILE))

#define MEX_IS_EPG_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_EPG_TILE))

#define MEX_EPG_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_EPG_TILE, MexEpgTileClass))

typedef struct _MexEpgTile MexEpgTile;
typedef struct _MexEpgTileClass MexEpgTileClass;
typedef struct _MexEpgTilePrivate MexEpgTilePrivate;

struct _MexEpgTile
{
  MxButton parent;

  MexEpgTilePrivate *priv;
};

struct _MexEpgTileClass
{
  MxButtonClass parent_class;
};

GType           mex_epg_tile_get_type         (void) G_GNUC_CONST;

ClutterActor *  mex_epg_tile_new              (void);
ClutterActor *  mex_epg_tile_new_with_event   (MexEpgEvent *event);

MexEpgEvent *   mex_epg_tile_get_event        (MexEpgTile *tile);
void            mex_epg_tile_set_event        (MexEpgTile  *tile,
                                               MexEpgEvent *event);

G_END_DECLS

#endif /* __MEX_EPG_TILE_H__ */
