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

#ifndef __MEX_DVB_PLAYER_H__
#define __MEX_DVB_PLAYER_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_DVB_PLAYER mex_dvb_player_get_type()

#define MEX_DVB_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_DVB_PLAYER, MexDvbPlayer))

#define MEX_DVB_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_DVB_PLAYER, MexDvbPlayerClass))

#define MEX_IS_DVB_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_DVB_PLAYER))

#define MEX_IS_DVB_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_DVB_PLAYER))

#define MEX_DVB_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_DVB_PLAYER, MexDvbPlayerClass))

typedef struct _MexDvbPlayer        MexDvbPlayer;
typedef struct _MexDvbPlayerClass   MexDvbPlayerClass;
typedef struct _MexDvbPlayerPrivate MexDvbPlayerPrivate;

struct _MexDvbPlayer
{
  /*< private >*/
  ClutterTexture       parent;
  MexDvbPlayerPrivate *priv;
};

struct _MexDvbPlayerClass
{
  /*< private >*/
  ClutterTextureClass parent_class;
};

GType		  mex_dvb_player_get_type    (void) G_GNUC_CONST;
ClutterActor *	  mex_dvb_player_new         (void);

G_END_DECLS

#endif /* __MEX_DVB_PLAYER_H__ */
