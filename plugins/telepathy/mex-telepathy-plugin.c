/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Collabora Ltd.
 *   @author Dario Freddi <dario.freddi@collabora.com>
 *   @author Jeremy Whiting <jeremy.whiting@collabora.com>
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
#include "mex-telepathy-channel.h"
#include "tpy-client-factory.h"

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

#include <telepathy-farstream/telepathy-farstream.h>

#include <telepathy-yell/interfaces.h>

#include <glib/gi18n.h>

#define MEX_LOG_DOMAIN_DEFAULT  telepathy_log_domain
MEX_LOG_DOMAIN_STATIC(telepathy_log_domain);

static const gchar *audio_contact_mimetypes[] =
{ "x-mex-audio-contact", "x-mex-av-contact", NULL };
static const gchar *av_contact_mimetypes[] = { "x-mex-av-contact", NULL };
static const gchar *pending_contact_mimetypes[] =
{ "x-mex-pending-contact", NULL };

static void tool_provider_iface_init (MexToolProviderInterface *iface);
static void model_provider_iface_init (MexModelProviderInterface *iface);
static void action_provider_iface_init (MexActionProviderInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MexTelepathyPlugin,
                         mex_telepathy_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_TOOL_PROVIDER,
                                                tool_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                action_provider_iface_init))

#define TELEPATHY_PLUGIN_PRIVATE(o)                         \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                        \
                                MEX_TYPE_TELEPATHY_PLUGIN,  \
                                MexTelepathyPluginPrivate))

struct _MexTelepathyPluginPrivate
{
  MexModelManager *manager;
  MexModel        *model;
  MexInfoBar      *info_bar;

  GList *models;
  GList *actions;
  GList *contacts;

  GList                      *channels;
  TpAccountManager           *account_manager;
  TpBaseClient               *client;
  TpBaseClient               *approver;
  TpChannelDispatchOperation *dispatch_operation;
  ClutterActor               *dialog;
  ClutterActor               *prompt_label;
  TpyAutomaticClientFactory  *factory;

  gboolean building_contact_list;
};

G_GNUC_UNUSED static void
remove_model (gpointer key G_GNUC_UNUSED, gpointer value, gpointer user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexModel *model = MEX_MODEL (value);

  mex_model_manager_remove_model (self->priv->manager, model);
}

G_GNUC_UNUSED static void
mex_telepathy_plugin_dispose (GObject *gobject)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (gobject);
  MexTelepathyPluginPrivate *priv = self->priv;

  mex_model_manager_remove_category (mex_model_manager_get_default (),
                                     "contacts");

  while (priv->models)
    {
      mex_model_info_free (priv->models->data);
      priv->models = g_list_delete_link (priv->models, priv->models);
    }

  while (priv->actions)
    {
      MexActionInfo *info = priv->actions->data;

      g_object_unref (info->action);
      g_strfreev (info->mime_types);
      g_free (info);

      priv->actions = g_list_delete_link (priv->actions, priv->actions);
    }

  while (priv->contacts)
    {
      g_object_unref (priv->contacts->data);
      priv->contacts = g_list_delete_link (priv->contacts, priv->contacts);
    }

  while (priv->channels)
    {
      g_object_unref (priv->channels->data);
      priv->channels = g_list_delete_link (priv->channels, priv->channels);
    }

  mex_model_clear (priv->model);
  g_object_unref (priv->model);

  g_object_unref (priv->account_manager);
  g_object_unref (priv->client);
  g_object_unref (priv->approver);
  g_object_unref (priv->dispatch_operation);
  g_object_unref (priv->factory);

  if (priv->dialog)
    {
      g_object_unref (priv->dialog);
      priv->dialog = NULL;
    }

  if (priv->prompt_label)
    {
      g_object_unref (priv->prompt_label);
      priv->prompt_label = NULL;
    }

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
mex_telepathy_plugin_compare_mex_contact (gconstpointer a,
                                          gconstpointer b)
{
  MexContact *mex_contact = MEX_CONTACT (a);
  TpContact *contact_b = TP_CONTACT (b);
  TpContact *contact_a = mex_contact_get_tp_contact (mex_contact);

  return tp_strdiff (tp_contact_get_identifier (
                       contact_a), tp_contact_get_identifier (contact_b));
}

