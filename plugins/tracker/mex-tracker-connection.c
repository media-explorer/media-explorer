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

#include "mex-tracker-connection.h"
#include "mex-tracker-marshal.h"

G_DEFINE_TYPE (MexTrackerConnection, mex_tracker_connection, G_TYPE_OBJECT)

#define TRACKER_CONNECTION_PRIVATE(o)                                   \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MEX_TYPE_TRACKER_CONNECTION,            \
                                MexTrackerConnectionPrivate))

enum
{
  CONNECTED,

  LAST_SIGNAL
};

struct _MexTrackerConnectionPrivate
{
  TrackerSparqlConnection *connection;
};

static void mex_tracker_get_connection_cb (GObject      *object,
                                           GAsyncResult *res,
                                           gpointer      data);

static guint signals[LAST_SIGNAL] = { 0, };

static void
mex_tracker_connection_get_property (GObject    *object,
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
mex_tracker_connection_set_property (GObject      *object,
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
mex_tracker_connection_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_connection_parent_class)->dispose (object);
}

static void
mex_tracker_connection_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_connection_parent_class)->finalize (object);
}

static void
mex_tracker_connection_class_init (MexTrackerConnectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexTrackerConnectionPrivate));

  object_class->get_property = mex_tracker_connection_get_property;
  object_class->set_property = mex_tracker_connection_set_property;
  object_class->dispose = mex_tracker_connection_dispose;
  object_class->finalize = mex_tracker_connection_finalize;

  signals[CONNECTED] =
    g_signal_new ("connected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  mex_tracker_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1, TRACKER_SPARQL_TYPE_CONNECTION);

}

static void
mex_tracker_connection_init (MexTrackerConnection *self)
{
  self->priv = TRACKER_CONNECTION_PRIVATE (self);

  tracker_sparql_connection_get_async (NULL,
                                       mex_tracker_get_connection_cb,
                                       self);
}

MexTrackerConnection *
mex_tracker_connection_new (void)
{
  return g_object_new (MEX_TYPE_TRACKER_CONNECTION, NULL);
}

static void
mex_tracker_get_connection_cb (GObject      *object,
                               GAsyncResult *res,
                               gpointer      data)
{
  GError *error = NULL;
  MexTrackerConnection *connection = (MexTrackerConnection *) data;
  MexTrackerConnectionPrivate *priv = connection->priv;

  priv->connection = tracker_sparql_connection_get_finish (res, &error);

  if (error)
    {
      g_warning ("Could not get connection to Tracker: %s",
                 error->message);
      g_error_free (error);
      return;
    }

  g_signal_emit (connection, signals[CONNECTED], 0, priv->connection);
}

TrackerSparqlConnection *
mex_tracker_connection_get_connection (MexTrackerConnection *connection)
{
  MexTrackerConnectionPrivate *priv;

  g_return_val_if_fail (connection != NULL, NULL);

  priv = connection->priv;

  return priv->connection;
}
