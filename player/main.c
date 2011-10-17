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
# include "config.h"
#endif

#include <gst/gst.h>
#include <stdlib.h>

#include <mex/mex.h>
#include <mex/mex-media-dbus-bridge.h>
#include <mex/mex-surface-player.h>

static MexSurfacePlayer *media_player = NULL;

static ClutterMedia *
_get_media_player (void)
{
  return CLUTTER_MEDIA (media_player);
}

int
main (int    argc,
      char **argv)
{
  MexPluginManager *pmanager;
  MexMediaDBUSBridge *bridge;
  GMainLoop *loop;
  GError *error = NULL;
  gchar **plugin_directories;

  plugin_directories = g_new0 (gchar *, 2);
  plugin_directories[0] = g_strdup (MEX_PLAYER_PLUGIN_PATH);

  g_thread_init (NULL);

  gst_init (&argc, &argv);
  mex_init (&argc, &argv);

  /* Setup player and expose it to mex */
  media_player = mex_surface_player_new ();
  mex_player_set_media_player_callback (_get_media_player);

  bridge = mex_media_dbus_bridge_new (CLUTTER_MEDIA (media_player));

  /* Load player's plugins */
  pmanager = mex_plugin_manager_get_default ();
  g_object_set (G_OBJECT (pmanager), "search-paths", plugin_directories, NULL);
  mex_plugin_manager_refresh (pmanager);

  /* Export player's interface on d-bus */
  if (!mex_media_dbus_bridge_register (bridge, &error))
    {
      g_error ("Error registering on D-BUS: %s", error->message);
      g_clear_error (&error);
      return EXIT_SUCCESS;
    }

  loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  return EXIT_SUCCESS;
}
