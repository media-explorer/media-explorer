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


#ifndef __MEX_CHANNEL_PROVIDER_H__
#define __MEX_CHANNEL_PROVIDER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_CHANNEL_PROVIDER (mex_channel_provider_get_type ())

#define MEX_CHANNEL_PROVIDER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                     \
                               MEX_TYPE_CHANNEL_PROVIDER, \
                               MexChannelProvider))

#define MEX_IS_CHANNEL_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_CHANNEL_PROVIDER))

#define MEX_CHANNEL_PROVIDER_IFACE(iface)                 \
  (G_TYPE_CHECK_CLASS_CAST ((iface),                      \
                            MEX_TYPE_CHANNEL_PROVIDER,    \
                            MexChannelProviderInterface))

#define MEX_IS_CHANNEL_PROVIDER_IFACE(iface) \
  (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_CHANNEL_PROVIDER))

#define MEX_CHANNEL_PROVIDER_GET_IFACE(obj)                     \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj),                        \
                                  MEX_TYPE_CHANNEL_PROVIDER,    \
                                  MexChannelProviderInterface))

typedef struct _MexChannelProvider          MexChannelProvider;
typedef struct _MexChannelProviderInterface MexChannelProviderInterface;

struct _MexChannelProviderInterface
{
  GTypeInterface g_iface;

  /* virtual functions */
  guint               (*get_n_channels) (MexChannelProvider *provider);
  const GPtrArray *   (*get_channels)   (MexChannelProvider *provider);
};

GType               mex_channel_provider_get_type       (void) G_GNUC_CONST;

guint               mex_channel_provider_get_n_channels (MexChannelProvider *provider);
const GPtrArray *   mex_channel_provider_get_channels   (MexChannelProvider *provider);

G_END_DECLS

#endif /* __MEX_CHANNEL_PROVIDER_H__ */
