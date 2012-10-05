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
#include "mex-upnp-plugin.h"

G_DEFINE_TYPE (MexUpnpPlugin, mex_upnp_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_UPNP_PLUGIN, MexUpnpPluginPrivate))

struct _MexUpnpPluginPrivate {
  MexModelManager *manager;
  GHashTable *video_models;
  GHashTable *image_models;

  GList *query_keys;
  GList *video_keys;
  GList *image_keys;
};

typedef enum {
  MEX_UPNP_CATEGORY_IMAGE,
  MEX_UPNP_CATEGORY_VIDEO,
} MexUpnpCategory;

static void
remove_model (gpointer key, gpointer value, gpointer user_data)
{
  MexUpnpPlugin *self = MEX_UPNP_PLUGIN (user_data);
  MexModel *model = MEX_MODEL (value);

  mex_model_manager_remove_model (self->priv->manager, model);
}

static void
mex_upnp_plugin_finalize (GObject *gobject)
{
  MexUpnpPlugin *self = MEX_UPNP_PLUGIN (gobject);
  MexUpnpPluginPrivate *priv = self->priv;

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

  G_OBJECT_CLASS (mex_upnp_plugin_parent_class)->finalize (gobject);
}

static void
mex_upnp_plugin_class_init (MexUpnpPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = mex_upnp_plugin_finalize;

  g_type_class_add_private (klass, sizeof (MexUpnpPluginPrivate));
}

static void
add_model (MexUpnpPlugin *self,
           GrlSource *source,
           MexUpnpCategory category)
{
  GList *metadata_keys;
  MexFeed *feed;
  GrlMedia *box;
  char *query, *cat_name, *placeholder;
  GHashTable *models;

  switch (category)
    {
    case MEX_UPNP_CATEGORY_IMAGE:
      cat_name = "pictures";
      query = "(upnp:class derivedfrom \"object.item.imageItem\")";
      models = self->priv->image_models;
      placeholder = "No pictures found";
      metadata_keys = self->priv->image_keys;
      box = grl_media_image_new ();
      break;
    case MEX_UPNP_CATEGORY_VIDEO:
      cat_name = "videos";
      query = "(upnp:class derivedfrom \"object.item.videoItem\")";
      models = self->priv->video_models;
      placeholder = "No videos found";
      metadata_keys = self->priv->video_keys;
      box = grl_media_video_new ();
      break;
    }

  grl_media_set_id (GRL_MEDIA (box), NULL);
  feed = mex_grilo_feed_new (source,
                             self->priv->query_keys,
                             metadata_keys, box);
  mex_model_set_sort_func (MEX_MODEL (feed),
                           mex_model_sort_time_cb,
                           GINT_TO_POINTER (TRUE));
  g_object_set (feed, "icon-name", "icon-panelheader-computer",
                "placeholder-text", placeholder,
                "category", cat_name,
                NULL);
  mex_grilo_feed_query (MEX_GRILO_FEED (feed), query, 0, G_MAXINT);

  g_hash_table_insert (models, source, feed);

  mex_model_manager_add_model (self->priv->manager, MEX_MODEL (feed));
}

static void
handle_new_source (MexUpnpPlugin *self, GrlSource *source)
{
  GrlSupportedOps ops;
  const char *id;
  GrlPlugin *plugin;

  plugin = grl_source_get_plugin (source);

  id = grl_plugin_get_id (plugin);
  if (g_strcmp0 (id,"grl-upnp") != 0)
    return;

  ops = grl_source_supported_operations (source);
  if ((ops & GRL_OP_QUERY) == 0)
    {
      g_warning ("UPnP source does not support Query, skipping for now");
      return;
    }

  add_model (self, source, MEX_UPNP_CATEGORY_VIDEO);
  add_model (self, source, MEX_UPNP_CATEGORY_IMAGE);
}

static void
registry_source_added_cb (GrlRegistry *registry,
                          GrlSource *source,
                          MexUpnpPlugin *plugin)
{
  handle_new_source (plugin, source);
}

static void
registry_source_removed_cb (GrlRegistry *registry,
                            GrlSource *source,
                            MexUpnpPlugin *self)
{
  MexUpnpPluginPrivate *priv = self->priv;
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
}

static void
mex_upnp_plugin_init (MexUpnpPlugin  *self)
{
  MexUpnpPluginPrivate *priv;
  GrlRegistry *registry;
  GList *sources, *iter;

  priv = self->priv = GET_PRIVATE (self);

  priv->image_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, NULL);
  priv->video_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, NULL);

  priv->query_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_TITLE,
                                                GRL_METADATA_KEY_MIME,
                                                GRL_METADATA_KEY_URL,
                                                NULL);

  priv->image_keys = grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                                GRL_METADATA_KEY_DESCRIPTION,
                                                GRL_METADATA_KEY_THUMBNAIL,
                                                GRL_METADATA_KEY_WIDTH,
                                                GRL_METADATA_KEY_HEIGHT,
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

  g_signal_connect (registry, "source-added",
                    G_CALLBACK (registry_source_added_cb), self);
  g_signal_connect (registry, "source-removed",
                    G_CALLBACK (registry_source_removed_cb), self);
}

MexUpnpPlugin *
mex_upnp_plugin_new (void)
{
  return g_object_new (MEX_TYPE_UPNP_PLUGIN, NULL);
}

static GType
mex_upnp_get_type (void)
{
  return MEX_TYPE_UPNP_PLUGIN;
}

MEX_DEFINE_PLUGIN ("UPnP",
		   "UPnP integration",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_upnp_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
