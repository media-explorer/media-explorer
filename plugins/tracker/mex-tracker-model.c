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

#include "mex-debug.h"
#include "mex-tracker-model.h"
#include "mex-tracker-metadatas.h"
#include "mex-tracker-notifications.h"
#include "mex-tracker-queue.h"
#include "mex-tracker-utils.h"

G_DEFINE_TYPE (MexTrackerModel, mex_tracker_model, MEX_TYPE_GENERIC_MODEL)

#define TRACKER_MODEL_PRIVATE(o)                                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TRACKER_MODEL,         \
                                MexTrackerModelPrivate))

#define TRACKER_QUERY_REQUEST                   \
  "SELECT %s "                                  \
  "WHERE { %s } "                               \
  "ORDER BY DESC(nfo:fileLastModified(?u)) "    \
  "OFFSET %u "                                  \
  "LIMIT %u"

enum
{
  PROP_0,

  PROP_FILTER,
  PROP_MAX_RESULTS,
  PROP_INITIAL_METADATAS,
  PROP_COMPLETE_METADATAS
};

struct _MexTrackerModelPrivate
{
  gchar *sparql_filter;
  guint  max_results;

  MexTrackerMetadatas *initial_metadatas;
  MexTrackerMetadatas *complete_metadatas;
};

static void
mex_tracker_model_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MexTrackerModelPrivate *priv = MEX_TRACKER_MODEL (object)->priv;

  switch (property_id)
    {
    case PROP_FILTER:
      g_value_set_string (value, priv->sparql_filter);
      break;

    case PROP_MAX_RESULTS:
      g_value_set_uint (value, priv->max_results);
      break;

    case PROP_INITIAL_METADATAS:
      g_value_set_object (value, priv->initial_metadatas);
      break;

    case PROP_COMPLETE_METADATAS:
      g_value_set_object (value, priv->complete_metadatas);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tracker_model_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MexTrackerModel        *model = (MexTrackerModel *) object;
  MexTrackerModelPrivate *priv  = model->priv;

  switch (property_id)
    {
    case PROP_FILTER:
      mex_tracker_model_set_filter (model, g_value_get_string (value));
      break;

    case PROP_MAX_RESULTS:
      priv->max_results = g_value_get_uint (value);
      break;

    case PROP_INITIAL_METADATAS:
      if (priv->initial_metadatas)
        g_object_unref (priv->initial_metadatas);
      priv->initial_metadatas = g_object_ref_sink (g_value_get_object (value));
      break;

    case PROP_COMPLETE_METADATAS:
      if (priv->complete_metadatas)
        g_object_unref (priv->initial_metadatas);
      priv->complete_metadatas = g_object_ref_sink (g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tracker_model_dispose (GObject *object)
{
  MexTrackerModel        *model = (MexTrackerModel *) object;
  MexTrackerModelPrivate *priv  = model->priv;

  if (priv->sparql_filter)
    {
      mex_tracker_model_set_filter (model, NULL);
    }

  if (priv->initial_metadatas)
    {
      g_object_unref (priv->initial_metadatas);
      priv->initial_metadatas = NULL;
    }

  if (priv->complete_metadatas)
    {
      g_object_unref (priv->complete_metadatas);
      priv->complete_metadatas = NULL;
    }

  G_OBJECT_CLASS (mex_tracker_model_parent_class)->dispose (object);
}

static void
mex_tracker_model_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_model_parent_class)->finalize (object);
}

static void
mex_tracker_model_class_init (MexTrackerModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexTrackerModelPrivate));

  object_class->get_property = mex_tracker_model_get_property;
  object_class->set_property = mex_tracker_model_set_property;
  object_class->dispose = mex_tracker_model_dispose;
  object_class->finalize = mex_tracker_model_finalize;

  pspec = g_param_spec_string ("filter", "Filter",
                               "Sparql filter to use",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_FILTER, pspec);

  pspec = g_param_spec_uint ("max-results", "Max results",
                             "Maximum results to be returned by Tracker",
                             0, G_MAXUINT, G_MAXUINT,
                             G_PARAM_READWRITE |
                             G_PARAM_STATIC_STRINGS |
                             G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_MAX_RESULTS, pspec);

  pspec = g_param_spec_object ("initial-metadatas", "Initial metadatas",
                               "Metadatas to query in first request "
                               "to populate the model",
                               MEX_TYPE_TRACKER_METADATAS,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_INITIAL_METADATAS, pspec);

  pspec = g_param_spec_object ("complete-metadatas", "Complete metadatas",
                               "Metadatas to query in a second request "
                               "to populate one item's metadatas",
                               MEX_TYPE_TRACKER_METADATAS,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_COMPLETE_METADATAS, pspec);
}

static void
mex_tracker_model_init (MexTrackerModel *self)
{
  self->priv = TRACKER_MODEL_PRIVATE (self);

  /* self->priv->max_count = G_MAXUINT; */
}

