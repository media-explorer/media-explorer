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
#include <mex/mex-grilo-feed.h>
#include "mex-upnp-plugin.h"

G_DEFINE_TYPE (MexUpnpPlugin, mex_upnp_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_UPNP_PLUGIN, MexUpnpPluginPrivate))

struct _MexUpnpPluginPrivate {
    MexModelManager *manager;
    GHashTable *video_models;
    GHashTable *image_models;
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
           GrlMediaPlugin *plugin,
           MexUpnpCategory category)
{
  MexFeed *feed;
  GrlMedia *box;
  MexModelInfo *info;
  char *query, *cat_name, *placeholder;
  GHashTable *models;

  switch (category)
    {
    case MEX_UPNP_CATEGORY_IMAGE:
      cat_name = "pictures";
      query = "(upnp:class derivedfrom \"object.item.imageItem\")";
      models = self->priv->image_models;
      placeholder = "No pictures found";
      box = grl_media_image_new ();
      break;
    case MEX_UPNP_CATEGORY_VIDEO:
      cat_name = "videos";
      query = "(upnp:class derivedfrom \"object.item.videoItem\")";
      models = self->priv->video_models;
      placeholder = "No videos found";
      box = grl_media_video_new ();
      break;
    }

  grl_media_set_id (GRL_MEDIA (box), NULL);
  feed = mex_grilo_feed_new (GRL_MEDIA_SOURCE (plugin), box);
  g_object_set (feed, "icon-name", "icon-panelheader-computer",
                "placeholder-text", placeholder,
                NULL);
  mex_grilo_feed_query (MEX_GRILO_FEED (feed), query, 0, G_MAXINT);

  g_hash_table_insert (models, plugin, feed);

  info = mex_model_info_new_with_sort_funcs (MEX_MODEL (feed), cat_name, 0);
  mex_model_manager_add_model (self->priv->manager, info);
  mex_model_info_free (info);
}

static void
handle_new_source_plugin (MexUpnpPlugin *self, GrlMediaPlugin *plugin)
{
  GrlSupportedOps ops;
  GrlMetadataSource *meta = GRL_METADATA_SOURCE (plugin);
  const char *id;

  id = grl_media_plugin_get_id (plugin);
  if (g_strcmp0 (id,"grl-upnp") != 0)
    return;

  ops = grl_metadata_source_supported_operations (meta);
  if ((ops & GRL_OP_QUERY) == 0)
    {
      g_warning ("UPnP source does not support Query, skipping for now");
      return;
    }

  add_model (self, plugin, MEX_UPNP_CATEGORY_VIDEO);
  add_model (self, plugin, MEX_UPNP_CATEGORY_IMAGE);
}

static void
registry_source_added_cb (GrlPluginRegistry *registry,
                          GrlMediaPlugin *source,
                          MexUpnpPlugin *plugin)
{
  handle_new_source_plugin (plugin, source);
}

static int
source_compare (gconstpointer listdata, gconstpointer userdata)
{
  GrlMediaSource *user_source, *list_source;

  g_object_get (MEX_GRILO_FEED (listdata),
                "grilo-source", &list_source,
                NULL);
  user_source = GRL_MEDIA_SOURCE (userdata);

  return (user_source == list_source) ? 0 : -1;
}

static void
registry_source_removed_cb (GrlPluginRegistry *registry,
                            GrlMediaPlugin *source,
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
  GrlPluginRegistry *registry;
  GList *plugins, *iter;

  priv = self->priv = GET_PRIVATE (self);

  priv->image_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, NULL);
  priv->video_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, NULL);

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

MexUpnpPlugin *
mex_upnp_plugin_new (void)
{
  return g_object_new (MEX_TYPE_UPNP_PLUGIN, NULL);
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_UPNP_PLUGIN;
}
