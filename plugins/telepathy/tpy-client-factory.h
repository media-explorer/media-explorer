/*
 * Factory creating telepathy-yell call channels.
 *
 * Copyright © 2011 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __TPY_AUTOMATIC_CLIENT_FACTORY_H__
#define __TPY_AUTOMATIC_CLIENT_FACTORY_H__

#include <telepathy-glib/simple-client-factory.h>

G_BEGIN_DECLS

typedef struct _TpyAutomaticClientFactory TpyAutomaticClientFactory;
typedef struct _TpyAutomaticClientFactoryClass TpyAutomaticClientFactoryClass;

struct _TpyAutomaticClientFactoryClass {
    /*<public>*/
    TpSimpleClientFactoryClass parent_class;
};

struct _TpyAutomaticClientFactory {
    /*<private>*/
    TpSimpleClientFactory parent;
};

GType tpy_automatic_client_factory_get_type (void);

#define TPY_TYPE_AUTOMATIC_CLIENT_FACTORY \
  (tpy_automatic_client_factory_get_type ())
#define TPY_AUTOMATIC_CLIENT_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TPY_TYPE_AUTOMATIC_CLIENT_FACTORY, \
                               TpyAutomaticClientFactory))
#define TPY_AUTOMATIC_CLIENT_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TPY_TYPE_AUTOMATIC_CLIENT_FACTORY, \
                            TpyAutomaticClientFactoryClass))
#define TPY_IS_AUTOMATIC_CLIENT_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TPY_TYPE_AUTOMATIC_CLIENT_FACTORY))
#define TPY_IS_AUTOMATIC_CLIENT_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TPY_TYPE_AUTOMATIC_CLIENT_FACTORY))
#define TPY_AUTOMATIC_CLIENT_FACTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TPY_TYPE_AUTOMATIC_CLIENT_FACTORY, \
                              TpyAutomaticClientFactoryClass))

TpyAutomaticClientFactory *tpy_automatic_client_factory_new (TpDBusDaemon *dbus);

G_END_DECLS

#endif
