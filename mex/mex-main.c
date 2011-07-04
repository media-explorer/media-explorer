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

#ifdef HAVE_LIRC
#include <lirc/lirc_client.h>
#include <unistd.h>
#include <fcntl.h>
#include <clutter/x11/clutter-x11.h>
#endif

#include <clutter/clutter.h>

#include "mex-log-private.h"
#include "mex-utils.h"

#include "mex-main.h"

/**
 * SECTION:mex-main
 * @short_description: Miscellaneous core functions
 *
 * The <application>Media Explorer</application> library should be initialized with
 * mex_init() or mex_init_with_args() before it can be used. You should pass
 * pointers to the main argc and argv variables so that the underlying
 * libraries can process their own command line options, as shown in the
 * following example:
 *
 * <example>
 * <title>Initializing the Media Explorer library</title>
 * <programlisting language="c">
 * int
 * main (int argc, char *argv[])
 * {
 *   mex_init (&amp;argc, &amp;argv);
 *   ...
 * }
 * </programlisting>
 * </example>
 *
 * It is allowed to give two NULL pointers to mex_init() in case you
 * don't want to the underlying libraries to parse the command line.
 *
 * You can also use a #GOptionEntry array to provide your own parameters
 * as shown in the next code fragment:
 *
 * <example>
 * <title>Initializing the Media Explorer library with additional options</title>
 * <programlisting language="c">
 * static GOptionEntry options[] =
 * {
 *   { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },
 *   { "beep", 'b', 0, G_OPTION_ARG_NONE, &beep, "Beep when done", NULL },
 *   { NULL }
 * };
 *
 * int
 * main (int argc, char *argv[])
 * {
 *    GError           *error = NULL;
 *    gboolean          result;
 *
 *    result = mex_init_with_args (&argc, &argv,
 *                                 " - My libmex application",
 *                                 options, NULL, &error);
 *
 *    if (result == FALSE)
 *      {
 *        g_print ("%s\n", error->message);
 *        g_error_free (error);
 *        return EXIT_FAILURE;
 *      }
 *   ...
 * }
 * </programlisting>
 * </example>
 */

static gboolean mex_is_initialized = FALSE;

#ifdef HAVE_LIRC
static struct lirc_config *mex_lirc_config = NULL;

static void
mex_lirc_do_event (ClutterEvent *event)
{
  const GSList *s;

  ClutterStageManager *stage_manager = clutter_stage_manager_get_default ();
  const GSList *stages = clutter_stage_manager_peek_stages (stage_manager);

  /* FIXME: We should probably check if the stage has focus via X */
  for (s = stages; s; s = s->next)
    {
      ClutterStage *stage = s->data;
      ClutterActor *actor = clutter_stage_get_key_focus (stage);

      if (!actor)
        continue;

      event->any.stage = stage;
      event->any.source = actor;

      clutter_do_event (event);
    }
}

static void
mex_lirc_create_key_event (ClutterKeyEvent *event, guint keyval)
{
  ClutterDeviceManager *manager = clutter_device_manager_get_default ();

  /* Event synthesis inspired/copied from Clutter X11 backend */
  event->flags = CLUTTER_EVENT_FLAG_SYNTHETIC;
  event->time = CurrentTime;
  event->keyval = keyval;
  event->unicode_value = clutter_keysym_to_unicode (event->keyval);
  event->device =
    clutter_device_manager_get_core_device (manager, CLUTTER_KEYBOARD_DEVICE);
}

static gboolean
mex_lirc_key_release_cb (gpointer keyval_p)
{
  ClutterEvent release = { 0, };
  guint keyval = GPOINTER_TO_UINT (keyval_p);

  mex_lirc_create_key_event ((ClutterKeyEvent *)&release.key, keyval);
  release.type = release.key.type = CLUTTER_KEY_RELEASE;

  mex_lirc_do_event (&release);

  return FALSE;
}

static void
mex_lirc_do_key_event (guint keyval)
{
  ClutterEvent press = { 0, };

  mex_lirc_create_key_event ((ClutterKeyEvent *)&press.key, keyval);
  press.type = press.key.type = CLUTTER_KEY_PRESS;

  mex_lirc_do_event ((ClutterEvent *)&press);

  g_timeout_add (50,
                 (GSourceFunc)mex_lirc_key_release_cb,
                 GUINT_TO_POINTER (keyval));
}

