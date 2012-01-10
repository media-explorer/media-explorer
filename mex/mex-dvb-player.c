/*
 * Mex - a media explorer
 *
 * Copyright © 2012 Intel Corporation.
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
/*
 * Clutter-GStreamer.
 *
 * GStreamer integration library for Clutter.
 *
 * clutter-gst-video-texture.c - ClutterTexture using GStreamer to display a
 *                               video stream.
 *
 * Authored By Matthew Allum     <mallum@openedhand.com>
 *             Damien Lespiau    <damien.lespiau@intel.com>
 *             Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
 *
 * Copyright (C) 2006 OpenedHand
 * Copyright (C) 2010, 2011 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>

#include <glib.h>
#include <gio/gio.h>
#include <gst/video/video.h>

#include "mex-dvb-player.h"
#include "mex-log.h"
#include "mex-private.h"

#define MEX_LOG_DOMAIN_DEFAULT  dvb_player_log_domain
MEX_LOG_DOMAIN(dvb_player_log_domain);

struct _MexDvbPlayerPrivate
{
  GstElement *pipeline;

  /* width / height (in pixels) of the frame data before applying the pixel
   * aspect ratio */
  gint buffer_width;
  gint buffer_height;

  /* Pixel aspect ration is par_n / par_d. this is set by the sink */
  guint par_n, par_d;

  /* natural width / height (in pixels) of the texture (after par applied) */
  guint texture_width;
  guint texture_height;

  CoglHandle idle_material;
  CoglColor idle_color_unpre;
};

enum {
  PROP_0,

  PROP_URI,
  PROP_PLAYING,
  PROP_PROGRESS,
  PROP_SUBTITLE_URI,
  PROP_SUBTITLE_FONT_NAME,
  PROP_AUDIO_VOLUME,
  PROP_CAN_SEEK,
  PROP_BUFFER_FILL,
  PROP_DURATION,

  PROP_PAR
};

static void mex_dvb_player_media_init (ClutterMediaIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexDvbPlayer,
                         mex_dvb_player,
                         CLUTTER_TYPE_TEXTURE,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_MEDIA,
                                                mex_dvb_player_media_init));

static void
set_uri (MexDvbPlayer *player,
         const gchar  *uri)
{
  g_message ("dvb-player: set uri %s", uri);
}

static const gchar *
get_uri (MexDvbPlayer *player)
{
  return NULL;
}

static void
set_playing (MexDvbPlayer *player,
             gboolean      playing)
{
  MexDvbPlayerPrivate *priv = player->priv;
  GstState state;

  if (playing)
    state = GST_STATE_PLAYING;
  else
    state = GST_STATE_PAUSED;

  gst_element_set_state (priv->pipeline, state);

  g_object_notify (G_OBJECT (player), "playing");
}


static gboolean
get_playing (MexDvbPlayer *player)
{
  MexDvbPlayerPrivate *priv = player->priv;
  GstState state, pending;
  gboolean playing;

  gst_element_get_state (priv->pipeline, &state, &pending, 0);

  if (pending)
    playing = (pending == GST_STATE_PLAYING);
  else
    playing = (state == GST_STATE_PLAYING);

  return playing;
}

static void
set_progress (MexDvbPlayer *player,
              gdouble       progress)
{
  g_message ("dvb-player: set uri %lf", progress);
}

static gdouble
get_progress (MexDvbPlayer *player)
{
  return 0.0;
}

static void
set_audio_volume (MexDvbPlayer *player,
                  gdouble       volume)
{
  g_message ("dvb-player: set uri %lf", volume);
}

static gdouble
get_audio_volume (MexDvbPlayer *player)
{
  return 0.0;
}

static gboolean
get_can_seek (MexDvbPlayer *player)
{
  return FALSE;
}

static gdouble
get_buffer_fill (MexDvbPlayer *player)
{
  return 1.0;
}

