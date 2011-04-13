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

#ifndef _MEX_MEDIA_DBUS_BRIDGE_H
#define _MEX_MEDIA_DBUS_BRIDGE_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_MEDIA_DBUS_BRIDGE mex_media_dbus_bridge_get_type()

#define MEX_MEDIA_DBUS_BRIDGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_MEDIA_DBUS_BRIDGE, MexMediaDBUSBridge))

#define MEX_MEDIA_DBUS_BRIDGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_MEDIA_DBUS_BRIDGE, MexMediaDBUSBridgeClass))

#define MEX_IS_MEDIA_DBUS_BRIDGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_MEDIA_DBUS_BRIDGE))

#define MEX_IS_MEDIA_DBUS_BRIDGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_MEDIA_DBUS_BRIDGE))

#define MEX_MEDIA_DBUS_BRIDGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_MEDIA_DBUS_BRIDGE, MexMediaDBUSBridgeClass))

typedef struct _MexMediaDBUSBridge MexMediaDBUSBridge;
typedef struct _MexMediaDBUSBridgeClass MexMediaDBUSBridgeClass;
typedef struct _MexMediaDBUSBridgePrivate MexMediaDBUSBridgePrivate;

struct _MexMediaDBUSBridge
{
  GObject parent;

  MexMediaDBUSBridgePrivate *priv;
};

struct _MexMediaDBUSBridgeClass
{
  GObjectClass parent_class;
};

GType mex_media_dbus_bridge_get_type (void) G_GNUC_CONST;

MexMediaDBUSBridge *mex_media_dbus_bridge_new (ClutterMedia *media);
gboolean mex_media_dbus_bridge_register (MexMediaDBUSBridge  *bridge,
                                         GError             **error);

G_END_DECLS

#endif /* _MEX_MEDIA_DBUS_BRIDGE_H */
