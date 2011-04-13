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
 * SECTION:mex-applet-provider
 * @short_description: Interface for sources of applets
 *
 * Implementing #MexAppletProvider means that the class can provide a list
 * of applets.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-applet-provider.h"

G_DEFINE_INTERFACE (MexAppletProvider, mex_applet_provider, G_TYPE_INVALID);

static void
mex_applet_provider_default_init (MexAppletProviderInterface *klass)
{
}

/**
 * mex_applet_provider_get_applets:
 * @provider: a #MexAppletProvider
 *
 * Retrieves the list of applets from a #MexAppletProvider. This is a list of
 * #MexApplet##s.
 *
 * Return value: The applets of @provider
 *
 */
const GList *
mex_applet_provider_get_applets (MexAppletProvider *provider)
{
  MexAppletProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_APPLET_PROVIDER (provider), NULL);

  iface = MEX_APPLET_PROVIDER_GET_IFACE (provider);

  if (G_LIKELY (iface->get_applets))
    return iface->get_applets (provider);

  g_warning (G_STRLOC ": MexAppletProvider of type '%s' does not implement "
             "get_applets()", g_type_name (G_OBJECT_TYPE (provider)));

  return NULL;
}
