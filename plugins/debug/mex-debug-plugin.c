/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dlfcn.h>

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <mex/mex-tool-provider.h>

#include "mex-debug-plugin.h"
#include "mex-gobject-list.h"

static void mex_tool_provider_iface_init (MexToolProviderInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MexDebugPlugin,
                         mex_debug_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_TOOL_PROVIDER,
                                                mex_tool_provider_iface_init))

#define GET_PRIVATE(o)                                  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                    \
                                MEX_TYPE_DEBUG_PLUGIN,  \
                                MexDebugPluginPrivate))

typedef struct
{
  void    (* toggle_verbose) (void);
  GList * (* get_summary)    (void);
  void    (* free_summary)   (GList *tuples);
} GObjectListSyms;

struct _MexDebugPluginPrivate
{
  GObjectListSyms gobject_list;

  GList *bindings;
};

static gboolean
init_gobject_list (MexDebugPlugin *plugin,
                   void           *dlhandle)
{
  MexDebugPluginPrivate *priv = plugin->priv;

  priv->gobject_list.toggle_verbose = dlsym (dlhandle,
                                             "gobject_list_toggle_verbose");
  priv->gobject_list.get_summary = dlsym (dlhandle,
                                             "gobject_list_get_summary");
  priv->gobject_list.free_summary = dlsym (dlhandle,
                                           "gobject_list_free_summary");

  return priv->gobject_list.toggle_verbose != NULL;
}

static gboolean
do_verbose (GObject             *instance,
            const gchar         *action_name,
            guint                key_val,
            ClutterModifierType  modifiers,
            gpointer             user_data)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (user_data);
  MexDebugPluginPrivate *priv = plugin->priv;

  priv->gobject_list.toggle_verbose ();

  return TRUE;
}

static gint
tuplecmp (gconstpointer pa,
          gconstpointer pb)
{
  const GObjectListTuple *a = pa, *b = pb;

  return b->value - a->value;
}

static gboolean
do_list (GObject             *instance,
         const gchar         *action_name,
         guint                key_val,
         ClutterModifierType  modifiers,
         gpointer             user_data)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (user_data);
  MexDebugPluginPrivate *priv = plugin->priv;
  GList *tuples, *l;
  gint i;

  tuples = priv->gobject_list.get_summary ();
  tuples = g_list_sort (tuples, tuplecmp);
  for (l = tuples, i = 0; l && i < 20; l = g_list_next (l), i++)
    {
      GObjectListTuple *tuple = l->data;;
      g_print ("%s: %d instances\n", tuple->str, tuple->value);
    }
  priv->gobject_list.free_summary (tuples);

  return TRUE;
}

/*
 * MexToolProvider implementation
 */

static const GList *
mex_debug_get_bindings (MexToolProvider *provider)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (provider);

  return plugin->priv->bindings;
}

static void
mex_tool_provider_iface_init (MexToolProviderInterface *iface)
{
  iface->get_bindings = mex_debug_get_bindings;
}

/*
 * GObject implementation
 */

static void
mex_debug_plugin_finalize (GObject *object)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (object);
  MexDebugPluginPrivate *priv = plugin->priv;

  while (priv->bindings)
    {
      g_free (priv->bindings->data);
      priv->bindings = g_list_delete_link (priv->bindings, priv->bindings);
    }

  G_OBJECT_CLASS (mex_debug_plugin_parent_class)->finalize (object);
}

static void
mex_debug_plugin_class_init (MexDebugPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexDebugPluginPrivate));

  object_class->finalize = mex_debug_plugin_finalize;
}

static void
append_binding (MexDebugPlugin *plugin,
                const gchar    *name,
                guint           key_val,
                GCallback       callback,
                gpointer        data)
{
  MexDebugPluginPrivate *priv = plugin->priv;
  MexToolProviderBinding *binding;

  binding = g_new0 (MexToolProviderBinding, 1);
  binding->action_name = name;
  binding->key_val = key_val;
  binding->modifiers = CLUTTER_CONTROL_MASK;
  binding->callback = callback;
  binding->data = data;

  priv->bindings = g_list_prepend (priv->bindings, binding);
}

static void
mex_debug_plugin_init (MexDebugPlugin *self)
{
  MexDebugPluginPrivate *priv;
  gboolean have_gobject_list;
  void *dlhandle;

  self->priv = priv = GET_PRIVATE (self);

  dlhandle = dlopen (NULL, RTLD_LAZY);
  if (dlhandle)
    {
      dlerror ();
      have_gobject_list = init_gobject_list (self, dlhandle);
    }

  if (have_gobject_list)
    {
      append_binding (self, "debug-verbose", CLUTTER_KEY_v,
                      G_CALLBACK (do_verbose), self);
      append_binding (self, "debug-list", CLUTTER_KEY_l,
                      G_CALLBACK (do_list), self);
    }
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_DEBUG_PLUGIN;
}
