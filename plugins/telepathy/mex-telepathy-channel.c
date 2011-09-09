/*
 * Mex - a media explorer
 *
 * Copyright  2011 Collabora Ltd.
 *   @author Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#include "mex-telepathy-channel.h"

#include <clutter-gst/clutter-gst.h>
#include <gst/farsight/fs-element-added-notifier.h>
#include <gst/farsight/fs-utils.h>
#include <gst/gstelement.h>
#include <telepathy-farstream/channel.h>
#include <telepathy-farstream/content.h>
#include <telepathy-glib/channel.h>
#include <telepathy-yell/call-channel.h>
#include <telepathy-yell/cli-call.h>

#include <mex/mex.h>

#define MEX_LOG_DOMAIN_DEFAULT  telepathy_channel_log_domain
MEX_LOG_DOMAIN_STATIC(telepathy_channel_log_domain);

G_DEFINE_TYPE (MexTelepathyChannel, mex_telepathy_channel, G_TYPE_OBJECT)

#define TELEPATHY_CHANNEL_PRIVATE(o)                            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TELEPATHY_CHANNEL,     \
                                MexTelepathyChannelPrivate))

struct _MexTelepathyChannelPrivate
{
  ClutterActor *video_call_page;
  ClutterActor *camera_button;
  ClutterActor *mute_button;
  ClutterActor *end_button;
  ClutterActor *title_label;
  ClutterActor *video_outgoing;

  GstElement   *incoming_sink;
  GstElement   *outgoing_sink;
  GstElement   *outgoing_mic;
  gboolean      sending_video;
  gboolean      muted;

  GstElement   *pipeline;
  guint         buswatch;
  TpConnection *connection;
  TpChannel    *channel;
  TfChannel    *tf_channel;
  GList        *notifiers;

  GstElement *video_input;
  GstElement *video_capsfilter;

  guint width;
  guint height;
  guint framerate;
};

enum
{
  SHOW_ACTOR,
  HIDE_ACTOR,
  LAST_SIGNAL
};

static guint
  mex_telepathy_channel_signals[LAST_SIGNAL] = { 0 };

enum
{
  PROP_0,

  PROP_CHANNEL,
  PROP_CONNECTION
};

static void
mex_telepathy_channel_dispose (GObject *gobject)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (gobject);
  MexTelepathyChannelPrivate *priv = self->priv;

  if (priv->video_call_page)
    {
      /* This will also destroy all children of this container */
      clutter_actor_destroy (priv->video_call_page);
      priv->video_call_page = NULL;
    }

  if (priv->incoming_sink)
    {
      g_object_unref (priv->incoming_sink);
      priv->incoming_sink = NULL;
    }

  if (priv->outgoing_sink)
    {
      g_object_unref (priv->outgoing_sink);
      priv->outgoing_sink = NULL;
    }
  if (priv->outgoing_mic)
    {
      g_object_unref (priv->outgoing_mic);
      priv->outgoing_mic = NULL;
    }
  if (priv->pipeline)
    {
      g_object_unref (priv->pipeline);
      priv->pipeline = NULL;
    }
  if (priv->connection)
    {
      g_object_unref (priv->connection);
      priv->connection = NULL;
    }
  if (priv->channel)
    {
      g_object_unref (priv->channel);
      priv->channel = NULL;
    }
  if (priv->tf_channel)
    {
      g_object_unref (priv->tf_channel);
      priv->tf_channel = NULL;
    }
  if (priv->notifiers)
    {
      g_object_unref (priv->notifiers);
      priv->notifiers = NULL;
    }

  if (priv->video_input)
    {
      g_object_unref (priv->video_input);
      priv->video_input = NULL;
    }
  if (priv->video_capsfilter)
    {
      g_object_unref (priv->video_capsfilter);
      priv->video_capsfilter = NULL;
    }
}

static void
mex_telepathy_channel_finalize (GObject *gobject)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (gobject);
  MexTelepathyChannelPrivate *priv = self->priv;

  g_source_remove (priv->buswatch);
  G_OBJECT_CLASS (mex_telepathy_channel_parent_class)->finalize (gobject);
}

