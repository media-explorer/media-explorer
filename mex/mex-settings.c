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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>

#include "mex-settings.h"

G_DEFINE_TYPE (MexSettings, mex_settings, G_TYPE_OBJECT)

#define SETTINGS_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SETTINGS, MexSettingsPrivate))

struct _MexSettingsPrivate
{
  gchar *config_dir;
  gchar **config_search_path;
};


static void
mex_settings_finalize (GObject *object)
{
  MexSettings *settings = MEX_SETTINGS (object);
  MexSettingsPrivate *priv = settings->priv;

  g_free (priv->config_dir);
  g_free (priv->config_search_path);

  G_OBJECT_CLASS (mex_settings_parent_class)->finalize (object);
}

static void
mex_settings_class_init (MexSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexSettingsPrivate));

  object_class->finalize = mex_settings_finalize;
}

static void
mex_settings_init (MexSettings *self)
{
  self->priv = SETTINGS_PRIVATE (self);
}

MexSettings *
mex_settings_get_default (void)
{
  static MexSettings *singleton = NULL;

  if (G_LIKELY (singleton))
    return singleton;

  singleton = g_object_new (MEX_TYPE_SETTINGS, NULL);
  return singleton;
}

const gchar *
mex_settings_get_config_dir (MexSettings *settings)
{
  MexSettingsPrivate *priv;
  GFile *config_dir;
  GError *error = NULL;

  g_return_val_if_fail (MEX_IS_SETTINGS (settings), NULL);
  priv = settings->priv;

  if (priv->config_dir)
    return priv->config_dir;

  priv->config_dir = g_build_filename (g_get_user_config_dir (),
                                       "mex",
                                       NULL);

  /* create the configuration directory if needed */
  config_dir = g_file_new_for_path (priv->config_dir);
  g_file_make_directory_with_parents (config_dir, NULL, &error);
  g_object_unref (config_dir);
  if (error && error->code != G_IO_ERROR_EXISTS)
    {
      g_critical ("Could not create config directory %s: %s",
                  priv->config_dir, error->message);
      g_clear_error (&error);
      return NULL;
    }
  else
    {
      /* directory already exists */
      g_clear_error (&error);
    }

  return priv->config_dir;
}

gchar *
mex_settings_find_config_file (MexSettings *settings,
                               const gchar *config_file)
{
  MexSettingsPrivate *priv;
  gint i;

  g_return_val_if_fail (MEX_IS_SETTINGS (settings), NULL);
  priv = settings->priv;

  if (priv->config_search_path == NULL)
    {
      priv->config_search_path = g_malloc0 (3 * sizeof (gchar *));

      priv->config_search_path[0] =
        (gchar *) mex_settings_get_config_dir (settings);
      priv->config_search_path[1] = PKGSYSCONFDIR;
    }

  for (i = 0; priv->config_search_path[i]; i++)
    {
      gchar *file, *path;
      GFile *g_file;
      gboolean found;

      path = priv->config_search_path[i];
      file = g_build_filename (path, config_file, NULL);

      g_file = g_file_new_for_path (file);

      found = g_file_query_exists (g_file, NULL);
      g_object_unref (g_file);

      if (found)
        return file;

      g_free (file);
    }

  return NULL;
}
