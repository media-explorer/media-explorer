/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#include "mex-telepathy-plugin.h"

#include "mex-contact.h"

#include <gst/gst.h>
#include <telepathy-glib/account.h>
#include <telepathy-glib/account-channel-request.h>
#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/connection-contact-list.h>
#include <telepathy-glib/contact.h>
#include <telepathy-glib/contact-operations.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/simple-client-factory.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/telepathy-glib.h>
#include <gst/farsight/fs-element-added-notifier.h>
#include <gst/farsight/fs-utils.h>
#include <telepathy-farstream/telepathy-farstream.h>
#include <telepathy-yell/telepathy-yell.h>
#include <clutter-gst/clutter-gst.h>

#include <telepathy-yell/interfaces.h>

#include <glib/gi18n.h>

typedef struct {
    GstElement *pipeline;
    guint buswatch;
    TpChannel *proxy;
    TfChannel *channel;
    GList *notifiers;

    GstElement *video_input;
    GstElement *video_capsfilter;

    guint width;
    guint height;
    guint framerate;
} ChannelContext;

static const gchar *audio_contact_mimetypes[] = { "x-mex-audio-contact", "x-mex-av-contact", NULL };
static const gchar *av_contact_mimetypes[] = { "x-mex-av-contact", NULL };
static const gchar *pending_contact_mimetypes[] = { "x-mex-pending-contact", NULL };

static void mex_tool_provider_iface_init (MexToolProviderInterface *iface);
static void model_provider_iface_init (MexModelProviderInterface *iface);
static void action_provider_iface_init (MexActionProviderInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MexTelepathyPlugin,
                         mex_telepathy_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_TOOL_PROVIDER,
                                                mex_tool_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                action_provider_iface_init))

#define TELEPATHY_PLUGIN_PRIVATE(o)                                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TELEPATHY_PLUGIN,         \
                                MexTelepathyPluginPrivate))

struct _MexTelepathyPluginPrivate {
  MexModelManager *manager;
  MexFeed *feed;

  GList *models;
  GList *actions;
  GList *contacts;

  ClutterActor *video_call_page;
  ClutterActor *video_incoming;
  ClutterActor *video_outgoing;
  GstElement *incoming_sink;
  GstElement *outgoing_sink;

  ChannelContext *current_context;

  TpAccountManager *account_manager;
};

static void
remove_model (gpointer key, gpointer value, gpointer user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexModel *model = MEX_MODEL (value);

  mex_model_manager_remove_model (self->priv->manager, model);
}

static void
mex_telepathy_plugin_dispose (GObject *gobject)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (gobject);
    MexTelepathyPluginPrivate *priv = self->priv;

    while (priv->models) {
        mex_model_info_free (priv->models->data);
        priv->models = g_list_delete_link (priv->models, priv->models);
    }

    while (priv->actions) {
        MexActionInfo *info = priv->actions->data;

        g_object_unref (info->action);
        g_strfreev (info->mime_types);
        g_free (info);

        priv->actions = g_list_delete_link (priv->actions, priv->actions);
    }

    g_object_unref(priv->account_manager);

    G_OBJECT_CLASS (mex_telepathy_plugin_parent_class)->dispose (gobject);
}

static void
mex_telepathy_plugin_finalize (GObject *object)
{
    G_OBJECT_CLASS (mex_telepathy_plugin_parent_class)->finalize (object);
}

static void
mex_telepathy_plugin_class_init (MexTelepathyPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = mex_telepathy_plugin_finalize;

  g_type_class_add_private (klass, sizeof (MexTelepathyPluginPrivate));
}

static gint
mex_telepathy_plugin_compare_mex_contact(gconstpointer a,
                                         gconstpointer b)
{
    MexContact *mex_contact = MEX_CONTACT (a);
    TpContact *contact_b = TP_CONTACT(b);
    TpContact *contact_a = mex_contact_get_tp_contact(mex_contact);

    return tp_strdiff(tp_contact_get_identifier(contact_a), tp_contact_get_identifier(contact_b));
}

