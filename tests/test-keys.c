/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#include <mex/mex.h>

int
main (int argc, char **argv)
{
  GDBusProxy *proxy;
  GError *err = NULL;

  struct { guint32 k; gchar *n;} keys[] = {
        { CLUTTER_KEY_Left, "Left"},
        { CLUTTER_KEY_Right, "Right"},
        { CLUTTER_KEY_Up, "Up" },
        { CLUTTER_KEY_Down, "Down" },
        { CLUTTER_KEY_Escape, "Back" },
        { CLUTTER_KEY_Return, "Enter" },
        { CLUTTER_KEY_i, "Info" },
        { CLUTTER_KEY_Home, "Home" }
  };

  g_type_init ();

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         MEX_DBUS_NAME,
                                         "/org/MediaExplorer/Input",
                                         "org.MediaExplorer.Input",
                                         NULL, &err);
  if (err)
    g_error ("%s", err->message);

  while (1)
    {
      gint i = g_random_int_range (0, G_N_ELEMENTS(keys));
      printf ("%s\n", keys[i].n);
      g_dbus_proxy_call_sync (proxy, "ControlKey",
                              g_variant_new ("(u)", keys[i].k),
                              G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
      if (err)
        g_error ("%s", err->message);

      g_usleep (1e6 / 6);
    }

  return 0;
}
