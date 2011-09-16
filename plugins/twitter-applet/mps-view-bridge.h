/*
 * Copyright (C) 2010 Intel Corporation.
 * Derived from meego-panel-status
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _MPS_VIEW_BRIDGE
#define _MPS_VIEW_BRIDGE

#include <glib-object.h>
#include <clutter/clutter.h>
#include <libsocialweb-client/sw-client.h>

G_BEGIN_DECLS

#define MPS_TYPE_VIEW_BRIDGE mps_view_bridge_get_type()

#define MPS_VIEW_BRIDGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPS_TYPE_VIEW_BRIDGE, MpsViewBridge))

#define MPS_VIEW_BRIDGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPS_TYPE_VIEW_BRIDGE, MpsViewBridgeClass))

#define MPS_IS_VIEW_BRIDGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPS_TYPE_VIEW_BRIDGE))

#define MPS_IS_VIEW_BRIDGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPS_TYPE_VIEW_BRIDGE))

#define MPS_VIEW_BRIDGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPS_TYPE_VIEW_BRIDGE, MpsViewBridgeClass))

typedef struct {
  GObject parent;
} MpsViewBridge;

typedef struct {
  GObjectClass parent_class;
} MpsViewBridgeClass;

GType mps_view_bridge_get_type (void);

MpsViewBridge *mps_view_bridge_new (void);
void mps_view_bridge_set_view (MpsViewBridge    *bridge,
                               SwClientItemView *view);
void mps_view_bridge_set_container (MpsViewBridge    *bridge,
                                    ClutterContainer *container);
typedef ClutterActor *(*MpsViewBridgeFactoryFunc) (MpsViewBridge *bridge,
                                          SwItem        *item,
                                          gpointer       userdata);
void mps_view_bridge_set_factory_func (MpsViewBridge            *bridge,
                                       MpsViewBridgeFactoryFunc  func,
                                       gpointer                  userdata);
SwClientItemView *mps_view_bridge_get_view (MpsViewBridge *bridge);
ClutterContainer *mps_view_bridge_get_container (MpsViewBridge *bridge);
void mps_view_bridge_set_paused (MpsViewBridge *bridge, gboolean paused);

G_END_DECLS

#endif /* _MPS_VIEW_BRIDGE */

