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

#include "mex-bg-backdrop-plugin.h"
#include "mex-backdrop.h"

#include <mex/mex-background-manager.h>

G_DEFINE_TYPE (MexBgBackdropPlugin, mex_bg_backdrop_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o)                                                  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MEX_TYPE_BG_BACKDROP_PLUGIN,               \
                                MexBgBackdropPluginPrivate))

struct _MexBgBackdropPluginPrivate
{
  MexBackgroundManager *manager;
  MexBackground        *background;
};

static void
mex_bg_backdrop_plugin_dispose (GObject *object)
{
  MexBgBackdropPluginPrivate *priv = MEX_BG_BACKDROP_PLUGIN (object)->priv;

  if (priv->background)
    {
      mex_background_manager_unregister (priv->manager,
                                         priv->background);
      g_object_unref (G_OBJECT (priv->background));
      priv->background = NULL;
    }

  G_OBJECT_CLASS (mex_bg_backdrop_plugin_parent_class)->finalize (object);
}

static void
mex_bg_backdrop_plugin_class_init (MexBgBackdropPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = mex_bg_backdrop_plugin_dispose;

  g_type_class_add_private (klass, sizeof (MexBgBackdropPluginPrivate));
}

static void
mex_bg_backdrop_plugin_init (MexBgBackdropPlugin *self)
{
  MexBgBackdropPluginPrivate *priv;

  self->priv = priv = GET_PRIVATE (self);

  priv->manager = mex_background_manager_get_default ();

  priv->background = (MexBackground *) mex_backdrop_new ();
  g_object_ref_sink (priv->background);

  mex_background_manager_register (mex_background_manager_get_default (),
                                   priv->background);
}

MexBgBackdropPlugin *
mex_bg_backdrop_plugin_new (void)
{
  return g_object_new (MEX_TYPE_BG_BACKDROP_PLUGIN, NULL);
}

static GType
mex_bg_backdrop_get_type (void)
{
  return MEX_TYPE_BG_BACKDROP_PLUGIN;
}

MEX_DEFINE_PLUGIN ("bg-backdrop",
		   "A bubbly background written with Clutter",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Øyvind Kolås <pippin@linux.intel.com>,"
                   "Michael Wood <michael.g.wood@linux.intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_bg_backdrop_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
