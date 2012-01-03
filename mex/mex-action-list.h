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

#ifndef __MEX_ACTION_LIST_H__
#define __MEX_ACTION_LIST_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_ACTION_LIST mex_action_list_get_type()

#define MEX_ACTION_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_ACTION_LIST, MexActionList))

#define MEX_ACTION_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_ACTION_LIST, MexActionListClass))

#define MEX_IS_ACTION_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_ACTION_LIST))

#define MEX_IS_ACTION_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_ACTION_LIST))

#define MEX_ACTION_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_ACTION_LIST, MexActionListClass))

typedef struct _MexActionList MexActionList;
typedef struct _MexActionListClass MexActionListClass;
typedef struct _MexActionListPrivate MexActionListPrivate;

struct _MexActionList
{
  MxWidget parent;

  MexActionListPrivate *priv;
};

struct _MexActionListClass
{
  MxWidgetClass parent_class;
};

GType mex_action_list_get_type (void) G_GNUC_CONST;

ClutterActor *mex_action_list_new (void);

void mex_action_list_refresh (MexActionList *action_list);

G_END_DECLS

#endif /* __MEX_ACTION_LIST_H__ */