static gdouble
get_duration (MexDvbPlayer *player)
{
  return 0.0;
}

static void
create_pipeline (MexDvbPlayer *player)
{
  MexDvbPlayerPrivate *priv = player->priv;
  GstElement *pipeline, *src, *capsfilter, *sink;
  GstCaps *caps;
  gboolean result;

  g_message ("*** create dvb pipeline");

  pipeline = gst_pipeline_new (NULL);

  src = gst_element_factory_make ("videotestsrc", NULL);

  capsfilter = gst_element_factory_make ("capsfilter", NULL);
  caps = gst_caps_new_simple ("video/x-raw-yuv",
                              "format", GST_TYPE_FOURCC,
                                        GST_STR_FOURCC ("I420"),
                              "framerate", GST_TYPE_FRACTION, 30, 1,
                              NULL);
  g_object_set (capsfilter, "caps", caps, NULL);

  sink = gst_element_factory_make ("cluttersink", NULL);
  g_object_set (G_OBJECT (sink),
                "texture", CLUTTER_TEXTURE (player),
                "qos", TRUE,
		NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, capsfilter, sink, NULL);
  result = gst_element_link_many (src, capsfilter, sink, NULL);
  g_assert (result);

  priv->pipeline = pipeline;
}

/* Clutter 1.4 has this symbol, we don't want to depend on 1.4 just for that
 * just yet */
static void
_cogl_color_set_alpha_byte (CoglColor     *color,
                            unsigned char  alpha)
{
  unsigned char red, green, blue;

  red = cogl_color_get_red_byte (color);
  green = cogl_color_get_green_byte (color);
  blue = cogl_color_get_blue_byte (color);

  cogl_color_set_from_4ub (color, red, green, blue, alpha);
}

static void
gen_texcoords_and_draw_cogl_rectangle (ClutterActor *self)
{
  ClutterActorBox box;

  clutter_actor_get_allocation_box (self, &box);

  cogl_rectangle_with_texture_coords (0, 0,
                                      box.x2 - box.x1,
                                      box.y2 - box.y1,
                                      0, 0, 1.0, 1.0);
}

static void
create_black_idle_material (MexDvbPlayer *dvb_player)
{
  MexDvbPlayerPrivate *priv = dvb_player->priv;

  priv->idle_material = cogl_material_new ();
  cogl_color_set_from_4ub (&priv->idle_color_unpre, 0, 0, 0, 0xff);
  cogl_material_set_color (priv->idle_material, &priv->idle_color_unpre);
}

static void
mex_dvb_player_media_init (ClutterMediaIface *iface)
{
}

/*
 * ClutterTexture implementation
 */

static void
mex_dvb_player_size_change (ClutterTexture *texture,
			    gint            width,
			    gint            height)
{
  MexDvbPlayer *dvb_player = MEX_DVB_PLAYER (texture);
  MexDvbPlayerPrivate *priv = dvb_player->priv;
  gboolean changed;

  /* we are being told the actual (as in number of pixels in the buffers)
   * frame size. Store the values to be used in preferred_width/height() */
  changed = (priv->buffer_width != width) || (priv->buffer_height != height);
  priv->buffer_width = width;
  priv->buffer_height = height;

  if (changed)
    {
      /* reset the computed texture dimensions if the underlying frames have
       * changed size */
      MEX_INFO ("frame size has been updated to %dx%d", width, height);

      priv->texture_width = priv->texture_height = 0;

      /* queue a relayout to ask containers/layout manager to ask for
       * the preferred size again */
      clutter_actor_queue_relayout (CLUTTER_ACTOR (texture));
    }
}

/*
 * Clutter actor implementation
 */

