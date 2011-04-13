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


#ifndef __MEX_URI_CHANNEL_PROVIDER_H__
#define __MEX_URI_CHANNEL_PROVIDER_H__

#include <glib-object.h>

#include <mex/mex-channel-provider.h>

G_BEGIN_DECLS

#define MEX_TYPE_URI_CHANNEL_PROVIDER mex_uri_channel_provider_get_type()

#define MEX_URI_CHANNEL_PROVIDER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                         \
                               MEX_TYPE_URI_CHANNEL_PROVIDER, \
                               MexUriChannelProvider))

#define MEX_URI_CHANNEL_PROVIDER_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                        \
                            MEX_TYPE_URI_CHANNEL_PROVIDER,  \
                            MexUriChannelProviderClass))

#define MEX_IS_URI_CHANNEL_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_URI_CHANNEL_PROVIDER))

#define MEX_IS_URI_CHANNEL_PROVIDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_URI_CHANNEL_PROVIDER))

#define MEX_URI_CHANNEL_PROVIDER_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                          \
                              MEX_TYPE_URI_CHANNEL_PROVIDER,  \
                              MexUriChannelProviderClass))

typedef struct _MexUriChannelProvider MexUriChannelProvider;
typedef struct _MexUriChannelProviderClass MexUriChannelProviderClass;
typedef struct _MexUriChannelProviderPrivate MexUriChannelProviderPrivate;

struct _MexUriChannelProvider
{
  GObject parent;

  MexUriChannelProviderPrivate *priv;
};

struct _MexUriChannelProviderClass
{
  GObjectClass parent_class;
};

GType                 mex_uri_channel_provider_get_type         (void) G_GNUC_CONST;

MexChannelProvider *  mex_uri_channel_provider_new              (const gchar *config_file);
const gchar *         mex_uri_channel_provider_get_config_file  (MexUriChannelProvider *provider);
gboolean              mex_uri_channel_provider_set_config_file  (MexUriChannelProvider *provider,
                                                                 const gchar           *config_file);

G_END_DECLS

#endif /* __MEX_URI_CHANNEL_PROVIDER_H__ */