static void
mex_telepathy_plugin_on_request_subscription (GObject           *source_object,
                                              GAsyncResult      *res,
                                              gpointer user_data G_GNUC_UNUSED)
{
  TpContact *contact = TP_CONTACT (source_object);

  GError *error = NULL;

  if (!tp_contact_request_subscription_finish (contact, res, &error))
    {
      MEX_WARNING ("Error subscribing to contact: %s\n", error->message);
      g_error_free (error);
      /* TODO: Maybe show stuff in the UI here? */
    }
}

static void
mex_telepathy_plugin_on_authorize_publication (GObject      *source_object,
                                               GAsyncResult *res,
                                               gpointer      user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  TpContact *contact = TP_CONTACT (source_object);

  GError *error = NULL;

  if (!tp_contact_authorize_publication_finish (contact, res, &error))
    {
      MEX_WARNING ("Error authorizing contact: %s\n", error->message);
      g_error_free (error);
      /* TODO: Maybe show stuff in the UI here? */
      return;
    }

  /* If we are not subscribed to the contact, do it at this point */
  if (tp_contact_get_subscribe_state (contact) == TP_SUBSCRIPTION_STATE_NO)
    {
      tp_contact_request_subscription_async (contact,
                                             _(
                                               "Please allow me to see your presence"),
                                             mex_telepathy_plugin_on_request_subscription,
                                             self);
    }
}

static void
mex_telepathy_plugin_on_accept_contact (MxAction *action,
                                        gpointer  user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  MexContent *content = mex_action_get_content (action);
  MexContact *mex_contact = MEX_CONTACT (content);
  TpContact *contact = mex_contact_get_tp_contact (mex_contact);

  tp_contact_authorize_publication_async (
    contact,
    mex_telepathy_plugin_on_authorize_publication,
    self);
}

static void
mex_telepathy_plugin_on_channel_ensured (GObject           *source,
                                         GAsyncResult      *result,
                                         gpointer user_data G_GNUC_UNUSED)
{
  gboolean success;
  GError *error = NULL;

  success = tp_account_channel_request_ensure_channel_finish (
    TP_ACCOUNT_CHANNEL_REQUEST (source), result, &error);
  if (!success)
    {
      MEX_WARNING ("Failed to create channel: %s", error->message);

      g_error_free (error);
    }
  else
    {
      MEX_INFO ("Channel successfully ensured");
    }

  g_object_unref (source);
}

static void
mex_telepathy_plugin_craft_channel_request (MexTelepathyPlugin *self,
                                            TpAccount          *account,
                                            TpContact          *contact,
                                            gboolean            audio,
                                            gboolean            video)
{
  MexTelepathyPluginPrivate *priv = self->priv;

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
    tp_contact_get_identifier (contact),

    TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO,
    G_TYPE_BOOLEAN,
    audio,

    TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO,
    G_TYPE_BOOLEAN,
    video,

    NULL);

  MEX_INFO ("Offer video channel to %s", tp_contact_get_identifier (contact));

  req = tp_account_channel_request_new (account, request,
                                        TP_USER_ACTION_TIME_CURRENT_TIME);

  tp_account_channel_request_ensure_channel_async (
    req,
    tp_base_client_get_bus_name (priv->client),
    NULL,
    mex_telepathy_plugin_on_channel_ensured,
    self);

  g_hash_table_unref (request);
}

