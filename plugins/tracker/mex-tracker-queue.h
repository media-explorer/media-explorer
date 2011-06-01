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

#ifndef _MEX_TRACKER_REQUEST_QUEUE_H_
#define _MEX_TRACKER_REQUEST_QUEUE_H_

#include <tracker-sparql.h>

/**/

typedef enum {
  MEX_TRACKER_OP_TYPE_QUERY,
  MEX_TRACKER_OP_TYPE_UPDATE,
} MexTrackerOpType;

typedef struct {
  MexTrackerOpType         type;
  GAsyncReadyCallback      callback;
  GCancellable            *cancel;
  gchar                   *request;
  /* const GList             *keys; */
  gpointer                 data;

  TrackerSparqlCursor *cursor;

  guint operation_id;

  guint skip;
  guint count;
  guint current;
} MexTrackerOp;

typedef struct _MexTrackerQueue MexTrackerQueue;

/**/

MexTrackerOp *mex_tracker_op_initiate_query (gchar               *request,
                                             GAsyncReadyCallback  callback,
                                             gpointer             data);

MexTrackerOp *mex_tracker_op_initiate_metadata (gchar               *request,
                                                GAsyncReadyCallback  callback,
                                                gpointer             data);

MexTrackerOp *mex_tracker_op_initiate_set_metadata (gchar               *request,
                                                    GAsyncReadyCallback  callback,
                                                    gpointer             data);

/**/

MexTrackerQueue *mex_tracker_queue_new (void);

void mex_tracker_queue_push (MexTrackerQueue *queue, MexTrackerOp *os);

void mex_tracker_queue_cancel (MexTrackerQueue *queue, MexTrackerOp *os);

void mex_tracker_queue_done (MexTrackerQueue *queue, MexTrackerOp *os);

#endif /* _MEX_TRACKER_REQUEST_QUEUE_H_ */
