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

#ifndef _MEX_TRACKER_MODEL_H
#define _MEX_TRACKER_MODEL_H

#include <glib-object.h>

#include <mex/mex-generic-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_TRACKER_MODEL mex_tracker_model_get_type()

#define MEX_TRACKER_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_TRACKER_MODEL, MexTrackerModel))

#define MEX_TRACKER_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_TRACKER_MODEL, MexTrackerModelClass))

#define MEX_IS_TRACKER_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_TRACKER_MODEL))

#define MEX_IS_TRACKER_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_TRACKER_MODEL))

#define MEX_TRACKER_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_TRACKER_MODEL, MexTrackerModelClass))

typedef struct _MexTrackerModel MexTrackerModel;
typedef struct _MexTrackerModelClass MexTrackerModelClass;
typedef struct _MexTrackerModelPrivate MexTrackerModelPrivate;

struct _MexTrackerModel
{
  MexGenericModel parent;

  MexTrackerModelPrivate *priv;
};

struct _MexTrackerModelClass
{
  MexGenericModelClass parent_class;
};

GType mex_tracker_model_get_type (void) G_GNUC_CONST;

MexTrackerModel *mex_tracker_model_new (void);

void mex_tracker_model_set_filter (MexTrackerModel *model,
                                   const gchar *sparql_filter);

void mex_tracker_model_set_metadata (MexTrackerModel *model,
                                     GList *metadata);

G_END_DECLS

#endif /* _MEX_TRACKER_MODEL_H */
