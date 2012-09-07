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

#include "mex-justintv-plugin.h"

G_DEFINE_TYPE (MexJustintvPlugin, mex_justintv_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o)                                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_JUSTINTV_PLUGIN,         \
                                MexJustintvPluginPrivate))

struct _MexJustintvPluginPrivate {
  MexModelManager *manager;
  GHashTable *video_models;

  GList *query_keys;
  GList *video_keys;
};

static void
remove_model (gpointer key, gpointer value, gpointer user_data)
{
  MexJustintvPlugin *self = MEX_JUSTINTV_PLUGIN (user_data);
  MexModel *model = MEX_MODEL (value);

  mex_model_manager_remove_model (self->priv->manager, model);
}

static void
mex_justintv_plugin_finalize (GObject *gobject)
{
  MexJustintvPlugin *self = MEX_JUSTINTV_PLUGIN (gobject);
  MexJustintvPluginPrivate *priv = self->priv;

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

  G_OBJECT_CLASS (mex_justintv_plugin_parent_class)->finalize (gobject);
}

static void
mex_justintv_plugin_class_init (MexJustintvPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = mex_justintv_plugin_finalize;

  g_type_class_add_private (klass, sizeof (MexJustintvPluginPrivate));
}

static void
add_model (MexJustintvPlugin *self,
           GrlSource *source)
{
  MexFeed *feed;
  GrlMedia *box;

  box = grl_media_video_new ();

  grl_media_set_id (GRL_MEDIA (box), NULL);
  feed = mex_grilo_feed_new (source,
                             self->priv->query_keys,
                             self->priv->video_keys, box);
  g_object_set (feed, "icon-name", "icon-panelheader-computer",
                "placeholder-text", "No videos found",
                "category", "videos",
                NULL);
  mex_grilo_feed_browse (MEX_GRILO_FEED (feed), 0, 100);

  g_hash_table_insert (self->priv->video_models, source, feed);

  mex_model_manager_add_model (self->priv->manager, MEX_MODEL (feed));
}

static void
handle_new_source (MexJustintvPlugin *self, GrlSource *source)
{
  const char *id;

  id = grl_source_get_id (source);
  if (g_strcmp0 (id,"grl-justintv") != 0)
    return;

  add_model (self, source);
}

static void
registry_source_added_cb (GrlRegistry *registry,
                          GrlSource *source,
                          MexJustintvPlugin *plugin)
{
  handle_new_source (plugin, source);
}

static void
registry_source_removed_cb (GrlRegistry *registry,
                            GrlSource *source,
                            MexJustintvPlugin *self)
{
  MexJustintvPluginPrivate *priv = self->priv;
  MexModel *model;

  model = g_hash_table_lookup (priv->video_models, source);
  if (model)
    {
      mex_model_manager_remove_model (priv->manager, model);
      g_hash_table_remove (priv->video_models, source);
    }
}

static void
mex_justintv_plugin_init (MexJustintvPlugin  *self)
{
  MexJustintvPluginPrivate *priv;
  GrlRegistry *registry;
  GList *sources, *iter;

  priv = self->priv = GET_PRIVATE (self);

  priv->video_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, NULL);

  priv->query_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_TITLE,
                                                GRL_METADATA_KEY_MIME,
                                                GRL_METADATA_KEY_URL,
                                                GRL_METADATA_KEY_PUBLICATION_DATE,
                                                NULL);

  priv->video_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_DESCRIPTION,
                                                GRL_METADATA_KEY_DURATION,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                GRL_METADATA_KEY_WIDTH,
                                                GRL_METADATA_KEY_HEIGHT,
                                                NULL);

  priv->manager = mex_model_manager_get_default ();

  registry = grl_registry_get_default ();
  sources = grl_registry_get_sources (registry, FALSE);
  for (iter = sources; iter != NULL; iter = iter->next)
    handle_new_source (self, GRL_SOURCE (iter->data));
  g_list_free (sources);

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (registry_source_added_cb), self);
  g_signal_connect (registry, "source-removed",
                    G_CALLBACK (registry_source_removed_cb), self);
}

MexJustintvPlugin *
mex_justintv_plugin_new (void)
{
  return g_object_new (MEX_TYPE_JUSTINTV_PLUGIN, NULL);
}

static GType
mex_justintv_get_type (void)
{
  return MEX_TYPE_JUSTINTV_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Justin TV",
		   "Justin TV integration",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_justintv_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
