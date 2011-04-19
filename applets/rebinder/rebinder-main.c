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
#include "config.h"
#endif

#include <signal.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gio.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "rebinder.h"
#include "rebinder-debug.h"
#include "mex-rebinder.h"

static Rebinder the_rebinder;

static gboolean opt_no_daemon = FALSE;
static gboolean opt_configure = FALSE;
static gboolean opt_no_evdev = FALSE;
static gboolean opt_no_fullscreen = FALSE;

static GOptionEntry entries[] =
{
  { "no-daemon", 'n', 0, G_OPTION_ARG_NONE, &opt_no_daemon,
    "Don't start daemonized", NULL },
  { "configure", 'c', 0, G_OPTION_ARG_NONE, &opt_configure,
    "Configure the bindings", NULL },
  { "no-evdev", 'E', 0, G_OPTION_ARG_NONE, &opt_no_evdev,
    "Do not listen to evdev devices", NULL },
  { "no-fullscreen", 'F', 0, G_OPTION_ARG_NONE, &opt_no_fullscreen,
    "Do not start full screen", NULL },
  { NULL }
};

static gboolean
is_evdev_enabled (void)
{
  return opt_no_evdev == FALSE;
}

static gboolean
is_fullscreen_enabled (void)
{
  return opt_no_fullscreen == FALSE;
}

static void
commit_bindings (Rebinder *rebinder)
{
  XSync(rebinder->dpy, False);
}

static Binding *
find_binding_by_keycode (Rebinder *rebinder,
                         gint      keycode)
{
  Binding *binding;
  gint i;

  for (i = 0; i < rebinder->evdev_bindings->len; i++)
    {
      binding = g_ptr_array_index (rebinder->evdev_bindings, i);

      if (binding->keycode == keycode)
        return binding;
    }

  return NULL;
}

static void
free_binding (gpointer data)
{
  Binding *binding = data;

  g_slice_free (Binding, binding);
}

static void
on_evdev_key_pressed (guint32  time_,
                      guint32  key,
                      guint32  state,
                      gpointer data)
{
  Rebinder *rebinder = data;
  Binding *binding;

  if (!state)
    {
      fakekey_release (rebinder->fake);
      return;
    }

  /* don't look at events with keycodes < 255, they go through X */
  if (key <= 255)
    return;

  /* Custom bindings (from rebinder.conf) */
  binding = find_binding_by_keycode (rebinder, key);

  /* Try to find the keycode in the static binding pool */
  if (binding == NULL)
    binding = find_evdev_binding_by_keycode (key);

  if (binding == NULL)
    return;

  MEX_NOTE (EVDEV, "Sending keysym 0x%08x (%s)", (int) binding->keysym,
            XKeysymToString (binding->keysym));

  fakekey_press_keysym (rebinder->fake, binding->keysym,  0);
}

static void
setup_binding (Rebinder *rebinder,
               Binding  *binding)
{
  FakeKey *fake = rebinder->fake;

  /* evdev bindings */
  if (is_evdev_enabled () && binding->keycode > 255)
    {
      Binding *evdev_binding;

      evdev_binding = g_slice_dup (Binding, binding);

      MEX_NOTE (REMAPPING, "evdev mapping %d to 0x%08x (%s)",
                evdev_binding->keycode, (int) evdev_binding->keysym,
                XKeysymToString (evdev_binding->keysym));

      g_ptr_array_add (rebinder->evdev_bindings, evdev_binding);

      return;
    }

  /* X bindings */
  else if (binding->keycode < fake->min_keycode ||
           binding->keycode > fake->max_keycode)
    {
      g_warning ("Cannot use the keycode %d as it's outside the allowed range "
                 "[%d..%d]", binding->keycode, fake->min_keycode,
                 fake->max_keycode);
      return;
    }

  MEX_NOTE (REMAPPING, "Rebinding %d to 0x%08x (%s)", binding->keycode,
            (guint) binding->keysym, XKeysymToString (binding->keysym));

  XChangeKeyboardMapping (rebinder->dpy,
                          binding->keycode, 1,
                          &binding->keysym, 1);
}

static void
load_bindings (Rebinder *rebinder)
{
  GError *error = NULL;
  gchar *filename;
  GKeyFile *file;
  gchar **keys;
  gint i;

  filename = g_build_filename (rebinder_get_config_dir (),
                               "rebinder.conf",
                               NULL);

  file = g_key_file_new ();
  g_key_file_load_from_file (file, filename, G_KEY_FILE_NONE, &error);

  /* no configuration file, nothing to do */
  if (error && error->code == G_FILE_ERROR_NOENT)
    goto out;

  if (error)
    {
      g_warning ("Could not load %s: %s", filename, error->message);
      goto out;
    }

  keys = g_key_file_get_keys (file, "bindings", NULL, &error);
  if (error)
    {
      g_warning ("Could not find a [bindings] section in %s", filename);
      goto out;
    }

  for (i = 0; keys[i]; i++)
    {
      Binding binding;
      gint keycode;

      keycode =  g_key_file_get_integer (file, "bindings", keys[i], NULL);

      /* save the original binding to restore it when the daemon exits */
      binding.name = keys[i];
      binding.keysym = XKeycodeToKeysym (rebinder->dpy, keycode, 0);
      binding.keycode = keycode;
      g_array_append_val (rebinder->original_bindings, binding);

      /* new binding */
      binding.name = keys[i];
      binding.keysym = XStringToKeysym (keys[i]);
      binding.keycode = keycode;

      setup_binding (rebinder, &binding);
    }

  commit_bindings (rebinder);

  g_strfreev (keys);

out:
  g_free (filename);
}

