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
 * SECTION:mex-action-provider
 * @short_description: Interface for sources of actions
 *
 * Implementing #MexActionProvider means that the class can provide a list
 * of actions that apply to an #MexContent with a particular mime-type.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-action-provider.h"

G_DEFINE_INTERFACE (MexActionProvider, mex_action_provider, G_TYPE_INVALID);

static void
mex_action_provider_default_init (MexActionProviderInterface *klass)
{
}

/**
 * mex_action_provider_get_actions:
 * @provider: a #MexActionProvider
 *
 * Retrieves the list of actions from a #MexActionProvider. This is a list of
 * #MexActionInfo##s.
 *
 * Return value: The actions of @provider
 *
 */
const GList *
mex_action_provider_get_actions (MexActionProvider *provider)
{
  MexActionProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_ACTION_PROVIDER (provider), NULL);

  iface = MEX_ACTION_PROVIDER_GET_IFACE (provider);

  if (G_LIKELY (iface->get_actions))
    return iface->get_actions (provider);

  g_warning (G_STRLOC ": MexActionProvider of type '%s' does not implement "
             "get_actions()", g_type_name (G_OBJECT_TYPE (provider)));

  return NULL;
}
