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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <grilo.h>
#include <mex/mex-grilo-tracker-feed.h>
#include "mex-tracker-plugin.h"

G_DEFINE_TYPE (MexTrackerPlugin, mex_tracker_plugin, G_TYPE_OBJECT)

#define MAX_TRACKER_RESULTS G_MAXINT

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TRACKER_PLUGIN, MexTrackerPluginPrivate))

#define MEX_LOG_DOMAIN_DEFAULT  tracker_log_domain
MEX_LOG_DOMAIN_STATIC(tracker_log_domain);


struct _MexTrackerPluginPrivate {
  MexModelManager *manager;
  GHashTable *video_models;
  GHashTable *image_models;
  GHashTable *music_models;

  GList *query_video_keys;
  GList *query_image_keys;
  GList *query_music_keys;
  GList *video_keys;
  GList *image_keys;
  GList *music_keys;
};

typedef enum {
  MEX_TRACKER_CATEGORY_IMAGE,
  MEX_TRACKER_CATEGORY_VIDEO,
  MEX_TRACKER_CATEGORY_MUSIC,
} MexTrackerCategory;

static void
remove_model (gpointer key, gpointer value, gpointer user_data)
{
  MexTrackerPlugin *self = MEX_TRACKER_PLUGIN (user_data);
  MexModel *model = MEX_MODEL (value);

  mex_model_manager_remove_model (self->priv->manager, model);
}

static void
mex_tracker_plugin_finalize (GObject *gobject)
{
  MexTrackerPlugin *self = MEX_TRACKER_PLUGIN (gobject);
  MexTrackerPluginPrivate *priv = self->priv;

  if (priv->query_video_keys)
    {
      g_list_free (priv->query_video_keys);
      priv->query_video_keys = NULL;
    }

  if (priv->query_image_keys)
    {
      g_list_free (priv->query_image_keys);
      priv->query_image_keys = NULL;
    }

  if (priv->query_music_keys)
    {
      g_list_free (priv->query_music_keys);
      priv->query_music_keys = NULL;
    }

  if (priv->video_keys)
    {
      g_list_free (priv->video_keys);
      priv->video_keys = NULL;
    }

  if (priv->image_keys)
    {
      g_list_free (priv->image_keys);
      priv->image_keys = NULL;
    }

  if (priv->music_keys)
    {
      g_list_free (priv->music_keys);
      priv->music_keys = NULL;
    }

  if (priv->video_models)
    {
      g_hash_table_foreach (priv->video_models, remove_model, self);
      g_hash_table_destroy (priv->video_models);
      priv->video_models = NULL;
    }

  if (priv->image_models)
    {
      g_hash_table_foreach (priv->image_models, remove_model, self);
      g_hash_table_destroy (priv->image_models);
      priv->image_models = NULL;
    }

  if (priv->music_models)
    {
      g_hash_table_foreach (priv->music_models, remove_model, self);
      g_hash_table_destroy (priv->music_models);
      priv->music_models = NULL;
    }

  G_OBJECT_CLASS (mex_tracker_plugin_parent_class)->finalize (gobject);
}

static void
mex_tracker_plugin_class_init (MexTrackerPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = mex_tracker_plugin_finalize;

  g_type_class_add_private (klass, sizeof (MexTrackerPluginPrivate));
}

