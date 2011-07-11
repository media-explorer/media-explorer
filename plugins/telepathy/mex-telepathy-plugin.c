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

#include <glib/gi18n.h>

G_DEFINE_TYPE (MexTelepathyPlugin, mex_telepathy_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o)                                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TELEPATHY_PLUGIN,         \
                                MexTelepathyPluginPrivate))

struct _MexTelepathyPluginPrivate {
  MexModelManager *manager;
  GHashTable *video_models;

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
mex_telepathy_plugin_finalize (GObject *gobject)
{
  MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (gobject);
  MexTelepathyPluginPrivate *priv = self->priv;

  if (priv->video_models)
    {
      g_hash_table_foreach (priv->video_models, remove_model, self);
      g_hash_table_destroy (priv->video_models);
      priv->video_models = NULL;
    }

  G_OBJECT_CLASS (mex_telepathy_plugin_parent_class)->finalize (gobject);
}

static void
mex_telepathy_plugin_class_init (MexTelepathyPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = mex_telepathy_plugin_finalize;

  g_type_class_add_private (klass, sizeof (MexTelepathyPluginPrivate));
}

void mex_telepathy_plugin_on_account_ready(GObject *source_object,
                                           GAsyncResult *res,
                                           gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (source_object);
    MexTelepathyPluginPrivate *priv = self->priv;
    TpAccount *account = TP_ACCOUNT(user_data);

    gboolean success = FALSE;

    success = tp_account_prepare_finish(account, res, NULL);

    if (!success) {
        // TODO Handle error
        printf("Fail in preparing the AM!\n");
        return;
    }
}

void mex_telepathy_plugin_on_account_manager_ready(GObject *source_object,
                                                   GAsyncResult *res,
                                                   gpointer user_data)
{
    MexTelepathyPlugin *self = MEX_TELEPATHY_PLUGIN (source_object);
    MexTelepathyPluginPrivate *priv = self->priv;

    gboolean success = FALSE;

    success = tp_account_manager_prepare_finish(priv->m_account_manager,
                                                res,
                                                NULL);

    if (!success) {
        // TODO Handle error
        printf("Fail in preparing the AM!\n");
        return;
    }

    // Get the accounts
    GList *accounts;
    accounts = tp_account_manager_get_valid_accounts (priv->m_account_manager);
    g_list_foreach (accounts, (GFunc) g_object_ref, NULL);
    accounts = g_list_first(accounts);

    while (accounts != NULL) {
        TpAccount *account = TP_ACCOUNT(accounts->data);

        tp_account_prepare_async(account,
                                 NULL,
                                 mex_telepathy_plugin_on_account_ready,
                                 account);

        accounts = g_list_next(accounts);
    }
}

static void
mex_telepathy_plugin_init (MexTelepathyPlugin  *self)
{
  MexFeed *feed;
  MexModelInfo *info;
  MexTelepathyPluginPrivate *priv;

  priv = self->priv = GET_PRIVATE (self);

  priv->video_models = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, NULL);

  priv->manager = mex_model_manager_get_default ();
  MexModelCategoryInfo contacts = { "contacts", _("Contacts"), "icon-panelheader-search", 0, "" };
  mex_model_manager_add_category(priv->manager, &contacts);

  feed = mex_feed_new("Contacts", "Feed");

  // TODO: remove this fake item and replace it with real contact information.
  MexContent *content = MEX_CONTENT (mex_program_new (feed));
  mex_content_set_metadata (content, MEX_CONTENT_METADATA_TITLE, "MeeGo");
  mex_content_set_metadata (content, MEX_CONTENT_METADATA_MIMETYPE,
                            "x-mex/contact");
  mex_model_add_content (MEX_MODEL (feed), content);

  info = mex_model_info_new_with_sort_funcs (MEX_MODEL (feed), "contacts", 0);
  mex_model_manager_add_model (priv->manager, info);
  mex_model_info_free (info);

  // Tp init
  priv->m_account_manager = tp_account_manager_dup();
  tp_account_manager_prepare_async(priv->m_account_manager,
                                   NULL,
                                   mex_telepathy_plugin_on_account_manager_ready,
                                   NULL);
}

MexTelepathyPlugin *
mex_telepathy_plugin_new (void)
{
  return g_object_new (MEX_TYPE_TELEPATHY_PLUGIN, NULL);
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_TELEPATHY_PLUGIN;
}
