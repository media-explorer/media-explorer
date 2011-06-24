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

/* mex-dbusinput-plugin.c
 * Allows you to push key codes into media explorer
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-dbusinput-plugin.h"
#include <gio/gio.h>
#include <string.h>


static const gchar introspection_xml[] =
"<node>"
"  <interface name='com.meego.mex.Input'>"
"    <method name='ControlKey'>"
"      <arg name='keyflag' direction='in' type='u'/>"
"    </method>"
"  </interface>"
"</node>";

G_DEFINE_TYPE (MexDbusinputPlugin, mex_dbusinput_plugin, G_TYPE_OBJECT)

#define DBUSINPUT_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_DBUSINPUT_PLUGIN, MexDbusinputPluginPrivate))

struct _MexDbusinputPluginPrivate
{
  ClutterStage *stage;

  guint owner_id;

  GDBusNodeInfo *introspection_data;
};

static void
mex_dbusinput_plugin_dispose (GObject *object)
{
  MexDbusinputPluginPrivate *priv = MEX_DBUSINPUT_PLUGIN (object)->priv;

  g_bus_unown_name (priv->owner_id);
  g_dbus_node_info_unref (priv->introspection_data);

  G_OBJECT_CLASS (mex_dbusinput_plugin_parent_class)->dispose (object);
}

static void
mex_dbusinput_plugin_finalize (GObject *object)
{
  MexDbusinputPluginPrivate *priv = MEX_DBUSINPUT_PLUGIN (object)->priv;


  G_OBJECT_CLASS (mex_dbusinput_plugin_parent_class)->finalize (object);
}

static void
mex_dbusinput_plugin_class_init (MexDbusinputPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexDbusinputPluginPrivate));

  object_class->dispose = mex_dbusinput_plugin_dispose;
  object_class->finalize = mex_dbusinput_plugin_finalize;
}

static void
_method_cb (GDBusConnection *connection,
            const gchar *sender,
            const gchar *object_path,
            const gchar *interface_name,
            const gchar *method_name,
            GVariant *parameters,
            GDBusMethodInvocation *invocation,
            MexDbusinputPlugin *self)
{
  MexDbusinputPluginPrivate *priv = MEX_DBUSINPUT_PLUGIN (self)->priv;

  if (g_strcmp0 (method_name, "ControlKey") == 0)
    {
      guint keyflag;
      ClutterEvent *event;
      ClutterKeyEvent *kevent;
      ClutterActor *focused;

      g_variant_get (parameters, "(u)", &keyflag);

      focused = clutter_stage_get_key_focus (CLUTTER_STAGE (priv->stage));

      event = clutter_event_new (CLUTTER_KEY_PRESS);
      kevent = (ClutterKeyEvent *)event;

      kevent->flags = 0;
      kevent->source = NULL;
      kevent->stage = CLUTTER_STAGE (priv->stage);
      kevent->keyval = keyflag;
      kevent->time = time (NULL);

      /* KEY PRESS */
      clutter_event_put (event);
      /* KEY RELEASE */
      kevent->type = CLUTTER_KEY_RELEASE;
      clutter_event_put (event);

      clutter_event_free (event);
    }
  g_dbus_method_invocation_return_value (invocation, NULL);
}

static const GDBusInterfaceVTable interface_table =
{
  (GDBusInterfaceMethodCallFunc) _method_cb,
  NULL,
  NULL
};

static void
_on_name_lost (GDBusConnection *connection,
               const gchar *name,
               gpointer user_data)
{
}

static void
_bus_acquired (GDBusConnection *connection,
               const gchar *name,
               MexDbusinputPlugin *self)
{
  MexDbusinputPluginPrivate *priv = MEX_DBUSINPUT_PLUGIN (self)->priv;

  GError *error=NULL;

  /* Note: Dbus object and name subject to change */
  g_dbus_connection_register_object (connection,
                                     "/com/meego/mex/InputControl",
                                     priv->introspection_data->interfaces[0],
                                     &interface_table,
                                     self,
                                     NULL,
                                     &error);
  if (error)
    {
      g_warning ("Problem registering object: %s", error->message);
      g_error_free (error);
    }
}

static void
mex_dbusinput_plugin_init (MexDbusinputPlugin *self)
{
  MexDbusinputPluginPrivate *priv = self->priv =
    DBUSINPUT_PLUGIN_PRIVATE (self);

  priv->stage = CLUTTER_STAGE (mex_get_stage ());

  priv->introspection_data =
    g_dbus_node_info_new_for_xml (introspection_xml, NULL);


  priv->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                   "com.meego.mex.inputctrl",
                                   G_BUS_NAME_OWNER_FLAGS_NONE,
                                   (GBusAcquiredCallback) _bus_acquired,
                                   NULL,
                                   NULL,
                                   self,
                                   NULL);

}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_DBUSINPUT_PLUGIN;
}