static void
mex_dvb_player_get_natural_size (MexDvbPlayer *texture,
				 gfloat       *width,
				 gfloat       *height)
{
  MexDvbPlayerPrivate *priv = texture->priv;
  guint dar_n, dar_d;
  gboolean ret;

  /* we cache texture_width and texture_height */

  if (G_UNLIKELY (priv->buffer_width == 0 || priv->buffer_height == 0))
    {
      /* we don't know the size of the frames yet default to 0,0 */
      priv->texture_width = 0;
      priv->texture_height = 0;
    }
  else if (G_UNLIKELY (priv->texture_width == 0 || priv->texture_height == 0))
    {
      MEX_INFO ("frame is %dx%d with par %d/%d",
		priv->buffer_width, priv->buffer_height,
		priv->par_n, priv->par_d);

      ret = gst_video_calculate_display_ratio (&dar_n, &dar_d,
                                               priv->buffer_width,
                                               priv->buffer_height,
                                               priv->par_n, priv->par_d,
                                               1, 1);
      if (ret == FALSE)
        dar_n = dar_d = 1;

      if (priv->buffer_height % dar_d == 0)
        {
          priv->texture_width = gst_util_uint64_scale (priv->buffer_height,
                                                       dar_n, dar_d);
          priv->texture_height = priv->buffer_height;
        }
      else if (priv->buffer_width % dar_n == 0)
        {
          priv->texture_width = priv->buffer_width;
          priv->texture_height = gst_util_uint64_scale (priv->buffer_width,
                                                        dar_d, dar_n);

        }
      else
        {
          priv->texture_width = gst_util_uint64_scale (priv->buffer_height,
                                                       dar_n, dar_d);
          priv->texture_height = priv->buffer_height;
        }

      MEX_INFO ("final size is %dx%d (calculated par is %d/%d)",
		priv->texture_width, priv->texture_height,
		dar_n, dar_d);
    }

  if (width)
    *width = (gfloat)priv->texture_width;

  if (height)
    *height = (gfloat)priv->texture_height;
}

static void
mex_dvb_player_get_preferred_width (ClutterActor *self,
				    gfloat        for_height,
				    gfloat       *min_width_p,
				    gfloat       *natural_width_p)
{
  MexDvbPlayer *texture = MEX_DVB_PLAYER (self);
  MexDvbPlayerPrivate *priv = texture->priv;
  gboolean sync_size, keep_aspect_ratio;
  gfloat natural_width, natural_height;

  /* Min request is always 0 since we can scale down or clip */
  if (min_width_p)
    *min_width_p = 0;

  sync_size = clutter_texture_get_sync_size (CLUTTER_TEXTURE (self));
  keep_aspect_ratio =
    clutter_texture_get_keep_aspect_ratio (CLUTTER_TEXTURE (self));

  mex_dvb_player_get_natural_size (texture, &natural_width, &natural_height);

  if (sync_size)
    {
      if (natural_width_p)
        {
          if (!keep_aspect_ratio ||
              for_height < 0 ||
              priv->buffer_height <= 0)
            {
              *natural_width_p = natural_width;
            }
          else
            {
              /* Set the natural width so as to preserve the aspect ratio */
              gfloat ratio =  natural_width /  natural_height;

              *natural_width_p = ratio * for_height;
            }
        }
    }
  else
    {
      if (natural_width_p)
        *natural_width_p = 0;
    }
}

static void
mex_dvb_player_get_preferred_height (ClutterActor *self,
                                     gfloat        for_width,
                                     gfloat       *min_height_p,
                                     gfloat       *natural_height_p)
{
  MexDvbPlayer *texture = MEX_DVB_PLAYER (self);
  MexDvbPlayerPrivate *priv = texture->priv;
  gboolean sync_size, keep_aspect_ratio;
  gfloat natural_width, natural_height;

  /* Min request is always 0 since we can scale down or clip */
  if (min_height_p)
    *min_height_p = 0;

  sync_size = clutter_texture_get_sync_size (CLUTTER_TEXTURE (self));
  keep_aspect_ratio =
    clutter_texture_get_keep_aspect_ratio (CLUTTER_TEXTURE (self));

  mex_dvb_player_get_natural_size (texture, &natural_width, &natural_height);

  if (sync_size)
    {
      if (natural_height_p)
        {
          if (!keep_aspect_ratio ||
              for_width < 0 ||
              priv->buffer_width <= 0)
            {
              *natural_height_p = natural_height;
            }
          else
            {
              /* Set the natural height so as to preserve the aspect ratio */
              gfloat ratio = natural_height / natural_width;

              *natural_height_p = ratio * for_width;
            }
        }
    }
  else
    {
      if (natural_height_p)
        *natural_height_p = 0;
    }
}

