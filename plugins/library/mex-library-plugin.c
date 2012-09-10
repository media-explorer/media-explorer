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

#include "mex-library-plugin.h"
#include <mex/mex-grilo-feed.h>

#include <glib/gi18n.h>

static void mex_model_provider_iface_init (MexModelProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexLibraryPlugin, mex_library_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                mex_model_provider_iface_init))

#define LIBRARY_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_LIBRARY_PLUGIN, MexLibraryPluginPrivate))

struct _MexLibraryPluginPrivate
{
  GList *models;
};


static void
mex_library_plugin_dispose (GObject *object)
{
  MexLibraryPluginPrivate *priv = MEX_LIBRARY_PLUGIN (object)->priv;

  while (priv->models)
    {
      g_object_unref (priv->models->data);
      priv->models = g_list_delete_link (priv->models, priv->models);
    }

  G_OBJECT_CLASS (mex_library_plugin_parent_class)->dispose (object);
}

static void
mex_library_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_library_plugin_parent_class)->finalize (object);
}

static void
mex_library_plugin_class_init (MexLibraryPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexLibraryPluginPrivate));

  object_class->dispose = mex_library_plugin_dispose;
  object_class->finalize = mex_library_plugin_finalize;
}

static const GList *
mex_library_plugin_get_models (MexModelProvider *self)
{
  MexLibraryPluginPrivate *priv = MEX_LIBRARY_PLUGIN (self)->priv;
  return priv->models;
}

static void
mex_model_provider_iface_init (MexModelProviderInterface *iface)
{
  iface->get_models = mex_library_plugin_get_models;
}

