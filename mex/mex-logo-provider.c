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
 * SECTION:mex-channel-logo-provider
 * @short_description: Interface for sources of channel logos
 *
 * Implementing #MexChannelLogoProvider means that the class can provide logos
 * for channels to the application.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-logo-provider.h"

G_DEFINE_INTERFACE (MexLogoProvider,
                    mex_logo_provider,
                    G_TYPE_INVALID);

static void
mex_logo_provider_default_init (MexLogoProviderInterface *klass)
{
}

/**
 * mex_logo_provider_get_logo:
 * @provider: a #MexLogoProvider
 * @channel: a #MexChannel
 *
 * Get the logo for @channel
 *
 * Return value: The URI to the channel logo or %NULL if none is found. The
 *               returned string has to be freed with g_free().
 *
 * Since: 0.2
 */
gchar *
mex_logo_provider_get_channel_logo (MexLogoProvider *provider,
                                    MexChannel      *channel)
{
  MexLogoProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_LOGO_PROVIDER (provider), NULL);

  iface = MEX_LOGO_PROVIDER_GET_IFACE (provider);

  if (iface->get_channel_logo)
    return iface->get_channel_logo (provider, channel);

  return NULL;
}
