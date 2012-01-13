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

#ifndef _MEX_MUSIC_GRID_VIEW_H
#define _MEX_MUSIC_GRID_VIEW_H


#include "mex-grid-view.h"

G_BEGIN_DECLS

#define MEX_TYPE_MUSIC_GRID_VIEW mex_music_grid_view_get_type()

#define MEX_MUSIC_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_MUSIC_GRID_VIEW, MexMusicGridView))

#define MEX_MUSIC_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_MUSIC_GRID_VIEW, MexMusicGridViewClass))

#define MEX_IS_MUSIC_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_MUSIC_GRID_VIEW))

#define MEX_IS_MUSIC_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_MUSIC_GRID_VIEW))

#define MEX_MUSIC_GRID_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_MUSIC_GRID_VIEW, MexMusicGridViewClass))

typedef struct _MexMusicGridView MexMusicGridView;
typedef struct _MexMusicGridViewClass MexMusicGridViewClass;
typedef struct _MexMusicGridViewPrivate MexMusicGridViewPrivate;

struct _MexMusicGridView
{
  MexGridView parent;

  MexMusicGridViewPrivate *priv;
};

struct _MexMusicGridViewClass
{
  MexGridViewClass parent_class;
};

GType mex_music_grid_view_get_type (void) G_GNUC_CONST;

ClutterActor *mex_music_grid_view_new (MexModel *model);

G_END_DECLS

#endif /* _MEX_MUSIC_GRID_VIEW_H */