static void
mex_telepathy_channel_on_video_shown (ClutterActor *actor,
                                      gpointer      user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;

  ClutterStage *stage;
  MxFocusManager *fmanager;

  stage = CLUTTER_STAGE (clutter_actor_get_stage (CLUTTER_ACTOR (actor)));

  if (stage)
    {
      fmanager = mx_focus_manager_get_for_stage (stage);
      mx_focus_manager_push_focus (fmanager, MX_FOCUSABLE (priv->end_button));
    }
}

static void
mex_telepathy_channel_on_video_closed (ClutterActor *actor,
                                       gpointer      user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);

  tp_channel_close_async (self->priv->channel, NULL, NULL);
}

static void
mex_telepathy_channel_on_hangup (MxAction *action,
                                 gpointer  user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);

  tp_channel_close_async (self->priv->channel, NULL, NULL);
}

static void
mex_telepathy_channel_toggle_camera (MxAction *action,
                                     gpointer  user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;
  TpyCallChannel *channel = TPY_CALL_CHANNEL (self->priv->channel);

  tpy_call_channel_send_video (channel, !priv->sending_video);
  priv->sending_video = !priv->sending_video;

  if (priv->sending_video)
    {
      clutter_actor_show (priv->video_outgoing);
      // We are starting, so change the button to pause.
      mx_stylable_set_style_class (MX_STYLABLE (
                                     priv->camera_button), "CameraOff");
      mx_action_set_display_name (action, "Camera Off");
    }
  else
    {
      clutter_actor_hide (priv->video_outgoing);
      // We are stopping, so change the button to play.
      mx_stylable_set_style_class (MX_STYLABLE (
                                     priv->camera_button), "CameraOn");
      mx_action_set_display_name (action, "Camera On");
    }
}

void
mex_telepathy_channel_toggle_mute (MxAction *action,
                                   gpointer  user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;
  GstStateChangeReturn ret;

  priv->muted = !priv->muted;

  if (priv->muted)
    {
      if (gst_element_set_state (priv->outgoing_mic, GST_STATE_PAUSED)
          == GST_STATE_CHANGE_FAILURE)
        MEX_WARNING ("failed to mute microphone");
      mx_stylable_set_style_class (MX_STYLABLE(
                                     priv->mute_button), "MediaUnmute");
      mx_action_set_display_name (action, "Mic On");
    }
  else
    {
      if (gst_element_set_state (priv->outgoing_mic, GST_STATE_PLAYING)
          == GST_STATE_CHANGE_FAILURE)
        MEX_WARNING ("failed to unmute microphone");

      mx_stylable_set_style_class (MX_STYLABLE(priv->mute_button), "MediaMute");
      mx_action_set_display_name (action, "Mic Off");
    }
}