static void
mex_telepathy_plugin_on_start_video_call (MxAction *action,
                                          gpointer  user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

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
mex_telepathy_plugin_trigger_notification_for_contact (MexTelepathyPlugin *self,
                                                       TpContact          *contact,
                                                       gboolean            added)
{
  MexTelepathyPluginPrivate *priv = self->priv;

  if (priv->building_contact_list)
    {
      return;
    }

  if (added)
    {
      if (tp_contact_get_subscribe_state (contact) == TP_SUBSCRIPTION_STATE_ASK)
        {
          mex_info_bar_new_notification (priv->info_bar,
                                         g_strdup_printf (_(
                                                            "The contact %s has added you to his contact list. Go to "
                                                            "\"Contacts\" to accept his request and start interacting "
                                                            "with him."),
                                                          tp_contact_get_alias (
                                                            contact)),
                                         20);
        }
      else
        {
          mex_info_bar_new_notification (priv->info_bar,
                                         g_strdup_printf (_("%s is now Online"),
                                                          tp_contact_get_alias (
                                                            contact)),
                                         5);
        }
    }
  else
    {
      mex_info_bar_new_notification (priv->info_bar,
                                     g_strdup_printf (_("%s has gone Offline"),
                                                      tp_contact_get_alias (
                                                        contact)),
                                     5);
    }
}

static void
mex_telepathy_plugin_on_should_add_to_model_changed (gpointer instance,
                                                     gboolean should_add,
                                                     gpointer user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexTelepathyPluginPrivate *priv = self->priv;

  MexContact *contact = MEX_CONTACT (instance);

  if (should_add)
    {
      mex_model_add_content (priv->model, MEX_CONTENT (contact));

      mex_telepathy_plugin_trigger_notification_for_contact (
        self,
        mex_contact_get_tp_contact (
          contact),
        TRUE);
    }
  else
    {
      mex_model_remove_content (priv->model, MEX_CONTENT (contact));

      mex_telepathy_plugin_trigger_notification_for_contact (
        self,
        mex_contact_get_tp_contact (
          contact),
        FALSE);
    }
}

static void
mex_telepathy_plugin_add_contact (gpointer contact_ptr,
                                  gpointer user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexTelepathyPluginPrivate *priv = self->priv;
  TpContact *contact = TP_CONTACT (contact_ptr);
  MexContact *mex_contact;

  MEX_DEBUG ("Adding %s", tp_contact_get_alias (contact));

  mex_contact = g_object_new (MEX_TYPE_CONTACT,
                              "contact", contact,
                              NULL);

  /* Ref this object: we need it alive until its removal */
  g_object_ref (mex_contact);

  priv->contacts = g_list_append (priv->contacts, mex_contact);

  if (mex_contact_should_add_to_model (mex_contact))
    {
      mex_model_add_content (priv->model, MEX_CONTENT (mex_contact));

      mex_telepathy_plugin_trigger_notification_for_contact (self,
                                                             contact,
                                                             TRUE);
    }

  g_signal_connect(mex_contact,
                   "should-add-to-model-changed",
                   G_CALLBACK(
                     mex_telepathy_plugin_on_should_add_to_model_changed),
                   self);
}

static void
mex_telepathy_plugin_remove_contact (gpointer contact_ptr,
                                     gpointer user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexTelepathyPluginPrivate *priv = self->priv;
  GList *found;
  TpContact *contact = TP_CONTACT (contact_ptr);
  gpointer found_element;

  MEX_DEBUG ("Removing %s", tp_contact_get_alias (contact));

  found = g_list_find_custom (priv->contacts, contact,
                              mex_telepathy_plugin_compare_mex_contact);
  if (found == NULL)
    {
      MEX_WARNING ("Could not find contact %s!",
                   tp_contact_get_identifier (contact));
      return;
    }

  found_element = found->data;

  mex_model_remove_content(priv->model, MEX_CONTENT (found_element));
  priv->contacts = g_list_remove(priv->contacts, found_element);

  MEX_DEBUG ("Contact %s removed successfully.", tp_contact_get_identifier (
               mex_contact_get_tp_contact (found_element)));

  g_object_unref (found_element);
}

static void
mex_telepathy_plugin_on_contact_list_changed(TpConnection *connection G_GNUC_UNUSED,
                                             GPtrArray    *added,
                                             GPtrArray    *removed,
                                             gpointer      user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  g_ptr_array_foreach (added, mex_telepathy_plugin_add_contact, self);
  g_ptr_array_foreach (removed, mex_telepathy_plugin_remove_contact, self);

  g_ptr_array_unref (added);
  g_ptr_array_unref (removed);
}

static void
mex_telepathy_plugin_on_connection_ready (TpConnection *connection,
                                          const GError *error,
                                          gpointer      user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexTelepathyPluginPrivate *priv = self->priv;
  GPtrArray *contacts;
  guint i;

  if (error != NULL)
    {
      MEX_WARNING ("%s", error->message);
      return;
    }

  MEX_DEBUG ("Connection ready!");

  contacts = tp_connection_dup_contact_list (connection);

  priv->building_contact_list = TRUE;

  for (i = 0; i < contacts->len; i++)
    {
      TpContact *contact = g_ptr_array_index (contacts, i);
      mex_telepathy_plugin_add_contact (contact, self);
    }

  priv->building_contact_list = FALSE;

  g_ptr_array_unref (contacts);

  g_signal_connect(connection,
                   "contact-list-changed",
                   G_CALLBACK (mex_telepathy_plugin_on_contact_list_changed),
                   self);
}

static void
mex_telepathy_plugin_on_account_status_changed (TpAccount  *account,
                                                guint       old_status,
                                                guint       new_status,
                                                guint       reason G_GNUC_UNUSED,
                                                gchar      *dbus_error_name,
                                                GHashTable *details G_GNUC_UNUSED,
                                                gpointer    user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  switch (new_status)
    {
    case TP_CONNECTION_STATUS_CONNECTED:
      if (old_status != TP_CONNECTION_STATUS_CONNECTED)
        {
          MEX_DEBUG ("Account got connected!");
          mex_telepathy_plugin_on_connection_ready (tp_account_get_connection (
                                                      account),
                                                    NULL,
                                                    self);
        }
      break;
    default:
      if (old_status == TP_CONNECTION_STATUS_CONNECTED)
        {
          MEX_WARNING ("Account got disconnected! %s", dbus_error_name);
          /* TODO: Maybe give more info? */
        }
    }
}

static void
tool_provider_iface_init (MexToolProviderInterface *iface G_GNUC_UNUSED)
{
}

static void
mex_telepathy_plugin_on_presence_request_finished (GObject      *source_object,
                                                   GAsyncResult *res,
                                                   gpointer      user_data G_GNUC_UNUSED)
{
  TpAccount *account = TP_ACCOUNT(source_object);

  gboolean success = FALSE;

  success = tp_account_request_presence_finish (account, res, NULL);

  if (!success)
    {
      /* TODO Handle error */
      MEX_WARNING ("Fail in setting a different presence!");
      return;
    }
}

static void
mex_telepathy_plugin_on_account_manager_ready (GObject      *source_object G_GNUC_UNUSED,
                                               GAsyncResult *res,
                                               gpointer      user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexTelepathyPluginPrivate *priv = self->priv;
  GList * accounts;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (priv->account_manager, res, &error))
    {
      MEX_WARNING ("Error preparing AM: %s\n", error->message);
      g_error_free (error);
      /* TODO: Maybe show stuff in the UI here? */
      return;
    }

  /* Get the accounts */
  for (accounts = tp_account_manager_get_valid_accounts (priv->account_manager);
       accounts != NULL; accounts = g_list_delete_link (accounts, accounts))
    {
      TpAccount *account = accounts->data;
      TpConnection *connection = tp_account_get_connection (account);

      if (connection == NULL)
        {
          /* Get the autopresence */
          guint type;
          gchar *status;
          gchar *message;

          MEX_INFO ("Account is not connected, setting it back to autopresence");
          g_object_get (account,
                        "automatic-presence-type", &type,
                        "automatic-status", &status,
                        "automatic-status-message", &message,
                        NULL);

          tp_account_request_presence_async (
            account,
            type,
            status,
            message,
            mex_telepathy_plugin_on_presence_request_finished,
            self);

          g_free(status);
          g_free(message);
        }
      else
        {
          connection = tp_account_get_connection (account);
          mex_telepathy_plugin_on_connection_ready (connection, NULL, self);
        }

      g_signal_connect(account,
                       "status-changed",
                       G_CALLBACK (
                         mex_telepathy_plugin_on_account_status_changed),
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

static void
mex_telepathy_plugin_on_show_call (MexTelepathyChannel         *channel G_GNUC_UNUSED,
                                   ClutterActor                *page,
                                   gpointer                     user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  mex_tool_provider_present_actor (MEX_TOOL_PROVIDER (self),
                                   page);
}

static void
mex_telepathy_plugin_on_hide_call (MexTelepathyChannel         *channel G_GNUC_UNUSED,
                                   ClutterActor                *page,
                                   gpointer                     user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  if (CLUTTER_ACTOR_IS_VISIBLE (page))
    mex_tool_provider_remove_actor (MEX_TOOL_PROVIDER (self),
                                    page);
}

static void
mex_telepathy_plugin_on_new_call_channel (TpSimpleHandler         *handler G_GNUC_UNUSED,
                                          TpAccount               *account        G_GNUC_UNUSED,
                                          TpConnection            *connection,
                                          GList                   *channels,
                                          GList                   *requests_satisfied G_GNUC_UNUSED,
                                          gint64                   user_action_time G_GNUC_UNUSED,
                                          TpHandleChannelsContext *handler_context,
                                          gpointer                 user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexTelepathyPluginPrivate *priv = self->priv;
  TpChannel *proxy = channels->data;
  MexTelepathyChannel *channel;

  tp_handle_channels_context_accept (handler_context);

  channel = g_object_new (MEX_TYPE_TELEPATHY_CHANNEL,
                          "connection", connection,
                          "channel", proxy,
                          NULL);
  g_signal_connect (channel,
                    "show-actor",
                    G_CALLBACK (mex_telepathy_plugin_on_show_call),
                    self);
  g_signal_connect (channel,
                    "hide-actor",
                    G_CALLBACK (mex_telepathy_plugin_on_hide_call),
                    self);

  priv->channels = g_list_append (priv->channels, g_object_ref (channel));
}

static void
mex_telepathy_plugin_on_handle_with (GObject           *source,
                                     GAsyncResult      *result,
                                     gpointer           user_data G_GNUC_UNUSED)
{
  TpChannelDispatchOperation *cdo = TP_CHANNEL_DISPATCH_OPERATION (source);
  GError *error = NULL;

  if (!tp_channel_dispatch_operation_handle_with_finish (cdo, result, &error))
    {
      MEX_WARNING ("HandleWith() failed: %s\n", error->message);
      g_error_free (error);
      return;
    }

  MEX_DEBUG ("HandleWith() succeeded\n");
}

static void
mex_telepathy_plugin_on_claim (GObject           *source,
                               GAsyncResult      *result,
                               gpointer           user_data G_GNUC_UNUSED)

{
  TpChannelDispatchOperation *cdo = TP_CHANNEL_DISPATCH_OPERATION (source);
  GError *error = NULL;
  GPtrArray *channels;
  guint i;

  if (!tp_channel_dispatch_operation_claim_with_finish (cdo, result, &error))
    {
      MEX_WARNING ("Claim() failed: %s\n", error->message);
      g_error_free (error);
      return;
    }

  MEX_DEBUG ("Claim() succeeded, close channels\n");

  channels = tp_channel_dispatch_operation_borrow_channels (cdo);
  for (i = 0; i < channels->len; i++)
    {
      TpChannel *channel = g_ptr_array_index (channels, i);

      tp_cli_channel_call_close (channel, -1, NULL, NULL, NULL, NULL);
    }
}

static void
mex_telepathy_plugin_hide_prompt_dialog (MexTelepathyPlugin *self)
{
  /* Hide the dialog. */
  clutter_actor_destroy (self->priv->dialog);
  g_object_unref (self->priv->dispatch_operation);
}

static void
mex_telepathy_plugin_on_incoming_call_accept (MxAction *action G_GNUC_UNUSED,
                                              gpointer  user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  MEX_DEBUG ("accept chosen");

  tp_channel_dispatch_operation_handle_with_async (
    self->priv->dispatch_operation,
    tp_base_client_get_bus_name (self->priv->client),
    mex_telepathy_plugin_on_handle_with,
    NULL);

  mex_telepathy_plugin_hide_prompt_dialog (self);
}

static void
mex_telepathy_plugin_on_incoming_call_deny (MxAction *action G_GNUC_UNUSED,
                                            gpointer         user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  MEX_DEBUG ("deny chosen");

  tp_channel_dispatch_operation_claim_with_async (self->priv->dispatch_operation,
                                                  TP_BASE_CLIENT (
                                                    self->priv->
                                                    approver),
                                                  mex_telepathy_plugin_on_claim,
                                                  NULL);

  mex_telepathy_plugin_hide_prompt_dialog (self);
}

static void
mex_telepathy_plugin_on_contact_fetched (TpConnection            *connection G_GNUC_UNUSED,
                                         guint                    n_contacts,
                                         TpContact       * const *contacts,
                                         guint                    n_failed    G_GNUC_UNUSED,
                                         const TpHandle          *failed      G_GNUC_UNUSED,
                                         const GError            *error       G_GNUC_UNUSED,
                                         gpointer                 user_data,
                                         GObject                 *weak_object G_GNUC_UNUSED)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);

  guint i = 0;

  for (i = 0; i < n_contacts; ++i)
    {
      /* Get the contacts. */
      TpContact *current = contacts[i];

      /* Add the alias to the label. */
      gchar *label_text = g_strdup_printf (_("Incoming call from %s "),
                                           tp_contact_get_alias (current));
      mx_label_set_text (MX_LABEL (self->priv->prompt_label), label_text);
    }
}

static void
mex_telepathy_plugin_incoming_call_prompt (MexTelepathyPlugin *self,
                                           TpConnection       *connection,
                                           TpChannel          *channel)
{
  MexTelepathyPluginPrivate *priv = self->priv;
  TpContactFeature features[] = { TP_CONTACT_FEATURE_ALIAS };
  MxAction *accept_action, *deny_action;
  ClutterActor *stack;
  TpHandle contactHandle;

  MEX_DEBUG ("showing accept/deny dialog");
    
  accept_action = mx_action_new_full (
      "accept", "Accept",
      G_CALLBACK (mex_telepathy_plugin_on_incoming_call_accept), self);

  deny_action = mx_action_new_full (
      "deny", "Deny",
      G_CALLBACK (mex_telepathy_plugin_on_incoming_call_deny), self);

  priv->dialog = mx_dialog_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->dialog),
                               "MexErrorDialog");
  clutter_actor_set_name (priv->dialog, "telepathy-prompt-dialog");

  stack = mx_window_get_child (mex_get_main_window ());
  mx_dialog_set_transient_parent (MX_DIALOG (priv->dialog), stack);

  mx_dialog_add_action (MX_DIALOG (priv->dialog), accept_action);
  mx_dialog_add_action (MX_DIALOG (priv->dialog), deny_action);

  priv->prompt_label = mx_label_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (
                                 priv->dialog), priv->prompt_label);

  contactHandle = tp_channel_get_handle (channel, NULL);
  if (contactHandle)
    tp_connection_get_contacts_by_handle (
      connection, 1, &contactHandle, 1,
      features,
      mex_telepathy_plugin_on_contact_fetched,
      self, NULL, NULL);

  clutter_actor_show (priv->dialog);
  clutter_actor_raise_top (priv->dialog);
  mex_push_focus (MX_FOCUSABLE (priv->dialog));
}

