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


#ifndef __MEX_COLUMN_H__
#define __MEX_COLUMN_H__

#include <mx/mx.h>
#include <mex/mex-utils.h>

G_BEGIN_DECLS

#define MEX_TYPE_COLUMN                                                 \
   (mex_column_get_type())
#define MEX_COLUMN(obj)                                                 \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                MEX_TYPE_COLUMN,                        \
                                MexColumn))
#define MEX_COLUMN_CLASS(klass)                                         \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             MEX_TYPE_COLUMN,                           \
                             MexColumnClass))
#define MEX_IS_COLUMN(obj)                                              \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                MEX_TYPE_COLUMN))
#define MEX_IS_COLUMN_CLASS(klass)                                      \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             MEX_TYPE_COLUMN))
#define MEX_COLUMN_GET_CLASS(obj)                                       \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               MEX_TYPE_COLUMN,                         \
                               MexColumnClass))

typedef struct _MexColumnPrivate MexColumnPrivate;
typedef struct _MexColumn      MexColumn;
typedef struct _MexColumnClass MexColumnClass;

struct _MexColumn
{
  MxWidget parent;

  MexColumnPrivate *priv;
};

struct _MexColumnClass
{
  MxWidgetClass parent_class;

  /* signals */
  void (* activated) (MexColumn *self);
};

GType mex_column_get_type (void) G_GNUC_CONST;
ClutterActor *mex_column_new (const char *label,
                              const char *icon_name);

const gchar * mex_column_get_label (MexColumn *column);
void          mex_column_set_label (MexColumn *column, const gchar *label);

const gchar * mex_column_get_icon_name (MexColumn *column);
void          mex_column_set_icon_name (MexColumn *column, const gchar *name);

ClutterActor* mex_column_get_placeholder_actor (MexColumn *column);
void          mex_column_set_placeholder_actor (MexColumn *column, ClutterActor *label);

void             mex_column_set_sort_func (MexColumn        *column,
                                           MexActorSortFunc  func,
                                           gpointer          userdata);
MexActorSortFunc mex_column_get_sort_func (MexColumn        *column,
                                           gpointer         *userdata);

void     mex_column_set_collapse_on_focus (MexColumn *column,
                                           gboolean   collapse);
gboolean mex_column_get_collapse_on_focus (MexColumn *column);

G_END_DECLS

#endif /* __MEX_COLUMN_H__ */