MexTrackerModel *
mex_tracker_model_new (MexTrackerMetadatas *initial_metadatas,
                       MexTrackerMetadatas  *complete_metadatas)
{
  return g_object_new (MEX_TYPE_TRACKER_MODEL,
                       "initial-metadatas", initial_metadatas,
                       "complete-metadatas", complete_metadatas,
                       NULL);
}

static void
mex_tracker_model_notif_cb (MexTrackerNotifications *notifs,
                            GArray                  *items,
                            MexTrackerModel         *model)
{
  g_message ("notify model=%p items=%u !", model, items->len);
}

void
mex_tracker_model_set_filter (MexTrackerModel *model,
                              const gchar *sparql_filter)

{
  MexTrackerModelPrivate *priv;
  MexTrackerNotifications *notifs;

  g_return_if_fail (model != NULL);

  priv = model->priv;
  notifs = mex_tracker_notifications_get ();

  if (priv->sparql_filter)
    {
      /* Stop dbus callback */
      g_signal_handlers_disconnect_by_func (notifs,
            G_CALLBACK (mex_tracker_model_notif_cb),
            model);

      g_free (priv->sparql_filter);
      priv->sparql_filter = NULL;
    }

  if (sparql_filter)
    {
      priv->sparql_filter = g_strdup (sparql_filter);

      /* Reinstall dbus callback*/
      g_signal_connect (notifs, "updated",
                        G_CALLBACK (mex_tracker_model_notif_cb),
                        model);
    }
}

static void
mex_tracker_model_query_result_cb (GObject      *source_object,
                                   GAsyncResult *result,
                                   MexTrackerOp *os)
{
  GError                 *error = NULL;
  MexContent             *content;
  MexTrackerModel        *model = (MexTrackerModel *) os->data;
  MexTrackerModelPrivate *priv  = model->priv;

  if (!tracker_sparql_cursor_next_finish (os->cursor,
                                          result,
                                          &error))
    {
      if (error != NULL)
        {
          g_warning ("\tCould not execute sparql query model=%p/filter=%s: %s",
                     model, priv->sparql_filter, error->message);
          g_error_free (error);
        }
      else
        {
          g_message ("\tEnd of parsing model=%p filter='%s'",
                     model, priv->sparql_filter);
        }

      mex_tracker_queue_done (mex_tracker_get_queue (), os);
      return;
    }

  content = mex_tracker_build_content (os->cursor);

  if (content)
    mex_model_add_content ((MexModel *) model, content);

  tracker_sparql_cursor_next_async (os->cursor, os->cancel,
             (GAsyncReadyCallback) mex_tracker_model_query_result_cb,
             (gpointer) os);
}

static void
mex_tracker_model_query_cb (GObject      *source_object,
                            GAsyncResult *result,
                            MexTrackerOp *os)
{
  GError *error = NULL;
  MexTrackerModel *model = (MexTrackerModel *) os->data;
  MexTrackerModelPrivate *priv = model->priv;
  TrackerSparqlConnection *connection;

  connection =
    mex_tracker_connection_get_connection (mex_tracker_get_connection ());

  os->cursor =
    tracker_sparql_connection_query_finish (connection, result, &error);

  if (error)
    {
      MEX_WARN (TRACKER,
                "Could not execute sparql query model=%p/filter=%s: %s",
                model, priv->sparql_filter, error->message);

      g_error_free (error);
      mex_tracker_queue_done (mex_tracker_get_queue (), os);

      return;
    }

  /* Start parsing results */
  tracker_sparql_cursor_next_async (os->cursor, NULL,
             (GAsyncReadyCallback) mex_tracker_model_query_result_cb,
             (gpointer) os);
}

void
mex_tracker_model_start (MexTrackerModel *model)
{
  MexTrackerModelPrivate *priv = model->priv;
  gchar                  *sparql_select, *sparql_final;
  MexTrackerOp           *os;
  const GList            *keys;

  if (!priv->sparql_filter)
    {
      MEX_WARN (MEX_DEBUG_TRACKER, "no filter to query with");
      return;
    }

  keys = mex_tracker_metadatas_get_metadata_list (priv->initial_metadatas);

  sparql_select = mex_tracker_get_select_string (keys);
  sparql_final = g_strdup_printf (TRACKER_QUERY_REQUEST,
                                  sparql_select,
                                  priv->sparql_filter,
                                  0,
                                  priv->max_results);
  g_free (sparql_select);

  g_message ("select : '%s'", sparql_final);

  os = mex_tracker_op_initiate_query (sparql_final,
                                      (GAsyncReadyCallback) mex_tracker_model_query_cb,
                                      model);

  os->skip  = 0;
  os->count = priv->max_results;
  os->data  = model;

  mex_tracker_queue_push (mex_tracker_get_queue (), os);
}
