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

#include "mex-bg-image-plugin.h"
#include "mex-background-image.h"

#include <mex/mex-background-manager.h>

G_DEFINE_TYPE (MexBgImagePlugin, mex_bg_image_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o)                                                  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MEX_TYPE_BG_IMAGE_PLUGIN,               \
                                MexBgImagePluginPrivate))

struct _MexBgImagePluginPrivate
{
  MexBackgroundManager *manager;
  MexBackground        *background;
};

static void
mex_bg_image_plugin_dispose (GObject *object)
{
  MexBgImagePluginPrivate *priv = MEX_BG_IMAGE_PLUGIN (object)->priv;

  if (priv->background)
    {
      mex_background_manager_unregister (priv->manager,
                                         priv->background);
      g_object_unref (G_OBJECT (priv->background));
      priv->background = NULL;
    }

  G_OBJECT_CLASS (mex_bg_image_plugin_parent_class)->finalize (object);
}

static void
mex_bg_image_plugin_class_init (MexBgImagePluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = mex_bg_image_plugin_dispose;

  g_type_class_add_private (klass, sizeof (MexBgImagePluginPrivate));
}

static void
mex_bg_image_plugin_init (MexBgImagePlugin *self)
{
  MexBgImagePluginPrivate *priv;
  MexBackground *background;

  self->priv = priv = GET_PRIVATE (self);

  priv->manager = mex_background_manager_get_default ();

  background = (MexBackground *) mex_background_image_new ();
  priv->background = g_object_ref_sink (background);

  mex_background_manager_register (mex_background_manager_get_default (),
                                   priv->background);
}

MexBgImagePlugin *
mex_bg_image_plugin_new (void)
{
  return g_object_new (MEX_TYPE_BG_IMAGE_PLUGIN, NULL);
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_BG_IMAGE_PLUGIN;
}