/*
 * ClutterTexture unconditionnaly sets the material color to:
 *    (opacity,opacity,opacity,opacity)
 * so we can't set a black material to the texture. Let's override paint()
 * for now.
 */
static void
mex_dvb_player_paint (ClutterActor *actor)
{
  MexDvbPlayer *dvb_player = (MexDvbPlayer *) actor;
  MexDvbPlayerPrivate *priv = dvb_player->priv;
  ClutterActorClass *actor_class;
  gboolean is_idle;

  //is_idle = clutter_gst_player_get_idle (CLUTTER_GST_PLAYER (dvb_player));
  is_idle = FALSE;
  if (G_UNLIKELY (is_idle))
    {
      CoglColor *color;
      gfloat alpha;

      /* blend the alpha of the idle material with the actor's opacity */
      color = cogl_color_copy (&priv->idle_color_unpre);
      alpha = clutter_actor_get_paint_opacity (actor) *
              cogl_color_get_alpha_byte (color) / 0xff;
      _cogl_color_set_alpha_byte (color, alpha);
      cogl_color_premultiply (color);
      cogl_material_set_color (priv->idle_material, color);

      cogl_set_source (priv->idle_material);

      /* draw */
      gen_texcoords_and_draw_cogl_rectangle (actor);
    }
  else
    {
      /* when not idle, just chain up to ClutterTexture::paint() */
      actor_class = CLUTTER_ACTOR_CLASS (mex_dvb_player_parent_class);
      actor_class->paint (actor);
    }

}

/*
 * GObject implementation
 */

static void
mex_dvb_player_dispose (GObject *object)
{
  MexDvbPlayer *self = MEX_DVB_PLAYER (object);
  MexDvbPlayerPrivate *priv = self->priv;

  if (priv->pipeline)
    {
      g_object_unref (priv->pipeline);
      priv->pipeline = NULL;
    }

  G_OBJECT_CLASS (mex_dvb_player_parent_class)->dispose (object);
}

static void
mex_dvb_player_finalize (GObject *object)
{
  MexDvbPlayer        *self;
  MexDvbPlayerPrivate *priv;

  self = MEX_DVB_PLAYER (object);
  priv = self->priv;

  if (priv->idle_material != COGL_INVALID_HANDLE)
    cogl_handle_unref (priv->idle_material);

  G_OBJECT_CLASS (mex_dvb_player_parent_class)->finalize (object);
}

