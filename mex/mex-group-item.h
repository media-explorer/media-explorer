/* mex-group-item.h */

#ifndef _MEX_GROUP_ITEM_H
#define _MEX_GROUP_ITEM_H
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

#include <mex/mex-generic-content.h>

G_BEGIN_DECLS

#define MEX_TYPE_GROUP_ITEM mex_group_item_get_type()

#define MEX_GROUP_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_GROUP_ITEM, MexGroupItem))

#define MEX_GROUP_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_GROUP_ITEM, MexGroupItemClass))

#define MEX_IS_GROUP_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_GROUP_ITEM))

#define MEX_IS_GROUP_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_GROUP_ITEM))

#define MEX_GROUP_ITEM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_GROUP_ITEM, MexGroupItemClass))

typedef struct _MexGroupItem MexGroupItem;
typedef struct _MexGroupItemClass MexGroupItemClass;
typedef struct _MexGroupItemPrivate MexGroupItemPrivate;

struct _MexGroupItem
{
  MexGenericContent parent;

  MexGroupItemPrivate *priv;
};

struct _MexGroupItemClass
{
  MexGenericContentClass parent_class;
};

GType mex_group_item_get_type (void) G_GNUC_CONST;

MexGroupItem *
mex_group_item_new (const gchar *title,
                    MexModel    *source_model,
                    gint         filter_key,
                    const gchar *filter_value,
                    gint         second_filter_key,
                    const gchar *second_filter_value,
                    gint         group_key);

MexModel* mex_group_item_get_model (MexGroupItem *item);

G_END_DECLS

#endif /* _MEX_GROUP_ITEM_H */
