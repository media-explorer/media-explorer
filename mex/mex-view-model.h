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

#ifndef __MEX_VIEW_MODEL_H__
#define __MEX_VIEW_MODEL_H__

#include <mex/mex-generic-model.h>

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

typedef enum
{
  MEX_FILTER_EQUAL,
  MEX_FILTER_NOT
} MexFilterCondition;

GType mex_view_model_get_type (void) G_GNUC_CONST;

MexModel *mex_view_model_new (MexModel *model);

void mex_view_model_set_limit (MexViewModel *self, guint limit);

void mex_view_model_set_offset (MexViewModel *self, guint offset);

void mex_view_model_set_start_content (MexViewModel *self, MexContent *content);
void mex_view_model_set_loop (MexViewModel *self, gboolean loop);

void mex_view_model_set_filter_by (MexViewModel       *model,
                                   MexContentMetadata  metadata_key,
                                   MexFilterCondition  filter_flags,
                                   const gchar        *value,
                                   ...);
void mex_view_model_set_group_by (MexViewModel        *model,
                                  MexContentMetadata  metadata_key);
void mex_view_model_set_order_by (MexViewModel       *model,
                                  MexContentMetadata  metadata_key,
                                  gboolean            descending);

gboolean mex_view_model_get_is_filtered (MexViewModel *model);

G_END_DECLS

#endif /* __MEX_VIEW_MODEL_H__ */
