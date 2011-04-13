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


#ifndef __MEX_CHANNEL_H__
#define __MEX_CHANNEL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_CHANNEL mex_channel_get_type()

#define MEX_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_CHANNEL, MexChannel))

#define MEX_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_CHANNEL, MexChannelClass))

#define MEX_IS_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_CHANNEL))

#define MEX_IS_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_CHANNEL))

#define MEX_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_CHANNEL, MexChannelClass))

typedef struct _MexChannel MexChannel;
typedef struct _MexChannelClass MexChannelClass;
typedef struct _MexChannelPrivate MexChannelPrivate;

struct _MexChannel
{
  GObject parent;

  MexChannelPrivate *priv;
};

struct _MexChannelClass
{
  GObjectClass parent_class;
};

GType mex_channel_get_type (void) G_GNUC_CONST;

MexChannel *  mex_channel_new                 (void);

const gchar * mex_channel_get_name            (MexChannel *channel);
void          mex_channel_set_name            (MexChannel  *channel,
                                               const gchar *name);
const gchar * mex_channel_get_uri             (MexChannel *channel);
void          mex_channel_set_uri             (MexChannel  *channel,
                                               const gchar *uri);
const gchar * mex_channel_get_thumbnail_uri   (MexChannel *channel);
void          mex_channel_set_thumbnail_uri   (MexChannel  *channel,
                                               const gchar *uri);
const gchar * mex_channel_get_logo_uri        (MexChannel *channel);
void          mex_channel_set_logo_uri        (MexChannel  *channel,
                                               const gchar *uri);

G_END_DECLS

#endif /* __MEX_CHANNEL_H__ */
