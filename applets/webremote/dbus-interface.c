/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright 2009, 2010 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 */


#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>
#include "dbus-interface.h"

void
httpdbus_send_keyvalue (HTTPDBusInterface *dbus_interface, gint keyval)
{
  /* we're using sync at the moment because we don't have a queue for events
   * yet.
   */
  GError *error=NULL;

  g_dbus_proxy_call_sync (dbus_interface->proxy,
                          "ControlKey",
                          g_variant_new ("(u)", keyval),
                          0,
                          -1,
                          NULL,
                          &error);
  if (error)
    {
      g_warning ("Problem calling ControlKey: %s", error->message);
      g_error_free (error);
    }
}


HTTPDBusInterface *httpdbus_interface_new (void)
{
  HTTPDBusInterface *dbus_interface;

  dbus_interface = g_new0 (HTTPDBusInterface, 1);
  GError *error=NULL;

  dbus_interface->connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                                               NULL, &error);
  if (error)
    {
      g_warning ("Failed to connect to dbus %s\n",
               error->message ? error->message : "Unknown");
      g_error_free (error);
    }
  else
    {
      dbus_interface->proxy =
        g_dbus_proxy_new_sync (dbus_interface->connection,
                               G_DBUS_PROXY_FLAGS_NONE,
                               NULL,
                               "com.meego.mex.inputctrl",
                               "/com/meego/mex/InputControl",
                               "com.meego.mex.Input",
                               NULL,
                               &error);
      if (error)
        {
          g_warning ("Could not create dbus proxy: %s", error->message);
          g_error_free (error);
        }
    }
  return dbus_interface;
}


void httpdbus_interface_free (HTTPDBusInterface *dbus_interface)
{
  g_object_unref (dbus_interface->connection);
  g_object_unref (dbus_interface->proxy);
  g_free (dbus_interface);
}