static void
mex_telepathy_plugin_on_channel_created (GObject *source,
                                         GAsyncResult *result,
                                         gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    TpChannel *channel;
    GError *error = NULL;

    channel = tp_account_channel_request_create_and_handle_channel_finish (
        TP_ACCOUNT_CHANNEL_REQUEST (source), result, NULL, &error);
    if (channel == NULL) {
        g_debug ("Failed to create channel: %s", error->message);

        g_error_free (error);
        return;
    }

    g_debug ("Channel created: %s", tp_proxy_get_object_path (channel));
}

static void
mex_telepathy_plugin_on_request_subscription(GObject *source_object,
                                             GAsyncResult *res,
                                             gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    TpContact *contact = TP_CONTACT (source_object);

    GError *error = NULL;

    if (!tp_contact_request_subscription_finish (contact, res, &error)) {
        g_print ("Error subscribing to contact: %s\n", error->message);
        g_object_unref(error);
        // TODO: Maybe show stuff in the UI here?
    }
}

static void
mex_telepathy_plugin_on_authorize_publication(GObject *source_object,
                                              GAsyncResult *res,
                                              gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    TpContact *contact = TP_CONTACT (source_object);

    GError *error = NULL;

    if (!tp_contact_authorize_publication_finish (contact, res, &error)) {
        g_print ("Error authorizing contact: %s\n", error->message);
        g_object_unref(error);
        // TODO: Maybe show stuff in the UI here?
        return;
    }

    // If we are not subscribed to the contact, do it at this point
    if (tp_contact_get_subscribe_state (contact) == TP_SUBSCRIPTION_STATE_NO) {
        tp_contact_request_subscription_async(contact,
                                              _("Please allow me to see your presence"),
                                              mex_telepathy_plugin_on_request_subscription,
                                              self);
    }
}

static void
mex_telepathy_plugin_on_accept_contact (MxAction *action,
                                        gpointer  user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    MexContent *content = mex_action_get_content (action);
    MexContact *mex_contact = MEX_CONTACT (content);
    TpContact *contact = mex_contact_get_tp_contact (mex_contact);

    tp_contact_authorize_publication_async(contact,
                                           mex_telepathy_plugin_on_authorize_publication,
                                           self);
}

static void
mex_telepathy_plugin_craft_channel_request (MexTelepathyPlugin *self,
                                            TpAccount *account,
                                            TpContact *contact,
                                            gboolean audio,
                                            gboolean video)
{
    TpAccountChannelRequest *req;
    GHashTable *request;
    request = tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE,
        G_TYPE_STRING,
        TPY_IFACE_CHANNEL_TYPE_CALL,

        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
        G_TYPE_UINT,
        TP_HANDLE_TYPE_CONTACT,

        TP_PROP_CHANNEL_TARGET_ID,
        G_TYPE_STRING,
        tp_contact_get_identifier(contact),

        TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO,
        G_TYPE_BOOLEAN,
        audio,

        TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO,
        G_TYPE_BOOLEAN,
        video,

        NULL);

    g_debug ("Offer video channel to %s", tp_contact_get_identifier(contact));

    req = tp_account_channel_request_new (account, request,
        TP_USER_ACTION_TIME_CURRENT_TIME);

    tp_account_channel_request_create_and_handle_channel_async (req,
                                                                NULL,
                                                                mex_telepathy_plugin_on_channel_created,
                                                                self);

    g_hash_table_unref (request);
}

static void
mex_telepathy_plugin_on_start_video_call (MxAction *action,
                                          gpointer  user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    MexContent *content = mex_action_get_content (action);
    MexContact *mex_contact = MEX_CONTACT (content);
    TpContact *contact = mex_contact_get_tp_contact (mex_contact);
    TpAccount *account = tp_connection_get_account (
                            tp_contact_get_connection (contact));

    mex_telepathy_plugin_craft_channel_request (self,
                                                account,
                                                contact,
                                                TRUE,
                                                TRUE);
}

static void
mex_telepathy_plugin_on_start_audio_call (MxAction *action,
                                          gpointer  user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    MexContent *content = mex_action_get_content (action);
    MexContact *mex_contact = MEX_CONTACT (content);
    TpContact *contact = mex_contact_get_tp_contact (mex_contact);
    TpAccount *account = tp_connection_get_account (
                            tp_contact_get_connection (contact));

    mex_telepathy_plugin_craft_channel_request (self,
                                                account,
                                                contact,
                                                TRUE,
                                                FALSE);
}