static gboolean
mex_lirc_read_cb (GIOChannel         *source,
                  GIOCondition        condition,
                  struct lirc_config *config)
{
  gboolean success = TRUE;

  while (condition & (G_IO_PRI | G_IO_IN))
    {
      gint error_code;
      gchar *lirc_code, *lirc_char;

      while (((error_code = lirc_nextcode (&lirc_code)) == 0) && lirc_code)
        {
          while ((lirc_code2char (config, lirc_code, &lirc_char) == 0) &&
                 (lirc_char != NULL))
            {
              if (g_str_equal (lirc_char, "up"))
                mex_lirc_do_key_event (CLUTTER_KEY_Up);
              else if (g_str_equal (lirc_char, "down"))
                mex_lirc_do_key_event (CLUTTER_KEY_Down);
              else if (g_str_equal (lirc_char, "left"))
                mex_lirc_do_key_event (CLUTTER_KEY_Left);
              else if (g_str_equal (lirc_char, "right"))
                mex_lirc_do_key_event (CLUTTER_KEY_Right);
              else if (g_str_equal (lirc_char, "enter"))
                mex_lirc_do_key_event (MEX_KEY_OK);
              else if (g_str_equal (lirc_char, "back"))
                mex_lirc_do_key_event (MEX_KEY_BACK);
              else if (g_str_equal (lirc_char, "home"))
                mex_lirc_do_key_event (MEX_KEY_HOME);
            }

          g_free (lirc_code);
        }

      condition = g_io_channel_get_buffer_condition (source);

      if (error_code == -1)
        {
          g_warning (G_STRLOC ": Error reading from source");
          success = FALSE;
        }
    }

  if (condition & G_IO_HUP)
    {
      g_warning (G_STRLOC ": Unexpected hang-up");
      success = FALSE;
    }

  if (condition & (G_IO_ERR | G_IO_NVAL))
    {
      g_warning (G_STRLOC ": Error or invalid request");
      success = FALSE;
    }

  if (condition & ~(G_IO_PRI | G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL))
    {
      g_warning ("Unexpected IO condition");
      success = FALSE;
    }

  return success;
}

static void
mex_lirc_deinit (struct lirc_config *config)
{
  if (config)
    lirc_freeconfig (config);
  lirc_deinit ();
}

static struct lirc_config *
mex_lirc_init (const gchar *name)
{
  int lirc_fd, res;
  gchar *tmp;

  lirc_fd = lirc_init ((char *)name, 1);

  if (lirc_fd == -1)
    g_warning (G_STRLOC ": LIRC initialisation failed");
  else
    {
      struct lirc_config *config = NULL;

      /* Load the default config */
      tmp = g_build_filename (mex_get_data_dir (), "mex-lircrc", NULL);
      res = lirc_readconfig (tmp, &config, NULL);
      g_free (tmp);

      if (res == 0)
        {
          int flags;
          GIOChannel *channel;

          /* Set non-blocking flag */
          flags = fcntl (lirc_fd, F_GETFL);
          fcntl (lirc_fd, F_SETFL, flags | O_NONBLOCK);

          /* Create IOChannel to do non-blocking reads */
          channel = g_io_channel_unix_new (lirc_fd);
          g_io_add_watch (channel,
                          G_IO_IN | G_IO_PRI | G_IO_ERR |
                          G_IO_NVAL | G_IO_HUP,
                          (GIOFunc)mex_lirc_read_cb,
                          config);

          return config;
        }
      else
        {
          g_warning (G_STRLOC ": Error reading LIRC config");
          lirc_deinit ();
        }
    }

  return NULL;
}
#endif

static void
mex_base_init (int *argc, char ***argv)
{
  /* initialize debugging infrastructure */
#ifdef MEX_ENABLE_DEBUG
  _mex_log_init_core_domains ();
#endif

  /* Initialise LIRC */
#ifdef HAVE_LIRC
  mex_lirc_config = mex_lirc_init ("mex");
#endif
}