static void
mex_telepathy_plugin_add_dispatch_operation (TpSimpleApprover              *approver G_GNUC_UNUSED,
                                             TpAccount                     *account  G_GNUC_UNUSED,
                                             TpConnection                  *connection,
                                             GList                         *channels,
                                             TpChannelDispatchOperation    *dispatch_operation,
                                             TpAddDispatchOperationContext *context,
                                             gpointer                       user_data)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
  MexTelepathyPluginPrivate *priv = self->priv;
  guint i;

  MEX_DEBUG ("add dispatch operation called");

  priv->dispatch_operation = g_object_ref (dispatch_operation);

  tp_add_dispatch_operation_context_accept (context);

  for (i = 0; i < g_list_length(channels); i++)
    {
      TpChannel *channel = g_list_nth_data (channels, i);
      mex_telepathy_plugin_incoming_call_prompt (self, connection, channel);
    }
}

static void
mex_telepathy_plugin_create_approver (MexTelepathyPlugin *self)
{
  MexTelepathyPluginPrivate *priv = self->priv;

  priv->approver = tp_simple_approver_new_with_am (
    priv->account_manager,
    "TpMexApprover",
    FALSE,
    mex_telepathy_plugin_add_dispatch_operation,
    self,
    NULL);

  tp_base_client_take_approver_filter (priv->approver,
                                       tp_asv_new (
                                         TP_PROP_CHANNEL_CHANNEL_TYPE,
                                         G_TYPE_STRING,
                                         TPY_IFACE_CHANNEL_TYPE_CALL,

                                         TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
                                         G_TYPE_UINT,
                                         TP_HANDLE_TYPE_CONTACT,

                                         TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO,
                                         G_TYPE_BOOLEAN,
                                         TRUE,

                                         NULL));

  tp_base_client_take_approver_filter (priv->approver,
                                       tp_asv_new (
                                         TP_PROP_CHANNEL_CHANNEL_TYPE,
                                         G_TYPE_STRING,
                                         TPY_IFACE_CHANNEL_TYPE_CALL,

                                         TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
                                         G_TYPE_UINT,
                                         TP_HANDLE_TYPE_CONTACT,

                                         TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO,
                                         G_TYPE_BOOLEAN,
                                         TRUE,

                                         NULL));

  tp_base_client_register (priv->approver, NULL);
}