void
mex_telepathy_channel_create_video_page (MexTelepathyChannel *self)
{
  MEX_DEBUG ("creating video page");
  MexTelepathyChannelPrivate *priv = MEX_TELEPATHY_CHANNEL (self)->priv;

  gchar *static_image_path;

  ClutterActor *toolbar;
  ClutterActor *toolbar_area;
  ClutterActor *video_preview_padding;
  ClutterActor *video_preview_area;
  ClutterActor *static_outgoing;
  ClutterActor *video_incoming_area;
  ClutterActor *video_incoming;
  ClutterActor *avatar_image;

  MxAction *end_action;
  MxAction *camera_action;
  MxAction *mute_action;

  GError *error = NULL;

  MexShadow *shadow = mex_shadow_new ();
  mex_shadow_set_radius_x (shadow, 15);
  mex_shadow_set_radius_y (shadow, 15);
  ClutterColor shadow_color = {0, 0, 0, 64};
  mex_shadow_set_color (shadow, &shadow_color);

  priv->video_outgoing = clutter_texture_new ();
  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (priv->video_outgoing),
                                         TRUE);

  priv->outgoing_sink =
    clutter_gst_video_sink_new (CLUTTER_TEXTURE (priv->video_outgoing));

  static_image_path = g_build_filename (mex_get_data_dir (),
                                        "style",
                                        "thumb-call-pip-off.png",
                                        NULL);

  static_outgoing = clutter_texture_new_from_file (static_image_path,
                                                   &error);
  if (error)
    {
      g_warning ("Error loading texture %s", error->message);
      g_clear_error (&error);
    }

  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE
                                         (static_outgoing), TRUE);

  video_incoming_area = mx_frame_new();
  video_incoming = clutter_texture_new ();
  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (video_incoming),
                                         TRUE);

  clutter_container_add (CLUTTER_CONTAINER (video_incoming_area),
                         video_incoming,
                         NULL);
  mx_bin_set_fill (MX_BIN (video_incoming_area),
                   TRUE,
                   TRUE);
  clutter_actor_set_width (video_incoming, 768.0);
  clutter_actor_set_height (video_incoming, 576.0);
  mx_stylable_set_style_class (MX_STYLABLE (video_incoming_area),
                               "CallWindow");
  clutter_actor_add_effect (video_incoming_area, 
                            CLUTTER_EFFECT (shadow));

  priv->incoming_sink =
    clutter_gst_video_sink_new (CLUTTER_TEXTURE (video_incoming));

  video_preview_area = mx_stack_new ();

  clutter_container_add (CLUTTER_CONTAINER (video_preview_area),
                         static_outgoing,
                         priv->video_outgoing,
                         NULL);

  mx_stylable_set_style_class (MX_STYLABLE (video_preview_area),
                               "PreviewStack");

  clutter_actor_set_height (video_preview_area, 150.0);
  clutter_actor_add_effect (video_preview_area, 
                            CLUTTER_EFFECT (shadow));
  
  video_preview_padding = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (video_preview_padding),
                               "PreviewPadding");
  clutter_container_add (MX_FRAME (video_preview_padding),
                         video_preview_area,
                         NULL);

  /* Top container */
  priv->video_call_page = mx_stack_new ();

  // Create the user label
  avatar_image = mx_image_new ();
  static_image_path = g_build_filename (mex_get_data_dir (),
                                        "style",
                                        "thumb-call-avatar-small.png",
                                        NULL);
  mx_image_set_from_file (MX_IMAGE (avatar_image),
                          static_image_path,
                          NULL);
  g_free (static_image_path);
  
  priv->title_label = mx_label_new ();
  mx_label_set_y_align (MX_LABEL (priv->title_label), MX_ALIGN_MIDDLE);
  mx_label_set_x_align (MX_LABEL (priv->title_label), MX_ALIGN_MIDDLE);


  end_action =
    mx_action_new_full ("End",
                        "Hang Up",
                        G_CALLBACK (mex_telepathy_channel_on_hangup),
                        self);

  priv->end_button = mex_action_button_new (end_action);
  mx_stylable_set_style_class (MX_STYLABLE (priv->end_button), "EndCall");

  camera_action =
    mx_action_new_full("Camera",
                       "Camera Off",
                       G_CALLBACK (mex_telepathy_channel_toggle_camera),
                       self);

  priv->camera_button = mex_action_button_new (camera_action);
  mx_stylable_set_style_class (MX_STYLABLE (priv->camera_button), "CameraOff");

  mute_action =
    mx_action_new_full("Mute",
                       "Mic Off",
                       G_CALLBACK (mex_telepathy_channel_toggle_mute),
                       self);

  priv->mute_button = mex_action_button_new (mute_action);
  mx_stylable_set_style_class (MX_STYLABLE (priv->mute_button), "MediaMute");


  toolbar = mx_box_layout_new ();
  mx_stylable_set_style_class (MX_STYLABLE (toolbar),
                               "MexCallControlsTitle");
  // Put the buttons in the toolbar
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (toolbar),
                                           CLUTTER_ACTOR (avatar_image),
                                           0,
                                           "expand",
                                           TRUE,
                                           NULL);

  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (toolbar),
                                           priv->title_label,
                                           1,
                                           "expand",
                                           TRUE,
                                           NULL);

  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (toolbar),
                                           priv->camera_button,
                                           2,
                                           "expand",
                                           TRUE,
                                           NULL);

  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (toolbar),
                                           priv->mute_button,
                                           3,
                                           "expand",
                                           TRUE,
                                           NULL);

  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (toolbar),
                                           priv->end_button,
                                           4,
                                           "expand",
                                           TRUE,
                                           NULL);

  toolbar_area = mx_frame_new ();
  mx_bin_set_child (MX_BIN (toolbar_area), toolbar);

  clutter_container_add (CLUTTER_CONTAINER (priv->video_call_page),
                         video_incoming_area,
                         video_preview_padding,
                         toolbar_area,
                         NULL);

  /* Arrange the preview video area on the page */
  mx_stack_child_set_x_align (MX_STACK (priv->video_call_page),
                              video_preview_padding,
                              MX_ALIGN_END);

  mx_stack_child_set_y_align (MX_STACK (priv->video_call_page),
                              video_preview_padding,
                              MX_ALIGN_START);

  mx_stack_child_set_x_fill (MX_STACK (priv->video_call_page),
                             video_preview_padding,
                             FALSE);

  mx_stack_child_set_y_fill (MX_STACK (priv->video_call_page),
                             video_preview_padding,
                             FALSE);

  mx_stack_child_set_x_fill (MX_STACK (priv->video_call_page),
                             video_incoming_area,
                             FALSE);
  
  mx_stack_child_set_y_fill (MX_STACK (priv->video_call_page),
                             video_incoming_area,
                             FALSE);
  
  /* Arrange the toolbar area on the page */
  mx_stack_child_set_x_align (MX_STACK (priv->video_call_page),
                              toolbar_area,
                              MX_ALIGN_MIDDLE);

  mx_stack_child_set_y_align (MX_STACK (priv->video_call_page),
                              toolbar_area,
                              MX_ALIGN_END);

  mx_stack_child_set_x_fill (MX_STACK (priv->video_call_page),
                             toolbar_area,
                             TRUE);

  mx_stack_child_set_y_fill (MX_STACK (priv->video_call_page),
                             toolbar_area,
                             FALSE);

  // Connect to hide signals.
  g_signal_connect (priv->video_call_page,
                    "hide",
                    G_CALLBACK (mex_telepathy_channel_on_video_closed),
                    self);

  // Connect to show signals.
  g_signal_connect (priv->video_call_page,
                    "show",
                    G_CALLBACK (mex_telepathy_channel_on_video_shown),
                    self);
}

