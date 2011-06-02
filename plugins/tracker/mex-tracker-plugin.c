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
/* #include <mex/mex-grilo-tracker-feed.h> */
#include "mex-tracker-plugin.h"
#include "mex-tracker-model.h"

G_DEFINE_TYPE (MexTrackerPlugin, mex_tracker_plugin, G_TYPE_OBJECT)

#define MAX_TRACKER_RESULTS G_MAXINT

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TRACKER_PLUGIN, MexTrackerPluginPrivate))

struct _MexTrackerPluginPrivate {
  MexModelManager *manager;
  GHashTable *video_models;
  GHashTable *image_models;
  GHashTable *music_models;

  GList *query_keys;
  GList *video_keys;
  GList *image_keys;
  GList *music_keys;
};

typedef enum {
  MEX_TRACKER_CATEGORY_IMAGE,
  MEX_TRACKER_CATEGORY_VIDEO,
  MEX_TRACKER_CATEGORY_MUSIC,
  MEX_TRACKER_CATEGORY_SERIES,
  MEX_TRACKER_CATEGORY_LASTSEEN,
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

  if (priv->query_keys)
    {
      g_list_free (priv->query_keys);
      priv->query_keys = NULL;
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

  mex_tracker_connection_init ();
}

static void
add_model (MexTrackerPlugin *self, MexTrackerCategory category)
{
  GList *metadata_keys;
  MexModel *model;
  MexModelInfo *info;
  gchar *query, *cat_name;
  /* GHashTable *models; */
  gint priority;

  switch (category)
    {
    case MEX_TRACKER_CATEGORY_IMAGE:
      cat_name = "pictures";
      query = "?u a nmm:Photo . ?u tracker:available true";
      /* models = self->priv->image_models; */
      metadata_keys = self->priv->image_keys;
      break;

    case MEX_TRACKER_CATEGORY_VIDEO:
      cat_name = "videos";
      query = "?u a nmm:Video . ?u tracker:available true";
      /* models = self->priv->video_models; */
      metadata_keys = self->priv->video_keys;
      break;

    case MEX_TRACKER_CATEGORY_MUSIC:
      cat_name = "music";
      query = "?u a nmm:MusicPiece . ?u tracker:available true";
      /* models = self->priv->music_models; */
      metadata_keys = self->priv->music_keys;
      break;

    case MEX_TRACKER_CATEGORY_SERIES:
      cat_name = "series";
      query = "?u a nmm:Video . ?u tracker:available true . FILTER(bound(nmm:episodeNumber(?u)))";
      /* models = self->priv->video_models; */
      metadata_keys = self->priv->video_keys;
      break;

    case MEX_TRACKER_CATEGORY_LASTSEEN:
      cat_name = "lastseen";
      query = "?u a nmm:Video . ?u tracker:available true . FILTER(bound(nie:contentAccessed(?u)))";
      /* models = self->priv->video_models; */
      metadata_keys = self->priv->video_keys;
      break;
    }

  model = (MexModel *) g_object_new (MEX_TYPE_TRACKER_MODEL, "filter", query, NULL);

  priority = -100;

  info = mex_model_info_new_with_sort_funcs (model, cat_name,
                                             priority);

  mex_tracker_model_start (MEX_TRACKER_MODEL (model));

  /* Set 'Newest' as the default sort function */
  info->default_sort_index = 2;

  mex_model_manager_add_model (self->priv->manager, info);
  mex_model_info_free (info);
}

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
  MexModelCategoryInfo series = { "series",
                                  _("Series"),
                                  "icon-panelheader-videos",
                                  20,
                                  _("Connect an external drive or update your network settings to see Series here.") };
  MexModelCategoryInfo lastseen = { "lastseen",
                                    _("Last Seen"),
                                    "icon-panelheader-videos",
                                    45,
                                    _("No last seen content yet... Go watch something !") };


  priv = self->priv = GET_PRIVATE (self);

  priv->image_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                              g_object_unref, NULL);
  priv->video_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                              g_object_unref, NULL);
  priv->music_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                              g_object_unref, NULL);

  priv->query_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_TITLE,
                                                GRL_METADATA_KEY_MIME,
                                                GRL_METADATA_KEY_URL,
                                                GRL_METADATA_KEY_DATE,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                NULL);

  priv->image_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_DESCRIPTION,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                GRL_METADATA_KEY_WIDTH,
                                                GRL_METADATA_KEY_HEIGHT,
                                                GRL_METADATA_KEY_PLAY_COUNT,
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
                                                GRL_METADATA_KEY_PLAY_COUNT,
                                                GRL_METADATA_KEY_LAST_PLAYED,
                                                NULL);

  priv->music_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_DURATION,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                GRL_METADATA_KEY_LAST_POSITION,
                                                GRL_METADATA_KEY_PLAY_COUNT,
                                                GRL_METADATA_KEY_LAST_PLAYED,
                                                GRL_METADATA_KEY_ARTIST,
                                                GRL_METADATA_KEY_ALBUM,
                                                NULL);

  priv->manager = mex_model_manager_get_default ();

  mex_model_manager_add_category (priv->manager, &series);
  mex_model_manager_add_category (priv->manager, &lastseen);

  add_model (self, MEX_TRACKER_CATEGORY_VIDEO);
  add_model (self, MEX_TRACKER_CATEGORY_SERIES);
  add_model (self, MEX_TRACKER_CATEGORY_IMAGE);
  add_model (self, MEX_TRACKER_CATEGORY_MUSIC);
  add_model (self, MEX_TRACKER_CATEGORY_LASTSEEN);
}

MexTrackerPlugin *
mex_tracker_plugin_new (void)
{
  return g_object_new (MEX_TYPE_TRACKER_PLUGIN, NULL);
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_TRACKER_PLUGIN;
}
