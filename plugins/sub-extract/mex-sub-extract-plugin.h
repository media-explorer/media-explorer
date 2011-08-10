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

#ifndef _MEX_SUB_EXTRACT_PLUGIN
#define _MEX_SUB_EXTRACT_PLUGIN

#include <glib-object.h>
#include <gmodule.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_SUB_EXTRACT_PLUGIN mex_sub_extract_plugin_get_type()

#define MEX_SUB_EXTRACT_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_SUB_EXTRACT_PLUGIN, MexSubExtractPlugin))

#define MEX_SUB_EXTRACT_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_SUB_EXTRACT_PLUGIN, MexSubExtractPluginClass))

#define MEX_IS_SUB_EXTRACT_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_SUB_EXTRACT_PLUGIN))

#define MEX_IS_SUB_EXTRACT_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_SUB_EXTRACT_PLUGIN))

#define MEX_SUB_EXTRACT_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_SUB_EXTRACT_PLUGIN, MexSubExtractPluginClass))

typedef struct _MexSubExtractPluginPrivate MexSubExtractPluginPrivate;

typedef struct {
  GObject parent;

  MexSubExtractPluginPrivate *priv;
} MexSubExtractPlugin;

typedef struct {
  GObjectClass parent_class;
} MexSubExtractPluginClass;

GType mex_sub_extract_plugin_get_type (void);

MexSubExtractPlugin* mex_sub_extract_plugin_new (void);

G_MODULE_EXPORT const GType mex_get_plugin_type (void);

G_END_DECLS

#endif
