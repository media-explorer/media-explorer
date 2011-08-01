/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Collabora Ltd.
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

G_DEFINE_TYPE(MexTelepathyChannel, mex_telepathy_channel, G_TYPE_OBJECT)

#define TELEPATHY_CHANNEL_PRIVATE(o)                                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TELEPATHY_CHANNEL,         \
                                MexTelepathyChannelPrivate))

struct _MexTelepathyChannelPrivate {
    ClutterActor *video_call_page;
    ClutterActor *video_incoming;
    ClutterActor *video_outgoing;
    ClutterActor *title_label;
    ClutterActor *hold_button;
    ClutterActor *duration_label;
    GstElement *incoming_sink;
    GstElement *outgoing_sink;
    gboolean sending_video;

    GstElement *pipeline;
    guint buswatch;
    TpConnection *connection;
    TpChannel *channel;
    TfChannel *tf_channel;
    GList *notifiers;
    GTimer *timer;

    GstElement *video_input;
    GstElement *video_capsfilter;

    guint width;
    guint height;
    guint framerate;
};

enum {
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
mex_telepathy_channel_finalize (GObject *gobject)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (gobject);
    MexTelepathyChannelPrivate *priv = self->priv;
    G_OBJECT_CLASS (mex_telepathy_channel_parent_class)->finalize (gobject);
}
static void on_video_closed(ClutterActor *actor,
                            gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    tp_channel_close_async (self->priv->channel, NULL, NULL);
}

void on_hangup(MxAction *action, gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    tp_channel_close_async (self->priv->channel, NULL, NULL);
}

void on_hold(MxAction *action, gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    MexTelepathyChannelPrivate *priv = self->priv;
    TpyCallChannel * channel = TPY_CALL_CHANNEL(self->priv->channel);

    tpy_call_channel_send_video(channel, !priv->sending_video);
    priv->sending_video = !priv->sending_video;

    if (priv->sending_video) {
        // We are starting, so change the button to pause.
        mx_stylable_set_style_class (MX_STYLABLE (priv->hold_button), "MediaPause");
    } else {
        // We are stopping, so change the button to play.
        mx_stylable_set_style_class (MX_STYLABLE (priv->hold_button), "MediaPlay");
    }
}

