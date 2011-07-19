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

#include <telepathy-glib/account.h>
#include <telepathy-glib/account-channel-request.h>
#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/connection-contact-list.h>
#include <telepathy-glib/contact.h>
#include <telepathy-glib/simple-client-factory.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/_gen/telepathy-interfaces.h>

#include <glib/gi18n.h>

static const gchar *audio_contact_mimetypes[] = { "x-mex-audio-contact", "x-mex-av-contact", NULL };
static const gchar *av_contact_mimetypes[] = { "x-mex-av-contact", NULL };

static void model_provider_iface_init (MexModelProviderInterface *iface);
static void action_provider_iface_init (MexActionProviderInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MexTelepathyPlugin,
                         mex_telepathy_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                action_provider_iface_init))


#define GET_PRIVATE(o)                                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TELEPATHY_PLUGIN,         \
                                MexTelepathyPluginPrivate))

struct _MexTelepathyPluginPrivate {
  MexModelManager *manager;
  MexFeed *feed;
  GList *models;
  GList *actions;
  GList *contacts;

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
        "org.freedesktop.Telepathy.Channel.Type.Call.DRAFT",

        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
        G_TYPE_UINT,
        TP_HANDLE_TYPE_CONTACT,

        TP_PROP_CHANNEL_TARGET_ID,
        G_TYPE_STRING,
        tp_contact_get_identifier(contact),

        "org.freedesktop.Telepathy.Channel.Type.Call.DRAFT.InitialAudio",
        G_TYPE_BOOLEAN,
        audio,

        "org.freedesktop.Telepathy.Channel.Type.Call.DRAFT.InitialVideo",
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
mex_telepathy_plugin_init (MexTelepathyPlugin  *self)
{
    MexModelInfo *info;
    MexTelepathyPluginPrivate *priv;
    MexActionInfo *action_info;

    priv = self->priv = GET_PRIVATE (self);
    priv->actions = NULL;
    priv->contacts = NULL;

    priv->manager = mex_model_manager_get_default ();
    MexModelCategoryInfo contacts = { "contacts", _("Contacts"), "icon-panelheader-search", 0, "" };
    mex_model_manager_add_category(priv->manager, &contacts);

    priv->feed = mex_feed_new("Contacts", "Feed");

    action_info = g_new0 (MexActionInfo, 1);
    action_info->action = mx_action_new_full ("startavcall",
                                              _("Video Call"),
                                              (GCallback)mex_telepathy_plugin_on_start_video_call,
                                              self);
    mx_action_set_icon (action_info->action, "x-mex-app-launch-mex");
    action_info->mime_types = g_strdupv ((gchar **)av_contact_mimetypes);
    action_info->priority = 100;
    priv->actions = g_list_append (priv->actions, action_info);

    action_info = g_new0 (MexActionInfo, 1);
    action_info->action = mx_action_new_full ("startacall",
                                              _("Audio Call"),
                                              (GCallback)mex_telepathy_plugin_on_start_audio_call,
                                              self);
    mx_action_set_icon (action_info->action, "x-mex-app-launch-mex");
    action_info->mime_types = g_strdupv ((gchar **)audio_contact_mimetypes);
    action_info->priority = 90;
    priv->actions = g_list_append (priv->actions, action_info);

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