static void
mex_telepathy_plugin_on_should_add_to_model_changed (gpointer instance,
                                                     gboolean should_add,
                                                     gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    MexContact *contact = MEX_CONTACT(instance);

    if (should_add) {
        mex_model_add_content(MEX_MODEL(priv->feed), MEX_CONTENT(contact));
    } else {
        mex_model_remove_content(MEX_MODEL(priv->feed), MEX_CONTENT(contact));
    }
}

static void mex_telepathy_plugin_add_contact(gpointer contact_ptr, gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    TpContact *contact = TP_CONTACT(contact_ptr);
    g_debug("Adding %s", tp_contact_get_alias(contact));

    MexContact *mex_contact;

    mex_contact = g_object_new (MEX_TYPE_CONTACT,
                                "contact", contact,
                                NULL);

    priv->contacts = g_list_append (priv->contacts, mex_contact);

    if (mex_contact_should_add_to_model(mex_contact)) {
        mex_model_add_content(MEX_MODEL(priv->feed), MEX_CONTENT(mex_contact));
    }

    g_signal_connect(mex_contact,
                     "should-add-to-model-changed",
                     G_CALLBACK(mex_telepathy_plugin_on_should_add_to_model_changed),
                     self);
}

static void mex_telepathy_plugin_remove_contact(gpointer contact_ptr, gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    TpContact *contact = TP_CONTACT(contact_ptr);
    g_debug("Removing %s", tp_contact_get_alias(contact));

    GList *found = g_list_find_custom(priv->contacts, contact, mex_telepathy_plugin_compare_mex_contact);
    if (found == NULL) {
        g_warning ("Could not find contact %s!", tp_contact_get_identifier(contact));
        return;
    }

    gpointer found_element = found->data;

    mex_model_remove_content(MEX_MODEL(priv->feed), MEX_CONTENT(found_element));
    priv->contacts = g_list_remove(priv->contacts, found_element);

    g_debug ("Contact %s removed successfully.", tp_contact_get_identifier(
                                                    mex_contact_get_tp_contact(found_element)));

    g_object_unref (found_element);
}

static void mex_telepathy_plugin_on_contact_list_changed(TpConnection *connection,
                                                         GPtrArray    *added,
                                                         GPtrArray    *removed,
                                                         gpointer      user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    g_ptr_array_foreach(added, mex_telepathy_plugin_add_contact, self);
    g_ptr_array_foreach(removed, mex_telepathy_plugin_remove_contact, self);

    g_ptr_array_unref(added);
    g_ptr_array_unref(removed);
}

static void mex_telepathy_plugin_on_connection_ready(TpConnection *connection,
                                                     const GError *error,
                                                     gpointer user_data)
{
    g_debug("Connection ready!");
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    if (error != NULL) {
        g_warning("%s", error->message);
        return;
    }

    GPtrArray *contacts = tp_connection_dup_contact_list(connection);

    g_ptr_array_foreach(contacts, mex_telepathy_plugin_add_contact, self);

    g_ptr_array_unref(contacts);

    g_signal_connect(connection,
                     "contact-list-changed",
                     G_CALLBACK(mex_telepathy_plugin_on_contact_list_changed),
                     self);
}

void mex_telepathy_plugin_on_account_status_changed(TpAccount  *account,
                                                    guint       old_status,
                                                    guint       new_status,
                                                    guint       reason,
                                                    gchar      *dbus_error_name,
                                                    GHashTable *details,
                                                    gpointer    user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    switch (new_status) {
        case TP_CONNECTION_STATUS_CONNECTED:
            if (old_status != TP_CONNECTION_STATUS_CONNECTED) {
                g_debug("Account got connected!");
                tp_connection_call_when_ready(tp_account_get_connection(account),
                                              mex_telepathy_plugin_on_connection_ready,
                                              self);
            }
            break;
        default:
            if (old_status == TP_CONNECTION_STATUS_CONNECTED) {
                g_warning("Account got disconnected! %s", dbus_error_name);
                // TODO: Maybe give more info?
            }
    }
}

static void
mex_tool_provider_iface_init (MexToolProviderInterface *iface)
{
}

