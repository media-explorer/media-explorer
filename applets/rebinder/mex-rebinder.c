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
#include "mex-rebinder-control-ginterface.h"

static void mex_rebinder_control_iface_init (MexRebinderControlIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexRebinder,
			 mex_rebinder,
			 G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_REBINDER_CONTROL_IFACE,
                                                mex_rebinder_control_iface_init))

#define REBINDER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_REBINDER, MexRebinderPrivate))

/*
 * TODO: Most of the core logic from rebinder-main.c has be migrated to this
 * object and probably folded in the main library.
 */

enum
{
  SIGNAL_QUIT,

  SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

struct _MexRebinderPrivate
{
  DBusGConnection *dbus;
};

/*
 * MexRebinderControl implementation
 */

static void
mex_rebinder_control_quit (MexRebinderControlIface *rebinder,
			   DBusGMethodInvocation   *context)
{
  g_signal_emit (rebinder, signals[SIGNAL_QUIT], 0);
  mex_rebinder_control_iface_return_from_quit (context);
}

static void
mex_rebinder_control_iface_init (MexRebinderControlIface *iface)
{
  MexRebinderControlIfaceClass *klass = (MexRebinderControlIfaceClass *)iface;

  mex_rebinder_control_iface_implement_quit (klass, mex_rebinder_control_quit);
}

/*
 * GObject implementation
 */

static void
mex_rebinder_finalize (GObject *object)
{
  MexRebinder *rebinder = MEX_REBINDER (object);
  MexRebinderPrivate *priv = rebinder->priv;

  if (priv->dbus)
    {
      g_object_unref (priv->dbus);
      priv->dbus = NULL;
    }
  G_OBJECT_CLASS (mex_rebinder_parent_class)->finalize (object);
}

static void
mex_rebinder_class_init (MexRebinderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexRebinderPrivate));

  object_class->finalize = mex_rebinder_finalize;

  signals[SIGNAL_QUIT] = g_signal_new ("quit",
				       MEX_TYPE_REBINDER,
				       G_SIGNAL_RUN_FIRST,
				       0,
				       NULL,
				       NULL,
				       g_cclosure_marshal_VOID__VOID,
				       G_TYPE_NONE,
				       0);
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

gboolean
mex_rebinder_register (MexRebinder  *rebinder,
		       const gchar  *name,
		       const gchar  *path,
		       GError      **error_in)
{
  MexRebinderPrivate *priv;
  guint32 request_status;
  GError *error = NULL;
  DBusGProxy *proxy;

  g_return_val_if_fail (MEX_IS_REBINDER (rebinder), FALSE);
  priv = rebinder->priv;

  priv->dbus = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (priv->dbus == NULL)
    {
      g_propagate_error (error_in, error);
      return FALSE;
    }

  proxy = dbus_g_proxy_new_for_name (priv->dbus,
				     DBUS_SERVICE_DBUS,
				     DBUS_PATH_DBUS,
				     DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (proxy, name,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_status,
                                          &error))
    {
      g_propagate_error (error_in, error);
      return FALSE;
    }

  g_object_unref (proxy);

  if (request_status != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    return FALSE;

  dbus_g_connection_register_g_object (priv->dbus, path, G_OBJECT (rebinder));

  return TRUE;
}
