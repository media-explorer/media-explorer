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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include "countries.h"
#include "mex-dvb-scanner.h"

#define MEX_DVB_SCANNER_INTERFACE_NAME  "org.MediaExplorer.DVB.Scanner"
#define MEX_DVB_SCANNER_OBJECT_PATH     "/org/MediaExplorer/DVB/Scanner"

extern int w_scan_main (int argc, char **argv);

typedef struct
{
  GMutex *lock;        /* this structure is shared between the w_scan thread and
                         the daemon thread */

  GMainLoop *loop;
  GThread   *w_scan_thread;

  GDBusConnection *dbus_connection;
  GDBusNodeInfo *introspection_data;

  guint scan_in_progress : 1;
  gint n_transponders, transponder;
  gint n_frequencies, frequency;
  gchar *country;
  gchar *error_message;

  guint progress_idle;
} Scanner;

static Scanner mex_dvb_scanner;

/*
 * methods called from the w_scan thread
 */

/* to call with the scanner lock held */
static double
get_progress (Scanner *scanner)
{
  /* 2nd phase, where we have the number of transponders to scan for further
   * information */
  if (scanner->n_transponders)
    return (double) (scanner->transponder - 1) / (scanner->n_transponders - 1);
  else if (scanner->n_frequencies)
    return (double) (scanner->frequency - 1) / (scanner->n_frequencies - 1);
  else
    return 0.0;
}

static gboolean
report_progress_main_context (gpointer data)
{
  Scanner *scanner = data;
  GError *local_error = NULL;
  GVariantBuilder *builder, *invalidated_builder;
  double progress;

  g_mutex_lock (scanner->lock);
  progress = get_progress (scanner);
  g_mutex_unlock (scanner->lock);

  builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
  invalidated_builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  g_variant_builder_add (builder,
                         "{sv}",
                         "progress",
                         g_variant_new_double (progress));
  g_dbus_connection_emit_signal (scanner->dbus_connection,
                                 NULL,
                                 MEX_DVB_SCANNER_OBJECT_PATH,
                                 "org.freedesktop.DBus.Properties",
                                 "PropertiesChanged",
                                 g_variant_new ("(sa{sv}as)",
                                                MEX_DVB_SCANNER_INTERFACE_NAME,
                                                builder,
                                                invalidated_builder),
                                 &local_error);
  if (local_error)
    {
      g_warning ("Could not send error signal: %s", local_error->message);
      g_error_free (local_error);
    }

  g_mutex_lock (scanner->lock);
  scanner->progress_idle = 0;
  g_mutex_unlock (scanner->lock);
  return FALSE;
}

void
mex_dvb_scanner_set_n_transponders (n_transponders)
{
  Scanner *scanner = &mex_dvb_scanner;

  g_mutex_lock (scanner->lock);
  scanner->n_transponders = n_transponders;
  scanner->transponder = 0 ;
  g_mutex_unlock (scanner->lock);
}

void
mex_dvb_scanner_next_transponder (void)
{
  Scanner *scanner = &mex_dvb_scanner;

  g_mutex_lock (scanner->lock);
  scanner->transponder++;
  /* compress progress DBus signals */
  if (scanner->progress_idle == 0)
    scanner->progress_idle = g_idle_add (report_progress_main_context, scanner);
  g_mutex_unlock (scanner->lock);
}

void
mex_dvb_scanner_next_frequency (void)
{
  Scanner *scanner = &mex_dvb_scanner;

  g_mutex_lock (scanner->lock);
  scanner->frequency++;
  /* compress progress DBus signals */
  if (scanner->progress_idle == 0)
    scanner->progress_idle = g_idle_add (report_progress_main_context, scanner);
  g_mutex_unlock (scanner->lock);
}

void
mex_dvb_scanner_set_n_frequencies (int n_frequencies)
{
  Scanner *scanner = &mex_dvb_scanner;

  g_mutex_lock (scanner->lock);
  scanner->n_frequencies = n_frequencies;
  scanner->frequency = 0 ;
  g_mutex_unlock (scanner->lock);
}

static gboolean
report_error_main_context (gpointer data)
{
  Scanner *scanner = data;
  GError *local_error = NULL;

  g_mutex_lock (scanner->lock);
  if (scanner->error_message == NULL)
    {
      g_warning ("Was asked to report an error without an error message");
      goto out;
    }

  g_dbus_connection_emit_signal (scanner->dbus_connection,
                                 NULL,
                                 MEX_DVB_SCANNER_OBJECT_PATH,
                                 MEX_DVB_SCANNER_INTERFACE_NAME,
                                 "Error",
                                 g_variant_new ("(s)", scanner->error_message),
                                 &local_error);
  if (local_error)
    g_warning ("Could not send error signal: %s", local_error->message);

out:
  g_mutex_unlock (scanner->lock);

  g_main_loop_quit (scanner->loop);

  return FALSE;
}

void
mex_dvb_scanner_report_error (const gchar *fmt,
                              ...)
{
  Scanner *scanner = &mex_dvb_scanner;
  va_list args;

  g_mutex_lock (scanner->lock);

  g_free (scanner->error_message);
  va_start (args, fmt);
  scanner->error_message = g_strdup_vprintf (fmt, args);
  va_end (args);

  scanner->scan_in_progress = FALSE;

  g_idle_add (report_error_main_context, scanner);

  g_mutex_unlock (scanner->lock);

  g_thread_exit (NULL);
}