void mex_telepathy_plugin_on_presence_request_finished(GObject *source_object,
                                                       GAsyncResult *res,
                                                       gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;
    TpAccount *account = TP_ACCOUNT(source_object);

    gboolean success = FALSE;

    success = tp_account_request_presence_finish(account, res, NULL);

    if (!success) {
        // TODO Handle error
        g_warning("Fail in setting a different presence!");
        return;
    }
}

void create_video_page(MexTelepathyPlugin *self)
{
    MexTelepathyPluginPrivate *priv = MEX_TELEPATHY_PLUGIN(self)->priv;

    // VideoCall widget init
    priv->video_call_page = mx_frame_new();
    clutter_actor_set_name(priv->video_call_page, "videocall-page");

    // Create the titlebar with a fixed height
    ClutterActor *titlebar = mx_box_layout_new();
    clutter_actor_set_height(titlebar, 48);
    mx_stylable_set_style_class (MX_STYLABLE (titlebar), "MexMediaControlsTitle");
    mx_box_layout_set_spacing( MX_BOX_LAYOUT(titlebar), 16);

    ClutterActor *title_label = mx_label_new_with_text("Call with ");
    mx_label_set_x_align(MX_LABEL(title_label), MX_ALIGN_MIDDLE);
    mx_label_set_y_align(MX_LABEL(title_label), MX_ALIGN_MIDDLE);
    clutter_container_add(CLUTTER_CONTAINER(titlebar), title_label, NULL);
    mx_box_layout_child_set_x_align( MX_BOX_LAYOUT(titlebar), title_label, MX_ALIGN_MIDDLE);
    mx_box_layout_child_set_expand( MX_BOX_LAYOUT(titlebar), title_label, TRUE);
    mx_box_layout_child_set_x_fill( MX_BOX_LAYOUT(titlebar), title_label, FALSE);

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

    ClutterActor *end_button = mx_button_new();
    mx_stylable_set_style_class (MX_STYLABLE (end_button), "MediaStop");

    ClutterActor *hold_button = mx_button_new();
    mx_stylable_set_style_class (MX_STYLABLE (hold_button), "MediaPause");

    ClutterActor *duration_label = mx_label_new_with_text("Duration - 0:00");
    mx_label_set_x_align(MX_LABEL(duration_label), MX_ALIGN_MIDDLE);
    mx_label_set_y_align(MX_LABEL(duration_label), MX_ALIGN_MIDDLE);

    // Put the buttons in the toolbar
    clutter_container_add(CLUTTER_CONTAINER(toolbar), end_button, hold_button, duration_label, NULL);
    // Align button to end so it will be centered
    mx_box_layout_child_set_x_align( MX_BOX_LAYOUT(toolbar), end_button, MX_ALIGN_END);
    mx_box_layout_child_set_expand( MX_BOX_LAYOUT(toolbar), end_button, TRUE);
    mx_box_layout_child_set_x_fill( MX_BOX_LAYOUT(toolbar), end_button, FALSE);
    mx_box_layout_child_set_y_fill( MX_BOX_LAYOUT(toolbar), end_button, FALSE);

    mx_box_layout_child_set_x_fill( MX_BOX_LAYOUT(toolbar), hold_button, FALSE);
    mx_box_layout_child_set_y_fill( MX_BOX_LAYOUT(toolbar), hold_button, FALSE);

    // Align label to start so it will be centered.
    mx_box_layout_child_set_x_align( MX_BOX_LAYOUT(toolbar), duration_label, MX_ALIGN_START);
    mx_box_layout_child_set_expand( MX_BOX_LAYOUT(toolbar), duration_label, TRUE);
    mx_box_layout_child_set_x_fill( MX_BOX_LAYOUT(toolbar), duration_label, FALSE);

    // Create the vertical container to put video above the toolbar.
    ClutterActor *vertical = mx_box_layout_new();
    mx_box_layout_set_orientation(MX_BOX_LAYOUT(vertical), MX_ORIENTATION_VERTICAL);
    clutter_container_add(CLUTTER_CONTAINER(vertical), titlebar, videocontainer, toolbar, NULL);

    // Finally put the vertical container in the frame.
    clutter_container_add(CLUTTER_CONTAINER(priv->video_call_page), vertical, NULL);
}