static void
mex_dvb_player_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MexDvbPlayer *dvb_player = MEX_DVB_PLAYER (object);
  MexDvbPlayerPrivate *priv = dvb_player->priv;

  switch (property_id)
    {
    case PROP_URI:
      set_uri (dvb_player, g_value_get_string (value));
      break;

    case PROP_PLAYING:
      set_playing (dvb_player, g_value_get_boolean (value));
      break;

    case PROP_PROGRESS:
      set_progress (dvb_player, g_value_get_double (value));
      break;

    case PROP_AUDIO_VOLUME:
      set_audio_volume (dvb_player, g_value_get_double (value));
      break;

    case PROP_PAR:
      priv->par_n = gst_value_get_fraction_numerator (value);
      priv->par_d = gst_value_get_fraction_denominator (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_dvb_player_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MexDvbPlayer *dvb_player = MEX_DVB_PLAYER (object);
  MexDvbPlayerPrivate *priv = dvb_player->priv;

  switch (property_id)
    {
    case PROP_URI:
      g_value_set_string (value, get_uri (dvb_player));
      break;

    case PROP_PLAYING:
      g_value_set_boolean (value, get_playing (dvb_player));
      break;

    case PROP_PROGRESS:
      g_value_set_double (value, get_progress (dvb_player));

    case PROP_AUDIO_VOLUME:
      g_value_set_double (value, get_audio_volume (dvb_player));
      break;

    case PROP_CAN_SEEK:
      g_value_set_boolean (value, get_can_seek (dvb_player));
      break;

    case PROP_BUFFER_FILL:
      g_value_set_double (value, get_buffer_fill (dvb_player));
      break;

    case PROP_DURATION:
      g_value_set_double (value, get_duration (dvb_player));
      break;

    case PROP_PAR:
      gst_value_set_fraction (value, priv->par_n, priv->par_d);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_dvb_player_class_init (MexDvbPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  ClutterTextureClass *texture_class = CLUTTER_TEXTURE_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexDvbPlayerPrivate));

  object_class->dispose      = mex_dvb_player_dispose;
  object_class->finalize     = mex_dvb_player_finalize;
  object_class->set_property = mex_dvb_player_set_property;
  object_class->get_property = mex_dvb_player_get_property;

  actor_class->paint = mex_dvb_player_paint;
  actor_class->get_preferred_width = mex_dvb_player_get_preferred_width;
  actor_class->get_preferred_height = mex_dvb_player_get_preferred_height;

  texture_class->size_change = mex_dvb_player_size_change;

  g_object_class_override_property (object_class,
                                    PROP_URI, "uri");
  g_object_class_override_property (object_class,
                                    PROP_PLAYING, "playing");
  g_object_class_override_property (object_class,
                                    PROP_PROGRESS, "progress");
  g_object_class_override_property (object_class,
                                    PROP_SUBTITLE_URI, "subtitle-uri");
  g_object_class_override_property (object_class,
                                    PROP_SUBTITLE_FONT_NAME,
                                    "subtitle-font-name");
  g_object_class_override_property (object_class,
                                    PROP_AUDIO_VOLUME, "audio-volume");
  g_object_class_override_property (object_class,
                                    PROP_CAN_SEEK, "can-seek");
  g_object_class_override_property (object_class,
                                    PROP_DURATION, "duration");
  g_object_class_override_property (object_class,
                                    PROP_BUFFER_FILL, "buffer-fill");

  pspec = gst_param_spec_fraction ("pixel-aspect-ratio",
                                   "Pixel Aspect Ratio",
                                   "Pixel aspect ratio of incoming frames",
                                   1, 100, 100, 1, 1, 1,
                                   MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PAR, pspec);
}

static void
idle_cb (MexDvbPlayer *dvb_player,
         GParamSpec             *pspec,
         gpointer                data)
{
  /* restore the idle material so we don't just display the last frame */
  clutter_actor_queue_redraw (CLUTTER_ACTOR (dvb_player));
}

static void
mex_dvb_player_init (MexDvbPlayer *dvb_player)
{
  MexDvbPlayerPrivate *priv;

  MEX_LOG_DOMAIN_INIT (dvb_player_log_domain, "dvb-player");

  dvb_player->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (dvb_player,
							 MEX_TYPE_DVB_PLAYER,
							 MexDvbPlayerPrivate);

  create_pipeline (dvb_player);
  create_black_idle_material (dvb_player);

  priv->par_n = priv->par_d = 1;

  g_signal_connect (dvb_player, "notify::idle", G_CALLBACK (idle_cb), NULL);
}

ClutterActor*
mex_dvb_player_new (void)
{
  return g_object_new (MEX_TYPE_DVB_PLAYER,
                       "disable-slicing", TRUE,
                       NULL);
}