static void
mex_telepathy_plugin_create_handler (MexTelepathyPlugin *self)
{
  MexTelepathyPluginPrivate *priv = self->priv;

  priv->client = tp_simple_handler_new_with_am (
    priv->account_manager,
    FALSE,
    FALSE,
    "TpMexHandler",
    TRUE,
    mex_telepathy_plugin_on_new_call_channel,
    self,
    NULL);

  tp_base_client_take_handler_filter (priv->client,
                                      tp_asv_new (
                                        TP_PROP_CHANNEL_CHANNEL_TYPE,
                                        G_TYPE_STRING,
                                        TPY_IFACE_CHANNEL_TYPE_CALL,

                                        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
                                        G_TYPE_UINT,
                                        TP_HANDLE_TYPE_CONTACT,

                                        TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO,
                                        G_TYPE_BOOLEAN,
                                        TRUE,

                                        NULL));

  tp_base_client_take_handler_filter (priv->client,
                                      tp_asv_new (
                                        TP_PROP_CHANNEL_CHANNEL_TYPE,
                                        G_TYPE_STRING,
                                        TPY_IFACE_CHANNEL_TYPE_CALL,

                                        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
                                        G_TYPE_UINT,
                                        TP_HANDLE_TYPE_CONTACT,

                                        TPY_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO,
                                        G_TYPE_BOOLEAN,
                                        TRUE,

                                        NULL));

  tp_base_client_add_handler_capabilities_varargs (
    priv->client,
    TPY_IFACE_CHANNEL_TYPE_CALL
    "/video/h264",
    TPY_IFACE_CHANNEL_TYPE_CALL
    "/shm",
    TPY_IFACE_CHANNEL_TYPE_CALL
    "/ice",
    TPY_IFACE_CHANNEL_TYPE_CALL
    "/gtalk-p2p",
    NULL);

  tp_base_client_register (priv->client, NULL);
}

