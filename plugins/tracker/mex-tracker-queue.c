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
#include "mex-tracker-queue.h"
#include "mex-tracker-utils.h"

/**/

struct _MexTrackerQueue {
  GList      *head;
  GList      *tail;
  GHashTable *operations;
  GHashTable *operations_ids;
};

/**/

static void
mex_tracker_op_terminate (MexTrackerOp *os)
{
  if (os == NULL)
    return;

  if (os->cursor)
    g_object_unref (os->cursor);

  g_object_unref (os->cancel);
  g_free (os->request);

  g_slice_free (MexTrackerOp, os);
}

static MexTrackerOp *
mex_tracker_op_initiate (gchar               *request,
                         GAsyncReadyCallback  callback,
                         gpointer             data)
{
  MexTrackerOp *os = g_slice_new0 (MexTrackerOp);

  os->request  = request;
  os->callback = callback;
  os->data     = data;
  os->cancel   = g_cancellable_new ();

  return os;
}

MexTrackerOp *
mex_tracker_op_initiate_query (gchar               *request,
                               GAsyncReadyCallback  callback,
                               gpointer             data)
{
  MexTrackerOp *os = mex_tracker_op_initiate (request,
                                              callback,
                                              data);

  os->type         = MEX_TRACKER_OP_TYPE_QUERY;
  /* os->operation_id = operation_id; */

  /* g_hash_table_insert (mex_tracker_operations, */
  /*                      GSIZE_TO_POINTER (operation_id), os); */

  return os;
}

MexTrackerOp *
mex_tracker_op_initiate_metadata (gchar               *request,
                                  GAsyncReadyCallback  callback,
                                  gpointer             data)
{
  MexTrackerOp *os = mex_tracker_op_initiate (request,
                                              callback,
                                              data);

  os->type = MEX_TRACKER_OP_TYPE_QUERY;

  return os;
}

MexTrackerOp *
mex_tracker_op_initiate_set_metadata (gchar               *request,
                                      GAsyncReadyCallback  callback,
                                      gpointer             data)
{
  MexTrackerOp *os = mex_tracker_op_initiate (request,
                                              callback,
                                              data);

  os->type = MEX_TRACKER_OP_TYPE_UPDATE;

  return os;
}

static void mex_tracker_op_start (MexTrackerOp *os);

static void
mex_tracker_op_restart (MexTrackerConnection    *connection,
                        TrackerSparqlConnection *tconnection,
                        MexTrackerOp            *os)
{
  g_signal_handlers_disconnect_by_func (connection,
                                        mex_tracker_op_restart,
                                        os);
  mex_tracker_op_start (os);
}

static void
mex_tracker_op_start (MexTrackerOp *os)
{
  MexTrackerConnection *connection = mex_tracker_get_connection ();
  TrackerSparqlConnection *tconnection =
    mex_tracker_connection_get_connection (connection);

  if (!tconnection)
    {
      g_signal_connect (connection, "connected",
                        G_CALLBACK (mex_tracker_op_restart),
                        os);
      return;
    }

  switch (os->type) {
  case MEX_TRACKER_OP_TYPE_QUERY:
    tracker_sparql_connection_query_async (tconnection,
                                           os->request,
                                           NULL,
                                           os->callback,
                                           os);
    break;

  case MEX_TRACKER_OP_TYPE_UPDATE:
    tracker_sparql_connection_update_async (tconnection,
                                            os->request,
                                            G_PRIORITY_DEFAULT,
                                            NULL,
                                            os->callback,
                                            os);
    break;

  default:
    g_assert_not_reached();
    break;
  }
}

/**/

MexTrackerQueue *
mex_tracker_queue_new (void)
{
  MexTrackerQueue *queue = g_new0 (MexTrackerQueue, 1);

  queue->operations     = g_hash_table_new (g_direct_hash, g_direct_equal);
  queue->operations_ids = g_hash_table_new (g_direct_hash, g_direct_equal);

  return queue;
}

void
mex_tracker_queue_push (MexTrackerQueue *queue,
                        MexTrackerOp    *os)
{
  gboolean first = FALSE;

  queue->tail = g_list_append (queue->tail, os);
  if (queue->tail->next)
    queue->tail = queue->tail->next;
  else {
    queue->head = queue->tail;
    first = TRUE;
  }

  g_assert (queue->tail->next == NULL);

  g_hash_table_insert (queue->operations, os, queue->tail);
  if (os->operation_id != 0)
    g_hash_table_insert (queue->operations_ids,
                         GSIZE_TO_POINTER (os->operation_id), os);

  if (first)
    mex_tracker_op_start (os);
}

void
mex_tracker_queue_cancel (MexTrackerQueue *queue,
                          MexTrackerOp    *os)
{
  GList *item = g_hash_table_lookup (queue->operations, os);

  if (!item)
    return;

  g_cancellable_cancel (os->cancel);

  g_hash_table_remove (queue->operations, os);
  if (os->operation_id != 0)
    g_hash_table_remove (queue->operations_ids,
                         GSIZE_TO_POINTER (os->operation_id));

  if (item == queue->head) {
    queue->head = queue->head->next;
  }
  if (item == queue->tail) {
    queue->tail = queue->tail->prev;
  }

  if (item->prev)
    item->prev->next = item->next;
  if (item->next)
    item->next->prev = item->prev;

  item->next = NULL;
  item->prev = NULL;
  g_list_free (item);
}

void
mex_tracker_queue_done (MexTrackerQueue *queue,
                        MexTrackerOp    *os)
{
  MexTrackerOp *next_os;

  mex_tracker_queue_cancel (queue, os);
  mex_tracker_op_terminate (os);

  if (!queue->head)
    return;

  next_os = queue->head->data;

  mex_tracker_op_start (next_os);
}
