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

/* Webremote settings */

#include "settings.h"
#include <mex/mex.h>

void
settings_load (MexWebRemote *webremote)
{
  gchar *conf_file_loc;
  GKeyFile *conf_file;

  conf_file_loc = mex_settings_find_config_file (mex_settings_get_default (),
                                             "mex-webremote.conf");
  if (conf_file_loc)
    {
      conf_file = g_key_file_new ();

      g_key_file_load_from_file (conf_file, conf_file_loc,
                                 G_KEY_FILE_NONE, NULL);
      g_free (conf_file_loc);

      if (conf_file)
        {
          webremote->opt_debug =
            g_key_file_get_boolean (conf_file, "settings", "debug", NULL);
          webremote->opt_port =
            g_key_file_get_integer (conf_file, "settings", "port", NULL);
          webremote->opt_interface =
            g_key_file_get_string (conf_file, "settings", "interface", NULL);
          webremote->opt_noauth =
            g_key_file_get_boolean (conf_file, "settings", "noauth", NULL);
          webremote->opt_auth =
            g_key_file_get_string (conf_file, "settings", "auth", NULL);

          g_key_file_free (conf_file);
        }
    }

  /* Setup the default port if nothing (correct) was set */
  if (webremote->opt_port <= 0)
    webremote->opt_port = 9090;

  /* Get the mex data directory where a bunch of assets are kept */
  webremote->mex_data_dir = mex_get_data_dir ();

  /* Fallback method */
  if (!webremote->mex_data_dir)
    webremote->mex_data_dir = MEXPKGDATADIR;
}
