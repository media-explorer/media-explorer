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

#include <string.h>
#include <glib.h>
#include <gio/gio.h>

extern int w_scan_main (int argc, char **argv);

typedef struct
{
  GDBusNodeInfo *introspection_data;
} Scanner;

static void
do_scan (void)
{
  gchar *argv[] = { "w_scan" };

  w_scan_main (1, argv);
}

/*
 * D-Bus
 */

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.MediaExplorer.DVB.Scanner'>"
  "    <method name='StartScan' />"
  "  </interface>"
  "</node>";

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  if (g_strcmp0 (method_name, "Scan") == 0)
    {
      do_scan();
      g_dbus_method_invocation_return_value (invocation, NULL);
    }
}

static GVariant *
handle_get_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GError          **error,
                     gpointer          user_data)
{
  return NULL;
}

static gboolean
handle_set_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GVariant         *value,
                     GError          **error,
                     gpointer          user_data)
{


  return TRUE;
}

static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  handle_get_property,
  handle_set_property
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  Scanner *scanner = user_data;
  GDBusInterfaceInfo *scanner_interface;
  guint registration_id;

  scanner_interface = scanner->introspection_data->interfaces[0];
  registration_id =
    g_dbus_connection_register_object (connection,
				       "/org/MediaExplorer/DVB/Scanner",
				       scanner_interface,
				       &interface_vtable,
				       NULL,  /* user_data */
				       NULL,  /* user_data_free_func */
				       NULL); /* GError** */
  g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
}

int
main (int argc, char *argv[])
{
  Scanner scanner;
  guint owner_id;
  GMainLoop *loop;

  g_type_init ();

  memset (&scanner, 0, sizeof (Scanner));

  scanner.introspection_data = g_dbus_node_info_new_for_xml (introspection_xml,
							     NULL);
  g_assert (scanner.introspection_data != NULL);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.media-explorer.DVB.Scanner",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             &scanner,
                             NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  g_bus_unown_name (owner_id);

  return 0;
}