/**
 * mex_init:
 * @argc: pointer to the argument list count
 * @argv: pointer to the argument list vector
 *
 * Utility function to initialize the Media Explorer library.
 *
 * This function should be called before calling any other GLib functions. If
 * this is not an option, your program must initialise the GLib thread system
 * using g_thread_init() before any other GLib functions are called.
 *
 * Return value: %TRUE on success, %FALSE on failure.
 *
 *
 * Since: 0.2
 */
gboolean
mex_init (int    *argc,
          char ***argv)
{
  gboolean retval;

  if (mex_is_initialized)
    return TRUE;

  if (!g_thread_supported ())
    g_thread_init (NULL);

  retval = clutter_init (argc, argv);

  mex_base_init (argc, argv);

  mex_is_initialized = TRUE;

  return retval;
}

/**
 * mex_deinit:
 *
 * Utility function to deinitialize the Media Explorer library.
 *
 * This function should be called before the process disappears. This
 * performs internal clean-up.
 *
 */
void
mex_deinit (void)
{
#ifdef HAVE_LIRC
  mex_lirc_deinit (mex_lirc_config);
  mex_lirc_config = NULL;
#endif
}

/**
 * mex_init_with_args:
 * @argc: a pointer to the number of command line arguments.
 * @argv: a pointer to the array of command line arguments.
 * @parameter_string: a string which is displayed in
 *    the first line of <option>--help</option> output, after
 *    <literal><replaceable>programname</replaceable> [OPTION...]</literal>
 * @entries: a %NULL-terminated array of #GOptionEntry<!-- -->s
 *    describing the options of your program
 * @translation_domain: a translation domain to use for translating
 *    the <option>--help</option> output for the options in @entries
 *    with gettext(), or %NULL
 * @error: a return location for errors
 *
 * This function does the same work as mex_init(). Additionally, it
 * allows you to add your own command line options, and it automatically
 * generates nicely formatted --help output. Clutter's #GOptionGroup
 * is added to the set of available options.
 *
 * This function should be called before calling any other GLib functions. If
 * this is not an option, your program must initialise the GLib thread system
 * using g_thread_init() before any other GLib functions are called.
 *
 * Return value: %TRUE on success, %FALSE on failure.
 *
 * Since: 0.2
 */
gboolean
mex_init_with_args (int            *argc,
                    char         ***argv,
                    const char     *parameter_string,
                    GOptionEntry   *entries,
                    const char     *translation_domain,
                    GError        **error)
{
  GOptionContext *context;
  gboolean res;

  if (mex_is_initialized)
    return TRUE;

  if (!g_thread_supported ())
    g_thread_init (NULL);

  context = g_option_context_new (parameter_string);

  g_option_context_add_group (context, clutter_get_option_group ());

  if (entries)
    g_option_context_add_main_entries (context, entries, translation_domain);

  res = g_option_context_parse (context, argc, argv, error);
  g_option_context_free (context);

  if (!res)
        return FALSE;

  mex_base_init (argc, argv);

  mex_is_initialized = TRUE;

  return TRUE;
}

/**/

static MxWindow *mex_main_window = NULL;
static ClutterActor *mex_stage = NULL;

ClutterActor *
mex_get_stage (void)
{
  return mex_stage;
}

void
mex_set_main_window (MxWindow *window)
{
  MexMMkeys *mmkeys;

  if (mex_main_window)
    g_object_unref (mex_main_window);

  if (window)
    {
      mex_main_window = g_object_ref (window);
      mex_stage =
        (ClutterActor *) mx_window_get_clutter_stage (mex_main_window);
      mmkeys = mex_mmkeys_get_default ();
      mex_mmkeys_set_stage (mmkeys, mex_stage);
    }
  else
    {
      mex_main_window = NULL;
      mex_stage = NULL;
    }
}

MxWindow *
mex_get_main_window (void)
{
  return mex_main_window;
}

gboolean
mex_get_fullscreen (void)
{
  if (!mex_main_window)
    return FALSE;

  return mx_window_get_fullscreen (mex_main_window);
}

void
mex_toggle_fullscreen (void)
{
  mex_set_fullscreen (!mex_get_fullscreen ());
}

void
mex_set_fullscreen (gboolean fullscreen)
{
  if (!mex_main_window)
    return;

  if (mex_get_fullscreen () == fullscreen)
    return;

  mx_window_set_fullscreen (mex_main_window, fullscreen);
}
