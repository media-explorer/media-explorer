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

#include <grilo.h>
#include <mex/mex-grilo-feed.h>

#include "mex-bliptv-plugin.h"

G_DEFINE_TYPE (MexBliptvPlugin, mex_bliptv_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o)                                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_BLIPTV_PLUGIN,         \
                                MexBliptvPluginPrivate))

struct _MexBliptvPluginPrivate {
  MexModelManager *manager;
  GHashTable *video_models;

  GList *query_keys;
  GList *video_keys;
};

static void
remove_model (gpointer key, gpointer value, gpointer user_data)
{
  MexBliptvPlugin *self = MEX_BLIPTV_PLUGIN (user_data);
  MexModel *model = MEX_MODEL (value);

  mex_model_manager_remove_model (self->priv->manager, model);
}

static void
mex_bliptv_plugin_finalize (GObject *gobject)
{
  MexBliptvPlugin *self = MEX_BLIPTV_PLUGIN (gobject);
  MexBliptvPluginPrivate *priv = self->priv;

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

  if (priv->video_models)
    {
      g_hash_table_foreach (priv->video_models, remove_model, self);
      g_hash_table_destroy (priv->video_models);
      priv->video_models = NULL;
    }

  G_OBJECT_CLASS (mex_bliptv_plugin_parent_class)->finalize (gobject);
}

static void
mex_bliptv_plugin_class_init (MexBliptvPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = mex_bliptv_plugin_finalize;

  g_type_class_add_private (klass, sizeof (MexBliptvPluginPrivate));
}

static void
add_model (MexBliptvPlugin *self,
           GrlMediaPlugin *plugin)
{
  MexFeed *feed;
  GrlMedia *box;

  box = grl_media_video_new ();

  grl_media_set_id (GRL_MEDIA (box), NULL);
  feed = mex_grilo_feed_new (GRL_MEDIA_SOURCE (plugin),
                             self->priv->query_keys,
                             self->priv->video_keys, box);
  g_object_set (feed, "icon-name", "icon-panelheader-computer",
                "placeholder-text", "No videos found",
                "category", "videos",
                NULL);
  mex_grilo_feed_browse (MEX_GRILO_FEED (feed), 0, 100);

  g_hash_table_insert (self->priv->video_models, plugin, feed);

  mex_model_manager_add_model (self->priv->manager, MEX_MODEL (feed));
}

static void
handle_new_source_plugin (MexBliptvPlugin *self, GrlMediaPlugin *plugin)
{
  const char *id;

  id = grl_media_plugin_get_id (plugin);
  if (g_strcmp0 (id,"grl-bliptv") != 0)
    return;

  add_model (self, plugin);
}

static void
registry_source_added_cb (GrlPluginRegistry *registry,
                          GrlMediaPlugin *source,
                          MexBliptvPlugin *plugin)
{
  handle_new_source_plugin (plugin, source);
}

static void
registry_source_removed_cb (GrlPluginRegistry *registry,
                            GrlMediaPlugin *source,
                            MexBliptvPlugin *self)
{
  MexBliptvPluginPrivate *priv = self->priv;
  MexModel *model;

  model = g_hash_table_lookup (priv->video_models, source);
  if (model)
    {
      mex_model_manager_remove_model (priv->manager, model);
      g_hash_table_remove (priv->video_models, source);
    }
}

static void
mex_bliptv_plugin_init (MexBliptvPlugin  *self)
{
  MexBliptvPluginPrivate *priv;
  GrlPluginRegistry *registry;
  GList *plugins, *iter;

  priv = self->priv = GET_PRIVATE (self);

  priv->video_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, NULL);

  priv->query_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_TITLE,
                                                GRL_METADATA_KEY_MIME,
                                                GRL_METADATA_KEY_URL,
                                                GRL_METADATA_KEY_DATE,
                                                NULL);

  priv->video_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_DESCRIPTION,
                                                GRL_METADATA_KEY_DURATION,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                GRL_METADATA_KEY_WIDTH,
                                                GRL_METADATA_KEY_HEIGHT,
                                                NULL);

  priv->manager = mex_model_manager_get_default ();

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

static GType
mex_blibtv_get_type (void)
{
  return MEX_TYPE_BLIPTV_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Blip TV",
		   "Blip TV integration",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_blibtv_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
