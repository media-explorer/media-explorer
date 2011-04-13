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

#ifndef _MEX_SURFACE_PLAYER_H
#define _MEX_SURFACE_PLAYER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_SURFACE_PLAYER mex_surface_player_get_type()

#define MEX_SURFACE_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_SURFACE_PLAYER, MexSurfacePlayer))

#define MEX_SURFACE_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_SURFACE_PLAYER, MexSurfacePlayerClass))

#define MEX_IS_SURFACE_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_SURFACE_PLAYER))

#define MEX_IS_SURFACE_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_SURFACE_PLAYER))

#define MEX_SURFACE_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_SURFACE_PLAYER, MexSurfacePlayerClass))

typedef struct _MexSurfacePlayer MexSurfacePlayer;
typedef struct _MexSurfacePlayerClass MexSurfacePlayerClass;
typedef struct _MexSurfacePlayerPrivate MexSurfacePlayerPrivate;

struct _MexSurfacePlayer
{
  GObject parent;

  MexSurfacePlayerPrivate *priv;
};

struct _MexSurfacePlayerClass
{
  GObjectClass parent_class;
};

GType mex_surface_player_get_type (void) G_GNUC_CONST;

MexSurfacePlayer *mex_surface_player_new (void);

G_END_DECLS

#endif /* _MEX_SURFACE_PLAYER_H */
