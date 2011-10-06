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

#ifndef _MEX_BG_BACKDROP_PLUGIN
#define _MEX_BG_BACKDROP_PLUGIN

#include <glib-object.h>
#include <gmodule.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_BG_BACKDROP_PLUGIN mex_bg_backdrop_plugin_get_type()

#define MEX_BG_BACKDROP_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               MEX_TYPE_BG_BACKDROP_PLUGIN,             \
                               MexBgBackdropPlugin))

#define MEX_BG_BACKDROP_PLUGIN_CLASS(klass)                             \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            MEX_TYPE_BG_BACKDROP_PLUGIN,                \
                            MexBgBackdropPluginClass))

#define MEX_IS_BG_BACKDROP_PLUGIN(obj)                          \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               MEX_TYPE_BG_BACKDROP_PLUGIN))

#define MEX_IS_BG_BACKDROP_PLUGIN_CLASS(klass)                  \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                            \
                            MEX_TYPE_BG_BACKDROP_PLUGIN))

#define MEX_BG_BACKDROP_PLUGIN_GET_CLASS(obj)                           \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              MEX_TYPE_BG_BACKDROP_PLUGIN,              \
                              MexBgBackdropPluginClass))

typedef struct _MexBgBackdropPluginPrivate MexBgBackdropPluginPrivate;

typedef struct {
  GObject parent;

  MexBgBackdropPluginPrivate *priv;
} MexBgBackdropPlugin;

typedef struct {
  GObjectClass parent_class;
} MexBgBackdropPluginClass;

GType mex_bg_backdrop_plugin_get_type (void);

MexBgBackdropPlugin* mex_bg_backdrop_plugin_new (void);

G_END_DECLS

#endif
