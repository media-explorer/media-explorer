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


/**
 * SECTION:mex-channel-provider
 * @short_description: Interface for sources of channels
 *
 * Implementing #MexChannelProvider means that the class can provide a list
 * of channels to the application.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-channel-provider.h"

G_DEFINE_INTERFACE (MexChannelProvider, mex_channel_provider, G_TYPE_INVALID);

static void
mex_channel_provider_default_init (MexChannelProviderInterface *klass)
{
}

/**
 * mex_channel_provider_get_n_channels:
 * @provider: a #MexChannelProvider
 *
 * Get the number of channels that #MexChannelProvider provides.
 *
 * Return value: The number of channels of @provider
 *
 * Since: 0.2
 */
guint
mex_channel_provider_get_n_channels (MexChannelProvider *provider)
{
  MexChannelProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_CHANNEL_PROVIDER (provider), 0);

  iface = MEX_CHANNEL_PROVIDER_GET_IFACE (provider);

  if (G_LIKELY (iface->get_n_channels))
    return iface->get_n_channels (provider);

  g_warning ("MexChannelProvider of type '%s' does not implement "
             "get_n_channels()", g_type_name (G_OBJECT_TYPE (provider)));

  return 0;
}

/**
 * mex_channel_provider_get_content:
 * @provider: a #MexChannelProvider
 *
 * Retrieves the list of channels from a #MexChannelProvider.
 *
 * Return value: The channels of @provider
 *
 * Since: 0.2
 */
const GPtrArray *
mex_channel_provider_get_channels (MexChannelProvider *provider)
{
  MexChannelProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_CHANNEL_PROVIDER (provider), NULL);

  iface = MEX_CHANNEL_PROVIDER_GET_IFACE (provider);

  if (G_LIKELY (iface->get_channels))
    return iface->get_channels (provider);

  g_warning ("MexChannelProvider of type '%s' does not implement "
             "get_channels()", g_type_name (G_OBJECT_TYPE (provider)));

  return NULL;
}
