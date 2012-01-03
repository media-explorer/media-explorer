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

#ifndef __MEX_COLUMN_VIEW_H__
#define __MEX_COLUMN_VIEW_H__

#include <glib-object.h>

#include <mex/mex-column.h>

G_BEGIN_DECLS

#define MEX_TYPE_COLUMN_VIEW mex_column_view_get_type()

#define MEX_COLUMN_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_COLUMN_VIEW, MexColumnView))

#define MEX_COLUMN_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_COLUMN_VIEW, MexColumnViewClass))

#define MEX_IS_COLUMN_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_COLUMN_VIEW))

#define MEX_IS_COLUMN_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_COLUMN_VIEW))

#define MEX_COLUMN_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_COLUMN_VIEW, MexColumnViewClass))

typedef struct _MexColumnView MexColumnView;
typedef struct _MexColumnViewClass MexColumnViewClass;
typedef struct _MexColumnViewPrivate MexColumnViewPrivate;

struct _MexColumnView
{
  MxWidget parent;

  MexColumnViewPrivate *priv;
};

struct _MexColumnViewClass
{
  MxWidgetClass parent_class;

  /* signals */
  void (* activated) (MexColumnView *self);
};

GType mex_column_view_get_type (void) G_GNUC_CONST;

ClutterActor *mex_column_view_new (const gchar *label,
                                    const gchar *icon_name);

MexColumn    *mex_column_view_get_column (MexColumnView *column);

const gchar  *mex_column_view_get_label (MexColumnView *column);
void          mex_column_view_set_label (MexColumnView *column,
                                         const gchar   *label);

const gchar * mex_column_view_get_icon_name (MexColumnView *column);
void          mex_column_view_set_icon_name (MexColumnView *column,
                                             const gchar   *name);

ClutterActor* mex_column_view_get_placeholder_actor (MexColumnView *column);
void          mex_column_view_set_placeholder_actor (MexColumnView *column,
                                                     ClutterActor  *label);

void          mex_column_view_set_focus (MexColumnView *column, gboolean focus);

G_END_DECLS

#endif /* __MEX_COLUMN_VIEW_H__ */