static gboolean
mex_telepathy_channel_on_bus_watch (GstBus     *bus,
                                    GstMessage *message,
                                    gpointer    user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;

  if (priv->tf_channel != NULL)
    tf_channel_bus_message (priv->tf_channel, message);

  if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ERROR)
    {
      GError *error = NULL;
      gchar *debug = NULL;
      gst_message_parse_error (message, &error, &debug);
      MEX_ERROR ("ERROR from element %s: %s\n",
                 GST_OBJECT_NAME (message->src), error->message);
      MEX_ERROR ("Debugging info: %s\n", (debug) ? debug : "none");
      g_error_free (error);
      g_free (debug);
    }
  return TRUE;
}

static void
mex_telepathy_channel_on_src_pad_added (TfContent *content,
                                        TpHandle   handle,
                                        FsStream  *stream,
                                        GstPad    *pad,
                                        FsCodec   *codec,
                                        gpointer   user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;

  gchar *cstr = fs_codec_to_string (codec);
  FsMediaType mtype;
  GstPad *sinkpad;
  GstElement *element;
  GstStateChangeReturn ret;

  MEX_DEBUG ("New src pad: %s", cstr);
  g_object_get (content, "media-type", &mtype, NULL);

  switch (mtype)
    {
    case FS_MEDIA_TYPE_AUDIO:
      element = gst_parse_bin_from_description (
        "audioconvert ! audioresample ! audioconvert ! autoaudiosink",
        TRUE, NULL);
      break;
    case FS_MEDIA_TYPE_VIDEO:
      element = self->priv->incoming_sink;
      break;
    default:
      MEX_WARNING ("Unknown media type");
      return;
    }

  gst_bin_add (GST_BIN (priv->pipeline), element);
  sinkpad = gst_element_get_pad (element, "sink");
  ret = gst_element_set_state (element, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE)
    {
      tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
      MEX_WARNING ("Failed to start sink pipeline !?");
      return;
    }

  if (GST_PAD_LINK_FAILED (gst_pad_link (pad, sinkpad)))
    {
      tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
      MEX_WARNING ("Couldn't link sink pipeline !?");
      return;
    }

  g_object_unref (sinkpad);
}

