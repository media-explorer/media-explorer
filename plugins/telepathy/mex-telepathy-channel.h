/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Collabora Ltd.
 *   @author Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifndef _MEX_TELEPATHY_CHANNEL
#define _MEX_TELEPATHY_CHANNEL

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define MEX_TYPE_TELEPATHY_CHANNEL mex_telepathy_channel_get_type()

#define MEX_TELEPATHY_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_TELEPATHY_CHANNEL, MexTelepathyChannel))

#define MEX_TELEPATHY_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_TELEPATHY_CHANNEL, MexTelepathyChannelClass))

#define MEX_IS_TELEPATHY_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_TELEPATHY_CHANNEL))

#define MEX_IS_TELEPATHY_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_TELEPATHY_CHANNEL))

#define MEX_TELEPATHY_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_TELEPATHY_CHANNEL, MexTelepathyChannelClass))

typedef struct _MexTelepathyChannelPrivate MexTelepathyChannelPrivate;

typedef struct {
  GObject parent;

  MexTelepathyChannelPrivate *priv;
} MexTelepathyChannel;

typedef struct {
  GObjectClass parent_class;
} MexTelepathyChannelClass;

GType mex_telepathy_channel_get_type (void);

MexTelepathyChannel* mex_telepathy_channel_new (void);

G_MODULE_EXPORT const GType mex_get_channel_type (void);

G_END_DECLS

#endif
