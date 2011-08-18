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

#include "mex-surface-player.h"
#include "mex-log.h"

#include <gst/gst.h>
#include <clutter/clutter.h>

#define MEX_LOG_DOMAIN_DEFAULT  surface_player_log_domain
MEX_LOG_DOMAIN(surface_player_log_domain);

static void clutter_media_iface_init (ClutterMediaIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexSurfacePlayer, mex_surface_player, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_MEDIA, clutter_media_iface_init))

#define SURFACE_PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SURFACE_PLAYER, MexSurfacePlayerPrivate))

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

struct _MexSurfacePlayerPrivate
{
  GstElement *pipeline;

  guint can_seek : 1;
  guint in_seek : 1;
  guint is_idle : 1;
  gdouble stacked_progress;
  gdouble target_progress;
  GstState stacked_state;
  gdouble buffer_fill;
  gdouble duration;
  gchar *uri;
  GstSeekFlags seek_flags;

  guint tick_timeout_id;
};

#define TICK_TIMEOUT 0.5

static void
mex_surface_player_set_progress (MexSurfacePlayer *player,
                                 gdouble           progress);
static gdouble
mex_surface_player_get_progress (MexSurfacePlayer *player);

static void
mex_surface_player_set_audio_volume (MexSurfacePlayer *player,
                                 gdouble           audio_volume);
static gdouble
mex_surface_player_get_audio_volume (MexSurfacePlayer *player);

static gboolean
mex_surface_player_get_playing (MexSurfacePlayer *player);

static void
mex_surface_player_set_playing (MexSurfacePlayer *player,
                                gboolean          playing);
static void
mex_surface_player_set_uri (MexSurfacePlayer *player,
                            const gchar      *uri);


static void
clutter_media_iface_init (ClutterMediaIface *iface)
{

}


