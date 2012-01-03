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

#ifndef __MEX_ACTION_BUTTON_H__
#define __MEX_ACTION_BUTTON_H__

#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_ACTION_BUTTON mex_action_button_get_type()

#define MEX_ACTION_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_ACTION_BUTTON, MexActionButton))

#define MEX_ACTION_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_ACTION_BUTTON, MexActionButtonClass))

#define MEX_IS_ACTION_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_ACTION_BUTTON))

#define MEX_IS_ACTION_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_ACTION_BUTTON))

#define MEX_ACTION_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_ACTION_BUTTON, MexActionButtonClass))

typedef struct _MexActionButton MexActionButton;
typedef struct _MexActionButtonClass MexActionButtonClass;
typedef struct _MexActionButtonPrivate MexActionButtonPrivate;

struct _MexActionButton
{
  MxButton parent;

  MexActionButtonPrivate *priv;
};

struct _MexActionButtonClass
{
  MxButtonClass parent_class;
};

GType mex_action_button_get_type (void) G_GNUC_CONST;

ClutterActor *mex_action_button_new (MxAction *action);

G_END_DECLS

#endif /* __MEX_ACTION_BUTTON_H__ */
