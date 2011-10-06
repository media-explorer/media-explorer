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

#ifndef __MEX_DEBUG_PLUGIN__
#define __MEX_DEBUG_PLUGIN__

#include <glib-object.h>
#include <gmodule.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_DEBUG_PLUGIN mex_debug_plugin_get_type()

#define MEX_DEBUG_PLUGIN(obj)                           \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               MEX_TYPE_DEBUG_PLUGIN,   \
                               MexDebugPlugin))

#define MEX_IS_DEBUG_PLUGIN(obj)                        \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MEX_TYPE_DEBUG_PLUGIN))

typedef struct _MexDebugPluginPrivate MexDebugPluginPrivate;

typedef struct
{
  GObject parent;
  MexDebugPluginPrivate *priv;
} MexDebugPlugin;

typedef struct
{
  GObjectClass parent_class;
} MexDebugPluginClass;

GType     mex_debug_plugin_get_type   (void);

G_END_DECLS

#endif /* __MEX_DEBUG_PLUGIN__ */