void mex_telepathy_plugin_on_account_manager_ready(GObject *source_object,
                                                   GAsyncResult *res,
                                                   gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    GError *error = NULL;

    if (!tp_proxy_prepare_finish (priv->account_manager, res, &error)) {
        g_print ("Error preparing AM: %s\n", error->message);
        g_object_unref(error);
        // TODO: Maybe show stuff in the UI here?
        return;
    }

    // Get the accounts
    GList *accounts;
    for (accounts = tp_account_manager_get_valid_accounts (priv->account_manager);
         accounts != NULL; accounts = g_list_delete_link (accounts, accounts)) {
        TpAccount *account = accounts->data;
        TpConnection *connection = tp_account_get_connection (account);
        GPtrArray *contacts;
        guint i;

        if (connection == NULL) {
            g_debug("Account is not connected, setting it back to autopresence");

            // Get the autopresence
            guint type;
            gchar *status;
            gchar *message;

            g_object_get(account,
                        "automatic-presence-type", &type,
                        "automatic-status", &status,
                        "automatic-status-message", &message,
                        NULL);

            tp_account_request_presence_async(account,
                                              type,
                                              status,
                                              message,
                                              mex_telepathy_plugin_on_presence_request_finished,
                                              self);

            g_free(status);
            g_free(message);
        } else {
            connection = tp_account_get_connection(account);
            tp_connection_call_when_ready(connection,
                                          mex_telepathy_plugin_on_connection_ready,
                                          self);
        }

        g_signal_connect(account,
                         "status-changed",
                         G_CALLBACK(mex_telepathy_plugin_on_account_status_changed),
                         self);
    }
}

static void
mex_telepathy_plugin_add_action (gchar              *action_id,
                                 gchar              *display_name,
                                 GCallback           callback,
                                 gchar              *icon,
                                 gchar             **mimetypes,
                                 gint                priority,
                                 MexTelepathyPlugin *self)
{
    MexTelepathyPluginPrivate *priv = self->priv;

    MexActionInfo *action_info;

    action_info = g_new0 (MexActionInfo, 1);
    action_info->action = mx_action_new_full (action_id,
                                              display_name,
                                              callback,
                                              self);
    mx_action_set_icon (action_info->action, icon);
    action_info->mime_types = mimetypes;
    action_info->priority = priority;
    priv->actions = g_list_append (priv->actions, action_info);
}

static gboolean
bus_watch_cb (GstBus *bus,
              GstMessage *message,
              gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN(user_data);
    ChannelContext *context = self->priv->current_context;

    if (context->channel != NULL)
        tf_channel_bus_message (context->channel, message);

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
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    ChannelContext *context = self->priv->current_context;
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

    gst_bin_add (GST_BIN (context->pipeline), element);
    sinkpad = gst_element_get_pad (element, "sink");
    ret = gst_element_set_state (element, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        tp_channel_close_async (TP_CHANNEL (context->proxy), NULL, NULL);
        g_warning ("Failed to start sink pipeline !?");
        return;
    }

    if (GST_PAD_LINK_FAILED (gst_pad_link (pad, sinkpad)))
    {
        tp_channel_close_async (TP_CHANNEL (context->proxy), NULL, NULL);
        g_warning ("Couldn't link sink pipeline !?");
        return;
    }

    g_object_unref (sinkpad);
}

static void
update_video_parameters (ChannelContext *context, gboolean restart)
{
    GstCaps *caps;
    GstClock *clock;

    if (restart)
    {
        /* Assuming the pipeline is in playing state */
        gst_element_set_locked_state (context->video_input, TRUE);
        gst_element_set_state (context->video_input, GST_STATE_NULL);
    }

    g_object_get (context->video_capsfilter, "caps", &caps, NULL);
    caps = gst_caps_make_writable (caps);

    gst_caps_set_simple (caps,
                         "framerate", GST_TYPE_FRACTION, context->framerate, 1,
                         "width", G_TYPE_INT, context->width,
                         "height", G_TYPE_INT, context->height,
                         NULL);

    g_object_set (context->video_capsfilter, "caps", caps, NULL);

    if (restart)
    {
        clock = gst_pipeline_get_clock (GST_PIPELINE (context->pipeline));
        /* Need to reset the clock if we set the pipeline back to ready by hand */
        if (clock != NULL)
        {
            gst_element_set_clock (context->video_input, clock);
            g_object_unref (clock);
        }

        gst_element_set_locked_state (context->video_input, FALSE);
        gst_element_sync_state_with_parent (context->video_input);
    }
}

