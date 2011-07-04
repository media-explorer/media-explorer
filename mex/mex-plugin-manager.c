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


#include "mex-plugin-manager.h"
#include "mex-marshal.h"
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>

G_DEFINE_TYPE (MexPluginManager, mex_plugin_manager, G_TYPE_OBJECT)

#define PLUGIN_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_PLUGIN_MANAGER, MexPluginManagerPrivate))

enum
{
  PROP_0,

  PROP_SEARCH_PATHS
};

enum
{
  PLUGIN_LOADED,

  LAST_SIGNAL
};

struct _MexPluginManagerPrivate
{
  gchar      **search_paths;
  GHashTable  *plugins;
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
mex_plugin_manager_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MexPluginManagerPrivate *priv = MEX_PLUGIN_MANAGER (object)->priv;

  switch (property_id)
    {
    case PROP_SEARCH_PATHS:
      g_value_set_pointer (value, priv->search_paths);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_plugin_manager_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MexPluginManagerPrivate *priv = MEX_PLUGIN_MANAGER (object)->priv;

  switch (property_id)
    {
    case PROP_SEARCH_PATHS:
      g_strfreev (priv->search_paths);
      priv->search_paths = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_plugin_manager_dispose (GObject *object)
{
  MexPluginManagerPrivate *priv = MEX_PLUGIN_MANAGER (object)->priv;

  if (priv->plugins)
    {
      g_hash_table_unref (priv->plugins);
      priv->plugins = NULL;
    }

  G_OBJECT_CLASS (mex_plugin_manager_parent_class)->dispose (object);
}

static void
mex_plugin_manager_finalize (GObject *object)
{
  MexPluginManagerPrivate *priv = MEX_PLUGIN_MANAGER (object)->priv;

  g_strfreev (priv->search_paths);

  G_OBJECT_CLASS (mex_plugin_manager_parent_class)->finalize (object);
}

static void
mex_plugin_manager_load_plugin (MexPluginManager *manager,
                                const gchar      *filename)
{
  GModule *module;
  gpointer symbol;
  GObject *plugin;
  GType plugin_type;
  gchar *plugin_name;
  gchar *plugin_suffix;

  MexPluginManagerPrivate *priv = manager->priv;

  /* Extract the plugin name */
  plugin_name = g_path_get_basename (filename);
  if ((plugin_suffix = g_strrstr (plugin_name, ".")))
    *plugin_suffix = '\0';

  /* Check if plugin already exists */
  if (g_hash_table_lookup (priv->plugins, plugin_name))
    {
      g_free (plugin_name);
      return;
    }

  module = g_module_open (filename, G_MODULE_BIND_LOCAL);
  if (!module)
    {
      g_warning (G_STRLOC ": Error opening module: %s",
                 g_module_error ());
      g_free (plugin_name);
      return;
    }

  if (!g_module_symbol (module, "mex_get_plugin_type", &symbol))
    {
      g_warning (G_STRLOC ": Unable to get symbol 'mex_get_plugin_type': %s",
                 g_module_error ());
      g_module_close (module);
      g_free (plugin_name);
      return;
    }

  plugin_type = (*(MexPluginGetTypeFunc)symbol)();

  if (!plugin_type)
    {
      g_warning (G_STRLOC ": Plugin '%s' didn't return a type", plugin_name);
      g_module_close (module);
      g_free (plugin_name);
      return;
    }

  /* Unloading modules usually has bad effects - don't allow it */
  g_module_make_resident (module);

  plugin = g_object_new (plugin_type, NULL);

  g_hash_table_insert (priv->plugins,
                       plugin_name,
                       plugin);

  g_signal_emit (manager, signals[PLUGIN_LOADED], 0, plugin);
}

static void
mex_plugin_manager_class_init (MexPluginManagerClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexPluginManagerPrivate));

  object_class->get_property = mex_plugin_manager_get_property;
  object_class->set_property = mex_plugin_manager_set_property;
  object_class->dispose = mex_plugin_manager_dispose;
  object_class->finalize = mex_plugin_manager_finalize;

  pspec = g_param_spec_pointer ("search-paths",
                                "Search paths",
                                "Paths to look in for plugins.",
                                G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SEARCH_PATHS, pspec);

  signals[PLUGIN_LOADED] =
    g_signal_new ("plugin-loaded",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexPluginManagerClass, plugin_loaded),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_OBJECT);
}

static void
mex_plugin_manager_init (MexPluginManager *self)
{
  MexPluginManagerPrivate *priv = self->priv = PLUGIN_MANAGER_PRIVATE (self);

  priv->search_paths = g_new0 (gchar *, 4);
  priv->search_paths[0] = g_strdup (PKGLIBDIR "/plugins");
  priv->search_paths[1] = g_build_filename (g_get_user_config_dir (),
                                            "mex",
                                            "plugins",
                                            NULL);
  priv->search_paths[2] = g_strdup (getenv ("MEX_PLUGIN_PATH"));


  priv->plugins = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         (GDestroyNotify)g_free,
                                         (GDestroyNotify)g_object_unref);
}

MexPluginManager *
mex_plugin_manager_get_default (void)
{
  static MexPluginManager *manager = NULL;

  if (!manager)
    manager = g_object_new (MEX_TYPE_PLUGIN_MANAGER, NULL);

  return manager;
}

GList *
mex_plugin_manager_get_plugins (MexPluginManager *manager)
{
  g_return_val_if_fail (MEX_IS_PLUGIN_MANAGER (manager), NULL);
  return g_hash_table_get_values (manager->priv->plugins);
}

void
mex_plugin_manager_refresh (MexPluginManager *manager)
{
  gint i;
  MexPluginManagerPrivate *priv = manager->priv;

  g_return_if_fail (MEX_IS_PLUGIN_MANAGER (manager));

  for (i = 0; priv->search_paths[i]; i++)
    {
      GDir *dir;
      const gchar *file;
      GList *files = NULL;
      GError *error = NULL;

      if (!g_file_test (priv->search_paths[i], G_FILE_TEST_IS_DIR))
        continue;

      if (!(dir = g_dir_open (priv->search_paths[i], 0, &error)))
        {
          g_warning (G_STRLOC ": Couldn't open directory: %s", error->message);
          g_error_free (error);
          continue;
        }

      /* Build the sorted plugin list */
      files = NULL;
      while ((file = g_dir_read_name (dir)))
        {
          gchar *full_file;

          if (!g_str_has_suffix (file, ".so") &&
              !g_str_has_suffix (file, ".la"))
            continue;

          full_file = g_build_filename (priv->search_paths[i], file, NULL);
          files = g_list_insert_sorted (files, full_file,
                                        (GCompareFunc)g_strcmp0);
        }

      g_dir_close (dir);

      /* Load the plugins */
      while (files)
        {
          gchar *full_file = files->data;

          /* Try to load plugin */
          mex_plugin_manager_load_plugin (manager, full_file);
          g_free (full_file);

          files = g_list_delete_link (files, files);
        }
    }
}

