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


#ifndef __MEX_ACTION_PROVIDER_H__
#define __MEX_ACTION_PROVIDER_H__

#include <glib-object.h>
#include <mex/mex-action-manager.h>

G_BEGIN_DECLS

#define MEX_TYPE_ACTION_PROVIDER (mex_action_provider_get_type ())

#define MEX_ACTION_PROVIDER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                     \
                               MEX_TYPE_ACTION_PROVIDER, \
                               MexActionProvider))

#define MEX_IS_ACTION_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_ACTION_PROVIDER))

#define MEX_ACTION_PROVIDER_IFACE(iface)                 \
  (G_TYPE_CHECK_CLASS_CAST ((iface),                      \
                            MEX_TYPE_ACTION_PROVIDER,    \
                            MexActionProviderInterface))

#define MEX_IS_ACTION_PROVIDER_IFACE(iface) \
  (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_ACTION_PROVIDER))

#define MEX_ACTION_PROVIDER_GET_IFACE(obj)                     \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj),                        \
                                  MEX_TYPE_ACTION_PROVIDER,    \
                                  MexActionProviderInterface))

typedef struct _MexActionProvider          MexActionProvider;
typedef struct _MexActionProviderInterface MexActionProviderInterface;

struct _MexActionProviderInterface
{
  GTypeInterface g_iface;

  /* virtual functions */
  const GList * (*get_actions)   (MexActionProvider *provider);
};

GType   mex_action_provider_get_type       (void) G_GNUC_CONST;

const GList * mex_action_provider_get_actions (MexActionProvider *provider);

G_END_DECLS

#endif /* __MEX_ACTION_PROVIDER_H__ */
