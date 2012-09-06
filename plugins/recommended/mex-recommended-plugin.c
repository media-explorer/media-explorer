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

#include <mex/mex-plugin.h>
#include <mex/mex-grilo-feed.h>

#include "mex-recommended-plugin.h"

static void mex_model_provider_iface_init (MexModelProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexRecommendedPlugin, mex_recommended_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                mex_model_provider_iface_init))

#define RECOMMENDED_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_RECOMMENDED_PLUGIN, MexRecommendedPluginPrivate))

struct _MexRecommendedPluginPrivate
{
  GList *models;
  MexModel *model;
};


static void
mex_recommended_plugin_dispose (GObject *object)
{
  MexRecommendedPluginPrivate *priv = MEX_RECOMMENDED_PLUGIN (object)->priv;

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  G_OBJECT_CLASS (mex_recommended_plugin_parent_class)->dispose (object);
}

static void
mex_recommended_plugin_finalize (GObject *object)
{
  MexRecommendedPluginPrivate *priv = MEX_RECOMMENDED_PLUGIN (object)->priv;

  g_list_free (priv->models);

  G_OBJECT_CLASS (mex_recommended_plugin_parent_class)->finalize (object);
}

static void
mex_recommended_plugin_class_init (MexRecommendedPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexRecommendedPluginPrivate));

  object_class->dispose = mex_recommended_plugin_dispose;
  object_class->finalize = mex_recommended_plugin_finalize;
}

static const GList *
mex_recommended_plugin_get_models (MexModelProvider *self)
{
  MexRecommendedPluginPrivate *priv = MEX_RECOMMENDED_PLUGIN (self)->priv;
  return priv->models;
}

static void
mex_model_provider_iface_init (MexModelProviderInterface *iface)
{
  iface->get_models = mex_recommended_plugin_get_models;
}

static void
mex_recommended_plugin_mimetype_set_cb (MexProgram *program)
{
  const gchar *mime = mex_content_get_metadata (MEX_CONTENT (program),
                                                MEX_CONTENT_METADATA_MIMETYPE);
  if (!mime || !(*mime))
    mex_content_set_metadata (MEX_CONTENT (program),
                              MEX_CONTENT_METADATA_MIMETYPE,
                              "x-mex/media");
}

static void
mex_recommended_plugin_model_changed_cb (GController          *controller,
                                         GControllerAction     action,
                                         GControllerReference *ref,
                                         MexModel             *model)
{
  gint i;
  gint n_indices = g_controller_reference_get_n_indices (ref);

  /* We set a custom media mimetype because we know that Apple Trailers
   * provides videos, but Apple doesn't include the mime-type in its xml :(
   */
  switch (action)
    {
    case G_CONTROLLER_ADD:
      for (i = 0; i < n_indices; i++)
        {
          gchar *notify;
          MexContent *content;
          const gchar *property_name;

          gint content_index = g_controller_reference_get_index_uint (ref, i);
          content = mex_model_get_content (model, content_index);

          /* Make sure the mime-type remains useful */
          property_name =
            mex_content_get_property_name (content,
                                           MEX_CONTENT_METADATA_MIMETYPE);
          if (property_name)
            {
              notify = g_strconcat ("notify::", property_name, NULL);
              g_signal_connect (content, notify,
                                G_CALLBACK (mex_recommended_plugin_mimetype_set_cb),
                                NULL);
              g_free (notify);
            }

          mex_recommended_plugin_mimetype_set_cb (MEX_PROGRAM (content));
        }
      break;

    default:
      break;
    }
}

static void
mex_recommended_plugin_init (MexRecommendedPlugin *self)
{
  GrlPlugin *source;
  GrlRegistry *registry;

  MexRecommendedPluginPrivate *priv = self->priv =
    RECOMMENDED_PLUGIN_PRIVATE (self);

  registry = grl_registry_get_default ();

  source = grl_registry_lookup_source (registry, "grl-apple-trailers");
  if (source)
    {
      MexFeed *feed = mex_grilo_feed_new (source, NULL, NULL, NULL);
      GController *controller = mex_model_get_controller (MEX_MODEL (feed));
      g_object_set (feed, "icon-name", "icon-recommended", "category",
                    "Recommended", NULL);

      g_signal_connect (controller, "changed",
                        G_CALLBACK (mex_recommended_plugin_model_changed_cb),
                        feed);

      mex_grilo_feed_browse (MEX_GRILO_FEED (feed), 0, 50);
      priv->model = MEX_MODEL (feed);
      priv->models = g_list_append (NULL, &priv->model);
    }
  else
    g_warning ("Apple trailers plugin not found");
}

static GType
mex_recommended_get_type (void)
{
  return MEX_TYPE_RECOMMENDED_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Recommanded",
		   "A demo plugin that presents the Apple trailers",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Robert Bradford <rob@linux.intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_recommended_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
