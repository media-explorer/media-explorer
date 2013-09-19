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
#include <glib/gi18n.h>

#include <clutter-gst/clutter-gst.h>

#include "mex-main.h"
#include "mex-content.h"
#include "mex-settings.h"
#include "mex-log-private.h"
#include "mex-model-manager.h"
#include "mex-grilo.h"
#include "mex-vt-manager.h"

/* Default Categories */
/* See mex/mex-model-manager.h */

static const MexModelCategoryInfo videos = {
    "videos", N_("Videos"), "icon-panelheader-videos", 20,
    N_("Connect an external drive or update your network settings to see Videos"
       " here."), MEX_CONTENT_METADATA_SERIES_NAME,
    MEX_CONTENT_METADATA_SEASON, TRUE
};
static const MexModelCategoryInfo live = {
    "live", N_("Live TV"), "icon-panelheader-tv", 25, "", 0, 0, TRUE
};
static const MexModelCategoryInfo pictures = {
    "pictures", N_("Photos"), "icon-panelheader-photos", 30,
    N_("Connect an external drive or update your network settings to see Photos"
      " here."), 0, 0, TRUE
};
static const MexModelCategoryInfo music = {
    "music", N_("Music"), "icon-panelheader-music", 40,
    N_("Connect an external drive or update your network settings to see Music"
       " here."), MEX_CONTENT_METADATA_ALBUM, 0, TRUE
};
static const MexModelCategoryInfo queue = {
    "queue", N_("Queue"), "icon-panelheader-queue", 50,
    N_("This is your custom playlist. Select Add to queue from the info menu on"
      " any video file to add it here."), 0, 0, TRUE
};

/**
 * mex_init_default_categories:
 *
 * Add the default categories to the model manager
 */
static void
mex_init_default_categories (void)
{
  MexModelManager *mmanager;

  mmanager = mex_model_manager_get_default ();

  mex_model_manager_add_category (mmanager, &live);
  mex_model_manager_add_category (mmanager, &videos);
  mex_model_manager_add_category (mmanager, &music);
  mex_model_manager_add_category (mmanager, &pictures);
  mex_model_manager_add_category (mmanager, &queue);
}


static void
grilo_load_default_plugins (GrlRegistry *registry)
{
  grl_registry_load_plugin_by_id (registry, "grl-tracker", NULL);

  grl_registry_load_plugin_by_id (registry, "grl-upnp", NULL);
  grl_registry_load_plugin_by_id (registry, "grl-filesystem", NULL);

  grl_registry_load_plugin_by_id (registry, "grl-lastfm-albumart", NULL);
}

/**
 * mex_init_load_plugins:
 *
 * Load the Grilo plugins.
 */
static void
mex_init_load_grilo_plugins (void)
{
  GrlRegistry *registry;
  GError *error = NULL;
  gchar *settings;

  registry = grl_registry_get_default ();

  settings = mex_settings_find_config_file (mex_settings_get_default (),
                                            "grilo-system.conf");
  if (settings)
    grl_registry_add_config_from_file (registry, settings, NULL);
  g_free (settings);


  settings = mex_settings_find_config_file (mex_settings_get_default (),
                                            "mex.conf");
  /* Load plugins */

  if (settings)
    {
      GKeyFile *mex_conf;
      gchar **enabled_plugins;

      mex_conf = g_key_file_new ();

      g_key_file_load_from_file (mex_conf,
                                 settings,
                                 G_KEY_FILE_NONE,
                                 NULL);

      enabled_plugins = g_key_file_get_string_list (mex_conf,
                                                    "grilo-plugins",
                                                    "enabled",
                                                    NULL,
                                                    NULL);
      g_key_file_free (mex_conf);

      if (enabled_plugins)
        {
          gint i;

          for (i=0; enabled_plugins[i]; i++)
            {
              if (!grl_registry_load_plugin_by_id (registry,
                                                   enabled_plugins[i],
                                                   &error))
                {
                  g_warning ("Tried to load specified grilo plugin: %s but failed: %s",
                             enabled_plugins[i],
                             (error) ? error->message : "");

                  if (error)
                    g_clear_error (&error);
                }
              else
                {
                  MEX_DEBUG ("Loaded grilo plugin: %s plugin",
                             enabled_plugins[i]);
                }
            }
          g_strfreev (enabled_plugins);
        }
      else
        {
          MEX_DEBUG ("No enabled plugins in mex.conf, loading default plugins");
          grilo_load_default_plugins (registry);
        }
      g_free (settings);
    }
  else
    {
      MEX_DEBUG ("No mex.conf found, loading default plugins");
      grilo_load_default_plugins (registry);
    }

}

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
  static gsize initialised;

  if (g_once_init_enter (&initialised))
    {
      /* initialize debugging infrastructure */
      _mex_log_init_core_domains ();

      mex_vt_manager_init ();

      /* initialise Clutter */
      if (!clutter_init (argc, argv))
        g_error ("Failed to initialize clutter");

      clutter_gst_init (argc, argv);

      mex_init_default_categories ();
      mex_init_load_grilo_plugins();

      g_once_init_leave (&initialised, TRUE);
    }
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

  mex_grilo_init (argc, argv);

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
  mex_vt_manager_deinit ();
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

static gboolean
mex_get_fullscreen (void)
{
  if (!mex_main_window)
    return FALSE;

  return mx_window_get_fullscreen (mex_main_window);
}

static void
mex_set_fullscreen (gboolean fullscreen)
{
  if (!mex_main_window)
    return;

  if (mex_get_fullscreen () == fullscreen)
    return;

  mx_window_set_fullscreen (mex_main_window, fullscreen);
}

void
mex_toggle_fullscreen (void)
{
  mex_set_fullscreen (!mex_get_fullscreen ());
}
