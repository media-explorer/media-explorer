/*
 * mex-networks - Connection Manager UI for Media Explorer 
 * Copyright © 2010-2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St 
 * - Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef _MTN_CONNMAN_SERVICE
#define _MTN_CONNMAN_SERVICE

#include <gio/gio.h>

G_BEGIN_DECLS

#define MTN_TYPE_CONNMAN_SERVICE mtn_connman_service_get_type()

#define MTN_CONNMAN_SERVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTN_TYPE_CONNMAN_SERVICE, MtnConnmanService))

#define MTN_CONNMAN_SERVICE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), MTN_TYPE_CONNMAN_SERVICE, MtnConnmanServiceClass))

#define MTN_IS_CONNMAN_SERVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTN_TYPE_CONNMAN_SERVICE))

#define MTN_IS_CONNMAN_SERVICE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), MTN_TYPE_CONNMAN_SERVICE))

#define MTN_CONNMAN_SERVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), MTN_TYPE_CONNMAN_SERVICE, MtnConnmanServiceClass))

typedef struct _MtnConnmanServicePrivate MtnConnmanServicePrivate;

typedef struct {
    GDBusProxy parent;
    MtnConnmanServicePrivate *priv;
} MtnConnmanService;

typedef struct {
    GDBusProxyClass parent_class;
    void (*property_changed) (MtnConnmanService *proxy,
                              GVariant          *property);
} MtnConnmanServiceClass;

GType              mtn_connman_service_get_type     (void);

GVariant*          mtn_connman_service_get_property (MtnConnmanService *service, const char *key);
void               mtn_connman_service_set_property (MtnConnmanService *service, const char *key, GVariant *value);

MtnConnmanService* mtn_connman_service_new_finish   (GAsyncResult *res, GError **error);
void               mtn_connman_service_new          (const char *object_path, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);

G_END_DECLS

#endif /* _MTN_CONNMAN_SERVICE */
