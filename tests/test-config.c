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

#include <mex.h>

int
main(int argc, const char *argv[])
{
  MexSettings *settings;
  gchar *file;

  if (argc != 2)
    {
      g_print ("Usage: %s file\n", argv[0]);
      return 1;
    }

  mex_init (NULL, NULL);

  settings = mex_settings_get_default ();
  file = mex_settings_find_config_file (settings, argv[1]);

  if (file)
    g_print ("found: %s\n", file);
  else
    g_print ("not found: %s\n", argv[1]);

  return 0;
}