static void
mex_telepathy_channel_update_video_parameters (MexTelepathyChannel *self,
                                               gboolean             restart)
{
  MexTelepathyChannelPrivate *priv = self->priv;
  GstCaps *caps;
  GstClock *clock;

  if (restart)
    {
      /* Assuming the pipeline is in playing state */
      gst_element_set_locked_state (priv->video_input, TRUE);
      gst_element_set_state (priv->video_input, GST_STATE_NULL);
    }

  g_object_get (priv->video_capsfilter, "caps", &caps, NULL);
  caps = gst_caps_make_writable (caps);

  gst_caps_set_simple (caps,
                       "framerate", GST_TYPE_FRACTION, priv->framerate, 1,
                       "width", G_TYPE_INT, priv->width,
                       "height", G_TYPE_INT, priv->height,
                       NULL);

  g_object_set (priv->video_capsfilter, "caps", caps, NULL);

  if (restart)
    {
      clock = gst_pipeline_get_clock (GST_PIPELINE (priv->pipeline));
      /* Need to reset the clock if we set the pipeline back to ready by hand */
      if (clock != NULL)
        {
          gst_element_set_clock (priv->video_input, clock);
          g_object_unref (clock);
        }

      gst_element_set_locked_state (priv->video_input, FALSE);
      gst_element_sync_state_with_parent (priv->video_input);
    }
}

static void
mex_telepathy_channel_on_video_framerate_changed (TfContent           *content,
                                                  GParamSpec          *spec,
                                                  MexTelepathyChannel *self)
{
  MexTelepathyChannelPrivate *priv = self->priv;

  guint framerate;

  g_object_get (content, "framerate", &framerate, NULL);

  if (framerate != 0)
    priv->framerate = framerate;

  mex_telepathy_channel_update_video_parameters (self, FALSE);
}

static void
mex_telepathy_channel_on_video_resolution_changed (TfContent           *content,
                                                   guint                width,
                                                   guint                height,
                                                   MexTelepathyChannel *self)
{
  MexTelepathyChannelPrivate *priv = self->priv;

  g_assert (width > 0 && height > 0);

  priv->width = width;
  priv->height = height;

  mex_telepathy_channel_update_video_parameters (self, TRUE);
}

static gboolean
mex_telepathy_channel_on_video_start_sending (TfContent *content,
                                              gpointer   user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;

  priv->sending_video = TRUE;

  return TRUE;
}