static void
on_video_framerate_changed (TfContent *content,
                            GParamSpec *spec,
                            ChannelContext *context)
{
    guint framerate;

    g_object_get (content, "framerate", &framerate, NULL);

    if (framerate != 0)
        context->framerate = framerate;

    update_video_parameters (context, FALSE);
}

static void
on_video_resolution_changed (TfContent *content,
                             guint width,
                             guint height,
                             ChannelContext *context)
{
    g_assert (width > 0 && height > 0);

    context->width = width;
    context->height = height;

    update_video_parameters (context, TRUE);
}

static gboolean on_video_start_sending(TfContent *content,
                                   gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN(user_data);
    MexTelepathyPluginPrivate *priv = self->priv;
    if (!priv->video_call_page)
        create_video_page(self);
    mex_tool_provider_present_actor(MEX_TOOL_PROVIDER(self),
                                    g_object_ref(priv->video_call_page));

    return TRUE;
}

static GstElement *
setup_video_source (MexTelepathyPlugin *self, TfContent *content)
{
    ChannelContext *context = self->priv->current_context;
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

    context->framerate = framerate;
    context->width = width;
    context->height = height;

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
        return;
    }
    GstPad *teesink = gst_element_get_pad(tee, "sink");
    gst_bin_add (GST_BIN(result), tee);
    if (GST_PAD_LINK_FAILED (gst_pad_link (pad, teesink)))
    {
        g_warning ("Couldn't link source pipeline to tee !?");
        return;
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

    context->video_input = result;
    context->video_capsfilter = capsfilter;

    g_signal_connect (content, "notify::framerate",
                      G_CALLBACK (on_video_framerate_changed),
                      context);

    g_signal_connect (content, "resolution-changed",
                      G_CALLBACK (on_video_resolution_changed),
                      context);

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
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    GstPad *srcpad, *sinkpad;
    FsMediaType mtype;
    GstElement *element;
    GstStateChangeReturn ret;
    ChannelContext *context = self->priv->current_context;

    g_debug ("Content added");

    g_object_get (content,
                  "sink-pad", &sinkpad,
                  "media-type", &mtype,
                  NULL);

    switch (mtype)
    {
        case FS_MEDIA_TYPE_AUDIO:
            element = gst_parse_bin_from_description (
                          "audiotestsrc is-live=1 ! audio/x-raw-int,rate=8000 ! queue"
                          " ! audioconvert ! audioresample ! audioconvert ",

                          TRUE, NULL);
            break;
        case FS_MEDIA_TYPE_VIDEO:
            element = setup_video_source (self, content);
            break;
        default:
            g_warning ("Unknown media type");
            goto out;
    }

    g_signal_connect (content, "src-pad-added",
                      G_CALLBACK (src_pad_added_cb), self);

    gst_bin_add (GST_BIN (context->pipeline), element);
    srcpad = gst_element_get_pad (element, "src");

    if (GST_PAD_LINK_FAILED (gst_pad_link (srcpad, sinkpad)))
    {
        tp_channel_close_async (TP_CHANNEL (context->proxy), NULL, NULL);
        g_warning ("Couldn't link source pipeline !?");
        return;
    }

    ret = gst_element_set_state (element, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        tp_channel_close_async (TP_CHANNEL (context->proxy), NULL, NULL);
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
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN(user_data);
    ChannelContext *context = self->priv->current_context;
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
        fs_element_added_notifier_add (notifier, GST_BIN (context->pipeline));

        context->notifiers = g_list_prepend (context->notifiers, notifier);
    }


    gst_bin_add (GST_BIN (context->pipeline), conference);
    gst_element_set_state (conference, GST_STATE_PLAYING);
}

static gboolean
dump_pipeline_cb (gpointer data)
{
    ChannelContext *context = data;

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (context->pipeline),
                                       GST_DEBUG_GRAPH_SHOW_ALL,
                                       "call-handler");

    return TRUE;
}

