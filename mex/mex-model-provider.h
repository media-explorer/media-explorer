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


#ifndef __MEX_MODEL_PROVIDER_H__
#define __MEX_MODEL_PROVIDER_H__

#include <glib-object.h>
#include <mex/mex-model.h>
#include <mex/mex-model-manager.h>

G_BEGIN_DECLS

#define MEX_TYPE_MODEL_PROVIDER (mex_model_provider_get_type ())

#define MEX_MODEL_PROVIDER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                     \
                               MEX_TYPE_MODEL_PROVIDER, \
                               MexModelProvider))

#define MEX_IS_MODEL_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_MODEL_PROVIDER))

#define MEX_MODEL_PROVIDER_IFACE(iface)                 \
  (G_TYPE_CHECK_CLASS_CAST ((iface),                      \
                            MEX_TYPE_MODEL_PROVIDER,    \
                            MexModelProviderInterface))

#define MEX_IS_MODEL_PROVIDER_IFACE(iface) \
  (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_MODEL_PROVIDER))

#define MEX_MODEL_PROVIDER_GET_IFACE(obj)                     \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj),                        \
                                  MEX_TYPE_MODEL_PROVIDER,    \
                                  MexModelProviderInterface))

typedef struct _MexModelProvider          MexModelProvider;
typedef struct _MexModelProviderInterface MexModelProviderInterface;

struct _MexModelProviderInterface
{
  GTypeInterface g_iface;

  /* virtual functions */
  const GList * (*get_models)      (MexModelProvider *provider);
  gboolean      (*model_activated) (MexModelProvider *provider,
                                    MexModel         *model);

  /* signals */
  void (* present_model)        (MexModelProvider *provider,
                                 MexModelInfo     *model_info);
};

GType   mex_model_provider_get_type       (void) G_GNUC_CONST;

const GList * mex_model_provider_get_models (MexModelProvider *provider);

gboolean mex_model_provider_model_activated (MexModelProvider *provider,
                                             MexModel         *model);

void mex_model_provider_present_model (MexModelProvider *provider,
                                       MexModelInfo     *model_info);

G_END_DECLS

#endif /* __MEX_MODEL_PROVIDER_H__ */
