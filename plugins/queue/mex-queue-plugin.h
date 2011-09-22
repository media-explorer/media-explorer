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

#ifndef _MEX_QUEUE_PLUGIN
#define _MEX_QUEUE_PLUGIN

#include <glib-object.h>
#include <gmodule.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_QUEUE_PLUGIN mex_queue_plugin_get_type()

#define MEX_QUEUE_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_QUEUE_PLUGIN, MexQueuePlugin))

#define MEX_QUEUE_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_QUEUE_PLUGIN, MexQueuePluginClass))

#define MEX_IS_QUEUE_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_QUEUE_PLUGIN))

#define MEX_IS_QUEUE_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_QUEUE_PLUGIN))

#define MEX_QUEUE_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_QUEUE_PLUGIN, MexQueuePluginClass))

typedef struct _MexQueuePluginPrivate MexQueuePluginPrivate;

typedef struct
{
  GObject parent;
  MexQueuePluginPrivate *priv;
} MexQueuePlugin;

typedef struct
{
  GObjectClass parent_class;
} MexQueuePluginClass;

GType mex_queue_plugin_get_type (void);

G_MODULE_EXPORT const GType mex_get_plugin_type (void);

G_END_DECLS

#endif /* _MEX_QUEUE_PLUGIN */