static void
mex_surface_player_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MexSurfacePlayer *player = MEX_SURFACE_PLAYER (object);
  MexSurfacePlayerPrivate *priv = player->priv;

  switch (property_id)
    {
      case PROP_URI:
        g_value_set_string (value, priv->uri);
        break;
      case PROP_PLAYING:
        g_value_set_boolean (value, mex_surface_player_get_playing (player));
        break;
      case PROP_PROGRESS:
        g_value_set_double (value, mex_surface_player_get_progress (player));
        break;
      case PROP_AUDIO_VOLUME:
        g_value_set_double (value, mex_surface_player_get_audio_volume (player));
        break;
      case PROP_CAN_SEEK:
        g_value_set_boolean (value, priv->can_seek);
        break;
      case PROP_BUFFER_FILL:
        g_value_set_double (value, priv->buffer_fill);
        break;
      case PROP_DURATION:
        g_value_set_double (value, priv->duration);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_surface_player_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MexSurfacePlayer *player = MEX_SURFACE_PLAYER (object);

  switch (property_id)
    {
      case PROP_URI:
        mex_surface_player_set_uri (player,
                                    g_value_get_string (value));
        break;
      case PROP_PLAYING:
        mex_surface_player_set_playing (player,
                                        g_value_get_boolean (value));
        break;
      case PROP_PROGRESS:
        mex_surface_player_set_progress (player,
                                         g_value_get_double (value));
        break;
      case PROP_AUDIO_VOLUME:
        mex_surface_player_set_audio_volume (player,
                                             g_value_get_double (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_surface_player_dispose (GObject *object)
{
  MexSurfacePlayer *player = MEX_SURFACE_PLAYER (object);
  MexSurfacePlayerPrivate *priv = player->priv;

  /* This will cause priv->uri to be freed and the timeout to be removed */
  mex_surface_player_set_uri (player, NULL);

  if (priv->pipeline)
    {
      g_object_unref (priv->pipeline);
      priv->pipeline = NULL;
    }

  G_OBJECT_CLASS (mex_surface_player_parent_class)->dispose (object);
}

static void
mex_surface_player_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_surface_player_parent_class)->finalize (object);
}

static void
mex_surface_player_class_init (MexSurfacePlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexSurfacePlayerPrivate));

  object_class->get_property = mex_surface_player_get_property;
  object_class->set_property = mex_surface_player_set_property;
  object_class->dispose = mex_surface_player_dispose;
  object_class->finalize = mex_surface_player_finalize;

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

/* Returns TRUE if duration changed measurably */
static gboolean
mex_surface_player_update_duration (MexSurfacePlayer *player)
{
  MexSurfacePlayerPrivate *priv = player->priv;
  gboolean success;
  GstFormat format = GST_FORMAT_TIME;
  gint64 duration;
  gdouble new_duration, difference;

  success = gst_element_query_duration (priv->pipeline, &format, &duration);
  if (G_UNLIKELY (success != TRUE))
    return FALSE;

  new_duration = (gdouble) duration / GST_SECOND;

  MEX_DEBUG ("update duration : %f", new_duration);

  /* while we store the new duration if it sligthly changes, the duration
   * signal is sent only if the new duration is at least one second different
   * from the old one (as the duration signal is mainly used to update the
   * time displayed in a UI */
  difference = ABS (priv->duration - new_duration);
  if (difference > 1e-3)
    {
      priv->duration = new_duration;

      if (difference > 1.0)
        return TRUE;
    }

  return FALSE;
}

static void
bus_message_error_cb (GstBus           *bus,
                      GstMessage       *message,
                      MexSurfacePlayer *player)
{
  GError *error = NULL;

  gst_message_parse_error (message, &error, NULL);

  MEX_DEBUG ("error : %s", error->message);

  g_signal_emit_by_name (player, "error", error);
  g_error_free (error);
}

static void
bus_message_buffering_cb (GstBus           *bus,
                          GstMessage       *message,
                          MexSurfacePlayer *player)
{
  MexSurfacePlayerPrivate *priv = player->priv;
  const GstStructure *str;
  gint buffer_percent;
  gboolean res;

  str = gst_message_get_structure (message);
  if (!str)
    return;

  res = gst_structure_get_int (str, "buffer-percent", &buffer_percent);
  if (res)
    {
      priv->buffer_fill = CLAMP ((gdouble) buffer_percent / 100.0,
                                 0.0,
                                 1.0);

      MEX_DEBUG ("buffering : %f", priv->buffer_fill);

      g_object_notify (G_OBJECT (player), "buffer-fill");
    }
}

static void
bus_message_eos_cb (GstBus           *bus,
                    GstMessage       *message,
                    MexSurfacePlayer *player)
{
  MEX_DEBUG ("end-of-stream");

  g_object_notify (G_OBJECT (player), "progress");
  g_signal_emit_by_name (player, "eos");
}

static void
bus_message_duration_cb (GstBus           *bus,
                         GstMessage       *message,
                         MexSurfacePlayer *player)
{
  gint64 duration;

  /* GstElements send a duration message on the bus with GST_CLOCK_TIME_NONE
   * as duration to signal a new duration */
  gst_message_parse_duration (message, NULL, &duration);
  if (G_UNLIKELY (duration != GST_CLOCK_TIME_NONE))
    return;

  if (mex_surface_player_update_duration (player))
    g_object_notify (G_OBJECT (player), "duration");
}

static void
bus_message_state_change_cb (GstBus                  *bus,
                             GstMessage              *message,
                             MexSurfacePlayer        *player)
{
  MexSurfacePlayerPrivate *priv = player->priv;
  GstState old_state, new_state;
  gpointer src;
  GEnumValue *old_state_value, *new_state_value;

  src = GST_MESSAGE_SRC (message);
  if (src != priv->pipeline)
    return;

  gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

  old_state_value =  g_enum_get_value (g_type_class_peek (GST_TYPE_STATE),
                                       old_state);
  new_state_value =  g_enum_get_value (g_type_class_peek (GST_TYPE_STATE),
                                       new_state);

  MEX_DEBUG ("state change : %s -> %s",
             old_state_value->value_nick,
             new_state_value->value_nick);

  if (old_state == GST_STATE_READY &&
      new_state == GST_STATE_PAUSED)
    {
      GstQuery *query;

      /* Determine whether we can seek */
      query = gst_query_new_seeking (GST_FORMAT_TIME);

      if (gst_element_query (priv->pipeline, query))
        {
          gboolean can_seek = FALSE;

          gst_query_parse_seeking (query, NULL, &can_seek,
                                   NULL,
                                   NULL);

          priv->can_seek = (can_seek == TRUE) ? TRUE : FALSE;
        }
      else
        {
          /* could not query for ability to seek by querying the
           * pipeline; let's crudely try by using the URI
           */
          if (priv->uri && g_str_has_prefix (priv->uri, "http://"))
            priv->can_seek = FALSE;
          else
            priv->can_seek = TRUE;
        }

      gst_query_unref (query);

      g_object_notify (G_OBJECT (player), "can-seek");

      if (mex_surface_player_update_duration (player))
        g_object_notify (G_OBJECT (player), "duration");
    }

  /* is_idle controls the drawing with the idle material */
  if (new_state == GST_STATE_NULL)
    priv->is_idle = TRUE;
  else if (new_state == GST_STATE_PLAYING)
    priv->is_idle = FALSE;
}

static void
bus_message_async_done_cb (GstBus           *bus,
                           GstMessage       *message,
                           MexSurfacePlayer *player)
{
  MexSurfacePlayerPrivate *priv = player->priv;

  if (priv->in_seek)
    {
      g_object_notify (G_OBJECT (player), "progress");

      priv->in_seek = FALSE;
      gst_element_set_state (priv->pipeline, priv->stacked_state);

      if (priv->stacked_progress)
        {
          mex_surface_player_set_progress (player, priv->stacked_progress);
        }
    }
}

static void
mex_surface_player_init (MexSurfacePlayer *self)
{
  MexSurfacePlayerPrivate *priv;
  GstBus *bus;

  MEX_DEBUG ("init");

  self->priv = SURFACE_PLAYER_PRIVATE (self);
  priv = self->priv;

  priv->pipeline = gst_element_factory_make ("playbin2", "pipeline");
  priv->seek_flags = GST_SEEK_FLAG_KEY_UNIT;

  bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));

  gst_bus_add_signal_watch (bus);

  g_signal_connect_object (bus, "message::error",
                           G_CALLBACK (bus_message_error_cb),
                           self,
                           0);

  g_signal_connect_object (bus, "message::buffering",
                           G_CALLBACK (bus_message_buffering_cb),
                           self,
                           0);

  g_signal_connect_object (bus, "message::eos",
                           G_CALLBACK (bus_message_eos_cb),
                           self,
                           0);

  g_signal_connect_object (bus,
                           "message::duration",
                           G_CALLBACK (bus_message_duration_cb),
                           self,
                           0);

  g_signal_connect_object (bus,
                           "message::state-changed",
                           G_CALLBACK (bus_message_state_change_cb),
                           self,
                           0);

  g_signal_connect_object (bus,
                           "message::async-done",
                           G_CALLBACK (bus_message_async_done_cb),
                           self,
                           0);

  gst_object_unref (GST_OBJECT (bus));
}

