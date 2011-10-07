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
#include "mex-player-common.h"

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
  GDBusProxy *proxy;

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
_generic_call_async_cb (GObject      *proxy,
                        GAsyncResult *res,
                        gpointer      userdata)
{
  GError *error = NULL;
  GVariant *result;

  result = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res,
                                     &error);

  if (error)
    {
      g_warning (G_STRLOC ": Error setting property: %s",
                 error->message);
    }

  g_variant_unref (result);
}

static void
_set_uri_call_cb (GObject      *proxy,
                  GAsyncResult *res,
                  gpointer      userdata)
{
  MexPlayerClient *client = MEX_PLAYER_CLIENT (userdata);
  GError *error = NULL;
  GVariant *result;

  result = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res,
                                     &error);
  g_variant_unref (result);

  if (error)
    {
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

  g_dbus_proxy_call (priv->proxy, "SetUri",
                     g_variant_new ("(s)", (uri) ? uri : ""),
                     G_DBUS_CALL_FLAGS_NONE, -1, NULL, _set_uri_call_cb,
                     client);
}

static void
mex_player_client_set_progress (MexPlayerClient *client,
                                gdouble          progress)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->progress = progress;

  g_dbus_proxy_call (priv->proxy, "SetProgress",
                     g_variant_new ("(d)", progress), G_DBUS_CALL_FLAGS_NONE,
                     -1, NULL, _generic_call_async_cb, client);
}

static void
mex_player_client_set_audio_volume (MexPlayerClient *client,
                                    gdouble          audio_volume)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->audio_volume = audio_volume;

  g_dbus_proxy_call (priv->proxy, "SetAudioVolume",
                     g_variant_new ("(d)", audio_volume),
                     G_DBUS_CALL_FLAGS_NONE, -1, NULL, _generic_call_async_cb,
                     client);
}

static void
mex_player_client_set_playing (MexPlayerClient *client,
                               gboolean         playing)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->playing = playing;

  g_dbus_proxy_call (priv->proxy, "SetPlaying",
                     g_variant_new ("(b)", playing),
                     G_DBUS_CALL_FLAGS_NONE, -1, NULL, _generic_call_async_cb,
                     client);
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
_progress_changed_cb (MexPlayerClient *client,
                      gdouble          progress)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->progress = progress;

  g_object_notify (G_OBJECT (client), "progress");
}

static void
_audio_volume_changed_cb (MexPlayerClient *client,
                          gdouble          audio_volume)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->audio_volume = audio_volume;

  g_object_notify (G_OBJECT (client), "audio-volume");
}

static void
_duration_changed_cb (MexPlayerClient *client,
                      gdouble          duration)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->duration = duration;

  g_object_notify (G_OBJECT (client), "duration");
}

static void
_playing_changed_cb (MexPlayerClient *client,
                     gboolean         playing)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->playing = playing;

  g_object_notify (G_OBJECT (client), "playing");
}

static void
_can_seek_changed_cb (MexPlayerClient *client,
                      gboolean         can_seek)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->can_seek = can_seek;

  g_object_notify (G_OBJECT (client), "can-seek");
}

static void
_buffer_fill_changed_cb (MexPlayerClient *client,
                         gdouble          buffer)
{
  MexPlayerClientPrivate *priv = client->priv;

  priv->buffer_fill = buffer;

  g_object_notify (G_OBJECT (client), "buffer-fill");
}

static void
_eos_cb (MexPlayerClient *client)
{
  g_signal_emit_by_name (client, "eos");
}

static void
player_signal_cb (GDBusProxy *proxy,
                  gchar      *sender_name,
                  gchar      *signal_name,
                  GVariant   *parameters,
                  gpointer    user_data)
{
  MexPlayerClient *client = MEX_PLAYER_CLIENT (user_data);
  gdouble d = 0;
  gboolean b = FALSE;

  g_return_if_fail (signal_name != NULL);

  if (g_str_equal (signal_name, "ProgressChanged"))
    {
      g_variant_get (parameters, "(d)", &d);
      _progress_changed_cb (client, d);
    }
  else if (g_str_equal (signal_name, "DurationChanged"))
    {
      g_variant_get (parameters, "(d)", &d);
      _duration_changed_cb (client, d);
    }
  else if (g_str_equal (signal_name, "PlayingChanged"))
    {
      g_variant_get (parameters, "(b)", &b);
      _playing_changed_cb (client, b);
    }
  else if (g_str_equal (signal_name, "CanSeekChanged"))
    {
      g_variant_get (parameters, "(b)", &b);
      _can_seek_changed_cb (client, b);
    }
  else if (g_str_equal (signal_name, "BufferFillChanged"))
    {
      g_variant_get (parameters, "(d)", &d);
      _buffer_fill_changed_cb (client, d);
    }
  else if (g_str_equal (signal_name, "AudioVolumeChanged"))
    {
      g_variant_get (parameters, "(d)", &d);
      _audio_volume_changed_cb (client, d);
    }
  else if (g_str_equal (signal_name, "EOS"))
    {
      _eos_cb (client);
    }
}

static void
mex_player_client_proxy_ready_cb (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
  MexPlayerClient *self = MEX_PLAYER_CLIENT (user_data);
  GError *error = NULL;

  self->priv->proxy = g_dbus_proxy_new_finish (result, &error);

  if (error)
    {
      g_critical (G_STRLOC ": Error connecting to remote player: %s",
                  error->message);
      g_error_free (error);
      return;
    }

  g_signal_connect (self->priv->proxy, "g-signal",
                    G_CALLBACK (player_signal_cb), self);
}

static void
mex_player_client_init (MexPlayerClient *self)
{
  MexPlayerClientPrivate *priv;

  self->priv = GET_PRIVATE (self);
  priv = self->priv;

  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE, NULL,
                            MEX_PLAYER_SERVICE_NAME,
                            MEX_PLAYER_OBJECT_PATH,
                            MEX_PLAYER_INTERFACE_NAME, NULL,
                            mex_player_client_proxy_ready_cb,
                            self);
}


MexPlayerClient *
mex_player_client_new (void)
{
  return g_object_new (MEX_TYPE_PLAYER_CLIENT, NULL);
}
