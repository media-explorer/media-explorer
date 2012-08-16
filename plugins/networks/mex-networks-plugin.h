/*
 * Mex - a media explorer
 *
 * Copyright © 2012, sleep(5) ltd.
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

/* mex-networks-plugin.h */

#ifndef __MEX_NETWORKS_PLUGIN_H__
#define __MEX_NETWORKS_PLUGIN_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_NETWORKS_PLUGIN                                        \
   (mex_networks_plugin_get_type())
#define MEX_NETWORKS_PLUGIN(obj)                                        \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                MEX_TYPE_NETWORKS_PLUGIN,               \
                                MexNetworksPlugin))
#define MEX_NETWORKS_PLUGIN_CLASS(klass)                                \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             MEX_TYPE_NETWORKS_PLUGIN,                  \
                             MexNetworksPluginClass))
#define MEX_IS_NETWORKS_PLUGIN(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                MEX_TYPE_NETWORKS_PLUGIN))
#define MEX_IS_NETWORKS_PLUGIN_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             MEX_TYPE_NETWORKS_PLUGIN))
#define MEX_NETWORKS_PLUGIN_GET_CLASS(obj)                              \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               MEX_TYPE_NETWORKS_PLUGIN,                \
                               MexNetworksPluginClass))

typedef struct _MexNetworksPlugin        MexNetworksPlugin;
typedef struct _MexNetworksPluginClass   MexNetworksPluginClass;
typedef struct _MexNetworksPluginPrivate MexNetworksPluginPrivate;

struct _MexNetworksPluginClass
{
  GObjectClass parent_class;
};

struct _MexNetworksPlugin
{
  GObject parent;

  /*<private>*/
  MexNetworksPluginPrivate *priv;
};

GType mex_networks_plugin_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __MEX_NETWORKS_PLUGIN_H__ */
