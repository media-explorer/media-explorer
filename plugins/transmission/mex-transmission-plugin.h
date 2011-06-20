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

#ifndef __MEX_TRANSMISSION_PLUGIN__
#define __MEX_TRANSMISSION_PLUGIN__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_TRANSMISSION_PLUGIN mex_transmission_plugin_get_type()

#define MEX_TRANSMISSION_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_TRANSMISSION_PLUGIN, MexTransmissionPlugin))

#define MEX_TRANSMISSION_PLUGIN_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                      \
                            MEX_TYPE_TRANSMISSION_PLUGIN, \
                            MexTransmissionPluginClass))

#define MEX_IS_TRANSMISSION_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_TRANSMISSION_PLUGIN))

#define MEX_IS_TRANSMISSION_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_TRANSMISSION_PLUGIN))

#define MEX_TRANSMISSION_PLUGIN_GET_CLASS(obj)              \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                        \
                              MEX_TYPE_TRANSMISSION_PLUGIN, \
                              MexTransmissionPluginClass))

typedef struct _MexTransmissionPluginPrivate MexTransmissionPluginPrivate;

typedef struct {
  GObject parent;
  MexTransmissionPluginPrivate *priv;
} MexTransmissionPlugin;

typedef struct {
  GObjectClass parent_class;
} MexTransmissionPluginClass;

GType mex_transmission_plugin_get_type (void);
GType mex_get_plugin_type (void);

G_END_DECLS

#endif /* __MEX_TRANSMISSION_PLUGIN__ */
