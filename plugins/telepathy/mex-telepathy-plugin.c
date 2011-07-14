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

#include <telepathy-glib/account.h>
#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/connection-contact-list.h>
#include <telepathy-glib/contact.h>
#include <telepathy-glib/simple-client-factory.h>

#include <glib/gi18n.h>
#include "mex-contact.h"

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

  TpAccountManager *m_account_manager;
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

static void mex_telepathy_plugin_add_contact(gpointer contact_ptr, gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    TpContact *contact = TP_CONTACT(contact_ptr);
    printf("Adding %s\n", tp_contact_get_alias(contact));

    MexContact *mex_contact;

    mex_contact = g_object_new (MEX_TYPE_CONTACT,
                                NULL);
    mex_contact_set_tp_contact(mex_contact, contact);

    mex_model_add_content(MEX_MODEL(priv->feed), MEX_CONTENT(mex_contact));
}

static void mex_telepathy_plugin_remove_contact(gpointer contact_ptr, gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (user_data);
    MexTelepathyPluginPrivate *priv = self->priv;

    TpContact *contact = TP_CONTACT(contact_ptr);
    printf("Removing %s\n", tp_contact_get_alias(contact));
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
    printf("Connection ready!\n");
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
                printf("Account got connected!\n");
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

    gboolean success = FALSE;

    success = tp_account_manager_prepare_finish(priv->m_account_manager,
                                                res,
                                                NULL);

    if (!success) {
        // TODO Handle error
        g_warning("Fail in preparing the AM!");
        return;
    }

    // Get the accounts
    GList *accounts;
    accounts = tp_account_manager_get_valid_accounts (priv->m_account_manager);
    accounts = g_list_first(accounts);

    while (accounts != NULL) {
        TpAccount *account = TP_ACCOUNT(accounts->data);

        // Get the connection, and wait until ready
        TpConnection *connection = 0;
        if (tp_account_get_connection_status(account, NULL) == TP_CONNECTION_STATUS_CONNECTED) {
                connection = tp_account_get_connection(account);
                tp_connection_call_when_ready(connection,
                                            mex_telepathy_plugin_on_connection_ready,
                                            self);
        } else {
            printf("Account is not connected, setting it back to autopresence\n");

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
        }

        g_signal_connect(account,
                        "status-changed",
                        G_CALLBACK(mex_telepathy_plugin_on_account_status_changed),
                        self);

        accounts = g_list_next(accounts);
    }
}

static void
mex_telepathy_plugin_init (MexTelepathyPlugin  *self)
{
    MexModelInfo *info;
    MexTelepathyPluginPrivate *priv;
    MexActionInfo *action_info;

    priv = self->priv = GET_PRIVATE (self);

    priv->manager = mex_model_manager_get_default ();
    MexModelCategoryInfo contacts = { "contacts", _("Contacts"), "icon-panelheader-search", 0, "" };
    mex_model_manager_add_category(priv->manager, &contacts);

    priv->feed = mex_feed_new("Contacts", "Feed");

    info = mex_model_info_new_with_sort_funcs (MEX_MODEL (priv->feed), "contacts", 0);

    priv->models = g_list_append (priv->models, info);

    static TpContactFeature contact_features[] = {
        TP_CONTACT_FEATURE_ALIAS,
        TP_CONTACT_FEATURE_AVATAR_DATA,
        TP_CONTACT_FEATURE_AVATAR_TOKEN,
        TP_CONTACT_FEATURE_PRESENCE,
        TP_CONTACT_FEATURE_CAPABILITIES
    };

    GQuark account_features[] = { TP_ACCOUNT_FEATURE_CONNECTION, 0 };
    GQuark connection_features[] = { tp_connection_get_feature_quark_contact_list(), 0 };

    TpDBusDaemon *daemon = tp_dbus_daemon_dup(NULL);
    TpSimpleClientFactory *factory = tp_simple_client_factory_new(daemon);
    tp_simple_client_factory_add_account_features(factory, account_features);
    tp_simple_client_factory_add_connection_features(factory, connection_features);
    tp_simple_client_factory_add_contact_features(factory, G_N_ELEMENTS (contact_features), contact_features);

    // Tp init
    priv->m_account_manager = tp_simple_client_factory_dup_account_manager(factory);

    GQuark am_features[] = { TP_ACCOUNT_MANAGER_FEATURE_ACCOUNT, 0 };

    tp_account_manager_prepare_async(priv->m_account_manager,
                                     am_features,
                                     mex_telepathy_plugin_on_account_manager_ready,
                                     self);

    g_object_unref(daemon);
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
