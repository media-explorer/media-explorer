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

/* mex-mpris-plugin.h */

#ifndef _MEX_MPRIS_PLUGIN_H
#define _MEX_MPRIS_PLUGIN_H

#include <glib-object.h>
#include <gmodule.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_MPRIS_PLUGIN mex_mpris_plugin_get_type()

#define MEX_MPRIS_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_MPRIS_PLUGIN, MexMprisPlugin))

#define MEX_MPRIS_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_MPRIS_PLUGIN, MexMprisPluginClass))

#define MEX_IS_MPRIS_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_MPRIS_PLUGIN))

#define MEX_IS_MPRIS_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_MPRIS_PLUGIN))

#define MEX_MPRIS_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_MPRIS_PLUGIN, MexMprisPluginClass))

typedef struct _MexMprisPlugin MexMprisPlugin;
typedef struct _MexMprisPluginClass MexMprisPluginClass;
typedef struct _MexMprisPluginPrivate MexMprisPluginPrivate;

struct _MexMprisPlugin
{
  GObject parent;

  MexMprisPluginPrivate *priv;
};

struct _MexMprisPluginClass
{
  GObjectClass parent_class;
};

GType mex_mpris_plugin_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* _MEX_MPRIS_PLUGIN_H */