void create_video_page(MexTelepathyChannel *self)
{
    g_debug("video page creating");
    MexTelepathyChannelPrivate *priv = MEX_TELEPATHY_CHANNEL(self)->priv;

    // VideoCall widget init
    priv->video_call_page = mx_frame_new();
    clutter_actor_set_name(priv->video_call_page, "videocall-page");

    // Create the titlebar with a fixed height
    ClutterActor *titlebar = mx_box_layout_new();
    clutter_actor_set_height(titlebar, 48);
    mx_stylable_set_style_class (MX_STYLABLE (titlebar), "MexMediaControlsTitle");
    mx_box_layout_set_spacing( MX_BOX_LAYOUT(titlebar), 16);

    priv->title_label = mx_label_new();
    mx_label_set_x_align(MX_LABEL(priv->title_label), MX_ALIGN_MIDDLE);
    mx_label_set_y_align(MX_LABEL(priv->title_label), MX_ALIGN_MIDDLE);
    clutter_container_add(CLUTTER_CONTAINER(titlebar), priv->title_label, NULL);
    mx_box_layout_child_set_x_align( MX_BOX_LAYOUT(titlebar), priv->title_label, MX_ALIGN_MIDDLE);
    mx_box_layout_child_set_expand( MX_BOX_LAYOUT(titlebar), priv->title_label, TRUE);
    mx_box_layout_child_set_x_fill( MX_BOX_LAYOUT(titlebar), priv->title_label, FALSE);

    // Create the horizontal video container to hold the two sinks.
    ClutterActor *videocontainer = mx_box_layout_new();
    priv->video_outgoing = clutter_texture_new();
    priv->video_incoming = clutter_texture_new();
    clutter_container_add(CLUTTER_CONTAINER(videocontainer), priv->video_outgoing, priv->video_incoming, NULL);
    priv->outgoing_sink = clutter_gst_video_sink_new(CLUTTER_TEXTURE(priv->video_outgoing));
    priv->incoming_sink = clutter_gst_video_sink_new(CLUTTER_TEXTURE(priv->video_incoming));

    // Create the toolbar with a fixed height
    ClutterActor *toolbar = mx_box_layout_new();
    clutter_actor_set_height(toolbar, 48);

    mx_stylable_set_style_class (MX_STYLABLE (toolbar), "MexMediaControlsTitle");
    mx_box_layout_set_spacing( MX_BOX_LAYOUT(toolbar), 16);

    MxAction *end_action = mx_action_new_full("End", "End Call", (GCallback)on_hangup, self);
    ClutterActor *end_button = mx_button_new();
    mx_button_set_action(MX_BUTTON(end_button), end_action);
    mx_stylable_set_style_class (MX_STYLABLE (end_button), "MediaStop");

    MxAction *hold_action = mx_action_new_full("Hold", "Hold Call", (GCallback)on_hold, self);
    priv->hold_button = mx_button_new();
    mx_button_set_action(MX_BUTTON(priv->hold_button), hold_action);
    mx_stylable_set_style_class (MX_STYLABLE (priv->hold_button), "MediaPause");

    priv->duration_label = mx_label_new_with_text("Duration - 0:00");
    mx_label_set_x_align(MX_LABEL(priv->duration_label), MX_ALIGN_MIDDLE);
    mx_label_set_y_align(MX_LABEL(priv->duration_label), MX_ALIGN_MIDDLE);

    // Put the buttons in the toolbar
    clutter_container_add(CLUTTER_CONTAINER(toolbar), end_button, priv->hold_button, priv->duration_label, NULL);
    // Align button to end so it will be centered
    mx_box_layout_child_set_x_align( MX_BOX_LAYOUT(toolbar), end_button, MX_ALIGN_END);
    mx_box_layout_child_set_expand( MX_BOX_LAYOUT(toolbar), end_button, TRUE);
    mx_box_layout_child_set_x_fill( MX_BOX_LAYOUT(toolbar), end_button, FALSE);
    mx_box_layout_child_set_y_fill( MX_BOX_LAYOUT(toolbar), end_button, FALSE);

    mx_box_layout_child_set_x_fill( MX_BOX_LAYOUT(toolbar), priv->hold_button, FALSE);
    mx_box_layout_child_set_y_fill( MX_BOX_LAYOUT(toolbar), priv->hold_button, FALSE);

    // Align label to start so it will be centered.
    mx_box_layout_child_set_x_align( MX_BOX_LAYOUT(toolbar), priv->duration_label, MX_ALIGN_START);
    mx_box_layout_child_set_expand( MX_BOX_LAYOUT(toolbar), priv->duration_label, TRUE);
    mx_box_layout_child_set_x_fill( MX_BOX_LAYOUT(toolbar), priv->duration_label, FALSE);

    // Create the vertical container to put video above the toolbar.
    ClutterActor *vertical = mx_box_layout_new();
    mx_box_layout_set_orientation(MX_BOX_LAYOUT(vertical), MX_ORIENTATION_VERTICAL);
    clutter_container_add(CLUTTER_CONTAINER(vertical), titlebar, videocontainer, toolbar, NULL);

    // Finally put the vertical container in the frame.
    clutter_container_add(CLUTTER_CONTAINER(priv->video_call_page), vertical, NULL);

    // Connect to hide signals.
    g_signal_connect(priv->video_call_page,
                     "hide",
                     G_CALLBACK(on_video_closed),
                     self);
}

static gboolean
update_duration_label(gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);

    // Get the elapsed time
    guint elapsed = g_timer_elapsed(self->priv->timer, NULL);
    guint minutes = elapsed / 60;
    guint seconds = elapsed % 60 % 10;
    gchar *text;
    text = g_strdup_printf("Duration - %d:%02d", minutes, seconds);

    // Update the label
    mx_label_set_text( MX_LABEL(self->priv->duration_label), text);

    return TRUE;
}

