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


#ifndef __MEX_CHANNEL_MANAGER_H__
#define __MEX_CHANNEL_MANAGER_H__

#include <glib-object.h>

#include <mex/mex-channel.h>
#include <mex/mex-channel-provider.h>
#include <mex/mex-logo-provider.h>

G_BEGIN_DECLS

#define MEX_TYPE_CHANNEL_MANAGER mex_channel_manager_get_type()

#define MEX_CHANNEL_MANAGER(obj)      \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                               MEX_TYPE_CHANNEL_MANAGER, MexChannelManager))

#define MEX_CHANNEL_MANAGER_CLASS(klass)  \
  (G_TYPE_CHECK_CLASS_CAST ((klass),      \
                            MEX_TYPE_CHANNEL_MANAGER, MexChannelManagerClass))

#define MEX_IS_CHANNEL_MANAGER(obj)   \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                               MEX_TYPE_CHANNEL_MANAGER))

#define MEX_IS_CHANNEL_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),        \
                            MEX_TYPE_CHANNEL_MANAGER))

#define MEX_CHANNEL_MANAGER_GET_CLASS(obj)  \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),        \
                              MEX_TYPE_CHANNEL_MANAGER, MexChannelManagerClass))

typedef struct _MexChannelManager MexChannelManager;
typedef struct _MexChannelManagerClass MexChannelManagerClass;
typedef struct _MexChannelManagerPrivate MexChannelManagerPrivate;

struct _MexChannelManager
{
  GObject parent;

  MexChannelManagerPrivate *priv;
};

struct _MexChannelManagerClass
{
  GObjectClass parent_class;
};

GType mex_channel_manager_get_type (void) G_GNUC_CONST;

MexChannelManager * mex_channel_manager_get_default           (void);

const GPtrArray *   mex_channel_manager_get_channels          (MexChannelManager *manager);
void                mex_channel_manager_add_provider          (MexChannelManager  *manager,
                                                               MexChannelProvider *provider);
void                mex_channel_manager_add_logo_provider     (MexChannelManager *manager,
                                                               MexLogoProvider   *provider);

guint               mex_channel_manager_get_n_channels        (MexChannelManager *manager);
gint                mex_channel_manager_get_channel_position  (MexChannelManager *manager,
                                                               MexChannel        *channel);
G_END_DECLS

#endif /* __MEX_CHANNEL_MANAGER_H__ */
