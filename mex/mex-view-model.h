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

#ifndef _MEX_VIEW_MODEL_H
#define _MEX_VIEW_MODEL_H

#include "mex-generic-model.h"

G_BEGIN_DECLS

#define MEX_TYPE_VIEW_MODEL mex_view_model_get_type()

#define MEX_VIEW_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_VIEW_MODEL, MexViewModel))

#define MEX_VIEW_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_VIEW_MODEL, MexViewModelClass))

#define MEX_IS_VIEW_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_VIEW_MODEL))

#define MEX_IS_VIEW_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_VIEW_MODEL))

#define MEX_VIEW_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_VIEW_MODEL, MexViewModelClass))

typedef struct _MexViewModel MexViewModel;
typedef struct _MexViewModelClass MexViewModelClass;
typedef struct _MexViewModelPrivate MexViewModelPrivate;

struct _MexViewModel
{
  MexGenericModel parent;

  MexViewModelPrivate *priv;
};

struct _MexViewModelClass
{
  MexGenericModelClass parent_class;
};

GType mex_view_model_get_type (void) G_GNUC_CONST;

MexModel *mex_view_model_new (MexModel *model);

void mex_view_model_start_at_content (MexViewModel *self,
                                      MexContent   *start_at_content,
                                      gboolean      loop);

void mex_view_model_start_at_offset (MexViewModel *self, guint offset);

void mex_view_model_start (MexViewModel *self);

void mex_view_model_stop (MexViewModel *self);

MexModel *mex_view_model_get_model (MexViewModel *self);

void mex_view_model_set_limit (MexViewModel *self, guint limit);

void mex_view_model_set_offset (MexViewModel *self, guint offset);


G_END_DECLS

#endif /* _MEX_VIEW_MODEL_H */
