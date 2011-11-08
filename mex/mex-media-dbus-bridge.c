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

#include "mex-media-dbus-bridge.h"

#include <mex/mex-player-common.h>
#include <mex/mex-player.h>


static const gchar introspection_xml[] =
"<node>"
"  <interface name='"MEX_PLAYER_INTERFACE_NAME"'>"
"    <method name='SetAudioVolume'>"
"      <arg name='volume' type='d' direction='in' />"
"    </method>"
"    <method name='GetAudioVolume'>"
"      <arg name='volume' type='d' direction='out' />"
"    </method>"
"    <signal name='AudioVolumeChanged'>"
"      <arg name='volume' type='d' direction='out' />"
"    </signal>"
"    <method name='SetUri'>"
"      <arg name='uri' type='s' direction='in' />"
"    </method>"
"    <method name='GetUri'>"
"      <arg name='uri' type='s' direction='out' />"
"    </method>"
"    <method name='SetPlaying'>"
"      <arg name='playing' type='b' direction='in' />"
"    </method>"
"    <method name='GetPlaying'>"
"      <arg name='playing' type='b' direction='out' />"
"    </method>"
"    <signal name='PlayingChanged'>"
"      <arg name='playing' type='b' direction='out' />"
"    </signal>"
"    <method name='SetProgress'>"
"      <arg name='progress' type='d' direction='in' />"
"    </method>"
"    <method name='GetProgress'>"
"      <arg name='progress' type='d' direction='out' />"
"    </method>"
"    <signal name='ProgressChanged'>"
"      <arg name='progress' type='d' />"
"    </signal>"
"    <method name='GetDuration'>"
"      <arg name='duration' type='d' direction='out' />"
"    </method>"
"    <signal name='DurationChanged'>"
"      <arg name='duration' type='d' />"
"    </signal>"
"    <method name='GetCanSeek'>"
"      <arg name='seekable' type='b' direction='out'/>"
"    </method>"
"    <signal name='Error'>"
"      <arg name='error' type='s' />"
"    </signal>"
"    <signal name='BufferFillChanged'>"
"      <arg name='buffer' type='d' />"
"    </signal>"
"    <signal name='CanSeekChanged'>"
"      <arg name='seekable' type='b' direction='out'/>"
"    </signal>"
"    <signal name='UriChanged'>"
"      <arg name='uri' type='s' direction='out'/>"
"    </signal>"
"    <signal name='EOS'/>"
"  </interface>"
"</node>";
;
/* NOTE: The bridge currently takes the clutter media object which is common
 * to both mex media players; internal and external (see ../player/). We have
 * now reached a point where we may want to do player specific behaviour.
 * To do this the internal player is set as a property here and it's api
 * for control is used instead of directly accessing the clutter media object.
 */

G_DEFINE_TYPE (MexMediaDBUSBridge, mex_media_dbus_bridge, G_TYPE_OBJECT)

#define MEDIA_DBUS_BRIDGE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MEDIA_DBUS_BRIDGE, MexMediaDBUSBridgePrivate))

enum
{
  PROP_0,
  PROP_MEDIA,

  PROP_LAST
};

struct _MexMediaDBUSBridgePrivate
{
  ClutterMedia *media;

  GDBusNodeInfo *introspection_data;
  GDBusConnection *connection;
};

static void
mex_media_dbus_bridge_set_media (MexMediaDBUSBridge *bridge,
                                 ClutterMedia       *media);

static void
mex_media_dbus_bridge_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (object);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  switch (property_id)
    {
      case PROP_MEDIA:
        g_value_set_object (value, priv->media);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_media_dbus_bridge_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (object);
  ClutterMedia *media;

  switch (property_id)
    {
      case PROP_MEDIA:
        media = (ClutterMedia *)g_value_get_object (value);
        mex_media_dbus_bridge_set_media (bridge, media);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_media_dbus_bridge_dispose (GObject *object)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (object);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  mex_media_dbus_bridge_set_media (bridge, NULL);

  if (priv->connection)
    {
      g_object_unref (priv->connection);
      priv->connection = NULL;
    }

  G_OBJECT_CLASS (mex_media_dbus_bridge_parent_class)->dispose (object);
}

static void
mex_media_dbus_bridge_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_media_dbus_bridge_parent_class)->finalize (object);
}

