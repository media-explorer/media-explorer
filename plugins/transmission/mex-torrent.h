/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#ifndef __MEX_TORRENT_H__
#define __MEX_TORRENT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_TORRENT mex_torrent_get_type()

#define MEX_TORRENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_TORRENT, MexTorrent))

#define MEX_TORRENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_TORRENT, MexTorrentClass))

#define MEX_IS_TORRENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_TORRENT))

#define MEX_IS_TORRENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_TORRENT))

#define MEX_TORRENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_TORRENT, MexTorrentClass))

typedef struct _MexTorrent MexTorrent;
typedef struct _MexTorrentClass MexTorrentClass;
typedef struct _MexTorrentPrivate MexTorrentPrivate;

struct _MexTorrent
{
  GObject parent;

  MexTorrentPrivate *priv;
};

struct _MexTorrentClass
{
  GObjectClass parent_class;
};

GType mex_torrent_get_type (void) G_GNUC_CONST;

MexTorrent *mex_torrent_new (void);

G_END_DECLS

#endif /* __MEX_TORRENT_H__ */