static GrlMedia *
mex_library_plugin_get_box_for_path (GrlSource   *source,
                                     const GList *keys,
                                     const gchar *path)
{
  gchar *uri;
  GrlMedia *box;
  GrlOperationOptions *options;

  GError *error = NULL;

  g_return_val_if_fail (path != NULL, NULL);

  uri = g_filename_to_uri (path, NULL, &error);
  if (!uri)
    {
      g_warning ("Error converting path to uri: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  options = grl_operation_options_new (NULL);
  box = grl_source_get_media_from_uri_sync (source, uri, keys,
                                            options,
                                            &error);
  g_free (uri);
  g_object_unref (options);

  if (!box)
    {
      g_warning ("Error getting media from uri: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  return box;
}

static void
mex_library_plugin_init (MexLibraryPlugin *self)
{
  GrlSource *source;
  GrlRegistry *registry;

  MexLibraryPluginPrivate *priv = self->priv =
    LIBRARY_PLUGIN_PRIVATE (self);

  registry = grl_registry_get_default ();

  source = grl_registry_lookup_source (registry, "grl-filesystem");
  if (source)
    {
      GList *query_keys;
      MexFeed *feed;
      GrlMedia *box;
      gchar **paths;
      GKeyFile *mex_settings_key;
      gint i;
      gsize paths_len;
      GList *metadata_keys;

      query_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                              GRL_METADATA_KEY_TITLE,
                                              GRL_METADATA_KEY_MIME,
                                              GRL_METADATA_KEY_URL,
                                              GRL_METADATA_KEY_PUBLICATION_DATE,
                                              NULL);

      /* Add the videos model */

      metadata_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                 GRL_METADATA_KEY_DESCRIPTION,
                                                 GRL_METADATA_KEY_DURATION,
                                                 GRL_METADATA_KEY_THUMBNAIL,
                                                 GRL_METADATA_KEY_WIDTH,
                                                 GRL_METADATA_KEY_HEIGHT,
                                                 NULL);

      mex_settings_key = mex_get_settings_key_file ();
      paths_len = 0;
      paths = g_key_file_get_string_list (mex_settings_key,
                                          "library-plugin",
                                          "video_paths",
                                          &paths_len,
                                          NULL);

      if (!paths)
        paths = g_new (gchar*, 2);
      else
        paths = (gchar **)g_realloc ((gpointer)paths,
                (paths_len + 2) * sizeof (gchar *));

     paths[paths_len] =
       g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS));

     paths_len++;
     paths[paths_len] = NULL;

      if (paths)
        {
          for (i = 0; i < paths_len; i++)
            {
              box =
                mex_library_plugin_get_box_for_path (source,
                                                     query_keys, paths[i]);
              if (box)
                {
                  feed = mex_grilo_feed_new (source,
                                             query_keys, metadata_keys, box);

                  g_object_set (feed, "icon-name", "icon-library",
                                "placeholder-text", "No videos found",
                                "category", "videos", NULL);

                  mex_grilo_feed_browse (MEX_GRILO_FEED (feed), 0, G_MAXINT);

                  priv->models = g_list_append (priv->models, feed);
                }
            }
        }

      if (metadata_keys)
        g_list_free (metadata_keys);
      if (paths)
        g_strfreev (paths);

      /* Add the photos model */

      metadata_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                 GRL_METADATA_KEY_DESCRIPTION,
                                                 GRL_METADATA_KEY_THUMBNAIL,
                                                 GRL_METADATA_KEY_WIDTH,
                                                 GRL_METADATA_KEY_HEIGHT,
                                                 NULL);

      paths_len = 0;
      paths = g_key_file_get_string_list (mex_settings_key,
                                          "library-plugin",
                                          "pictures_paths",
                                          &paths_len,
                                          NULL);

      paths = (gchar **)g_realloc ((gpointer)paths,
                                   (paths_len + 2) * sizeof (gchar *));

      if (!paths)
        paths = g_new (gchar*, 2);
      else
        paths[paths_len] =
          g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES));

      paths_len++;
      paths[paths_len] = NULL;

      if (paths)
        {
          for (i = 0; i < paths_len; i++)
            {
              box =
                mex_library_plugin_get_box_for_path (source,
                                                     query_keys, paths[i]);
              if (box)
                {
                  feed = mex_grilo_feed_new (source,
                                             query_keys, metadata_keys, box);
                  g_object_set (feed, "icon-name", "icon-library",
                                "placeholder-text", "No pictures found",
                                "category", "pictures", NULL);

                  mex_grilo_feed_browse (MEX_GRILO_FEED (feed), 0, G_MAXINT);

                  priv->models = g_list_append (priv->models, feed);
                }
            }
        }

      if (metadata_keys)
        g_list_free (metadata_keys);
      if (paths)
        g_strfreev (paths);

      /* Add the music model */
      metadata_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                 GRL_METADATA_KEY_DESCRIPTION,
                                                 GRL_METADATA_KEY_THUMBNAIL,
                                                 GRL_METADATA_KEY_WIDTH,
                                                 GRL_METADATA_KEY_HEIGHT,
                                                 GRL_METADATA_KEY_ARTIST,
                                                 GRL_METADATA_KEY_ALBUM,
                                                 NULL);

      paths_len = 0;
      paths = g_key_file_get_string_list (mex_settings_key,
                                          "library-plugin",
                                          "music_paths",
                                          &paths_len,
                                          NULL);

      if (!paths)
        paths = g_new (gchar*, 2);
      else
        paths = (gchar **)g_realloc ((gpointer)paths,
            (paths_len + 2) * sizeof (gchar *));

      paths[paths_len] =
        g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_MUSIC));

      paths_len++;
      paths[paths_len] = NULL;

      if (paths)
        {
          for (i = 0; i < paths_len; i++)
            {
              box =
                mex_library_plugin_get_box_for_path (source,
                                                     query_keys, paths[i]);
              if (box)
                {
                  feed = mex_grilo_feed_new (source,
                                             query_keys, metadata_keys, box);
                  g_object_set (feed, "icon-name", "icon-library",
                                "placeholder-text", "No Music found",
                                "category", "music", NULL);

                  mex_grilo_feed_browse (MEX_GRILO_FEED (feed), 0, G_MAXINT);

                  priv->models = g_list_append (priv->models, feed);
                }
            }
        }

      /* clean up */

      if (metadata_keys)
        g_list_free (metadata_keys);
      if (paths)
        g_strfreev (paths);

      if (mex_settings_key)
        g_key_file_free (mex_settings_key);

      g_list_free (query_keys);
    }
  else
    g_warning ("Filesystem plugin not found");
}

static GType
mex_library_get_type (void)
{
  return MEX_TYPE_LIBRARY_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Library",
		   "Provides a filesystem view",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Chris Lord <chris@linux.intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_library_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
