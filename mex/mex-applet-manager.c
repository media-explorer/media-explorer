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


#include "mex-applet-manager.h"
#include "mex-marshal.h"
#include "mex-log.h"

#define MEX_LOG_DOMAIN_DEFAULT  applet_manager_log_domain
MEX_LOG_DOMAIN(applet_manager_log_domain);

G_DEFINE_TYPE (MexAppletManager, mex_applet_manager, G_TYPE_OBJECT)

#define APPLET_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_APPLET_MANAGER, MexAppletManagerPrivate))

enum
{
  APPLET_ADDED,
  APPLET_REMOVED,

  LAST_SIGNAL
};

struct _MexAppletManagerPrivate
{
  GHashTable  *applets;
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
mex_applet_manager_dispose (GObject *object)
{
  MexAppletManagerPrivate *priv = MEX_APPLET_MANAGER (object)->priv;

  if (priv->applets)
    {
      g_hash_table_unref (priv->applets);
      priv->applets = NULL;
    }

  G_OBJECT_CLASS (mex_applet_manager_parent_class)->dispose (object);
}

static void
mex_applet_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_applet_manager_parent_class)->finalize (object);
}

static void
mex_applet_manager_class_init (MexAppletManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexAppletManagerPrivate));

  object_class->dispose = mex_applet_manager_dispose;
  object_class->finalize = mex_applet_manager_finalize;

  signals[APPLET_ADDED] =
    g_signal_new ("applet-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexAppletManagerClass, applet_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  MEX_TYPE_APPLET);

  signals[APPLET_REMOVED] =
    g_signal_new ("applet-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexAppletManagerClass, applet_removed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
mex_applet_manager_init (MexAppletManager *self)
{
  MexAppletManagerPrivate *priv = self->priv = APPLET_MANAGER_PRIVATE (self);

  MEX_DEBUG ("Applet manager initialised");
  priv->applets = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         NULL,
                                         (GDestroyNotify)g_object_unref);
}

MexAppletManager *
mex_applet_manager_get_default (void)
{
  static MexAppletManager *manager = NULL;

  if (!manager)
    manager = g_object_new (MEX_TYPE_APPLET_MANAGER, NULL);

  return manager;
}

GList *
mex_applet_manager_get_applets (MexAppletManager *manager)
{
  g_return_val_if_fail (MEX_IS_APPLET_MANAGER (manager), NULL);
  return g_hash_table_get_values (manager->priv->applets);
}

void
mex_applet_manager_add_applet (MexAppletManager *manager,
                               MexApplet        *applet)
{
  MexAppletManagerPrivate *priv;

  g_return_if_fail (MEX_IS_APPLET_MANAGER (manager));

  priv = manager->priv;

  if (g_hash_table_lookup (priv->applets, mex_applet_get_id (applet)))
    {
      g_warning (G_STRLOC ": Applet '%s' already exists",
                 mex_applet_get_id (applet));
      return;
    }

  MEX_DEBUG ("Added applet with id %s", mex_applet_get_id (applet));

  g_hash_table_insert (priv->applets,
                       (gpointer)mex_applet_get_id (applet),
                       g_object_ref_sink (applet));
  g_signal_emit (manager, signals[APPLET_ADDED], 0, applet);
}

void
mex_applet_manager_remove_applet (MexAppletManager *manager,
                                  const gchar      *id)
{
  MexAppletManagerPrivate *priv;

  g_return_if_fail (MEX_IS_APPLET_MANAGER (manager));

  priv = manager->priv;

  if (!g_hash_table_remove (priv->applets, id))
    {
      g_warning (G_STRLOC ": Applet '%s' is unrecognised", id);
      return;
    }

  g_signal_emit (manager, signals[APPLET_REMOVED], 0, id);
}
