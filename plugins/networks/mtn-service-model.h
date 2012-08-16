/*
 * mex-networks - Connection Manager UI for Media Explorer 
 * Copyright © 2010-2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifndef _MTN_SERVICE_MODEL
#define _MTN_SERVICE_MODEL

#include <clutter/clutter.h>

#include "mtn-connman.h"

G_BEGIN_DECLS

enum {
    MTN_SERVICE_MODEL_COL_INDEX = 0,
    MTN_SERVICE_MODEL_COL_PRESENT,
    MTN_SERVICE_MODEL_COL_HIDDEN,
    MTN_SERVICE_MODEL_COL_OBJECT_PATH,
    MTN_SERVICE_MODEL_COL_WIDGET,

    MTN_SERVICE_MODEL_COUNT_COLS
};

#define MTN_TYPE_SERVICE_MODEL mtn_service_model_get_type()

#define MTN_SERVICE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTN_TYPE_SERVICE_MODEL, MtnServiceModel))

#define MTN_SERVICE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MTN_TYPE_SERVICE_MODEL, MtnServiceModelClass))

#define MTN_IS_SERVICE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTN_TYPE_SERVICE_MODEL))

#define MTN_IS_SERVICE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MTN_TYPE_SERVICE_MODEL))

#define MTN_SERVICE_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MTN_TYPE_SERVICE_MODEL, MtnServiceModelClass))

typedef struct _MtnServiceModelPrivate MtnServiceModelPrivate;

typedef struct {
  ClutterListModel parent;
  MtnServiceModelPrivate *priv;
} MtnServiceModel;

typedef struct {
  ClutterListModelClass parent_class;
} MtnServiceModelClass;

GType            mtn_service_model_get_type    (void);

void             mtn_service_model_set_connman (MtnServiceModel *model, MtnConnman *connman);

MtnServiceModel* mtn_service_model_new         (MtnConnman *connman);

G_END_DECLS

#endif /* _MTN_SERVICE_MODEL */