static void
_file_monitor_changed_cb (GFileMonitor      *monitor,
                          GFile             *file,
                          GFile             *other_file,
                          GFileMonitorEvent  event,
                          Rebinder          *rebinder)
{
  if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
      event == G_FILE_MONITOR_EVENT_DELETED ||
      event == G_FILE_MONITOR_EVENT_CREATED)
  load_bindings (rebinder);
}

static void
setup_monitor (Rebinder *rebinder)
{
  GFile *config_file;
  gchar *filename;
  GError *error = NULL;

  filename = g_build_filename (rebinder_get_config_dir (),
                               "rebinder.conf",
                               NULL);
  config_file = g_file_new_for_path (filename);
  g_free (filename);

  rebinder->monitor = g_file_monitor (config_file,
                                      G_FILE_MONITOR_NONE,
                                      NULL,
                                      &error);

  if (error)
    {
      g_critical (G_STRLOC ": Error setting up file monitor: %s",
                  error->message);
      g_clear_error (&error);
    }
  else
    {
      g_signal_connect (rebinder->monitor,
                        "changed",
                        (GCallback)_file_monitor_changed_cb,
                        rebinder);
    }

  g_object_unref (config_file);
}

static void
restore_bindings (Rebinder *rebinder)
{
  gint i;

  for (i = 0; i < rebinder->original_bindings->len; i++)
    {
      Binding *binding =
        &g_array_index (rebinder->original_bindings, Binding, i);

      if (binding->keycode != NoSymbol && binding->keycode <= 255)
        setup_binding (rebinder, binding);
    }

  commit_bindings (rebinder);
}

static void
on_int_term_signaled (int sig)
{
  g_main_loop_quit (the_rebinder.mainloop);
}

static gboolean
request_dbus_name (const gchar *name)
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
  guint32 request_status;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (connection == NULL) {
    g_warning ("Failed to open connection to DBus: %s", error->message);
    g_error_free (error);
    return FALSE;
  }

  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (proxy, name,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_status,
                                          &error)) {
    g_warning ("Failed to request name: %s", error->message);
    g_error_free (error);
    return FALSE;
  }

  return request_status == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
}

static void
on_rebinder_quit (MexRebinder *rebinder,
                  Rebinder    *the_rebinder)
{
  g_main_loop_quit (the_rebinder->mainloop);
}

int
main(int argc, char *argv[])
{
  GOptionContext *context;
  GError *error = NULL;

  memset (&the_rebinder, 0, sizeof (Rebinder));

  g_thread_init (NULL);
  g_type_init ();

  /* Options */
  context = g_option_context_new ("- key rebinder");

  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("Failed to parse options: %s\n", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

#ifdef REBINDER_ENABLE_DEBUG
  rebinder_debug_init ();
#endif

  if (opt_configure)
    {
      /* lauched in configuration mode */

      if (request_dbus_name ("com.meego.rebinderConfigure") == FALSE)
        {
          g_message ("Could not request DBus name");
          return EXIT_SUCCESS;
        }

      clutter_init (&argc, &argv);

      /* we expect to be build against clutter-glx */
      the_rebinder.dpy = clutter_x11_get_default_display ();

      the_rebinder.config = rebinder_configure (&the_rebinder,
						NULL,
                                                is_evdev_enabled (),
                                                is_fullscreen_enabled ());

      clutter_main ();

      rebinder_configure_free (the_rebinder.config);
    }
  else
    {
      /* launched in daemon mode */
      MexRebinder *rebinder;
      gboolean registered;

      rebinder = mex_rebinder_new ();
      registered = mex_rebinder_register (rebinder,
                                          "com.meego.rebinder",
                                          "/com/meego/rebinder",
                                          &error);
      if (registered == FALSE)
        {
          const gchar prefix[] = "Could not request DBus name";

          if (error)
            g_message ("%s: %s", prefix, error->message);
          else
            g_message ("%s", prefix);

          return EXIT_FAILURE;
        }

      g_signal_connect (rebinder, "quit",
                        G_CALLBACK (on_rebinder_quit), &the_rebinder);

      the_rebinder.dpy = XOpenDisplay (NULL);
      if (G_UNLIKELY (the_rebinder.dpy == NULL))
        {
          g_error ("Could not open display");
          return EXIT_FAILURE;
        }

      the_rebinder.fake = fakekey_init (the_rebinder.dpy);
      if (G_UNLIKELY (the_rebinder.fake == NULL))
        {
          g_error ("Could not initialize fakekey");
          return EXIT_FAILURE;
        }

      if (opt_no_daemon == FALSE)
        daemon (0, 0);

      signal (SIGINT, on_int_term_signaled);
      signal (SIGTERM, on_int_term_signaled);

      the_rebinder.original_bindings =
        g_array_new (FALSE, FALSE, sizeof (Binding));

      /* listens to evdev events and setup everything needed */
      if (is_evdev_enabled ())
        {
          the_rebinder.evdev_manager = rebinder_evdev_manager_get_default ();
          rebinder_evdev_manager_set_key_notifier (the_rebinder.evdev_manager,
                                                   on_evdev_key_pressed,
                                                   &the_rebinder);
          the_rebinder.evdev_bindings =
            g_ptr_array_new_with_free_func (free_binding);
        }

      load_bindings (&the_rebinder);
      setup_monitor (&the_rebinder);

      the_rebinder.mainloop = g_main_loop_new (NULL, TRUE);
      g_main_loop_run (the_rebinder.mainloop);

      restore_bindings (&the_rebinder);

      g_main_loop_unref (the_rebinder.mainloop);
      g_object_unref (the_rebinder.monitor);
    }

  return EXIT_SUCCESS;
}
