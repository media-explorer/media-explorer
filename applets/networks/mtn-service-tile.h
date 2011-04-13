/*
 * mex-networks - Connection Manager UI for Media Explorer 
 * Copyright © 2010-2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St 
 * - Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef _MTN_SERVICE_TILE
#define _MTN_SERVICE_TILE

#include <mx/mx.h>

G_BEGIN_DECLS

#define MTN_TYPE_SERVICE_TILE mtn_service_tile_get_type()

#define MTN_SERVICE_TILE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTN_TYPE_SERVICE_TILE, MtnServiceTile))

#define MTN_SERVICE_TILE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), MTN_TYPE_SERVICE_TILE, MtnServiceTileClass))

#define MTN_IS_SERVICE_TILE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTN_TYPE_SERVICE_TILE))

#define MTN_IS_SERVICE_TILE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), MTN_TYPE_SERVICE_TILE))

#define MTN_SERVICE_TILE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), MTN_TYPE_SERVICE_TILE, MtnServiceTileClass))

typedef struct _MtnServiceTilePrivate MtnServiceTilePrivate;

typedef struct {
    MxButton parent;

    MtnServiceTilePrivate *priv;
} MtnServiceTile;

typedef struct {
    MxButtonClass parent_class;
} MtnServiceTileClass;

GType           mtn_service_tile_get_type        (void);

void            mtn_service_tile_set_object_path (MtnServiceTile *tile, const char *object_path);
const char*     mtn_service_tile_get_object_path (MtnServiceTile *tile);

MtnServiceTile* mtn_service_tile_new             (const char *object_path);

G_END_DECLS

#endif /* _MTN_SERVICE_TILE */
