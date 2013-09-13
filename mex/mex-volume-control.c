/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
 * Copyright © 2012 sleep(5) Ltd.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-volume-control.h"
#include "mex-player.h"
#include "mex-music-player.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include <math.h>

#include <glib/gi18n-lib.h>

G_DEFINE_TYPE (MexVolumeControl, mex_volume_control, MX_TYPE_FRAME)

#define VOLUME_CONTROL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_VOLUME_CONTROL, MexVolumeControlPrivate))

enum
{
  PROP_0,

  PROP_VOLUME
};

struct _MexVolumeControlPrivate
{
  ClutterMedia *media;
  ClutterMedia *music;
  ClutterActor *volume;

  gdouble previous_vol_value;
  gdouble vol_value;
};

static void
update_style_class (MexVolumeControl *self)
{
  MexVolumeControlPrivate *priv = self->priv;
  gchar *new_style_class;

  new_style_class = g_strdup_printf ("volume-%.0f", priv->vol_value * 10);

  mx_stylable_set_style_class (MX_STYLABLE (priv->volume), new_style_class);
  g_free (new_style_class);
}

static void
update_volume_and_style_class (MexVolumeControl *self)
{
  MexVolumeControlPrivate *priv = self->priv;

  clutter_media_set_audio_volume (priv->media, priv->vol_value);
  clutter_media_set_audio_volume (priv->music, priv->vol_value);
  update_style_class (self);
}

static void
mex_volume_control_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MexVolumeControl *volume = MEX_VOLUME_CONTROL (object);
  MexVolumeControlPrivate *priv = volume->priv;

  switch (property_id)
    {
    case PROP_VOLUME:
      g_value_set_double (value, priv->vol_value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_volume_control_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MexVolumeControl *volume = MEX_VOLUME_CONTROL (object);
  MexVolumeControlPrivate *priv = volume->priv;

  switch (property_id)
    {
    case PROP_VOLUME:
      priv->vol_value = g_value_get_double (value);
      update_volume_and_style_class (volume);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_volume_control_dispose (GObject *object)
{
  MexVolumeControl *self = MEX_VOLUME_CONTROL (object);
  MexVolumeControlPrivate *priv = self->priv;

  if (priv->volume)
    {
      clutter_actor_destroy (priv->volume);
      priv->volume = NULL;
    }

  G_OBJECT_CLASS (mex_volume_control_parent_class)->dispose (object);
}

static void
mex_volume_control_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_volume_control_parent_class)->finalize (object);
}

static void
mex_volume_control_class_init (MexVolumeControlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexVolumeControlPrivate));

  object_class->get_property = mex_volume_control_get_property;
  object_class->set_property = mex_volume_control_set_property;
  object_class->dispose = mex_volume_control_dispose;
  object_class->finalize = mex_volume_control_finalize;

  pspec = g_param_spec_double ("volume",
                               "Volume",
                               "Audio volume",
                               0.0, 1.0, 1.0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_VOLUME, pspec);
}

void
mex_volume_control_volume_up (MexVolumeControl *self)
{
  MexVolumeControlPrivate *priv = self->priv;

  priv->vol_value += 0.1;
  priv->vol_value = CLAMP (priv->vol_value, 0.0, 1.0);
  update_volume_and_style_class (self);
  g_object_notify (G_OBJECT (self), "volume");
}

void
mex_volume_control_volume_down (MexVolumeControl *self)
{
  MexVolumeControlPrivate *priv = self->priv;

  priv->vol_value -= 0.1;
  priv->vol_value = CLAMP (priv->vol_value, 0.0, 1.0);
  update_volume_and_style_class (self);
  g_object_notify (G_OBJECT (self), "volume");
}

void
mex_volume_control_volume_mute (MexVolumeControl *self)
{
  MexVolumeControlPrivate *priv = self->priv;

  if (priv->vol_value == 0.0)
    {
      priv->vol_value = priv->previous_vol_value;
    }
  else
    {
      priv->previous_vol_value = priv->vol_value;
      priv->vol_value = 0.0;
    }

  update_volume_and_style_class (self);
  g_object_notify (G_OBJECT (self), "volume");
}

static void
on_audio_volume_changed (ClutterMedia *media,
                         GParamSpec *pspec,
                         MexVolumeControl *self)
{
  MexVolumeControlPrivate *priv = self->priv;
  static gboolean first_notification = TRUE;
  gdouble new_volume;

  if ((clutter_media_get_playing (priv->music) && priv->music == media) ||
      (clutter_media_get_playing (priv->media) && priv->media == media))
    new_volume = clutter_media_get_audio_volume (media);
  else
    return;

  if (fabs (priv->vol_value - new_volume) < 0.01)
    return;

  priv->vol_value = CLAMP (new_volume, 0.0, 1.0);
  update_style_class (self);

  /* Pulse audio sends a notification when playing a stream for the first
   * time. We want to ignore this to we don't show the volume control when
   * playing the first stream and opening the pulse connection */
  if (G_UNLIKELY (first_notification))
    {
      first_notification = FALSE;
      return;
    }

  g_object_notify (G_OBJECT (self), "volume");
}

static void
mex_volume_control_init (MexVolumeControl *self)
{
  MexVolumeControlPrivate *priv = self->priv = VOLUME_CONTROL_PRIVATE (self);
  gchar *new_style_class;
  MexPlayer *player;
  MexMusicPlayer *music;

  player = mex_player_get_default ();
  priv->media = mex_player_get_clutter_media (player);

  music = mex_music_player_get_default ();
  priv->music = mex_music_player_get_clutter_media (music);

  priv->volume = mx_button_new ();
  mx_widget_set_disabled (MX_WIDGET (priv->volume), TRUE);

  priv->vol_value = clutter_media_get_audio_volume (priv->media);

  /* The media sound can also be changed from another process changint the
   * stream audio with pulse audio, adjust the volume on those changes */
  g_signal_connect (priv->media, "notify::audio-volume",
                    G_CALLBACK (on_audio_volume_changed), self);
  g_signal_connect (priv->music, "notify::audio-volume",
                    G_CALLBACK (on_audio_volume_changed), self);

  new_style_class = g_strdup_printf ("volume-%.0f", priv->vol_value * 10);

  mx_stylable_set_style_class (MX_STYLABLE (priv->volume), new_style_class);
  g_free (new_style_class);

  clutter_actor_add_child (CLUTTER_ACTOR (self), priv->volume);
}

ClutterActor *
mex_volume_control_new (void)
{
  return g_object_new (MEX_TYPE_VOLUME_CONTROL, NULL);
}
