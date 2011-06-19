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

#include <grilo.h>

#include "mex-debug.h"
#include "mex-grilo.h"
#include "mex-settings.h"

static GAsyncQueue  *grilo_operation_list = NULL;
static GMainContext *grilo_thread_context = NULL;
static GStaticMutex  grilo_thread_startup = G_STATIC_MUTEX_INIT;


static GStaticMutex  grilo_sources_mutex;
static GList        *grilo_sources       = NULL;

static void
source_added_cb (GrlPluginRegistry *registry,
                 GrlMediaPlugin *source,
                 gpointer user_data)
{
  g_message ("Detected new source available: name='%s' id='%s'",
             grl_metadata_source_get_name (GRL_METADATA_SOURCE (source)),
             grl_metadata_source_get_id (GRL_METADATA_SOURCE (source)));

  g_message ("\tPlugin's name: %s", grl_media_plugin_get_name (GRL_MEDIA_PLUGIN (source)));
  g_message ("\tPlugin's description: %s", grl_media_plugin_get_description (GRL_MEDIA_PLUGIN (source)));
  g_message ("\tPlugin's author: %s", grl_media_plugin_get_author (GRL_MEDIA_PLUGIN (source)));
  g_message ("\tPlugin's license: %s", grl_media_plugin_get_license (GRL_MEDIA_PLUGIN (source)));
  g_message ("\tPlugin's version: %s", grl_media_plugin_get_version (GRL_MEDIA_PLUGIN (source)));
  g_message ("\tPlugin's web site: %s", grl_media_plugin_get_site (GRL_MEDIA_PLUGIN (source)));
}

static gpointer
mex_grilo_thread_func (gpointer data)
{
  gchar             *settings;
  GError            *error = NULL;
  GrlPluginRegistry *registry;
  GMainLoop         *loop;

  grilo_thread_context =  g_main_context_new ();
  g_main_context_push_thread_default (grilo_thread_context);

  loop = g_main_loop_new (grilo_thread_context, FALSE);

  grl_init (NULL, NULL);

  g_static_mutex_unlock (&grilo_thread_startup);

  registry = grl_plugin_registry_get_default ();

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (source_added_cb), NULL);

  settings = mex_settings_find_config_file (mex_settings_get_default (),
                                            "grilo-system.conf");
  if (settings)
    grl_plugin_registry_add_config_from_file (registry, settings, NULL);
  g_free (settings);

  settings = mex_settings_find_config_file (mex_settings_get_default (),
                                            "mex.conf");

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
              if (!grl_plugin_registry_load_by_id (registry,
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
                  MEX_NOTE (MISC, "loaded: %s plugin", enabled_plugins[i]);
                }
            }
          g_strfreev (enabled_plugins);
        }
      g_free (settings);
    }
  else
    {
      error = NULL;

      MEX_NOTE (MISC, "No mex.conf found, loading default plugins");

      /* Tracker is our first choice of plugin */
      if (!grl_plugin_registry_load_by_id (registry, "grl-tracker", NULL))
        {
          /* try and load the upnp and filesystem plugins instead */
         if (!grl_plugin_registry_load_by_id (registry, "grl-upnp", &error) &&
             !grl_plugin_registry_load_by_id (registry, "grl-filesystem", &error))
           {
             g_warning ("Could not load fallback plugins\n \
                        please check that grilo-plugins has been correctly \
                        installed : %s", error->message);
           }
        }
    }

  g_main_loop_run (loop);

  g_main_context_pop_thread_default (grilo_thread_context);

  return NULL;
}

static gboolean
mex_grilo_idle_cb (gpointer data)
{
  MexGriloOperation *op;

  g_async_queue_lock (grilo_operation_list);

  if (g_async_queue_length_unlocked (grilo_operation_list) < 1)
    {
      g_async_queue_unlock (grilo_operation_list);
      return FALSE;
    }

  op = (MexGriloOperation *) g_async_queue_pop_unlocked (grilo_operation_list);

  g_async_queue_unlock (grilo_operation_list);

  mex_grilo_operation_start (op);

  return TRUE;
}

void
mex_grilo_init (void)
{
  static gboolean initialized = FALSE;
  GError *error = NULL;

  if (initialized)
    return;

  initialized = TRUE;

  g_static_mutex_lock (&grilo_thread_startup);

  grilo_operation_list = g_async_queue_new ();
  grilo_thread_context =  g_main_context_new ();

  /* TODO/FIXME: wait for the thread_context to be set up. */
  g_thread_create (mex_grilo_thread_func,
                   NULL,
                   FALSE,
                   &error);

  g_static_mutex_lock (&grilo_thread_startup);
  g_static_mutex_unlock (&grilo_thread_startup);

  if (error != NULL)
    {
      g_warning (G_STRLOC ": Cannot launch Grilo thread: %s", error->message);
      g_error_free (error);
      g_assert_not_reached ();
    }
}

void
mex_grilo_schedule (MexGriloOperation *operation)
{
  g_return_if_fail (MEX_IS_GRILO_OPERATION (operation));

  g_message ("schedule lock");
  g_async_queue_lock (grilo_operation_list);

  g_async_queue_push_unlocked (grilo_operation_list, operation);

  if (g_async_queue_length_unlocked (grilo_operation_list) <= 1)
    {
      /* Rearm idle callback */
      GSource *source = g_idle_source_new ();
      g_source_set_callback (source,
                             mex_grilo_idle_cb,
                             NULL,
                             NULL);
      g_source_attach (source, grilo_thread_context);
    }

  g_async_queue_unlock (grilo_operation_list);
  g_message ("schedule unlock");
}