MexSurfacePlayer *
mex_surface_player_new (void)
{
  return g_object_new (MEX_TYPE_SURFACE_PLAYER, NULL);
}

static gboolean
tick_timeout_cb (gpointer data)
{
  g_object_notify (G_OBJECT (data), "progress");

  return TRUE;
}

static void
mex_surface_player_set_uri (MexSurfacePlayer *player,
                            const gchar      *uri)
{
  MexSurfacePlayerPrivate *priv = player->priv;
  GstState state, pending;

  if (!priv->pipeline)
    return;

  MEX_DEBUG ("set uri : %s -> %s", priv->uri, uri);

  g_free (priv->uri);

  if (uri)
    {
      priv->uri = g_strdup (uri);

      /* Ensure the tick timeout is installed.
       *
       * We also have it installed in PAUSED state, because
       * seeks etc may have a delayed effect on the position.
       */
      if (priv->tick_timeout_id == 0)
        {
          priv->tick_timeout_id =
            g_timeout_add (TICK_TIMEOUT * 1000, tick_timeout_cb, player);
        }
    }
  else
    {
      priv->uri = NULL;

      if (priv->tick_timeout_id != 0)
        {
          g_source_remove (priv->tick_timeout_id);
          priv->tick_timeout_id = 0;
        }
    }

  priv->can_seek = FALSE;
  priv->duration = 0.0;

  gst_element_get_state (priv->pipeline, &state, &pending, 0);

  if (pending)
    state = pending;

  gst_element_set_state (priv->pipeline, GST_STATE_NULL);

  g_object_set (priv->pipeline, "uri", uri, NULL);

  /*
   * Restore state.
   */
  if (uri)
    gst_element_set_state (priv->pipeline, state);

  /*
   * Emit notififications for all these to make sure UI is not showing
   * any properties of the old URI.
   */
  g_object_notify (G_OBJECT (player), "uri");
  g_object_notify (G_OBJECT (player), "can-seek");
  g_object_notify (G_OBJECT (player), "duration");
  g_object_notify (G_OBJECT (player), "progress");
}

