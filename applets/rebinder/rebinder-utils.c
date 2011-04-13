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

#include <glib.h>
#include <gio/gio.h>

#include "rebinder.h"

const gchar *
rebinder_get_config_dir (void)
{
  static gchar *path = NULL;
  GFile *config_dir;
  GError *error = NULL;

  if (path)
    return path;

  path = g_build_filename (g_get_user_config_dir (), "mex", NULL);

  /* create the configuration directory if needed */
  config_dir = g_file_new_for_path (path);
  g_file_make_directory_with_parents (config_dir, NULL, &error);
  g_object_unref (config_dir);
  if (error && error->code != G_IO_ERROR_EXISTS)
    {
      g_critical ("Could not create config directory %s: %s",
                  path, error->message);
      g_clear_error (&error);
      return NULL;
    }

  return path;
}
