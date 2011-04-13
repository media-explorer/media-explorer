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


#ifndef _MEX_PROXY_MODEL_H
#define _MEX_PROXY_MODEL_H

#include <glib-object.h>
#include <mex/mex-generic-model.h>
#include <mex/mex-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_PROXY_MODEL mex_proxy_model_get_type()

#define MEX_PROXY_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_PROXY_MODEL, MexProxyModel))

#define MEX_PROXY_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_PROXY_MODEL, MexProxyModelClass))

#define MEX_IS_PROXY_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_PROXY_MODEL))

#define MEX_IS_PROXY_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_PROXY_MODEL))

#define MEX_PROXY_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_PROXY_MODEL, MexProxyModelClass))

typedef struct _MexProxyModel MexProxyModel;
typedef struct _MexProxyModelClass MexProxyModelClass;
typedef struct _MexProxyModelPrivate MexProxyModelPrivate;

struct _MexProxyModel
{
  MexGenericModel parent;

  MexProxyModelPrivate *priv;
};

struct _MexProxyModelClass
{
  MexGenericModelClass parent_class;
};

GType mex_proxy_model_get_type (void) G_GNUC_CONST;

MexModel *mex_proxy_model_new (void);

MexModel *mex_proxy_model_get_model (MexProxyModel *proxy);
void      mex_proxy_model_set_model (MexProxyModel *proxy,
                                     MexModel      *model);

G_END_DECLS

#endif /* _MEX_PROXY_MODEL_H */
