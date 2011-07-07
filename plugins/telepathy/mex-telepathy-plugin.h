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

#ifndef _MEX_TELEPATHY_PLUGIN
#define _MEX_TELEPATHY_PLUGIN

#include <glib-object.h>
#include <gmodule.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_TELEPATHY_PLUGIN mex_telepathy_plugin_get_type()

#define MEX_TELEPATHY_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_TELEPATHY_PLUGIN, MexTelepathyPlugin))

#define MEX_TELEPATHY_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_TELEPATHY_PLUGIN, MexTelepathyPluginClass))

#define MEX_IS_TELEPATHY_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_TELEPATHY_PLUGIN))

#define MEX_IS_TELEPATHY_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_TELEPATHY_PLUGIN))

#define MEX_TELEPATHY_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_TELEPATHY_PLUGIN, MexTelepathyPluginClass))

typedef struct _MexTelepathyPluginPrivate MexTelepathyPluginPrivate;

typedef struct {
  GObject parent;

  MexTelepathyPluginPrivate *priv;
} MexTelepathyPlugin;

typedef struct {
  GObjectClass parent_class;
} MexTelepathyPluginClass;

GType mex_telepathy_plugin_get_type (void);

MexTelepathyPlugin* mex_telepathy_plugin_new (void);

G_MODULE_EXPORT const GType mex_get_plugin_type (void);

G_END_DECLS

#endif