static void
mex_telepathy_plugin_init (MexTelepathyPlugin *self)
{
  MexModelInfo *info;
  MexTelepathyPluginPrivate *priv;
  TpDBusDaemon *tp_daemon;
  MexModelCategoryInfo contacts = { "contacts", _("Contacts"), "contact", 10,
                                    _("None of your contacts are online at the moment"),
                                    TRUE };
  GQuark account_features[] = {
    TP_ACCOUNT_FEATURE_CONNECTION,
    0
  };
  GQuark connection_features[] = {
    TP_CONNECTION_FEATURE_CONTACT_LIST,
    0
  };
  static TpContactFeature contact_features[] = {
    TP_CONTACT_FEATURE_ALIAS,
    TP_CONTACT_FEATURE_AVATAR_DATA,
    TP_CONTACT_FEATURE_AVATAR_TOKEN,
    TP_CONTACT_FEATURE_PRESENCE,
    TP_CONTACT_FEATURE_CAPABILITIES
  };


  tf_init ();
  tpy_cli_init ();

  priv = self->priv = TELEPATHY_PLUGIN_PRIVATE (self);
  priv->actions = NULL;
  priv->contacts = NULL;
  priv->dialog = NULL;
  priv->prompt_label = NULL;

  /* log domain */
  MEX_LOG_DOMAIN_INIT (telepathy_log_domain, "telepathy");

  priv->manager = mex_model_manager_get_default ();
  mex_model_manager_add_category(priv->manager, &contacts);

  priv->model = mex_generic_model_new("Contacts", "Feed");

  mex_telepathy_plugin_add_action ("startavcall",
                                   _("Video Call"),
                                   G_CALLBACK (
                                     mex_telepathy_plugin_on_start_video_call),
                                   "call-video",
                                   g_strdupv ((gchar **)av_contact_mimetypes),
                                   80,
                                   self);

  mex_telepathy_plugin_add_action ("startaudiocall",
                                   _("Audio Call"),
                                   G_CALLBACK (
                                     mex_telepathy_plugin_on_start_audio_call),
                                   "call-audio",
                                   g_strdupv ((gchar * *)audio_contact_mimetypes),
                                   50,
                                   self);

  mex_telepathy_plugin_add_action ("acceptcontact",
                                   _("Accept Contact Request"),
                                   G_CALLBACK (
                                     mex_telepathy_plugin_on_accept_contact),
                                   "contact-add",
                                   g_strdupv ((gchar **)
                                              pending_contact_mimetypes),
                                   100,
                                   self);

  info = mex_model_info_new_with_sort_funcs (priv->model, "contacts", 0);

  priv->models = g_list_append (priv->models, info);

  priv->info_bar = MEX_INFO_BAR (mex_info_bar_get_default ());

  tp_daemon = tp_dbus_daemon_dup (NULL);
  priv->factory = tpy_automatic_client_factory_new (tp_daemon);
  tp_simple_client_factory_add_account_features (TP_SIMPLE_CLIENT_FACTORY (
                                                   priv->factory),
                                                 account_features);
  tp_simple_client_factory_add_connection_features (TP_SIMPLE_CLIENT_FACTORY (
                                                      priv->factory),
                                                    connection_features);
  tp_simple_client_factory_add_contact_features (TP_SIMPLE_CLIENT_FACTORY (
                                                   priv->factory),
                                                 G_N_ELEMENTS (contact_features),
                                                 contact_features);

  /* Tp init */
  priv->account_manager = tp_account_manager_new_with_factory (
    TP_SIMPLE_CLIENT_FACTORY (priv->factory));
  tp_account_manager_set_default (priv->account_manager);
  tp_proxy_prepare_async (priv->account_manager,
                          NULL,
                          mex_telepathy_plugin_on_account_manager_ready,
                          self);


  mex_telepathy_plugin_create_approver (self);
  mex_telepathy_plugin_create_handler (self);

  g_object_unref (tp_daemon);
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
