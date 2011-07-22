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

#include "dbus-service.h"
#include "gio/gio.h"
#include "webremote.h"
#include "mex-webremote.h"

static const gchar introspection_xml[] =
"<node>"
" <interface name ='"MEX_WEBREMOTE_DBUS_INTERFACE"' >"
"  <method name='Quit'>"
"  </method>"
" </interface>"
"</node>";

static void
_method_cb (GDBusConnection *connection,
            const gchar *sender,
            const gchar *object_path,
            const gchar *interface_name,
            const gchar *method_name,
            GVariant *parameters,
            GDBusMethodInvocation *invocation,
            gpointer user_data)
{
  if (g_strcmp0 (method_name, "Quit") == 0)
    mex_webremote_quit ();

  g_dbus_method_invocation_return_value (invocation, NULL);
}


static const GDBusInterfaceVTable interface_table =
{
  (GDBusInterfaceMethodCallFunc) _method_cb,
  NULL,
  NULL
};

static void
_bus_acquired (GDBusConnection *connection,
               const gchar *name,
               gpointer userdata)
{
  GDBusNodeInfo *introspection_data;
  GError *error=NULL;

  introspection_data =
    g_dbus_node_info_new_for_xml (introspection_xml, NULL);

  g_dbus_connection_register_object (connection,
                                     MEX_WEBREMOTE_DBUS_PATH,
                                     introspection_data->interfaces[0],
                                     &interface_table,
                                     NULL, /* user data */
                                     NULL,
                                     &error);

  g_dbus_node_info_unref (introspection_data);

  if (error)
    {
      g_warning ("Problem registering object on dbus %s", error->message);
      g_error_free (error);
    }
}

gint
dbus_service_start ()
{
  gint owner_id;

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             MEX_WEBREMOTE_DBUS_SERVICE,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             (GBusAcquiredCallback) _bus_acquired,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
  return owner_id;
}

void
dbus_service_stop (gint owner_id)
{
  g_bus_unown_name (owner_id);
}

