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

#define GET_PRIVATE(o)                                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TRACKER_PLUGIN,        \
                                MexTrackerPluginPrivate))

struct _MexTrackerPluginPrivate {
  MexModelManager *manager;
  GHashTable *models;

  MexTrackerMetadatas *initial_metadatas;
  MexTrackerMetadatas *video_metadatas;
  MexTrackerMetadatas *image_metadatas;
  MexTrackerMetadatas *music_metadatas;
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

  if (priv->initial_metadatas)
    {
      g_object_unref (priv->initial_metadatas);
      priv->initial_metadatas = NULL;
    }

  if (priv->video_metadatas)
    {
      g_object_unref (priv->video_metadatas);
      priv->video_metadatas = NULL;
    }

  if (priv->image_metadatas)
    {
      g_object_unref (priv->image_metadatas);
      priv->image_metadatas = NULL;
    }

  if (priv->music_metadatas)
    {
      g_object_unref (priv->music_metadatas);
      priv->music_metadatas = NULL;
    }

  if (priv->models)
    {
      g_hash_table_foreach (priv->models, remove_model, self);
      g_hash_table_destroy (priv->models);
      priv->models = NULL;
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
  MexTrackerPluginPrivate *priv = self->priv;
  MexTrackerMetadatas *metadata_keys;
  MexTrackerModel *model;
  MexModelInfo *info;
  gchar *query, *cat_name;
  gint priority;

  switch (category)
    {
    case MEX_TRACKER_CATEGORY_IMAGE:
      cat_name = "pictures";
      query = "?u a nmm:Photo . ?u tracker:available true";
      metadata_keys = priv->image_metadatas;
      break;

    case MEX_TRACKER_CATEGORY_VIDEO:
      cat_name = "videos";
      query = "?u a nmm:Video . ?u tracker:available true";
      metadata_keys = priv->video_metadatas;
      break;

    case MEX_TRACKER_CATEGORY_MUSIC:
      cat_name = "music";
      query = "?u a nmm:MusicPiece . ?u tracker:available true";
      metadata_keys = priv->music_metadatas;
      break;

    case MEX_TRACKER_CATEGORY_SERIES:
      cat_name = "series";
      query = "?u a nmm:Video . ?u tracker:available true . FILTER(bound(nmm:episodeNumber(?u)))";
      metadata_keys = priv->video_metadatas;
      break;

    case MEX_TRACKER_CATEGORY_LASTSEEN:
      cat_name = "lastseen";
      query = "?u a nmm:Video . ?u tracker:available true . FILTER(bound(nie:contentAccessed(?u)))";
      metadata_keys = priv->video_metadatas;
      break;
    }

  model = mex_tracker_model_new (priv->initial_metadatas, metadata_keys);

  g_hash_table_insert (priv->models, model, model);

  mex_tracker_model_set_filter (model, query);
  mex_tracker_model_start (model);

  priority = -100;

  info = mex_model_info_new_with_sort_funcs (MEX_MODEL (model),
                                             cat_name,
                                             priority);


  /* Set 'Newest' as the default sort function */
  info->default_sort_index = 2;

  mex_model_manager_add_model (priv->manager, info);
  mex_model_info_free (info);
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

  priv->models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                        g_object_unref, NULL);

  priv->initial_metadatas =
    mex_tracker_metadatas_new (MEX_CONTENT_METADATA_ID,
                               MEX_CONTENT_METADATA_TITLE,
                               MEX_CONTENT_METADATA_MIMETYPE,
                               MEX_CONTENT_METADATA_STREAM,
                               MEX_CONTENT_METADATA_DATE,
                               MEX_CONTENT_METADATA_STREAM,
                               MEX_CONTENT_METADATA_INVALID);

  priv->image_metadatas =
    mex_tracker_metadatas_new (MEX_CONTENT_METADATA_ID,
                               MEX_CONTENT_METADATA_STILL,
                               MEX_CONTENT_METADATA_WIDTH,
                               MEX_CONTENT_METADATA_HEIGHT,
                               MEX_CONTENT_METADATA_PLAY_COUNT,
                               MEX_CONTENT_METADATA_LAST_PLAYED_DATE,
                               MEX_CONTENT_METADATA_CAMERA_MODEL,
                               MEX_CONTENT_METADATA_EXPOSURE_TIME,
                               MEX_CONTENT_METADATA_ISO_SPEED,
                               MEX_CONTENT_METADATA_FLASH_USED,
                               MEX_CONTENT_METADATA_ORIENTATION,
                               MEX_CONTENT_METADATA_CREATION_DATE,
                               MEX_CONTENT_METADATA_INVALID);

  priv->video_metadatas =
    mex_tracker_metadatas_new (MEX_CONTENT_METADATA_ID,
                               MEX_CONTENT_METADATA_SYNOPSIS,
                               MEX_CONTENT_METADATA_DURATION,
                               MEX_CONTENT_METADATA_STREAM,
                               MEX_CONTENT_METADATA_WIDTH,
                               MEX_CONTENT_METADATA_HEIGHT,
                               MEX_CONTENT_METADATA_LAST_POSITION,
                               MEX_CONTENT_METADATA_PLAY_COUNT,
                               MEX_CONTENT_METADATA_LAST_PLAYED_DATE,
                               MEX_CONTENT_METADATA_INVALID);

  priv->music_metadatas =
    mex_tracker_metadatas_new (MEX_CONTENT_METADATA_ID,
                               MEX_CONTENT_METADATA_DURATION,
                               MEX_CONTENT_METADATA_STREAM,
                               MEX_CONTENT_METADATA_LAST_POSITION,
                               MEX_CONTENT_METADATA_PLAY_COUNT,
                               MEX_CONTENT_METADATA_LAST_PLAYED_DATE,
                               MEX_CONTENT_METADATA_ARTIST,
                               MEX_CONTENT_METADATA_ALBUM,
                               MEX_CONTENT_METADATA_INVALID);

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
