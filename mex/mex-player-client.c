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

#include "mex-player-client.h"
#include "mex-media-player-bindings.h"
#include "mex-player-common.h"

#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <clutter/clutter.h>

static void clutter_media_iface_init (ClutterMediaIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexPlayerClient, mex_player_client, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_MEDIA, clutter_media_iface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_PLAYER_CLIENT, MexPlayerClientPrivate))

enum {
  PROP_0,

  /* ClutterMedia properties */
  PROP_URI,
  PROP_PLAYING,
  PROP_PROGRESS,
  PROP_SUBTITLE_URI,
  PROP_SUBTITLE_FONT_NAME,
  PROP_AUDIO_VOLUME,
  PROP_CAN_SEEK,
  PROP_BUFFER_FILL,
  PROP_DURATION,
};

struct _MexPlayerClientPrivate
{
  DBusGProxy *proxy;
  DBusGConnection *connection;

  /* We basically cache these values */
  gdouble progress;
  gdouble duration;
  gboolean playing;
  gchar *uri;
  gboolean can_seek;
  gdouble buffer_fill;
  gdouble audio_volume;
};

static void
clutter_media_iface_init (ClutterMediaIface *iface)
{

}


static void
mex_player_client_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MexPlayerClient *self = MEX_PLAYER_CLIENT (object);
  MexPlayerClientPrivate *priv = self->priv;

  switch (property_id)
    {
      case PROP_PLAYING:
        g_value_set_boolean (value, priv->playing);
        break;

      case PROP_PROGRESS:
        g_value_set_double (value, priv->progress);
        break;

      case PROP_AUDIO_VOLUME:
        g_value_set_double (value, priv->audio_volume);
        break;

      case PROP_DURATION:
        g_value_set_double (value, priv->duration);
        break;

      case PROP_URI:
        g_value_set_string (value, priv->uri);
        break;

      case PROP_CAN_SEEK:
        g_value_set_boolean (value, priv->can_seek);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_generic_call_async_cb (DBusGProxy *proxy,
                        GError     *error,
                        gpointer    userdata)
{
  const gchar *api_call = (const gchar *)userdata;

  if (error)
    {
      g_warning (G_STRLOC ": Error making %s call: %s",
                 api_call,
                 error->message);
    }
}

static void
_set_uri_call_cb (DBusGProxy *proxy,
                  GError     *error,
                  gpointer    userdata)
{
  MexPlayerClient        *client = (MexPlayerClient *) userdata;
  MexPlayerClientPrivate *priv   = client->priv;

  if (error) {
    g_warning (G_STRLOC ": Error making SetUri call: %s",
               error->message);
    return;
  }

  g_object_notify (G_OBJECT (client), "uri");
}

static void
mex_player_client_set_uri (MexPlayerClient *client,
                           const gchar     *uri)
{
  MexPlayerClientPrivate *priv = client->priv;

  g_free (priv->uri);
  priv->uri = NULL;

  priv->uri = g_strdup (uri);

  com_meego_mex_MediaPlayer_set_uri_async (priv->proxy,
                                           uri,
                                           _set_uri_call_cb,
                                           client);
}

static void
mex_player_client_set_progress (MexPlayerClient *client,
                                gdouble          progress)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->progress = progress;

  com_meego_mex_MediaPlayer_set_progress_async (priv->proxy,
                                                progress,
                                                _generic_call_async_cb,
                                                "SetProgress");
}

static void
mex_player_client_set_audio_volume (MexPlayerClient *client,
                                    gdouble          audio_volume)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->audio_volume = audio_volume;

  com_meego_mex_MediaPlayer_set_audio_volume_async (priv->proxy,
                                                    audio_volume,
                                                    _generic_call_async_cb,
                                                    "SetAudioVolume");
}

static void
mex_player_client_set_playing (MexPlayerClient *client,
                               gboolean         playing)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->playing = playing;

  com_meego_mex_MediaPlayer_set_playing_async (priv->proxy,
                                               playing,
                                               _generic_call_async_cb,
                                               "SetPlaying");
}

