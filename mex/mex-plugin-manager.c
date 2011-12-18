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

#include <string.h>
#include <stdlib.h>

#include <gmodule.h>

#include "mex-marshal.h"
#include "mex-plugin.h"
#include "mex-plugin-manager.h"

G_DEFINE_TYPE (MexPluginManager, mex_plugin_manager, G_TYPE_OBJECT)

#define PLUGIN_MANAGER_PRIVATE(o)                         \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                      \
                                MEX_TYPE_PLUGIN_MANAGER,  \
                                MexPluginManagerPrivate))

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
  GList       *descriptions;
  GHashTable  *plugins;
};

static guint signals[LAST_SIGNAL] = { 0, };

/* Build the list of paths to search for plugins */
#ifdef G_OS_WIN32

#define PLUGIN_EXTENSION ".dll"

static gchar **
build_plugin_search_paths (void)
{
  gchar **search_paths;
  gchar *mex_directory;

  mex_directory = g_win32_get_package_installation_directory_of_module (NULL);
  search_paths = g_new0 (gchar *, 3);
  search_paths[0] = g_build_filename (mex_directory,
                                      "lib", PACKAGE_NAME, "plugins",
                                      NULL);
  g_free (mex_directory);
  search_paths[1] = g_strdup (getenv ("MEX_PLUGIN_PATH"));

  return search_paths;
}
#else

#define PLUGIN_EXTENSION ".so"

static gchar **
build_plugin_search_paths (void)
{
  gchar **search_paths;

  search_paths = g_new0 (gchar *, 3);
  search_paths[0] = g_strdup (PKGLIBDIR "/plugins");
  search_paths[1] = g_strdup (getenv ("MEX_PLUGIN_PATH"));

  return search_paths;
}
#endif

/*
 * GObject implementation
 */

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

static gint
sort_by_prority (gconstpointer ap,
                 gconstpointer bp)
{
  const MexPluginDescription *a = ap;
  const MexPluginDescription *b = bp;

  return a->priority - b->priority;
}

static void
mex_plugin_manager_open_plugin (MexPluginManager *manager,
                                const gchar      *filename)
{
  GModule *module;
  gpointer symbol;
  MexPluginDescription *plugin_info;
  GType plugin_type;
  gchar *plugin_name;
  gchar *plugin_suffix;

  MexPluginManagerPrivate *priv = manager->priv;

  /* Extract the plugin name */
  plugin_name = g_path_get_basename (filename);
  if ((plugin_suffix = g_strrstr (plugin_name, ".")))
    *plugin_suffix = '\0';

  module = g_module_open (filename, G_MODULE_BIND_LOCAL);
  if (!module)
    {
      g_warning (G_STRLOC ": Error opening module: %s",
                 g_module_error ());
      g_free (plugin_name);
      return;
    }

  if (!g_module_symbol (module, "mex_plugin_info", &symbol))
    {
      g_warning (G_STRLOC ": Unable to get symbol 'mex_plugin_info': %s",
                 g_module_error ());
      g_module_close (module);
      g_free (plugin_name);
      return;
    }

  plugin_info = (MexPluginDescription *) symbol;
  /* Check if plugin is already loaded */
  if (g_hash_table_lookup (priv->plugins, plugin_info))
    {
      g_module_close (module);
      g_free (plugin_name);
      return;
    }

  plugin_type = plugin_info->get_type ();
  if (!plugin_type)
    {
      g_warning (G_STRLOC ": Plugin '%s' didn't return a type", plugin_name);
      g_module_close (module);
      g_free (plugin_name);
      return;
    }

  /* Unloading modules usually has bad effects - don't allow it */
  g_module_make_resident (module);

  priv->descriptions = g_list_insert_sorted (priv->descriptions, plugin_info,
                                             sort_by_prority);
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

  priv->search_paths = build_plugin_search_paths ();

  priv->plugins = g_hash_table_new (g_direct_hash, g_direct_equal);
}

MexPluginManager *
mex_plugin_manager_get_default (void)
{
  static MexPluginManager *manager = NULL;

  if (!manager)
    manager = g_object_new (MEX_TYPE_PLUGIN_MANAGER, NULL);

  return manager;
}

void
mex_plugin_manager_refresh (MexPluginManager *manager)
{
  MexPluginManagerPrivate *priv = manager->priv;
  GList *l;
  gint i;

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

          if (!g_str_has_suffix (file, PLUGIN_EXTENSION))
            continue;

          full_file = g_build_filename (priv->search_paths[i], file, NULL);
          files = g_list_prepend (files, full_file);
        }

      g_dir_close (dir);

      /* Load the plugins descriptions */
      while (files)
        {
          gchar *full_file = files->data;

          mex_plugin_manager_open_plugin (manager, full_file);
          g_free (full_file);

          files = g_list_delete_link (files, files);
        }
    }

  /* in open_plugins() we inserted PluginDescriptions in prioriy order in
   * priv->descriptions, time to load the plugins */
  for (l = priv->descriptions; l; l = g_list_next (l))
    {
      MexPluginDescription *desc = l->data;
      GType plugin_type;
      GObject *plugin;

      /* already loaded */
      if (g_hash_table_lookup (priv->plugins, desc))
        continue;

      plugin_type = desc->get_type ();
      plugin = g_object_new (plugin_type, NULL);
      g_hash_table_insert (priv->plugins, desc, plugin);

      /*
       * MexPlugin has been introduced late-ish. For those plugin we keep the
       * same semantics than before (for now™) ie. the code to initialize the
       * plugin is called at creation time. Later™ we could decouple the
       * instanciation of the plugin object itslef with the work to start doing
       * something.
       */
      if (MEX_IS_PLUGIN (plugin))
	mex_plugin_start (MEX_PLUGIN (plugin));

      g_signal_emit (manager, signals[PLUGIN_LOADED], 0, plugin);
    }
}