static void
new_tf_channel_cb (GObject *source,
                   GAsyncResult *result,
                   gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN(user_data);
    ChannelContext *context = self->priv->current_context;

    g_debug ("New TfChannel");

    context->channel = TF_CHANNEL (g_async_initable_new_finish (
                                       G_ASYNC_INITABLE (source), result, NULL));


    if (context->channel == NULL)
    {
        g_warning ("Failed to create channel");
        return;
    }

    g_debug ("Adding timeout");
    g_timeout_add_seconds (5, dump_pipeline_cb, context);

    g_signal_connect (context->channel, "fs-conference-added",
                      G_CALLBACK (conference_added_cb), self);

    g_signal_connect (context->channel, "content-added",
                      G_CALLBACK (content_added_cb), self);
}

static void
proxy_invalidated_cb (TpProxy *proxy,
                      guint domain,
                      gint code,
                      gchar *message,
                      gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN(user_data);
    ChannelContext *context = self->priv->current_context;

    g_debug ("Channel closed");
    if (context->pipeline != NULL)
    {
        gst_element_set_state (context->pipeline, GST_STATE_NULL);
        g_object_unref (context->pipeline);
    }

    if (context->channel != NULL)
        g_object_unref (context->channel);

    g_list_foreach (context->notifiers, (GFunc) g_object_unref, NULL);
    g_list_free (context->notifiers);

    g_object_unref (context->proxy);

    g_slice_free (ChannelContext, context);

    //g_main_loop_quit (loop);
}

static void
new_call_channel_cb (TpSimpleHandler *handler,
                     TpAccount *account,
                     TpConnection *connection,
                     GList *channels,
                     GList *requests_satisfied,
                     gint64 user_action_time,
                     TpHandleChannelsContext *handler_context,
                     gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN(user_data);
    if (!self->priv->video_call_page)
        create_video_page(self);

    ChannelContext *context;
    TpChannel *proxy;
    GstBus *bus;
    GstElement *pipeline;
    GstStateChangeReturn ret;

    g_debug ("New channel");

    proxy = channels->data;

    pipeline = gst_pipeline_new (NULL);

    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);

    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        tp_channel_close_async (TP_CHANNEL (proxy), NULL, NULL);
        g_object_unref (pipeline);
        g_warning ("Failed to start an empty pipeline !?");
        return;
    }

    context = g_slice_new0 (ChannelContext);
    context->pipeline = pipeline;
    self->priv->current_context = context;

    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    context->buswatch = gst_bus_add_watch (bus, bus_watch_cb, self);
    g_object_unref (bus);

    tf_channel_new_async (proxy, new_tf_channel_cb, self);

    tp_handle_channels_context_accept (handler_context);

    tpy_cli_channel_type_call_call_accept (TP_PROXY(proxy), -1,
            NULL, NULL, NULL, NULL);

    context->proxy = g_object_ref (proxy);
    g_signal_connect (proxy, "invalidated",
                      G_CALLBACK (proxy_invalidated_cb),
                      self);
}

void handler_create(MexTelepathyPlugin *self)
{
    TpBaseClient *client;
    TpDBusDaemon *bus;

    tf_init ();

    bus = tp_dbus_daemon_dup (NULL);

    client = tp_simple_handler_new (bus,
                                    FALSE,
                                    FALSE,
                                    "TpMexPlugin",
                                    TRUE,
                                    new_call_channel_cb,
                                    self,
                                    NULL);

    tp_base_client_take_handler_filter (client,
                                        tp_asv_new (
                                            TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
                                            TPY_IFACE_CHANNEL_TYPE_CALL,
                                            TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT,
                                            TP_HANDLE_TYPE_CONTACT,
                                            TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO, G_TYPE_BOOLEAN,
                                            TRUE,
                                            NULL));

    tp_base_client_take_handler_filter (client,
                                        tp_asv_new (
                                            TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
                                            TPY_IFACE_CHANNEL_TYPE_CALL,
                                            TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT,
                                            TP_HANDLE_TYPE_CONTACT,
                                            TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO, G_TYPE_BOOLEAN,
                                            TRUE,
                                            NULL));

    tp_base_client_add_handler_capabilities_varargs (client,
            TPY_IFACE_CHANNEL_TYPE_CALL "/video/h264",
            TPY_IFACE_CHANNEL_TYPE_CALL "/shm",
            TPY_IFACE_CHANNEL_TYPE_CALL "/ice",
            TPY_IFACE_CHANNEL_TYPE_CALL "/gtalk-p2p",
            NULL);

    tp_base_client_register (client, NULL);
}