static void
add_model (MexTrackerPlugin *self,
           GrlMediaPlugin *plugin,
           MexTrackerCategory category)
{
  GList *metadata_keys, *query_keys;
  MexFeed *feed, *dir_feed;
  GrlMedia *box;
  gchar *query, *cat_name;
  GHashTable *models;
  const gchar *source_name = grl_metadata_source_get_name (GRL_METADATA_SOURCE (plugin));
  gint priority;

  switch (category)
    {
    case MEX_TRACKER_CATEGORY_IMAGE:
      cat_name = "pictures";
      query = "?urn a nfo:FileDataObject . "
              "?urn tracker:available true . "
              "FILTER (nfo:width(?urn) > 100)"
              "FILTER (nfo:height(?urn) > 100)"
              "FILTER (fn:starts-with(nie:mimeType(?urn),'image/'))";
      models = self->priv->image_models;
      metadata_keys = self->priv->image_keys;
      query_keys = self->priv->query_image_keys;
      box = grl_media_image_new ();
      break;

    case MEX_TRACKER_CATEGORY_VIDEO:
      cat_name = "videos";
      query = "?urn a nfo:FileDataObject . "
              "?urn tracker:available true . "
              "FILTER (fn:starts-with(nie:mimeType(?urn),'video/'))";
      models = self->priv->video_models;
      metadata_keys = self->priv->video_keys;
      query_keys = self->priv->query_video_keys;
      box = grl_media_video_new ();
      break;

    case MEX_TRACKER_CATEGORY_MUSIC:
      cat_name = "music";
      query = "?urn a nfo:FileDataObject . "
              "?urn tracker:available true . "
              "FILTER (fn:starts-with(nie:mimeType(?urn),'audio/'))";
      models = self->priv->music_models;
      metadata_keys = self->priv->music_keys;
      query_keys = self->priv->query_music_keys;
      box = grl_media_audio_new ();
      break;
    }

  grl_media_set_id (GRL_MEDIA (box), NULL);
  feed = mex_grilo_tracker_feed_new (GRL_MEDIA_SOURCE (plugin),
                                     query_keys,
                                     metadata_keys,
                                     NULL, box);
  mex_model_set_sort_func (MEX_MODEL (feed),
                           mex_model_sort_time_cb,
                           GINT_TO_POINTER (TRUE));
  mex_grilo_feed_query (MEX_GRILO_FEED (feed), query, 0, MAX_TRACKER_RESULTS);

  g_hash_table_insert (models, plugin, feed);

  /* set the local files source to appear first */
  if (!g_strcmp0 (source_name, "Local files"))
    priority = -100;
  else
    priority = 0;

  dir_feed = mex_grilo_tracker_feed_new (GRL_MEDIA_SOURCE (plugin),
                                         query_keys,
                                         metadata_keys,
                                         query, NULL);
  mex_model_set_sort_func (MEX_MODEL (dir_feed),
                           mex_model_sort_alpha_cb,
                           GINT_TO_POINTER (FALSE));
  mex_grilo_feed_browse (MEX_GRILO_FEED (dir_feed), 0, G_MAXINT);

  g_object_set (G_OBJECT (feed),
                "category", cat_name,
                "priority", priority,
                "alt-model", dir_feed,
                "alt-model-string", _("Show Folders"),
                NULL);

  mex_model_manager_add_model (self->priv->manager, MEX_MODEL (feed));
}

static void
handle_new_source_plugin (MexTrackerPlugin *self, GrlMediaPlugin *plugin)
{
  GrlSupportedOps ops;
  GrlMetadataSource *meta = GRL_METADATA_SOURCE (plugin);
  const char *id;

  id = grl_media_plugin_get_id (plugin);
  if (g_strcmp0 (id, "grl-tracker") != 0)
    return;

  ops = grl_metadata_source_supported_operations (meta);
  if ((ops & GRL_OP_QUERY) == 0)
    return;

  grl_media_source_notify_change_start (GRL_MEDIA_SOURCE (plugin), NULL);

  add_model (self, plugin, MEX_TRACKER_CATEGORY_VIDEO);
  add_model (self, plugin, MEX_TRACKER_CATEGORY_IMAGE);
  add_model (self, plugin, MEX_TRACKER_CATEGORY_MUSIC);
}

static void
registry_source_added_cb (GrlPluginRegistry *registry,
                          GrlMediaPlugin *source,
                          MexTrackerPlugin *plugin)
{
  handle_new_source_plugin (plugin, source);
}

/*static int
source_compare (gconstpointer listdata, gconstpointer userdata)
{
  GrlMediaSource *user_source, *list_source;

  g_object_get (MEX_GRILO_FEED (listdata),
                "grilo-source", &list_source,
                NULL);
  user_source = GRL_MEDIA_SOURCE (userdata);

  return (user_source == list_source) ? 0 : -1;
}*/

static void
registry_source_removed_cb (GrlPluginRegistry *registry,
                            GrlMediaPlugin *source,
                            MexTrackerPlugin *self)
{
  MexTrackerPluginPrivate *priv = self->priv;
  MexModel *model;

  model = g_hash_table_lookup (priv->video_models, source);
  if (model)
    {
      mex_model_manager_remove_model (priv->manager, model);
      g_hash_table_remove (priv->video_models, source);
    }

  model = g_hash_table_lookup (priv->image_models, source);
  if (model)
    {
      mex_model_manager_remove_model (priv->manager, model);
      g_hash_table_remove (priv->image_models, source);
    }

  model = g_hash_table_lookup (priv->music_models, source);
  if (model)
    {
      mex_model_manager_remove_model (priv->manager, model);
      g_hash_table_remove (priv->music_models, source);
    }
}

