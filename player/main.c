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

#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <gst/gst.h>
#include <stdlib.h>

#include <mex/mex.h>
#include <mex/mex-media-dbus-bridge.h>
#include <mex/mex-surface-player.h>

int
main (int    argc,
      char **argv)
{
  MexMediaDBUSBridge *bridge;
  MexSurfacePlayer *media;
  GMainLoop *loop;
  GError *error = NULL;

  g_thread_init (NULL);
  gst_init (&argc, &argv);
  mex_init (&argc, &argv);

  media = mex_surface_player_new ();
  bridge = mex_media_dbus_bridge_new (CLUTTER_MEDIA (media));
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
