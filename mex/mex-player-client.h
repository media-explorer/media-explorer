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

#ifndef __MEX_PLAYER_CLIENT_H__
#define __MEX_PLAYER_CLIENT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_PLAYER_CLIENT mex_player_client_get_type()

#define MEX_PLAYER_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_PLAYER_CLIENT, MexPlayerClient))

#define MEX_PLAYER_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_PLAYER_CLIENT, MexPlayerClientClass))

#define MEX_IS_PLAYER_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_PLAYER_CLIENT))

#define MEX_IS_PLAYER_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_PLAYER_CLIENT))

#define MEX_PLAYER_CLIENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_PLAYER_CLIENT, MexPlayerClientClass))

typedef struct _MexPlayerClient MexPlayerClient;
typedef struct _MexPlayerClientClass MexPlayerClientClass;
typedef struct _MexPlayerClientPrivate MexPlayerClientPrivate;

struct _MexPlayerClient
{
  GObject parent;

  MexPlayerClientPrivate *priv;
};

struct _MexPlayerClientClass
{
  GObjectClass parent_class;
};

GType mex_player_client_get_type (void) G_GNUC_CONST;

MexPlayerClient *mex_player_client_new (void);

G_END_DECLS

#endif /* __MEX_PLAYER_CLIENT_H__ */