static const gchar *
get_config_dir (void)
{
  GFile *config_dir_file;
  GError *error = NULL;
  static gchar *config_dir = NULL;

  if (config_dir)
    return config_dir;

  config_dir = g_build_filename (g_get_user_config_dir (), "mex", "dvb", NULL);

  /* create the configuration directory if needed */
  config_dir_file = g_file_new_for_path (config_dir);
  g_file_make_directory_with_parents (config_dir_file, NULL, &error);
  g_object_unref (config_dir_file);
  if (error && error->code != G_IO_ERROR_EXISTS)
    {
      g_critical ("Could not create config directory %s: %s",
                  config_dir, error->message);
      g_clear_error (&error);
      return NULL;
    }
  else
    {
      /* directory already exists */
      g_clear_error (&error);
    }

  return config_dir;
}

static gboolean
exit_daemon (gpointer data)
{
  Scanner *scanner = data;

  g_main_loop_quit (scanner->loop);

  return FALSE;
}

static gpointer
scanning_thread_main (gpointer data)
{
  Scanner *scanner = data;
  gchar *output;
  gchar *argv[] = { "w_scan",
                     "-G",
                     "-E", "0",           /* no scrambled channels */
                     "-w", NULL,          /* output file */
                     NULL, NULL};         /* country */
  gint argc = 6;

  argv[5] = g_build_filename (get_config_dir (), "channels.conf", NULL);

  if (scanner->country)
    {
      argv[argc++] = "-c";
      argv[argc++] = scanner->country;
    }

  w_scan_main (argc, argv);

  g_free (argv[5]);

  g_timeout_add_seconds (10, exit_daemon, scanner);

  g_thread_exit (NULL);
}

static void
do_start_scan (Scanner *scanner)
{
  GError *error = NULL;

  scanner->scan_in_progress = TRUE;

  scanner->w_scan_thread = g_thread_create (scanning_thread_main,
                                            scanner,
                                            TRUE,         /* joinable */
                                            &error);
  if (error)
    {
      g_warning ("Could not create scanning thread: %s", error->message);
      g_error_free (error);
      return;
    }
}

static void
do_set_country (Scanner     *scanner,
                const gchar *country)
{
  g_mutex_lock (scanner->lock);
  if (scanner->scan_in_progress)
    {
      g_warning ("Called SetCountry() while a scan was in progress");
      goto out;
    }

  g_free (scanner->country);
  scanner->country = g_strdup (country);

out:
  g_mutex_unlock (scanner->lock);
}

/*
 * D-Bus
 */

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.MediaExplorer.DVB.Scanner'>"
  "    <method name='StartScan' />"
  "    <method name='SetCountry'>"
  "      <arg name='country' type='s' />"
  "    </method>"
  "    <property type='d' name='progress' access='read' />"
  "    <signal name='Error'>"
  "      <arg name='message' type='s' />"
  "    </signal>"
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
  Scanner *scanner = user_data;

  if (g_strcmp0 (method_name, "StartScan") == 0)
    {
      do_start_scan (scanner);
      g_dbus_method_invocation_return_value (invocation, NULL);
    }
  else if (g_strcmp0 (method_name, "SetCountry") == 0)
    {
      GVariant *arg;
      const gchar *country;

      arg = g_variant_get_child_value (parameters, 0);
      country = g_variant_get_string (arg, NULL);

      do_set_country (scanner, country);

      g_variant_unref (arg);
      g_dbus_method_invocation_return_value (invocation, NULL);
    }
  else
    {
      g_assert_not_reached ();
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
  Scanner *scanner = user_data;
  GVariant *ret = NULL;

  if (g_strcmp0 (property_name, "progress") == 0)
    {
      g_mutex_lock (scanner->lock);
      ret = g_variant_new_double (get_progress (scanner));
      g_mutex_unlock (scanner->lock);
    }

  return ret;
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
				       MEX_DVB_SCANNER_OBJECT_PATH,
				       scanner_interface,
				       &interface_vtable,
				       scanner,  /* user_data */
				       NULL,     /* user_data_free_func */
				       NULL);    /* GError** */
  g_assert (registration_id > 0);

  scanner->dbus_connection = connection;
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
  guint owner_id;
  Scanner *scanner;

  g_type_init ();
  g_thread_init (NULL);

  memset (&mex_dvb_scanner, 0, sizeof (Scanner));
  scanner = &mex_dvb_scanner;

  scanner->lock = g_mutex_new ();
  scanner->introspection_data = g_dbus_node_info_new_for_xml (introspection_xml,
                                                              NULL);
  g_assert (scanner->introspection_data != NULL);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.media-explorer.DVB.Scanner",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             scanner,
                             NULL);

  scanner->loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (scanner->loop);

  g_thread_join (scanner->w_scan_thread);

  g_mutex_free (scanner->lock);
  g_dbus_connection_flush_sync (scanner->dbus_connection, NULL, NULL);
  g_bus_unown_name (owner_id);

  return 0;
}
