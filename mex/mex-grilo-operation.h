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

#ifndef _MEX_GRILO_OPERATION_H
#define _MEX_GRILO_OPERATION_H

#include <glib-object.h>

#include "mex-content.h"
#include "mex-metadatas.h"

G_BEGIN_DECLS

#define MEX_TYPE_GRILO_OPERATION mex_grilo_operation_get_type()

#define MEX_GRILO_OPERATION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_GRILO_OPERATION, MexGriloOperation))

#define MEX_GRILO_OPERATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_GRILO_OPERATION, MexGriloOperationClass))

#define MEX_IS_GRILO_OPERATION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_GRILO_OPERATION))

#define MEX_IS_GRILO_OPERATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_GRILO_OPERATION))

#define MEX_GRILO_OPERATION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_GRILO_OPERATION, MexGriloOperationClass))

typedef struct _MexGriloOperation MexGriloOperation;
typedef struct _MexGriloOperationClass MexGriloOperationClass;
typedef struct _MexGriloOperationPrivate MexGriloOperationPrivate;

struct _MexGriloOperation
{
  GObject parent;

  MexGriloOperationPrivate *priv;
};

struct _MexGriloOperationClass
{
  GObjectClass parent_class;
};

typedef MexContent *(*MexCreateContentCb) (gpointer data);

GType mex_grilo_operation_get_type (void) G_GNUC_CONST;

MexGriloOperation *mex_grilo_operation_new (void);

void mex_grilo_operation_search (MexGriloOperation *operation,
                                 const gchar       *source_id,
                                 const gchar       *text,
                                 MexMetadatas      *metadatas);

void mex_grilo_operation_search_all (MexGriloOperation *operation,
                                     const gchar       *text,
                                     MexMetadatas      *metadatas);

void mex_grilo_operation_metadata (MexGriloOperation *operation,
                                   MexContent        *content,
                                   MexMetadatas      *metadatas);

void mex_grilo_operation_stop (MexGriloOperation *operation);

void mex_grilo_operation_set_create_content_cb (MexGriloOperation  *operation,
                                                MexCreateContentCb  callback,
                                                gpointer            data);

/* TODO: move to private */
void mex_grilo_operation_start (MexGriloOperation *operation);


G_END_DECLS

#endif /* _MEX_GRILO_OPERATION_H */
