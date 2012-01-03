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

#ifndef __MEX_AGGREGATE_MODEL_H__
#define __MEX_AGGREGATE_MODEL_H__

#include <glib-object.h>
#include <mex/mex-generic-model.h>
#include <mex/mex-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_AGGREGATE_MODEL mex_aggregate_model_get_type()

#define MEX_AGGREGATE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_AGGREGATE_MODEL, MexAggregateModel))

#define MEX_AGGREGATE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_AGGREGATE_MODEL, MexAggregateModelClass))

#define MEX_IS_AGGREGATE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_AGGREGATE_MODEL))

#define MEX_IS_AGGREGATE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_AGGREGATE_MODEL))

#define MEX_AGGREGATE_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_AGGREGATE_MODEL, MexAggregateModelClass))

typedef struct _MexAggregateModel MexAggregateModel;
typedef struct _MexAggregateModelClass MexAggregateModelClass;
typedef struct _MexAggregateModelPrivate MexAggregateModelPrivate;

struct _MexAggregateModel
{
  MexGenericModel parent;

  MexAggregateModelPrivate *priv;
};

struct _MexAggregateModelClass
{
  MexGenericModelClass parent_class;

  /* signals, not vfuncs */
  void (*model_added)   (MexAggregateModel *aggregate,
                         MexModel          *model);
  void (*model_removed) (MexAggregateModel *aggregate,
                         MexModel          *model);
};

GType mex_aggregate_model_get_type (void) G_GNUC_CONST;

MexModel *mex_aggregate_model_new (void);


void mex_aggregate_model_add_model (MexAggregateModel *aggregate,
                                    MexModel          *model);

void mex_aggregate_model_remove_model (MexAggregateModel *aggregate,
                                       MexModel          *model);

void mex_aggregate_model_clear (MexAggregateModel *aggregate);

const GList *mex_aggregate_model_get_models (MexAggregateModel *aggregate);

MexModel *mex_aggregate_model_get_model_for_content (MexAggregateModel *aggregate,
                                                     MexContent        *content);

G_END_DECLS

#endif /* __MEX_AGGREGATE_MODEL_H__ */
