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
#include "mex-media-player-ginterface.h"

#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <mex/mex-player-common.h>
#include <mex/mex-player.h>

/* NOTE: The bridge currently takes the clutter media object which is common
 * to both mex media players; internal and external (see ../player/). We have
 * now reached a point where we may want to do player specific behaviour.
 * To do this the internal player is set as a property here and it's api
 * for control is used instead of directly accessing the clutter media object.
 */

static void mex_media_player_iface_init (MexMediaPlayerIface *iface);
G_DEFINE_TYPE_WITH_CODE (MexMediaDBUSBridge,
                         mex_media_dbus_bridge,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MEDIA_PLAYER_IFACE,
                                                mex_media_player_iface_init))

#define MEDIA_DBUS_BRIDGE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MEDIA_DBUS_BRIDGE, MexMediaDBUSBridgePrivate))

enum
{
  PROP_0,
  PROP_MEDIA,
  PROP_PLAYER,

  PROP_LAST
};

struct _MexMediaDBUSBridgePrivate
{
  ClutterMedia *media;
  MexPlayer *player;
};

static void
mex_media_dbus_bridge_set_media (MexMediaDBUSBridge *bridge,
                                 ClutterMedia       *media);

static void
mex_media_dbus_bridge_set_player (MexMediaDBUSBridge *bridge,
                                  MexPlayer          *player);

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
      case PROP_PLAYER:
        g_value_set_object (value, priv->player);
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
  MexPlayer *player;

  switch (property_id)
    {
      case PROP_MEDIA:
        media = (ClutterMedia *)g_value_get_object (value);
        mex_media_dbus_bridge_set_media (bridge, media);
        break;
      case PROP_PLAYER:
        player = (MexPlayer *)g_value_get_object (value);
        mex_media_dbus_bridge_set_player (bridge, player);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_media_dbus_bridge_dispose (GObject *object)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (object);

  mex_media_dbus_bridge_set_media (bridge, NULL);

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

 pspec = g_param_spec_object ("player",
                              "MexPlayer",
                              "The internal MexPlayer",
                               MEX_TYPE_PLAYER,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PLAYER, pspec);
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
mex_media_dbus_bridge_set_uri (MexMediaPlayerIface   *player_iface,
                               const gchar           *uri,
                               DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  clutter_media_set_uri (priv->media, uri);

  mex_media_player_iface_return_from_set_uri (context);
}

static void
mex_media_dbus_bridge_set_playing (MexMediaPlayerIface   *player_iface,
                                   gboolean               playing,
                                   DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  clutter_media_set_playing (priv->media, playing);

  mex_media_player_iface_return_from_set_playing (context);
}

static void
mex_media_dbus_bridge_set_progress (MexMediaPlayerIface   *player_iface,
                                    gdouble                progress,
                                    DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  if (priv->player)
    if (mex_player_get_idle_mode (priv->player))
      {
        mex_media_player_iface_return_from_set_progress (context);
        return;
      }

  clutter_media_set_progress (priv->media, progress);

  mex_media_player_iface_return_from_set_progress (context);
}

static void
mex_media_dbus_bridge_set_audio_volume (MexMediaPlayerIface   *player_iface,
                                        gdouble                audio_volume,
                                        DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  clutter_media_set_audio_volume (priv->media, audio_volume);

  mex_media_player_iface_return_from_set_audio_volume (context);
}

static void
mex_media_dbus_bridge_get_uri (MexMediaPlayerIface   *player_iface,
                               DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;
  const gchar *uri;

  uri = clutter_media_get_uri (priv->media);

  mex_media_player_iface_return_from_get_uri (context, uri);
}

static void
mex_media_dbus_bridge_get_playing (MexMediaPlayerIface   *player_iface,
                                   DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;
  gboolean playing;

  playing = clutter_media_get_playing (priv->media);

  if (priv->player)
    if (mex_player_get_idle_mode (priv->player))
      playing = FALSE;

  mex_media_player_iface_return_from_get_playing (context, playing);
}

static void
mex_media_dbus_bridge_get_progress (MexMediaPlayerIface   *player_iface,
                                    DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;
  gdouble progress;

  progress = clutter_media_get_progress (priv->media);

  if (priv->player)
    if (mex_player_get_idle_mode (priv->player))
      progress = 0.0;

  mex_media_player_iface_return_from_get_progress (context, progress);
}

static void
mex_media_dbus_bridge_get_audio_volume (MexMediaPlayerIface   *player_iface,
                                        DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;
  gdouble audio_volume;

  audio_volume = clutter_media_get_audio_volume (priv->media);

  mex_media_player_iface_return_from_get_audio_volume (context, audio_volume);
}

static void
mex_media_dbus_bridge_get_duration (MexMediaPlayerIface   *player_iface,
                                    DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;
  gdouble duration;

  duration = clutter_media_get_duration (priv->media);

  if (priv->player)
    if (mex_player_get_idle_mode (priv->player))
      duration = 0.0;

  mex_media_player_iface_return_from_get_duration (context, duration);
}


static void
mex_media_dbus_bridge_get_can_seek (MexMediaPlayerIface   *player_iface,
                                    DBusGMethodInvocation *context)
{
  MexMediaDBUSBridge *bridge = MEX_MEDIA_DBUS_BRIDGE (player_iface);
  MexMediaDBUSBridgePrivate *priv = bridge->priv;
  gboolean can_seek;

  can_seek = clutter_media_get_can_seek (priv->media);

  mex_media_player_iface_return_from_get_can_seek (context, can_seek);
}

static void
mex_media_player_iface_init (MexMediaPlayerIface *iface)
{
  MexMediaPlayerIfaceClass *klass = (MexMediaPlayerIfaceClass *)iface;

  mex_media_player_iface_implement_set_uri (klass,
                                            mex_media_dbus_bridge_set_uri);
  mex_media_player_iface_implement_get_uri (klass,
                                            mex_media_dbus_bridge_get_uri);

  mex_media_player_iface_implement_set_playing (klass,
                                                mex_media_dbus_bridge_set_playing);
  mex_media_player_iface_implement_get_playing (klass,
                                                mex_media_dbus_bridge_get_playing);

  mex_media_player_iface_implement_set_audio_volume (klass,
                                                mex_media_dbus_bridge_set_audio_volume);
  mex_media_player_iface_implement_get_audio_volume (klass,
                                                mex_media_dbus_bridge_get_audio_volume);

  mex_media_player_iface_implement_set_progress (klass,
                                                 mex_media_dbus_bridge_set_progress);
  mex_media_player_iface_implement_get_progress (klass,
                                                 mex_media_dbus_bridge_get_progress);

  mex_media_player_iface_implement_get_duration (klass,
                                                 mex_media_dbus_bridge_get_duration);

  mex_media_player_iface_implement_get_can_seek (klass,
                                                 mex_media_dbus_bridge_get_can_seek);
}

static void
_media_notify_cb (ClutterMedia       *media,
                  GParamSpec         *pspec,
                  MexMediaDBUSBridge *bridge)

{
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  if (g_str_equal (pspec->name, "playing"))
    {
      gboolean playing;

      playing = clutter_media_get_playing (priv->media);

      if (priv->player)
        if (mex_player_get_idle_mode (priv->player))
          playing = FALSE;

      mex_media_player_iface_emit_playing_changed (bridge,
                                                   playing);
    }
  else if (g_str_equal (pspec->name, "progress"))
    {
      gdouble progress;

      progress = clutter_media_get_progress (priv->media);

      if (priv->player)
        if (mex_player_get_idle_mode (priv->player))
          progress = 0.0;

      mex_media_player_iface_emit_progress_changed (bridge,
                                                    progress);
    }
  else if (g_str_equal (pspec->name, "duration"))
    {
      gdouble duration;

      duration = clutter_media_get_duration (priv->media);

      if (priv->player)
        if (mex_player_get_idle_mode (priv->player))
          duration = 0.0;

      mex_media_player_iface_emit_duration_changed (bridge,
                                                    duration);
    }
  else if (g_str_equal (pspec->name, "buffer-fill"))
    {
      gdouble buffer_fill;

      buffer_fill = clutter_media_get_buffer_fill (priv->media);
      mex_media_player_iface_emit_buffer_fill_changed (bridge,
                                                       buffer_fill);
    }
  else if (g_str_equal (pspec->name, "can-seek"))
    {
      gboolean can_seek;

      can_seek = clutter_media_get_can_seek (priv->media);
      mex_media_player_iface_emit_can_seek_changed (bridge,
                                                    can_seek);
    }
  else if (g_str_equal (pspec->name, "audio-volume"))
    {
      gdouble audio_volume;

      audio_volume = clutter_media_get_audio_volume (priv->media);
      mex_media_player_iface_emit_audio_volume_changed (bridge,
                                                        audio_volume);
    }
}

static void
_media_error_cb (ClutterMedia       *media,
                 GError             *error,
                 MexMediaDBUSBridge *bridge)
{
  mex_media_player_iface_emit_error (bridge, error->message);
}

static void
_media_eos_cb (ClutterMedia       *media,
               MexMediaDBUSBridge *bridge)
{
  mex_media_player_iface_emit_eos (bridge);
}

static void
mex_media_dbus_bridge_set_player (MexMediaDBUSBridge *bridge,
                                  MexPlayer *player)
{
  MexMediaDBUSBridgePrivate *priv = bridge->priv;

  priv->player = player;
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

static gboolean
request_name (void)
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
  guint32 request_status;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (connection == NULL)
    {
      g_printerr ("Failed to open connection to DBus: %s\n", error->message);
      g_error_free (error);
      return FALSE;
    }

  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (proxy,
                                          MEX_PLAYER_SERVICE_NAME,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE, &request_status,
                                          &error)) {
    g_printerr ("Failed to request name: %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }

  return request_status == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
}


gboolean
mex_media_dbus_bridge_register (MexMediaDBUSBridge  *bridge,
                                GError             **error_in)
{
  DBusGConnection *connection;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (connection == NULL)
    {
      g_propagate_error (error_in, error);
      return FALSE;
    }

  dbus_g_connection_register_g_object (connection,
                                       MEX_PLAYER_OBJECT_PATH,
                                       G_OBJECT (bridge));

  if (!request_name ())
  {
    g_warning (G_STRLOC ": Player instance already running");
  }

  return TRUE;
}
