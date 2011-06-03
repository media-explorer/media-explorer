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

#include "mex-tracker-notifications.h"
#include "mex-tracker-marshal.h"

#include <gio/gio.h>
#include <tracker-sparql.h>

G_DEFINE_TYPE (MexTrackerNotifications,
               mex_tracker_notifications,
               G_TYPE_OBJECT)

#define TRACKER_NOTIFICATIONS_PRIVATE(o)                                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MEX_TYPE_TRACKER_NOTIFICATIONS,         \
                                MexTrackerNotificationsPrivate))

enum
{
  UPDATED,

  LAST_SIGNAL
};

struct _MexTrackerNotificationsPrivate
{
  GDBusConnection *connection;
  guint            signal_id;
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
tracker_dbus_signal_cb (GDBusConnection         *connection,
                        const gchar             *sender_name,
                        const gchar             *object_path,
                        const gchar             *interface_name,
                        const gchar             *signal_name,
                        GVariant                *parameters,
                        MexTrackerNotifications *notifications)

{
  gchar *class_name;
  GArray *items = g_array_new (FALSE, FALSE, sizeof (guint));
  GHashTable *table = g_hash_table_new (g_direct_hash, g_direct_equal);
  gint graph = 0, subject = 0, predicate = 0, object = 0;
  GVariantIter *iter1, *iter2;

  g_variant_get (parameters, "(&sa(iiii)a(iiii))", &class_name, &iter1, &iter2);

  /* MEX_DEBUG ("Tracker update event for class=%s ins=%lu del=%lu evt=%p", */
  /*            class_name, */
  /*            (unsigned long) g_variant_iter_n_children (iter1), */
  /*            (unsigned long) g_variant_iter_n_children (iter2), */
  /*            evt); */

  /* Process deleted items */
  while (g_variant_iter_loop (iter1, "(iiii)", &graph,
                              &subject, &predicate, &object))
    {
      g_hash_table_insert (table, GINT_TO_POINTER (subject), (gpointer) 0x42);
      g_array_append_val (items, subject);
    }

  while (g_variant_iter_loop (iter2, "(iiii)", &graph,
                              &subject, &predicate, &object))
    {
      if (g_hash_table_lookup (table, GINT_TO_POINTER (subject)) == NULL)
        g_array_append_val (items, subject);
    }

  g_variant_iter_free (iter1);
  g_variant_iter_free (iter2);
  g_hash_table_unref (table);

  g_signal_emit (notifications, signals[UPDATED], 0, items);

  g_array_unref (items);
}

static void
mex_tracker_notifications_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tracker_notifications_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tracker_notifications_dispose (GObject *object)
{
  MexTrackerNotificationsPrivate *priv = TRACKER_NOTIFICATIONS_PRIVATE (object);

  if (priv->signal_id)
    {
      g_dbus_connection_signal_unsubscribe (priv->connection, priv->signal_id);
      priv->signal_id = 0;
    }

  if (priv->connection)
    {
      g_object_unref (priv->connection);
      priv->connection = NULL;
    }

  G_OBJECT_CLASS (mex_tracker_notifications_parent_class)->dispose (object);
}

static void
mex_tracker_notifications_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_notifications_parent_class)->finalize (object);
}

static void
mex_tracker_notifications_class_init (MexTrackerNotificationsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexTrackerNotificationsPrivate));

  object_class->get_property = mex_tracker_notifications_get_property;
  object_class->set_property = mex_tracker_notifications_set_property;
  object_class->dispose = mex_tracker_notifications_dispose;
  object_class->finalize = mex_tracker_notifications_finalize;

  signals[UPDATED] =
    g_signal_new ("updated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  mex_tracker_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1, G_TYPE_ARRAY);

}

static void
mex_tracker_notifications_init (MexTrackerNotifications *self)
{
  MexTrackerNotificationsPrivate *priv = TRACKER_NOTIFICATIONS_PRIVATE (self);

  self->priv = priv;

  priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  priv->signal_id =
    g_dbus_connection_signal_subscribe (priv->connection,
                                        TRACKER_DBUS_SERVICE,
                                        TRACKER_DBUS_INTERFACE_RESOURCES,
                                        "GraphUpdated",
                                        TRACKER_DBUS_OBJECT_RESOURCES,
                                        NULL,
                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                        (GDBusSignalCallback) tracker_dbus_signal_cb,
                                        self,
                                        NULL);

}

MexTrackerNotifications *
mex_tracker_notifications_get (void)
{
  static MexTrackerNotifications *notifications = NULL;

  if (!notifications)
    {
      notifications = g_object_new (MEX_TYPE_TRACKER_NOTIFICATIONS, NULL);
      g_object_ref_sink (notifications);
    }

  return notifications;
}