static void
mex_media_dbus_bridge_class_init (MexMediaDBUSBridgeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexMediaDBUSBridgePrivate));

  object_class->get_property = mex_media_dbus_bridge_get_property;
  object_class->set_property = mex_media_dbus_bridge_set_property;
  object_class->dispose = mex_media_dbus_bridge_dispose;
  object_class->finalize = mex_media_dbus_bridge_finalize;

  pspec = g_param_spec_object ("media",
                               "Media",
                               "The Clutter Media to bridge onto the bus",
                               CLUTTER_TYPE_MEDIA,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_MEDIA, pspec);
}

static void
mex_media_dbus_bridge_init (MexMediaDBUSBridge *self)
{
  self->priv = MEDIA_DBUS_BRIDGE_PRIVATE (self);
}

MexMediaDBUSBridge *
mex_media_dbus_bridge_new (ClutterMedia *media)
{
  return g_object_new (MEX_TYPE_MEDIA_DBUS_BRIDGE,
                       "media", media,
                       NULL);
}

static void
_media_notify_cb (ClutterMedia       *media,
                  GParamSpec         *pspec,
                  MexMediaDBUSBridge *bridge)

{
  MexMediaDBUSBridgePrivate *priv = bridge->priv;
  GVariant *parameters;
  const gchar *signal_name;

  if (!priv->connection)
    return;

  if (g_str_equal (pspec->name, "playing"))
    {
      gboolean playing;

      playing = clutter_media_get_playing (priv->media);

      signal_name = "PlayingChanged";
      parameters = g_variant_new ("(b)", playing);
    }
  else if (g_str_equal (pspec->name, "progress"))
    {
      gdouble progress;

      progress = clutter_media_get_progress (priv->media);

      signal_name = "ProgressChanged";
      parameters = g_variant_new ("(d)", progress);
    }
  else if (g_str_equal (pspec->name, "duration"))
    {
      gdouble duration;

      duration = clutter_media_get_duration (priv->media);

      signal_name = "DurationChanged";
      parameters = g_variant_new ("(d)", duration);
    }
  else if (g_str_equal (pspec->name, "buffer-fill"))
    {
      gdouble buffer_fill;

      buffer_fill = clutter_media_get_buffer_fill (priv->media);

      signal_name = "BufferFillChanged";
      parameters = g_variant_new ("(d)", buffer_fill);
    }
  else if (g_str_equal (pspec->name, "can-seek"))
    {
      gboolean can_seek;

      can_seek = clutter_media_get_can_seek (priv->media);

      signal_name = "CanSeekChanged";
      parameters = g_variant_new ("(b)", can_seek);
    }
  else if (g_str_equal (pspec->name, "audio-volume"))
    {
      gdouble audio_volume;

      audio_volume = clutter_media_get_audio_volume (priv->media);

      signal_name = "AudioVolumeChanged";
      parameters = g_variant_new ("(d)", audio_volume);
    }
  else if (g_str_equal (pspec->name, "uri"))
    {
      gchar *uri;

      uri = clutter_media_get_uri (priv->media);

      signal_name = "UriChanged";
      parameters = g_variant_new ("(s)", uri);
      g_free (uri);
    }
  else
    return;

  g_dbus_connection_emit_signal (priv->connection, NULL, MEX_PLAYER_OBJECT_PATH,
                                 MEX_PLAYER_INTERFACE_NAME, signal_name,
                                 parameters, NULL);
}

static void
_media_error_cb (ClutterMedia       *media,
                 GError             *error,
                 MexMediaDBUSBridge *bridge)
{
  g_dbus_connection_emit_signal (bridge->priv->connection, NULL,
                                 MEX_PLAYER_OBJECT_PATH,
                                 MEX_PLAYER_INTERFACE_NAME, "Error",
                                 g_variant_new ("(s)", error->message), NULL);
}

static void
_media_eos_cb (ClutterMedia       *media,
               MexMediaDBUSBridge *bridge)
{
  g_dbus_connection_emit_signal (bridge->priv->connection, NULL,
                                 MEX_PLAYER_OBJECT_PATH,
                                 MEX_PLAYER_INTERFACE_NAME, "EOS", NULL, NULL);
}