static GstElement *
mex_telepathy_channel_setup_video_source (MexTelepathyChannel *self,
                                          TfContent           *content)
{
  MexTelepathyChannelPrivate *priv = self->priv;
  GstElement *result, *input, *rate, *scaler, *colorspace, *capsfilter;
  GstCaps *caps;
  guint framerate = 0, width = 0, height = 0;
  GstPad *pad, *ghost;

  result = gst_bin_new ("video_input");
  input = gst_element_factory_make ("autovideosrc", NULL);
  rate = gst_element_factory_make ("videomaxrate", NULL);
  scaler = gst_element_factory_make ("videoscale", NULL);
  colorspace = gst_element_factory_make ("ffmpegcolorspace", NULL);
  capsfilter = gst_element_factory_make ("capsfilter", NULL);

  g_assert (input && rate && scaler && colorspace && capsfilter);
  g_object_get (content,
                "framerate", &framerate,
                "width", &width,
                "height", &height,
                NULL);

  if (framerate == 0)
    framerate = 15;

  if (width == 0 || height == 0)
    {
      width = 320;
      height = 240;
    }

  priv->framerate = framerate;
  priv->width = width;
  priv->height = height;

  caps = gst_caps_new_simple ("video/x-raw-yuv",
                              "width", G_TYPE_INT, width,
                              "height", G_TYPE_INT, height,
                              "framerate", GST_TYPE_FRACTION, framerate, 1,
                              NULL);

  g_object_set (G_OBJECT (capsfilter), "caps", caps, NULL);

  gst_bin_add_many (GST_BIN (result), input, rate, scaler,
                    colorspace, capsfilter, NULL);
  g_assert (gst_element_link_many (input, rate, scaler,
                                   colorspace, capsfilter, NULL));

  pad = gst_element_get_static_pad (capsfilter, "src");
  g_assert (pad != NULL);

  GstElement *tee = gst_element_factory_make("tee", NULL);
  if (!tee)
    {
      MEX_WARNING("Couldn't create tee element !?");
      return NULL;
    }
  GstPad *teesink = gst_element_get_pad(tee, "sink");
  gst_bin_add (GST_BIN(result), tee);
  if (GST_PAD_LINK_FAILED (gst_pad_link (pad, teesink)))
    {
      MEX_WARNING ("Couldn't link source pipeline to tee !?");
      return NULL;
    }
  pad = gst_element_get_request_pad (tee, "src%d");

  // Link the tee to the preview widget.
  GstPad *teesrc = gst_element_get_request_pad(tee, "src%d");
  GstPad *outsink = gst_element_get_pad (self->priv->outgoing_sink, "sink");
  gst_bin_add (GST_BIN(result), self->priv->outgoing_sink);
  gst_pad_link (teesrc, outsink);

  ghost = gst_ghost_pad_new ("src", pad);
  gst_element_add_pad (result, ghost);

  g_object_unref (pad);

  priv->video_input = result;
  priv->video_capsfilter = capsfilter;

  g_signal_connect (content, "notify::framerate",
                    G_CALLBACK (
                      mex_telepathy_channel_on_video_framerate_changed),
                    self);

  g_signal_connect (content, "resolution-changed",
                    G_CALLBACK (
                      mex_telepathy_channel_on_video_resolution_changed),
                    self);

  g_signal_connect (content, "start-sending",
                    G_CALLBACK (mex_telepathy_channel_on_video_start_sending),
                    self);

  return result;
}

static void
mex_telepathy_channel_on_content_added (TfChannel *channel,
                                        TfContent *content,
                                        gpointer   user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;
  GstPad *srcpad, *sinkpad;
  FsMediaType mtype;
  GstElement *element;
  GstStateChangeReturn ret;

  MEX_DEBUG ("Content added");

  g_object_get (content,
                "sink-pad", &sinkpad,
                "media-type", &mtype,
                NULL);

  switch (mtype)
    {
    case FS_MEDIA_TYPE_AUDIO:
      MEX_DEBUG ("Audio content added");
      element = gst_parse_bin_from_description (
        "autoaudiosrc ! audioresample ! audioconvert ", TRUE, NULL);
      priv->outgoing_mic = element;
      priv->muted = FALSE;
      break;
    case FS_MEDIA_TYPE_VIDEO:
      MEX_DEBUG ("Video content added");
      element = mex_telepathy_channel_setup_video_source (self, content);
      break;
    default:
      MEX_WARNING ("Unknown media type");
      g_object_unref (sinkpad);
      return;
    }

  g_signal_connect (content, "src-pad-added",
                    G_CALLBACK (mex_telepathy_channel_on_src_pad_added), self);

  gst_bin_add (GST_BIN (priv->pipeline), element);
  srcpad = gst_element_get_pad (element, "src");

  if (GST_PAD_LINK_FAILED (gst_pad_link (srcpad, sinkpad)))
    {
      tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
      MEX_WARNING ("Couldn't link source pipeline !?");
      return;
    }

  ret = gst_element_set_state (element, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE)
    {
      tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
      MEX_WARNING ("source pipeline failed to start!?");
      return;
    }

  g_object_unref (srcpad);
  g_object_unref (sinkpad);
}