static void
mex_surface_player_set_playing (MexSurfacePlayer *player,
                                gboolean          playing)
{
  MexSurfacePlayerPrivate *priv = player->priv;

  if (!priv->pipeline)
    return;

  MEX_DEBUG ("set playing : %i", playing);

  if (priv->uri)
    {
      GstState state = GST_STATE_PAUSED;

      if (playing)
        state = GST_STATE_PLAYING;

      priv->in_seek = FALSE;

      gst_element_set_state (priv->pipeline, state);
    }
  else
    {
      if (playing)
        g_warning ("Unable to start playing: no URI is set");
    }

  g_object_notify (G_OBJECT (player), "playing");
  g_object_notify (G_OBJECT (player), "progress");
}

static gboolean
mex_surface_player_get_playing (MexSurfacePlayer *player)
{
  MexSurfacePlayerPrivate *priv = player->priv;
  GstState state, pending;
  gboolean playing;

  if (!priv->pipeline)
    return FALSE;

  gst_element_get_state (priv->pipeline, &state, &pending, 0);

  if (pending)
    playing = (pending == GST_STATE_PLAYING);
  else
    playing = (state == GST_STATE_PLAYING);

  return playing;
}

static void
mex_surface_player_set_progress (MexSurfacePlayer *player,
                                 gdouble           progress)
{
  MexSurfacePlayerPrivate *priv = player->priv;
  GstState pending;
  GstQuery *duration_q;
  gint64 position;

  if (!priv->pipeline)
    return;

  MEX_DEBUG ("set progress : %f", progress);

  priv->target_progress = progress;

  if (priv->in_seek)
    {
      priv->stacked_progress = progress;
      return;
    }

  gst_element_get_state (priv->pipeline, &priv->stacked_state, &pending, 0);

  if (pending)
    priv->stacked_state = pending;

  gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);

  duration_q = gst_query_new_duration (GST_FORMAT_TIME);

  if (gst_element_query (priv->pipeline, duration_q))
    {
      gint64 duration = 0;

      gst_query_parse_duration (duration_q, NULL, &duration);

      position = progress * duration;
    }
  else
    position = 0;

  gst_query_unref (duration_q);

  gst_element_seek (priv->pipeline,
                    1.0,
                    GST_FORMAT_TIME,
                    GST_SEEK_FLAG_FLUSH | priv->seek_flags,
                    GST_SEEK_TYPE_SET,
                    position,
                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  priv->in_seek = TRUE;
  priv->stacked_progress = 0.0;
}

static gdouble
mex_surface_player_get_progress (MexSurfacePlayer *player)
{
  MexSurfacePlayerPrivate *priv = player->priv;
  GstQuery *position_q, *duration_q;
  gdouble progress;

  if (!priv->pipeline)
    return 0.0;

  /* When seeking, the progress returned by playbin2 is 0.0. We want that to be
   * the last known position instead as returning 0.0 will have some ugly
   * effects, say on a progress bar getting updated from the progress tick. */
  if (priv->in_seek)
    {
      return priv->target_progress;
    }

  position_q = gst_query_new_position (GST_FORMAT_TIME);
  duration_q = gst_query_new_duration (GST_FORMAT_TIME);

  if (gst_element_query (priv->pipeline, position_q) &&
      gst_element_query (priv->pipeline, duration_q))
    {
      gint64 position, duration;

      position = duration = 0;

      gst_query_parse_position (position_q, NULL, &position);
      gst_query_parse_duration (duration_q, NULL, &duration);

      progress = CLAMP ((gdouble) position / (gdouble) duration, 0.0, 1.0);
    }
  else
    progress = 0.0;

  gst_query_unref (position_q);
  gst_query_unref (duration_q);

  return progress;
}

static void
mex_surface_player_set_audio_volume (MexSurfacePlayer *player,
                                     gdouble           volume)
{
  MexSurfacePlayerPrivate *priv = player->priv;

  if (!priv->pipeline)
    return;

  MEX_DEBUG ("set audio volume : %f", volume);

  /* the :volume property is in the [0, 10] interval */
  g_object_set (G_OBJECT (priv->pipeline), "volume", volume * 10.0, NULL);
  g_object_notify (G_OBJECT (player), "audio-volume");
}

static gdouble
mex_surface_player_get_audio_volume (MexSurfacePlayer *player)
{
  MexSurfacePlayerPrivate *priv = player->priv;
  gdouble volume = 0.0;

  if (!priv->pipeline)
    return 0.0;

  /* the :volume property is in the [0, 10] interval */
  g_object_get (priv->pipeline, "volume", &volume, NULL);

  volume = CLAMP (volume / 10.0, 0.0, 1.0);

  return volume;
}