static gboolean
bus_watch_cb (GstBus *bus,
              GstMessage *message,
              gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    MexTelepathyChannelPrivate *priv = self->priv;

    if (priv->tf_channel != NULL)
        tf_channel_bus_message (priv->tf_channel, message);

    if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ERROR)
    {
        GError *error = NULL;
        gchar *debug = NULL;
        gst_message_parse_error (message, &error, &debug);
        g_printerr ("ERROR from element %s: %s\n",
                    GST_OBJECT_NAME (message->src), error->message);
        g_printerr ("Debugging info: %s\n", (debug) ? debug : "none");
        g_error_free (error);
        g_free (debug);
    }
    return TRUE;
}

static void
src_pad_added_cb (TfContent *content,
                  TpHandle handle,
                  FsStream *stream,
                  GstPad *pad,
                  FsCodec *codec,
                  gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
    MexTelepathyChannelPrivate *priv = self->priv;

    gchar *cstr = fs_codec_to_string (codec);
    FsMediaType mtype;
    GstPad *sinkpad;
    GstElement *element;
    GstStateChangeReturn ret;

    g_debug ("New src pad: %s", cstr);
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
            g_warning ("Unknown media type");
            return;
    }

    gst_bin_add (GST_BIN (priv->pipeline), element);
    sinkpad = gst_element_get_pad (element, "sink");
    ret = gst_element_set_state (element, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
        g_warning ("Failed to start sink pipeline !?");
        return;
    }

    if (GST_PAD_LINK_FAILED (gst_pad_link (pad, sinkpad)))
    {
        tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
        g_warning ("Couldn't link sink pipeline !?");
        return;
    }

    g_object_unref (sinkpad);
}

static void
update_video_parameters (MexTelepathyChannel *self, gboolean restart)
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
on_video_framerate_changed (TfContent *content,
                            GParamSpec *spec,
                            MexTelepathyChannel *self)
{
    MexTelepathyChannelPrivate *priv = self->priv;

    guint framerate;

    g_object_get (content, "framerate", &framerate, NULL);

    if (framerate != 0)
        priv->framerate = framerate;

    update_video_parameters (self, FALSE);
}

static void
on_video_resolution_changed (TfContent *content,
                             guint width,
                             guint height,
                             MexTelepathyChannel *self)
{
    MexTelepathyChannelPrivate *priv = self->priv;
    g_assert (width > 0 && height > 0);

    priv->width = width;
    priv->height = height;

    update_video_parameters (self, TRUE);
}

static gboolean on_video_start_sending(TfContent *content,
                                   gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    MexTelepathyChannelPrivate *priv = self->priv;
    priv->sending_video = TRUE;

    return TRUE;
}

static GstElement *
setup_video_source (MexTelepathyChannel *self, TfContent *content)
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
    if (!tee) {
        g_warning("Couldn't create tee element !?");
        return NULL;
    }
    GstPad *teesink = gst_element_get_pad(tee, "sink");
    gst_bin_add (GST_BIN(result), tee);
    if (GST_PAD_LINK_FAILED (gst_pad_link (pad, teesink)))
    {
        g_warning ("Couldn't link source pipeline to tee !?");
        return NULL;
    }
    pad = gst_element_get_request_pad (tee, "src%d");

    // Link the tee to the preview widget.
    GstPad *teesrc = gst_element_get_request_pad(tee, "src%d");
    GstPad *outsink = gst_element_get_pad (self->priv->outgoing_sink, "sink");
    gst_bin_add (GST_BIN(result), self->priv->outgoing_sink);
    gst_pad_link(teesrc, outsink);

    ghost = gst_ghost_pad_new ("src", pad);
    gst_element_add_pad (result, ghost);

    g_object_unref (pad);

    priv->video_input = result;
    priv->video_capsfilter = capsfilter;

    g_signal_connect (content, "notify::framerate",
                      G_CALLBACK (on_video_framerate_changed),
                      self);

    g_signal_connect (content, "resolution-changed",
                      G_CALLBACK (on_video_resolution_changed),
                      self);

    g_signal_connect (content, "start-sending",
                      G_CALLBACK (on_video_start_sending),
                      self);

    return result;
}