static void
mex_telepathy_channel_conference_added (TfChannel  *channel,
                                        GstElement *conference,
                                        gpointer    user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;

  GKeyFile *keyfile;

  MEX_INFO ("Conference added");

  /* Add notifier to set the various element properties as needed */
  keyfile = fs_utils_get_default_element_properties (conference);
  if (keyfile != NULL)
    {
      FsElementAddedNotifier *notifier;
      MEX_INFO ("Loaded default codecs for %s", GST_ELEMENT_NAME (conference));

      notifier = fs_element_added_notifier_new ();
      fs_element_added_notifier_set_properties_from_keyfile (notifier, keyfile);
      fs_element_added_notifier_add (notifier, GST_BIN (priv->pipeline));

      priv->notifiers = g_list_prepend (priv->notifiers, notifier);
    }

  gst_bin_add (GST_BIN (priv->pipeline), conference);
  gst_element_set_state (conference, GST_STATE_PLAYING);

  g_signal_emit(self,
                mex_telepathy_channel_signals[SHOW_ACTOR],
                0,
                g_object_ref (priv->video_call_page));
}

static gboolean
mex_telepathy_channel_dump_pipeline (gpointer user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);

  if (self->priv->pipeline)
    {
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (self->priv->pipeline),
                                         GST_DEBUG_GRAPH_SHOW_ALL,
                                         "call-handler");

      return TRUE;
    }

  return FALSE;
}

static void
mex_telepathy_channel_new_tf_channel (GObject      *source,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;

  MEX_DEBUG ("New TfChannel");

  priv->tf_channel = TF_CHANNEL (g_async_initable_new_finish (
                                   G_ASYNC_INITABLE (source), result, NULL));

  if (priv->tf_channel == NULL)
    {
      MEX_WARNING ("Failed to create channel");
      return;
    }

  MEX_DEBUG ("Adding timeout");
  g_timeout_add_seconds (5, mex_telepathy_channel_dump_pipeline, self);

  g_signal_connect (priv->tf_channel, "fs-conference-added",
                    G_CALLBACK (mex_telepathy_channel_conference_added), self);

  g_signal_connect (priv->tf_channel, "content-added",
                    G_CALLBACK (mex_telepathy_channel_on_content_added), self);
}

static void
mex_telepathy_channel_on_call_state_changed (TpyCallChannel *channel,
                                             TpyCallState    state,
                                             TpyCallFlags    flags,
                                             GValueArray    *state_reason,
                                             GHashTable     *state_details,
                                             gpointer        user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);

  if (state == TPY_CALL_STATE_ENDED)
    {
      tp_channel_close_async (self->priv->channel, NULL, NULL);
    }
}

static void
mex_telepathy_channel_on_proxy_invalidated (TpProxy *proxy,
                                            guint    domain,
                                            gint     code,
                                            gchar   *message,
                                            gpointer user_data)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
  MexTelepathyChannelPrivate *priv = self->priv;

  MEX_INFO ("Channel closed");
  if (priv->pipeline != NULL)
    {
      gst_element_set_state (priv->pipeline, GST_STATE_NULL);
      g_object_unref (priv->pipeline);
      priv->pipeline = NULL;
    }

  if (priv->tf_channel != NULL)
    g_object_unref (priv->tf_channel);

  g_list_foreach (priv->notifiers, (GFunc)g_object_unref, NULL);
  g_list_free (priv->notifiers);

  g_object_unref (priv->channel);

  g_signal_emit (self,
                 mex_telepathy_channel_signals[HIDE_ACTOR],
                 0,
                 g_object_ref (priv->video_call_page));
}

static void
mex_telepathy_channel_on_contact_fetched (TpConnection     *connection,
                                          guint             n_contacts,
                                          TpContact *const *contacts,
                                          guint             n_failed,
                                          const TpHandle   *failed,
                                          const GError     *error,
                                          gpointer          user_data,
                                          GObject          *weak_object)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);

  int i = 0;

  for (i = 0; i < n_contacts; ++i)
    {
      // Get the contacts.
      TpContact *current = contacts[i];

      // Connect to alias change signal.
      // Add the alias to the label.
      mx_label_set_text (MX_LABEL (self->priv->title_label),
                         tp_contact_get_alias (current));
    }
}

