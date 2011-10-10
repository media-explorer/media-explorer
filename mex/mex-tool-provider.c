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
 * SECTION:mex-tool-provider
 * @short_description: Interface for sources of tools
 *
 * Implementing #MexToolProvider means that the class can provide a list
 * of tools (focusable actors that provide some useful functionality).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-tool-provider.h"

enum
{
  PRESENT_ACTOR,
  REMOVE_ACTOR,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_INTERFACE (MexToolProvider, mex_tool_provider, G_TYPE_INVALID);

static void
mex_tool_provider_default_init (MexToolProviderInterface *klass)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      initialized = TRUE;

      signals[PRESENT_ACTOR] =
        g_signal_new ("present-actor",
                      G_TYPE_FROM_INTERFACE (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MexToolProviderInterface, present_actor),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, CLUTTER_TYPE_ACTOR);

      signals[REMOVE_ACTOR] =
        g_signal_new ("remove-actor",
                      G_TYPE_FROM_INTERFACE (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MexToolProviderInterface, remove_actor),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, CLUTTER_TYPE_ACTOR);
    }
}

/**
 * mex_tool_provider_get_tools:
 * @provider: a #MexToolProvider
 *
 * Retrieves the list of tools from a #MexToolProvider. This is a list of
 * #ClutterActor##s.
 *
 * Return value: The tools of @provider
 *
 */
const GList *
mex_tool_provider_get_tools (MexToolProvider *provider)
{
  MexToolProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_TOOL_PROVIDER (provider), NULL);

  iface = MEX_TOOL_PROVIDER_GET_IFACE (provider);

  if (G_LIKELY (iface->get_tools))
    return iface->get_tools (provider);

  return NULL;
}

/**
 * mex_tool_provider_get_bindings:
 * @provider: a #MexToolProvider
 *
 * Retrieves a list of key-bindings from a #MexToolProvider. This is a list of
 * #MexToolProviderBinding##s. See clutter_binding_pool_install_action().
 *
 * Return value: Key-bindings associated with @provider
 */
const GList *
mex_tool_provider_get_bindings (MexToolProvider *provider)
{
  MexToolProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_TOOL_PROVIDER (provider), NULL);

  iface = MEX_TOOL_PROVIDER_GET_IFACE (provider);

  if (G_LIKELY (iface->get_bindings))
    return iface->get_bindings (provider);

  return NULL;
}

void
mex_tool_provider_set_tool_mode        (MexToolProvider *provider,
                                        MexToolMode mode,
                                        guint duration)
{
  MexToolProviderInterface *iface;

  g_return_if_fail (MEX_IS_TOOL_PROVIDER (provider));

  iface = MEX_TOOL_PROVIDER_GET_IFACE (provider);

  if (G_LIKELY (iface->set_tool_mode))
    iface->set_tool_mode (provider, mode, duration);
}

void
mex_tool_provider_present_actor (MexToolProvider *provider,
                                 ClutterActor    *actor)
{
  g_return_if_fail (MEX_IS_TOOL_PROVIDER (provider));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  g_signal_emit (provider, signals[PRESENT_ACTOR], 0, actor);
}

void
mex_tool_provider_remove_actor (MexToolProvider *provider,
                                ClutterActor    *actor)
{
  g_return_if_fail (MEX_IS_TOOL_PROVIDER (provider));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  g_signal_emit (provider, signals[REMOVE_ACTOR], 0, actor);
}
