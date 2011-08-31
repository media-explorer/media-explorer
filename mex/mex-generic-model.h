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

#ifndef __MEX_GENERIC_MODEL_H__
#define __MEX_GENERIC_MODEL_H__

#include <glib-object.h>

#include <mex/mex-content.h>
#include <mex/mex-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_GENERIC_MODEL                  \
  (mex_generic_model_get_type())
#define MEX_GENERIC_MODEL(obj)                          \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               MEX_TYPE_GENERIC_MODEL,  \
                               MexGenericModel))
#define MEX_GENERIC_MODEL_CLASS(klass)                  \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                    \
                            MEX_TYPE_GENERIC_MODEL,     \
                            MexGenericModelClass))
#define IS_MEX_GENERIC_MODEL(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MEX_TYPE_GENERIC_MODEL))
#define IS_MEX_GENERIC_MODEL_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                    \
                            MEX_TYPE_GENERIC_MODEL))
#define MEX_GENERIC_MODEL_GET_CLASS(obj)                \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              MEX_TYPE_GENERIC_MODEL,   \
                              MexGenericModelClass))

typedef struct _MexGenericModelPrivate MexGenericModelPrivate;
typedef struct _MexGenericModel      MexGenericModel;
typedef struct _MexGenericModelClass MexGenericModelClass;

struct _MexGenericModel
{
  GObject parent;

  MexGenericModelPrivate *priv;
};

struct _MexGenericModelClass
{
  GObjectClass parent_class;
};

GType mex_generic_model_get_type (void) G_GNUC_CONST;

MexModel *mex_generic_model_new (const gchar *title,
                                 const gchar *icon_name);

const gchar *mex_generic_model_get_title (MexGenericModel *model);

const gchar *mex_generic_model_get_icon_name (MexGenericModel *model);

G_END_DECLS

#endif /* __MEX_GENERIC_MODEL_H__ */