static void
content_added_cb (TfChannel *channel,
                  TfContent *content,
                  gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL (user_data);
    MexTelepathyChannelPrivate *priv = self->priv;
    GstPad *srcpad, *sinkpad;
    FsMediaType mtype;
    GstElement *element;
    GstStateChangeReturn ret;

    g_debug ("Content added");

    g_object_get (content,
                  "sink-pad", &sinkpad,
                  "media-type", &mtype,
                  NULL);

    switch (mtype)
    {
        case FS_MEDIA_TYPE_AUDIO:
            g_debug ("Audio content added");
            element = gst_parse_bin_from_description (
                          "audiotestsrc is-live=1 ! audio/x-raw-int,rate=8000 ! queue"
                          " ! audioconvert ! audioresample ! audioconvert ",

                          TRUE, NULL);
            break;
        case FS_MEDIA_TYPE_VIDEO:
            g_debug ("Video content added");
            element = setup_video_source (self, content);
            break;
        default:
            g_warning ("Unknown media type");
            goto out;
    }

    g_signal_connect (content, "src-pad-added",
                      G_CALLBACK (src_pad_added_cb), self);

    gst_bin_add (GST_BIN (priv->pipeline), element);
    srcpad = gst_element_get_pad (element, "src");

    if (GST_PAD_LINK_FAILED (gst_pad_link (srcpad, sinkpad)))
    {
        tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
        g_warning ("Couldn't link source pipeline !?");
        return;
    }

    ret = gst_element_set_state (element, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
        g_warning ("source pipeline failed to start!?");
        return;
    }

    g_object_unref (srcpad);
out:
    g_object_unref (sinkpad);
}

static void
conference_added_cb (TfChannel *channel,
                     GstElement *conference,
                     gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    MexTelepathyChannelPrivate *priv = self->priv;

    GKeyFile *keyfile;

    g_debug ("Conference added");

    /* Add notifier to set the various element properties as needed */
    keyfile = fs_utils_get_default_element_properties (conference);
    if (keyfile != NULL)
    {
        FsElementAddedNotifier *notifier;
        g_debug ("Loaded default codecs for %s", GST_ELEMENT_NAME (conference));

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
                  g_object_ref(priv->video_call_page));
}

static gboolean
dump_pipeline_cb (gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);

    if (self->priv->pipeline) {
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN(self->priv->pipeline),
                                           GST_DEBUG_GRAPH_SHOW_ALL,
                                           "call-handler");

        return TRUE;
    }

    return FALSE;
}

static void
new_tf_channel_cb (GObject *source,
                   GAsyncResult *result,
                   gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    MexTelepathyChannelPrivate *priv = self->priv;

    g_debug ("New TfChannel");

    priv->tf_channel = TF_CHANNEL (g_async_initable_new_finish (
                                   G_ASYNC_INITABLE (source), result, NULL));


    if (priv->tf_channel == NULL)
    {
        g_warning ("Failed to create channel");
        return;
    }

    g_debug ("Adding timeout");
    g_timeout_add_seconds (5, dump_pipeline_cb, self);
    g_timeout_add_seconds (1, update_duration_label, self);

    g_signal_connect (priv->tf_channel, "fs-conference-added",
                      G_CALLBACK (conference_added_cb), self);

    g_signal_connect (priv->tf_channel, "content-added",
                      G_CALLBACK (content_added_cb), self);
}

static void
on_call_state_changed_cb (TpyCallChannel *channel,
                          TpyCallState state,
                          TpyCallFlags flags,
                          GValueArray *state_reason,
                          GHashTable *state_details,
                          gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    MexTelepathyChannelPrivate *priv = self->priv;

    if (state == TPY_CALL_STATE_ENDED) {
        tp_channel_close_async (self->priv->channel, NULL, NULL);
    }
}