void
mex_telepathy_channel_initialize_channel (MexTelepathyChannel *self)
{
  MexTelepathyChannelPrivate *priv = self->priv;

  GstBus *bus;
  GstElement *pipeline;
  GstStateChangeReturn ret;

  MEX_INFO ("New channel");

  TpHandle contactHandle = tp_channel_get_handle (priv->channel, NULL);
  TpContactFeature features[] = { TP_CONTACT_FEATURE_ALIAS };
  if (contactHandle)
    tp_connection_get_contacts_by_handle (
      priv->connection, 1, &contactHandle, 1,
      features,
      mex_telepathy_channel_on_contact_fetched,
      self, NULL, NULL);

  pipeline = gst_pipeline_new (NULL);

  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);

  if (ret == GST_STATE_CHANGE_FAILURE)
    {
      tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
      g_object_unref (pipeline);
      MEX_WARNING ("Failed to start an empty pipeline !?");
      return;
    }

  priv->pipeline = pipeline;

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  priv->buswatch = gst_bus_add_watch (bus, mex_telepathy_channel_on_bus_watch,
                                      self);
  g_object_unref (bus);

  tf_channel_new_async (priv->channel, mex_telepathy_channel_new_tf_channel,
                        self);

  tpy_cli_channel_type_call_call_accept (TP_PROXY (priv->channel), -1,
                                         NULL, NULL, NULL, NULL);

  priv->channel = g_object_ref (priv->channel);
  g_signal_connect (priv->channel, "invalidated",
                    G_CALLBACK (mex_telepathy_channel_on_proxy_invalidated),
                    self);

  g_signal_connect (TPY_CALL_CHANNEL (priv->channel), "state-changed",
                    G_CALLBACK (mex_telepathy_channel_on_call_state_changed),
                    self);
}

static void
mex_telepathy_channel_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (object);
  MexTelepathyChannelPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_object (value, priv->channel);
      break;
    case PROP_CONNECTION:
      g_value_set_object (value, priv->connection);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_telepathy_channel_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (object);
  MexTelepathyChannelPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_CHANNEL:
      priv->channel = TP_CHANNEL (g_value_get_object (value));
      mex_telepathy_channel_initialize_channel(self);
      break;

    case PROP_CONNECTION:
      priv->connection = TP_CONNECTION (g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_telepathy_channel_class_init (MexTelepathyChannelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = mex_telepathy_channel_get_property;
  object_class->set_property = mex_telepathy_channel_set_property;
  object_class->dispose = mex_telepathy_channel_dispose;
  object_class->finalize = mex_telepathy_channel_finalize;

  g_type_class_add_private (klass, sizeof (MexTelepathyChannelPrivate));

  /* log domain */
  MEX_LOG_DOMAIN_INIT (telepathy_channel_log_domain, "telepathy-channel");

  // Signals
  mex_telepathy_channel_signals[SHOW_ACTOR] =
    g_signal_new ("show-actor",
                  MEX_TYPE_TELEPATHY_CHANNEL,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE,
                  1,
                  CLUTTER_TYPE_ACTOR);
  mex_telepathy_channel_signals[HIDE_ACTOR] =
    g_signal_new ("hide-actor",
                  MEX_TYPE_TELEPATHY_CHANNEL,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE,
                  1,
                  CLUTTER_TYPE_ACTOR);

  // Properties
  GParamSpec *pspec;
  pspec = g_param_spec_object ("channel",
                               "Channel",
                               "Telepathy Channel Object",
                               TP_TYPE_CHANNEL,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CHANNEL, pspec);

  pspec = g_param_spec_object ("connection",
                               "Connection",
                               "Telepathy Connection Object",
                               TP_TYPE_CONNECTION,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CONNECTION, pspec);
}

static void
mex_telepathy_channel_init (MexTelepathyChannel *self)
{
  self->priv = TELEPATHY_CHANNEL_PRIVATE (self);
  self->priv->connection = NULL;
  self->priv->channel = NULL;
  self->priv->tf_channel = NULL;
  mex_telepathy_channel_create_video_page (self);
}

MexTelepathyChannel *
mex_telepathy_channel_new (void)
{
  return g_object_new (MEX_TYPE_TELEPATHY_CHANNEL, NULL);
}

G_MODULE_EXPORT const GType
mex_get_channel_type (void)
{
  return MEX_TYPE_TELEPATHY_CHANNEL;
}