static void
mex_player_client_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MexPlayerClient *player_client = MEX_PLAYER_CLIENT (object);

  switch (property_id)
    {
      case PROP_URI:
        mex_player_client_set_uri (player_client,
                                   g_value_get_string (value));
        break;
      case PROP_PLAYING:
        mex_player_client_set_playing (player_client,
                                       g_value_get_boolean (value));
        break;
      case PROP_PROGRESS:
        mex_player_client_set_progress (player_client,
                                        g_value_get_double (value));
        break;
      case PROP_AUDIO_VOLUME:
        mex_player_client_set_audio_volume (player_client,
                                            g_value_get_double (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_player_client_dispose (GObject *object)
{
  MexPlayerClient *self = MEX_PLAYER_CLIENT (object);
  MexPlayerClientPrivate *priv = self->priv;

  if (priv->connection)
    {
      dbus_g_connection_unref (priv->connection);
      priv->connection = NULL;
    }

  if (priv->proxy)
    {
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  G_OBJECT_CLASS (mex_player_client_parent_class)->dispose (object);
}

static void
mex_player_client_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_player_client_parent_class)->finalize (object);
}

static void
mex_player_client_class_init (MexPlayerClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexPlayerClientPrivate));

  object_class->get_property = mex_player_client_get_property;
  object_class->set_property = mex_player_client_set_property;
  object_class->dispose = mex_player_client_dispose;
  object_class->finalize = mex_player_client_finalize;


  /* Copied from the list in clutter-gst */
  g_object_class_override_property (object_class,
                                    PROP_URI,
                                    "uri");
  g_object_class_override_property (object_class,
                                    PROP_PLAYING,
                                    "playing");
  g_object_class_override_property (object_class,
                                    PROP_PROGRESS,
                                    "progress");
  g_object_class_override_property (object_class,
                                    PROP_SUBTITLE_URI,
                                    "subtitle-uri");
  g_object_class_override_property (object_class,
                                    PROP_SUBTITLE_FONT_NAME,
                                    "subtitle-font-name");
  g_object_class_override_property (object_class,
                                    PROP_AUDIO_VOLUME,
                                    "audio-volume");
  g_object_class_override_property (object_class,
                                    PROP_CAN_SEEK,
                                    "can-seek");
  g_object_class_override_property (object_class,
                                    PROP_DURATION,
                                    "duration");
  g_object_class_override_property (object_class,
                                    PROP_BUFFER_FILL,
                                    "buffer-fill");
}

static void
_progress_changed_cb (DBusGProxy      *proxy,
                      gdouble          progress,
                      MexPlayerClient *client)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->progress = progress;

  g_object_notify (G_OBJECT (client), "progress");
}

static void
_audio_volume_changed_cb (DBusGProxy      *proxy,
                          gdouble          audio_volume,
                          MexPlayerClient *client)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->audio_volume = audio_volume;

  g_object_notify (G_OBJECT (client), "audio-volume");
}

static void
_duration_changed_cb (DBusGProxy      *proxy,
                      gdouble          duration,
                      MexPlayerClient *client)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->duration = duration;

  g_object_notify (G_OBJECT (client), "duration");
}

static void
_playing_changed_cb (DBusGProxy      *proxy,
                     gboolean         playing,
                     MexPlayerClient *client)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->playing = playing;

  g_object_notify (G_OBJECT (client), "playing");
}

static void
_can_seek_changed_cb (DBusGProxy      *proxy,
                      gboolean         can_seek,
                      MexPlayerClient *client)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->can_seek = can_seek;

  g_object_notify (G_OBJECT (client), "can-seek");
}

static void
_buffer_fill_changed_cb (DBusGProxy      *proxy,
                         gdouble          buffer,
                         MexPlayerClient *client)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->buffer_fill = buffer;

  g_object_notify (G_OBJECT (client), "buffer-fill");
}

static void
_eos_cb (DBusGProxy      *proxy,
         MexPlayerClient *client)
{
  MexPlayerClientPrivate *priv = client->priv;

  g_signal_emit_by_name (client, "eos");
}

static void
mex_player_client_init (MexPlayerClient *self)
{
  MexPlayerClientPrivate *priv;
  GError *error = NULL;

  self->priv = GET_PRIVATE (self);
  priv = self->priv;

  priv->connection = dbus_g_bus_get (DBUS_BUS_STARTER, &error);

  if (!priv->connection)
  {
    g_critical (G_STRLOC ": Error getting DBUS connection: %s",
                error->message);
    return;
  }

  priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
                                           MEX_PLAYER_SERVICE_NAME,
                                           MEX_PLAYER_OBJECT_PATH,
                                           MEX_PLAYER_INTERFACE_NAME);

  dbus_g_proxy_add_signal (priv->proxy,
                           "ProgressChanged",
                           G_TYPE_DOUBLE,
                           G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy,
                           "AudioVolumeChanged",
                           G_TYPE_DOUBLE,
                           G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy,
                           "DurationChanged",
                           G_TYPE_DOUBLE,
                           G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy,
                           "PlayingChanged",
                           G_TYPE_BOOLEAN,
                           G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy,
                           "CanSeekChanged",
                           G_TYPE_BOOLEAN,
                           G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy,
                           "BufferFillChanged",
                           G_TYPE_DOUBLE,
                           G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy,
                           "EOS",
                           G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (priv->proxy,
                               "ProgressChanged",
                               (GCallback)_progress_changed_cb,
                               self,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy,
                               "DurationChanged",
                               (GCallback)_duration_changed_cb,
                               self,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy,
                               "PlayingChanged",
                               (GCallback)_playing_changed_cb,
                               self,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy,
                               "CanSeekChanged",
                               (GCallback)_can_seek_changed_cb,
                               self,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy,
                               "BufferFillChanged",
                               (GCallback)_buffer_fill_changed_cb,
                               self,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy,
                               "AudioVolumeChanged",
                               (GCallback)_audio_volume_changed_cb,
                               self,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy,
                               "EOS",
                               (GCallback)_eos_cb,
                               self,
                               NULL);
}

MexPlayerClient *
mex_player_client_new (void)
{
  return g_object_new (MEX_TYPE_PLAYER_CLIENT, NULL);
}
