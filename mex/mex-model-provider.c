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
 * SECTION:mex-model-provider
 * @short_description: Interface for sources of models
 *
 * Implementing #MexModelProvider means that the class can provide a list
 * of models that belong in given categories.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-model-provider.h"

enum
{
  PRESENT_MODEL,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_INTERFACE (MexModelProvider, mex_model_provider, G_TYPE_INVALID);

static void
mex_model_provider_default_init (MexModelProviderInterface *klass)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      initialized = TRUE;

      signals[PRESENT_MODEL] =
        g_signal_new ("present-model",
                      G_TYPE_FROM_INTERFACE (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MexModelProviderInterface,
                                       present_model),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);
    }
}

/**
 * mex_model_provider_get_models:
 * @provider: a #MexModelProvider
 *
 * Retrieves the list of models from a #MexModelProvider. This is a list of
 * #MexModel##s.
 *
 * Return value: The models of @provider
 *
 */
const GList *
mex_model_provider_get_models (MexModelProvider *provider)
{
  MexModelProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_MODEL_PROVIDER (provider), NULL);

  iface = MEX_MODEL_PROVIDER_GET_IFACE (provider);

  if (G_LIKELY (iface->get_models))
    return iface->get_models (provider);

  return NULL;
}

gboolean
mex_model_provider_model_activated (MexModelProvider *provider,
                                    MexModel         *model)
{
  MexModelProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_MODEL_PROVIDER (provider), FALSE);
  g_return_val_if_fail (MEX_IS_MODEL (model), FALSE);

  iface = MEX_MODEL_PROVIDER_GET_IFACE (provider);

  if (iface->model_activated)
    return iface->model_activated (provider, model);

  return FALSE;
}

void
mex_model_provider_present_model (MexModelProvider *provider,
                                  MexModel         *model)
{
  g_return_if_fail (MEX_IS_MODEL_PROVIDER (provider));
  g_return_if_fail (model != NULL);

  g_signal_emit (provider, signals[PRESENT_MODEL], 0, model);
}