static void
mex_telepathy_plugin_init (MexTelepathyPlugin  *self)
{
    MexModelInfo *info;
    MexTelepathyPluginPrivate *priv;

    priv = self->priv = TELEPATHY_PLUGIN_PRIVATE (self);
    priv->actions = NULL;
    priv->contacts = NULL;

    priv->manager = mex_model_manager_get_default ();
    MexModelCategoryInfo contacts = { "contacts", _("Contacts"), "icon-panelheader-search", 10,
                                      _("None of your contacts are online at the moment") };
    mex_model_manager_add_category(priv->manager, &contacts);

    priv->feed = mex_feed_new("Contacts", "Feed");

    mex_telepathy_plugin_add_action("startavcall",
                                    _("Video Call"),
                                    (GCallback)mex_telepathy_plugin_on_start_video_call,
                                    "icon-panelheader-videos",
                                    g_strdupv ((gchar **)av_contact_mimetypes),
                                    80,
                                    self);

    mex_telepathy_plugin_add_action("startaudiocall",
                                    _("Audio Call"),
                                    (GCallback)mex_telepathy_plugin_on_start_audio_call,
                                    "icon-panelheader-music",
                                    g_strdupv ((gchar **)audio_contact_mimetypes),
                                    50,
                                    self);

    mex_telepathy_plugin_add_action("acceptcontact",
                                    _("Accept Contact Request"),
                                    (GCallback)mex_telepathy_plugin_on_accept_contact,
                                    "media-addtoqueue-mex",
                                    g_strdupv ((gchar **)pending_contact_mimetypes),
                                    100,
                                    self);

    info = mex_model_info_new_with_sort_funcs (MEX_MODEL (priv->feed), "contacts", 0);

    priv->models = g_list_append (priv->models, info);

    const GQuark account_features[] = {
        TP_ACCOUNT_FEATURE_CONNECTION,
        0 };
    const GQuark connection_features[] = {
        TP_CONNECTION_FEATURE_CONTACT_LIST,
        0 };
    static TpContactFeature contact_features[] = {
        TP_CONTACT_FEATURE_ALIAS,
        TP_CONTACT_FEATURE_AVATAR_DATA,
        TP_CONTACT_FEATURE_AVATAR_TOKEN,
        TP_CONTACT_FEATURE_PRESENCE,
        TP_CONTACT_FEATURE_CAPABILITIES
    };

    TpDBusDaemon *daemon = tp_dbus_daemon_dup(NULL);
    TpSimpleClientFactory *factory = tp_simple_client_factory_new(daemon);
    tp_simple_client_factory_add_account_features(factory, account_features);
    tp_simple_client_factory_add_connection_features(factory, connection_features);
    tp_simple_client_factory_add_contact_features(factory,
                                                  G_N_ELEMENTS (contact_features),
                                                  contact_features);

    // Tp init
    priv->account_manager = tp_simple_client_factory_ensure_account_manager (factory);
    tp_proxy_prepare_async (priv->account_manager,
                            NULL,
                            mex_telepathy_plugin_on_account_manager_ready,
                            self);

    g_object_unref(daemon);
    g_object_unref(factory);

    handler_create(self);
}

MexTelepathyPlugin *
mex_telepathy_plugin_new (void)
{
  return g_object_new (MEX_TYPE_TELEPATHY_PLUGIN, NULL);
}

static const GList *
mex_telepathy_plugin_get_actions (MexActionProvider *action_provider)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (action_provider);
  MexTelepathyPluginPrivate *priv = self->priv;

  return priv->actions;
}

static const GList *
mex_telepathy_plugin_get_models (MexModelProvider *model_provider)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (model_provider);
  MexTelepathyPluginPrivate *priv = self->priv;

  return priv->models;
}

static void
model_provider_iface_init (MexModelProviderInterface *iface)
{
  iface->get_models = mex_telepathy_plugin_get_models;
}

static void
action_provider_iface_init (MexActionProviderInterface *iface)

{
  iface->get_actions = mex_telepathy_plugin_get_actions;
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_TELEPATHY_PLUGIN;
}
