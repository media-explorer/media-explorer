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

#ifndef _MEX_MUSIC_PLAYER_H
#define _MEX_MUSIC_PLAYER_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_MUSIC_PLAYER mex_music_player_get_type()

#define MEX_MUSIC_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_MUSIC_PLAYER, MexMusicPlayer))

#define MEX_MUSIC_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_MUSIC_PLAYER, MexMusicPlayerClass))

#define MEX_IS_MUSIC_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_MUSIC_PLAYER))

#define MEX_IS_MUSIC_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_MUSIC_PLAYER))

#define MEX_MUSIC_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_MUSIC_PLAYER, MexMusicPlayerClass))

typedef struct _MexMusicPlayer MexMusicPlayer;
typedef struct _MexMusicPlayerClass MexMusicPlayerClass;
typedef struct _MexMusicPlayerPrivate MexMusicPlayerPrivate;

struct _MexMusicPlayer
{
  MxWidget parent;

  MexMusicPlayerPrivate *priv;
};

struct _MexMusicPlayerClass
{
  MxWidgetClass parent_class;
};

GType mex_music_player_get_type (void) G_GNUC_CONST;

MexMusicPlayer *mex_music_player_get_default (void);

void mex_music_player_play (MexMusicPlayer *player);
void mex_music_player_play_toggle (MexMusicPlayer *player);
void mex_music_player_stop (MexMusicPlayer *player);
void mex_music_player_next (MexMusicPlayer *player);
void mex_music_player_previous (MexMusicPlayer *player);
void mex_music_player_quit (MexMusicPlayer *player);
gboolean mex_music_player_is_playing (MexMusicPlayer *player);
ClutterMedia *mex_music_player_get_clutter_media (MexMusicPlayer *player);
void mex_music_player_seek_us (MexMusicPlayer *player, gint64 seek_offset_us);
void mex_music_player_set_uri (MexMusicPlayer *player, const gchar *uri);

G_END_DECLS

#endif /* _MEX_MUSIC_PLAYER_H */
