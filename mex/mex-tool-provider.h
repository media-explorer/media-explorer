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


#ifndef __MEX_TOOL_PROVIDER_H__
#define __MEX_TOOL_PROVIDER_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_TOOL_PROVIDER (mex_tool_provider_get_type ())

#define MEX_TOOL_PROVIDER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                     \
                               MEX_TYPE_TOOL_PROVIDER, \
                               MexToolProvider))

#define MEX_IS_TOOL_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_TOOL_PROVIDER))

#define MEX_TOOL_PROVIDER_IFACE(iface)                 \
  (G_TYPE_CHECK_CLASS_CAST ((iface),                      \
                            MEX_TYPE_TOOL_PROVIDER,    \
                            MexToolProviderInterface))

#define MEX_IS_TOOL_PROVIDER_IFACE(iface) \
  (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_TOOL_PROVIDER))

#define MEX_TOOL_PROVIDER_GET_IFACE(obj)                     \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj),                        \
                                  MEX_TYPE_TOOL_PROVIDER,    \
                                  MexToolProviderInterface))

typedef struct _MexToolProvider          MexToolProvider;
typedef struct _MexToolProviderInterface MexToolProviderInterface;

typedef struct
{
  const gchar         *action_name;
  guint                key_val;
  ClutterModifierType  modifiers;
  GCallback            callback;
  gpointer             data;
  GDestroyNotify       notify;
} MexToolProviderBinding;

struct _MexToolProviderInterface
{
  GTypeInterface g_iface;

  /* virtual functions */
  const GList * (*get_tools)    (MexToolProvider *provider);
  const GList * (*get_bindings) (MexToolProvider *provider);

  /* signals */
  void (* present_actor)       (MexToolProvider *provider,
                                ClutterActor    *actor);
  void (* remove_actor)         (MexToolProvider *provider,
                                ClutterActor     *actor);
};

GType   mex_tool_provider_get_type       (void) G_GNUC_CONST;

const GList * mex_tool_provider_get_tools    (MexToolProvider *provider);
const GList * mex_tool_provider_get_bindings (MexToolProvider *provider);

void mex_tool_provider_present_actor (MexToolProvider *provider,
                                      ClutterActor    *actor);
void mex_tool_provider_remove_actor (MexToolProvider *provider,
                                     ClutterActor    *actor);

G_END_DECLS

#endif /* __MEX_TOOL_PROVIDER_H__ */
