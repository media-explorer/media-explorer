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

#ifndef _MEX_QUEUE_MODEL_H
#define _MEX_QUEUE_MODEL_H

#include <glib-object.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_QUEUE_MODEL mex_queue_model_get_type()

#define MEX_QUEUE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_QUEUE_MODEL, MexQueueModel))

#define MEX_QUEUE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_QUEUE_MODEL, MexQueueModelClass))

#define MEX_IS_QUEUE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_QUEUE_MODEL))

#define MEX_IS_QUEUE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_QUEUE_MODEL))

#define MEX_QUEUE_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_QUEUE_MODEL, MexQueueModelClass))

typedef struct _MexQueueModel MexQueueModel;
typedef struct _MexQueueModelClass MexQueueModelClass;
typedef struct _MexQueueModelPrivate MexQueueModelPrivate;

struct _MexQueueModel
{
  MexGenericModel parent;

  MexQueueModelPrivate *priv;
};

struct _MexQueueModelClass
{
  MexGenericModelClass parent_class;
};

GType mex_queue_model_get_type (void) G_GNUC_CONST;

MexModel *mex_queue_model_dup_singleton ();

G_END_DECLS

#endif /* _MEX_QUEUE_MODEL_H */