static void
mex_media_dbus_bridge_set_media (MexMediaDBUSBridge *bridge,
                                 ClutterMedia       *media)
{
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  if (priv->media)
    {
      g_object_unref (priv->media);
      priv->media = NULL;
    }

  if (media)
    {
      priv->media = g_object_ref_sink (media);

      g_signal_connect_object (media,
                               "notify",
                               (GCallback)_media_notify_cb,
                               bridge,
                               0);

      g_signal_connect_object (media,
                               "error",
                               (GCallback)_media_error_cb,
                               bridge,
                               0);


      g_signal_connect_object (media,
                               "eos",
                               (GCallback)_media_eos_cb,
                               bridge,
                               0);

      /*
       * Trigger some notifications to ensure current state communicated over
       * the bus
       */

      g_object_notify (G_OBJECT (media), "audio-volume");
      g_object_notify (G_OBJECT (media), "buffer-fill");
      g_object_notify (G_OBJECT (media), "can-seek");
      g_object_notify (G_OBJECT (media), "duration");
     /* FIXME: Dbus bindings unaware of idle mode
      * Playing signal will cause screensaver to be inhibited
      * g_object_notify (G_OBJECT (media), "playing");
      */
      g_object_notify (G_OBJECT (media), "progress");
    }
}

/******************************************************************************/

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  MexMediaDBUSBridgePrivate *priv = MEX_MEDIA_DBUS_BRIDGE (user_data)->priv;
  const gchar *uri;
  gdouble d;
  gboolean b;
  gchar *s;
  GVariant *return_value = NULL;

  g_return_if_fail (method_name != NULL);

  if (g_str_equal (method_name, "SetAudioVolume"))
    {
      g_variant_get (parameters, "(d)", &d);
      clutter_media_set_audio_volume (priv->media, d);
    }
  else if (g_str_equal (method_name, "GetAudioVolume"))
    {
      d = clutter_media_get_audio_volume (priv->media);
      return_value = g_variant_new ("(d)", d);
    }
  else if (g_str_equal (method_name, "SetUri"))
    {
      g_variant_get (parameters, "(s)", &s);
      clutter_media_set_uri (priv->media, s);
      g_free (s);
    }
  else if (g_str_equal (method_name, "GetUri"))
    {
      uri = clutter_media_get_uri (priv->media);
      return_value = g_variant_new ("(s)", uri ? uri : "");
    }
  else if (g_str_equal (method_name, "SetPlaying"))
    {
      g_variant_get (parameters, "(b)", &b);
      clutter_media_set_playing (priv->media, b);
    }
  else if (g_str_equal (method_name, "GetPlaying"))
    {
      b = clutter_media_get_playing (priv->media);
      return_value = g_variant_new ("(b)", b);
    }
  else if (g_str_equal (method_name, "SetProgress"))
    {
      g_variant_get (parameters, "(d)", &d);
      clutter_media_set_progress (priv->media, d);
    }
  else if (g_str_equal (method_name, "GetProgress"))
    {
      d = clutter_media_get_progress (priv->media);
      return_value = g_variant_new ("(d)", d);
    }
  else if (g_str_equal (method_name, "GetDuration"))
    {
      d = clutter_media_get_duration (priv->media);
      return_value = g_variant_new ("(d)", d);
    }
  else if (g_str_equal (method_name, "GetCanSeek"))
    {
      b = clutter_media_get_can_seek (priv->media);
      return_value = g_variant_new ("(b)", b);
    }

  g_dbus_method_invocation_return_value (invocation, return_value);
}

static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  NULL,
  NULL
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         bridge)
{
  MexMediaDBUSBridgePrivate *priv = MEX_MEDIA_DBUS_BRIDGE (bridge)->priv;
  guint registration_id;

  priv->connection = g_object_ref (connection);

  registration_id =
    g_dbus_connection_register_object (connection,
                                       MEX_PLAYER_OBJECT_PATH,
                                       priv->introspection_data->interfaces[0],
                                       &interface_vtable,
                                       bridge, bridge, NULL);
  g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
}

G_GNUC_NORETURN static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  g_error (G_STRLOC ": Player instance already running");
}


gboolean
mex_media_dbus_bridge_register (MexMediaDBUSBridge  *bridge,
                                GError             **error_in)
{
  GError *error = NULL;

  bridge->priv->introspection_data =
    g_dbus_node_info_new_for_xml (introspection_xml, &error);

  g_assert_no_error (error);

  g_bus_own_name (G_BUS_TYPE_SESSION, MEX_PLAYER_SERVICE_NAME,
                  G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired,
                  on_name_acquired, on_name_lost, bridge, NULL);

  return TRUE;
}
