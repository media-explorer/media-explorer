/*
 * mex-networks - Connection Manager UI for Media Explorer
 * Copyright © 2010-2011, Intel Corporation.
 * Copyright © 2012, sleep(5) ltd.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifndef _MTN_CONNMAN
#define _MTN_CONNMAN

#include <gio/gio.h>

G_BEGIN_DECLS

#define MTN_TYPE_CONNMAN mtn_connman_get_type()

#define MTN_CONNMAN(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTN_TYPE_CONNMAN, MtnConnman))

#define MTN_CONNMAN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), MTN_TYPE_CONNMAN, MtnConnmanClass))

#define MTN_IS_CONNMAN(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTN_TYPE_CONNMAN))

#define MTN_IS_CONNMAN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), MTN_TYPE_CONNMAN))

#define MTN_CONNMAN_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), MTN_TYPE_CONNMAN, MtnConnmanClass))

typedef struct _MtnConnmanPrivate MtnConnmanPrivate;

typedef struct {
    GDBusProxy parent;

    MtnConnmanPrivate *priv;
} MtnConnman;

typedef struct {
    GDBusProxyClass parent_class;

    void (*property_changed) (MtnConnman *proxy,
                              GVariant   *property);
    void (*services_changed) (MtnConnman *proxy,
                              GVariant   *value);
} MtnConnmanClass;

GType       mtn_connman_get_type     (void);

GVariant*   mtn_connman_get_property (MtnConnman *connman, const char *key);
void        mtn_connman_set_property (MtnConnman *connman, const char *key, GVariant *value);
GVariant   *mtn_connman_get_services (MtnConnman *connman);

MtnConnman* mtn_connman_new_finish   (GAsyncResult *res, GError **error);
void        mtn_connman_new          (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);

G_END_DECLS

#endif /* _MTN_CONNMAN */