static void
mex_tracker_plugin_init (MexTrackerPlugin  *self)
{
  MexTrackerPluginPrivate *priv;
  GrlPluginRegistry *registry;
  GList *plugins, *iter;

  priv = self->priv = GET_PRIVATE (self);

  priv->image_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                              g_object_unref, NULL);
  priv->video_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                              g_object_unref, NULL);
  priv->music_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                              g_object_unref, NULL);

  priv->query_video_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                      GRL_METADATA_KEY_TITLE,
                                                      GRL_METADATA_KEY_MIME,
                                                      GRL_METADATA_KEY_URL,
                                                      GRL_METADATA_KEY_DATE,
                                                      GRL_METADATA_KEY_THUMBNAIL,
                                                      GRL_METADATA_KEY_PLAY_COUNT,
                                                      NULL);

  priv->query_image_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                      GRL_METADATA_KEY_TITLE,
                                                      GRL_METADATA_KEY_MIME,
                                                      GRL_METADATA_KEY_URL,
                                                      GRL_METADATA_KEY_DATE,
                                                      GRL_METADATA_KEY_THUMBNAIL,
                                                      GRL_METADATA_KEY_PLAY_COUNT,
                                                      NULL);

  priv->query_music_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                      GRL_METADATA_KEY_TITLE,
                                                      GRL_METADATA_KEY_MIME,
                                                      GRL_METADATA_KEY_URL,
                                                      GRL_METADATA_KEY_DATE,
                                                      GRL_METADATA_KEY_THUMBNAIL,
                                                      GRL_METADATA_KEY_PLAY_COUNT,
                                                      GRL_METADATA_KEY_ALBUM,
                                                      GRL_METADATA_KEY_ARTIST,
                                                      NULL);

  priv->image_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_DESCRIPTION,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                GRL_METADATA_KEY_WIDTH,
                                                GRL_METADATA_KEY_HEIGHT,
                                                GRL_METADATA_KEY_LAST_PLAYED,
                                                GRL_METADATA_KEY_CAMERA_MODEL,
                                                GRL_METADATA_KEY_EXPOSURE_TIME,
                                                GRL_METADATA_KEY_ISO_SPEED,
                                                GRL_METADATA_KEY_FLASH_USED,
                                                GRL_METADATA_KEY_ORIENTATION,
                                                GRL_METADATA_KEY_CREATION_DATE,
                                                NULL);

  priv->video_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_DESCRIPTION,
                                                GRL_METADATA_KEY_DURATION,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                GRL_METADATA_KEY_WIDTH,
                                                GRL_METADATA_KEY_HEIGHT,
                                                GRL_METADATA_KEY_LAST_POSITION,
                                                GRL_METADATA_KEY_LAST_PLAYED,
                                                NULL);

  priv->music_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_DURATION,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                GRL_METADATA_KEY_LAST_POSITION,
                                                GRL_METADATA_KEY_LAST_PLAYED,
                                                NULL);

  priv->manager = mex_model_manager_get_default ();

  /* log domain */
  MEX_LOG_DOMAIN_INIT (tracker_log_domain, "tracker");

  registry = grl_plugin_registry_get_default ();
  plugins = grl_plugin_registry_get_sources (registry, FALSE);
  for (iter = plugins; iter != NULL; iter = iter->next)
    handle_new_source_plugin (self, GRL_MEDIA_PLUGIN (iter->data));
  g_list_free (plugins);

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (registry_source_added_cb), self);
  g_signal_connect (registry, "source-removed",
                    G_CALLBACK (registry_source_removed_cb), self);
}

MexTrackerPlugin *
mex_tracker_plugin_new (void)
{
  return g_object_new (MEX_TYPE_TRACKER_PLUGIN, NULL);
}

static GType
mex_tracker_get_type (void)
{
  return MEX_TYPE_TRACKER_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Tracker",
		   "Tracker integration",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_tracker_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
