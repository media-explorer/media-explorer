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

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gio.h>

typedef struct
{
  GDBusProxy *proxy;
  GMainLoop *loop;

} TestDvbScanner;

static void
on_dbus_signal_received (GDBusProxy     *proxy,
                         gchar          *sender_name,
                         gchar          *signal_name,
                         GVariant       *parameters,
                         TestDvbScanner *scanner)
{
  if (g_strcmp0 (signal_name, "Error") == 0)
    {
      GVariant *arg;
      const gchar *message;

      arg = g_variant_get_child_value (parameters, 0);
      message = g_variant_get_string (arg, NULL);
      g_variant_unref (arg);

      g_print ("*** Received an error from the DVB scanner daemon:\n%s",
               message);
    }
}

static void
do_start_scan (TestDvbScanner *scanner)
{
  GError *error = NULL;

  g_dbus_proxy_call_sync (scanner->proxy,
                          "StartScan",
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          &error);
  if (error)
    g_warning ("Could not call StartScan(): %s", error->message);
}

static void
handle_key (TestDvbScanner *scanner,
            gchar           key)
{
  switch (key)
    {
    case 'q':
      g_main_loop_quit (scanner->loop);
      break;
    case 's':
      do_start_scan (scanner);
      break;
    }
}

static gboolean
on_stdin_new_data (GIOChannel   *input,
                   GIOCondition  condition,
                   gpointer      data)
{
  TestDvbScanner *scanner = data;
  GError *error = NULL;
  gchar keys[9];
  gsize bytes_read = 0;
  guint i;

  g_io_channel_read_chars (input, keys, sizeof (keys) - 1, &bytes_read, &error);
  if (error)
    {
      g_error ("Could not read from stdin: %s", error->message);
      g_main_loop_quit (scanner->loop);
      return FALSE;
    }

  keys[bytes_read] = '\0';
  for (i = 0; i < bytes_read; i++)
    handle_key (scanner, keys[i]);

  return TRUE;
}

int
main (int argc, char **argv)
{
  TestDvbScanner scanner;
  GIOChannel *input;
  GIOFlags input_flags;
  GError *err = NULL;
  struct termios ternimal_ios, scanner_ios;

  memset (&scanner, 0, sizeof (TestDvbScanner));
  g_type_init ();

  scanner.proxy =
    g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   NULL,
                                   "org.media-explorer.DVB.Scanner",
                                   "/org/MediaExplorer/DVB/Scanner",
                                   "org.MediaExplorer.DVB.Scanner",
                                   NULL, &err);
  if (err)
    g_error ("%s", err->message);

  g_signal_connect (scanner.proxy, "g-signal",
                    G_CALLBACK (on_dbus_signal_received), &scanner);

  scanner.loop = g_main_loop_new (NULL, FALSE);

  tcgetattr (STDIN_FILENO, &ternimal_ios);
  memcpy (&scanner_ios, &ternimal_ios, sizeof (scanner_ios));
  scanner_ios.c_lflag &=  ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT |
                            ECHOKE | ICRNL);
  tcsetattr (STDIN_FILENO, TCSANOW, &scanner_ios);

  input = g_io_channel_unix_new (0);
  g_io_channel_set_encoding (input, NULL, NULL);
  input_flags = g_io_channel_get_flags (input);
  g_io_channel_set_flags (input, input_flags | G_IO_FLAG_NONBLOCK, NULL );
  g_io_channel_set_buffered (input, FALSE);
  g_io_add_watch (input, G_IO_IN, on_stdin_new_data, &scanner);

  g_main_loop_run (scanner.loop);

  tcsetattr (STDIN_FILENO, TCSANOW, &ternimal_ios);

  return 0;
}
