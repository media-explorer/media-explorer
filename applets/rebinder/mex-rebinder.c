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

#include <glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "mex-rebinder.h"

G_DEFINE_TYPE (MexRebinder, mex_rebinder, G_TYPE_OBJECT)

#define REBINDER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_REBINDER, MexRebinderPrivate))

/*
 * TODO: Most of the core logic from rebinder-main.c has be migrated to this
 * object and probably folded in the main library.
 */

struct _MexRebinderPrivate
{
  DBusGConnection *dbus;
  DBusGProxy *proxy;
};

static void
mex_rebinder_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_rebinder_parent_class)->finalize (object);
}

static void
mex_rebinder_class_init (MexRebinderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexRebinderPrivate));

  object_class->finalize = mex_rebinder_finalize;
}

static void
mex_rebinder_init (MexRebinder *self)
{
  self->priv = REBINDER_PRIVATE (self);
}

MexRebinder *
mex_rebinder_new (void)
{
  return g_object_new (MEX_TYPE_REBINDER, NULL);
}

static gboolean
request_dbus_name (const gchar *name)
{
}

gboolean
mex_rebinder_register (MexRebinder  *rebinder,
		       const gchar  *name,
		       GError      **error_in)
{
  MexRebinderPrivate *priv;
  guint32 request_status;
  GError *error = NULL;

  g_return_val_if_fail (MEX_IS_REBINDER (rebinder), FALSE);
  priv = rebinder->priv;

  priv->dbus = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (priv->dbus == NULL)
    {
      g_propagate_error (error_in, error);
      return FALSE;
    }

  priv->proxy = dbus_g_proxy_new_for_name (priv->dbus,
					   DBUS_SERVICE_DBUS,
					   DBUS_PATH_DBUS,
					   DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (priv->proxy, name,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_status,
                                          &error))
    {
      g_propagate_error (error_in, error);
      return FALSE;
    }

  return request_status == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;

}
