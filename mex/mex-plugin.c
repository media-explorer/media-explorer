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

#include "mex-plugin.h"

G_DEFINE_TYPE (MexPlugin, mex_plugin, G_TYPE_OBJECT)

#define PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_PLUGIN, MexPluginPrivate))

struct _MexPluginPrivate
{
};


static void
mex_plugin_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_plugin_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_plugin_parent_class)->finalize (object);
}

static void
mex_plugin_class_init (MexPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /* g_type_class_add_private (klass, sizeof (MexPluginPrivate)); */

  object_class->get_property = mex_plugin_get_property;
  object_class->set_property = mex_plugin_set_property;
  object_class->finalize = mex_plugin_finalize;
}

static void
mex_plugin_init (MexPlugin *self)
{
  /* self->priv = PLUGIN_PRIVATE (self); */
}

MexPlugin *
mex_plugin_new (void)
{
  return g_object_new (MEX_TYPE_PLUGIN, NULL);
}

/**
 * mex_plugin_start:
 *
 * The creation of the plugin object should be separate from started the
 * initialization work the plugin needs to do. This initialization work can be
 * done in mex_plugin_start().
 *
 * The plugin must ensure that all the data allocated and all the tasks
 * started in mex_plugin_start() are de-allocated/stopped in mex_plugin_stop().
 */
void
mex_plugin_start (MexPlugin *plugin)
{
  MexPluginClass *klass;

  g_return_if_fail (MEX_IS_PLUGIN (plugin));

  klass = MEX_PLUGIN_GET_CLASS (plugin);
  if (klass->start)
    klass->start (plugin);
}

/**
 * mex_plugin_stop:
 *
 * The pendant of mex_plugin_start(), unsurprisingly.
 */
void
mex_plugin_stop	(MexPlugin *plugin)
{
  MexPluginClass *klass;

  g_return_if_fail (MEX_IS_PLUGIN (plugin));

  klass = MEX_PLUGIN_GET_CLASS (plugin);
  if (klass->stop)
    klass->stop (plugin);
}
