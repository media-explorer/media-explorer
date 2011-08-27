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


static void
mex_base_init (int *argc, char ***argv)
{
  /* initialize debugging infrastructure */
#ifdef MEX_ENABLE_DEBUG
  _mex_log_init_core_domains ();
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
  if (mex_is_initialized)
    return TRUE;

  if (!g_thread_supported ())
    g_thread_init (NULL);

  mex_base_init (argc, argv);

  mex_is_initialized = TRUE;

  return TRUE;
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
  mex_set_main_window (NULL);
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

/**
 * mex_get_stage:
 *
 * Returns: (transfer none): the main #MxWindow
 *
 * Since: 0.2
 */
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

/**
 * mex_get_main_window:
 *
 * Returns: (transfer none): the main #MxWindow
 *
 * Since: 0.2
 */
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