static void
proxy_invalidated_cb (TpProxy *proxy,
                      guint domain,
                      gint code,
                      gchar *message,
                      gpointer user_data)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);
    MexTelepathyChannelPrivate *priv = self->priv;

    g_debug ("Channel closed");
    if (priv->pipeline != NULL)
    {
        gst_element_set_state (priv->pipeline, GST_STATE_NULL);
        g_object_unref (priv->pipeline);
        priv->pipeline = NULL;
    }

    if (priv->tf_channel != NULL)
        g_object_unref (priv->tf_channel);

    g_list_foreach (priv->notifiers, (GFunc) g_object_unref, NULL);
    g_list_free (priv->notifiers);

    g_object_unref (priv->channel);

    g_timer_stop(self->priv->timer);
    g_signal_emit(self,
                  mex_telepathy_channel_signals[HIDE_ACTOR],
                  0,
                  g_object_ref(priv->video_call_page));
}

static void on_contact_fetched(TpConnection *connection,
                               guint n_contacts,
                               TpContact * const *contacts,
                               guint n_failed,
                               const TpHandle *failed,
                               const GError *error,
                               gpointer user_data,
                               GObject *weak_object)
{
    MexTelepathyChannel *self = MEX_TELEPATHY_CHANNEL(user_data);

    int i = 0;
    for (i = 0; i < n_contacts; ++i) {
        // Get the contacts.
        TpContact *current = contacts[i];

        // Connect to alias change signal.
        // Add the alias to the label.
        mx_label_set_text( MX_LABEL(self->priv->title_label), tp_contact_get_alias(current));
    }
}

void
initialize_channel (MexTelepathyChannel *self)
{
    MexTelepathyChannelPrivate *priv = self->priv;

    if (!priv->video_call_page)
        create_video_page(self);

    GstBus *bus;
    GstElement *pipeline;
    GstStateChangeReturn ret;

    g_debug ("New channel");

    TpHandle contactHandle = tp_channel_get_handle(priv->channel, NULL);
    TpContactFeature features[] = {TP_CONTACT_FEATURE_ALIAS};
    if (contactHandle)
        tp_connection_get_contacts_by_handle(priv->connection, 1, &contactHandle, 1,
                                         features, on_contact_fetched,
                                         self, NULL, NULL);

    pipeline = gst_pipeline_new (NULL);

    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);

    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        tp_channel_close_async (TP_CHANNEL (priv->channel), NULL, NULL);
        g_object_unref (pipeline);
        g_warning ("Failed to start an empty pipeline !?");
        return;
    }

    priv->pipeline = pipeline;

    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    priv->buswatch = gst_bus_add_watch (bus, bus_watch_cb, self);
    g_object_unref (bus);

    tf_channel_new_async (priv->channel, new_tf_channel_cb, self);

    tpy_cli_channel_type_call_call_accept (TP_PROXY(priv->channel), -1,
            NULL, NULL, NULL, NULL);

    priv->channel = g_object_ref (priv->channel);
    g_signal_connect (priv->channel, "invalidated",
                      G_CALLBACK (proxy_invalidated_cb),
                      self);

    g_signal_connect (TPY_CALL_CHANNEL(priv->channel), "state-changed",
                      G_CALLBACK (on_call_state_changed_cb),
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
            if (value != NULL)
                priv->channel = TP_CHANNEL(g_value_get_pointer (value) );
            initialize_channel(self);
            break;

        case PROP_CONNECTION:
            if (value != NULL)
                priv->connection = TP_CONNECTION( g_value_get_pointer (value) );
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
    object_class->finalize = mex_telepathy_channel_finalize;

    g_type_class_add_private (klass, sizeof (MexTelepathyChannelPrivate));

    // Signals
    mex_telepathy_channel_signals[SHOW_ACTOR] =
        g_signal_new("show-actor",
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
        g_signal_new("hide-actor",
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
    pspec = g_param_spec_pointer ("channel",
                                  "Channel",
                                  "Telepathy Channel Object",
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_CHANNEL, pspec);

    pspec = g_param_spec_pointer ("connection",
                                  "Connection",
                                  "Telepathy Connection Object",
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_CONNECTION, pspec);
}

static void
mex_telepathy_channel_init (MexTelepathyChannel  *self)
{
    self->priv = TELEPATHY_CHANNEL_PRIVATE(self);
    self->priv->connection = NULL;
    self->priv->channel = NULL;
    self->priv->tf_channel = NULL;
    self->priv->timer = g_timer_new();
    create_video_page(self);
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
