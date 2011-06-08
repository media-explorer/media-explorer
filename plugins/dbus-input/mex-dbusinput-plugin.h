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

/* mex-dbusinput-plugin.h */

#ifndef _MEX_DBUSINPUT_PLUGIN_H
#define _MEX_DBUSINPUT_PLUGIN_H

#include <glib-object.h>
#include <gmodule.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_DBUSINPUT_PLUGIN mex_dbusinput_plugin_get_type()

#define MEX_DBUSINPUT_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_DBUSINPUT_PLUGIN, MexDbusinputPlugin))

#define MEX_DBUSINPUT_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_DBUSINPUT_PLUGIN, MexDbusinputPluginClass))

#define MEX_IS_DBUSINPUT_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_DBUSINPUT_PLUGIN))

#define MEX_IS_DBUSINPUT_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_DBUSINPUT_PLUGIN))

#define MEX_DBUSINPUT_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_DBUSINPUT_PLUGIN, MexDbusinputPluginClass))

typedef struct _MexDbusinputPlugin MexDbusinputPlugin;
typedef struct _MexDbusinputPluginClass MexDbusinputPluginClass;
typedef struct _MexDbusinputPluginPrivate MexDbusinputPluginPrivate;

struct _MexDbusinputPlugin
{
  GObject parent;

  MexDbusinputPluginPrivate *priv;
};

struct _MexDbusinputPluginClass
{
  GObjectClass parent_class;
};

GType mex_dbusinput_plugin_get_type (void) G_GNUC_CONST;

G_MODULE_EXPORT const GType mex_get_plugin_type (void);

G_END_DECLS

#endif /* _MEX_DBUSINPUT_PLUGIN_H */
