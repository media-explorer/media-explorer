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

#ifndef _MEX_QUEUE_BUTTON_H
#define _MEX_QUEUE_BUTTON_H

#include <glib-object.h>
#include <mx/mx.h>
#include "mex-action-button.h"

G_BEGIN_DECLS

#define MEX_TYPE_QUEUE_BUTTON mex_queue_button_get_type()

#define MEX_QUEUE_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_QUEUE_BUTTON, MexQueueButton))

#define MEX_QUEUE_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_QUEUE_BUTTON, MexQueueButtonClass))

#define MEX_IS_QUEUE_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_QUEUE_BUTTON))

#define MEX_IS_QUEUE_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_QUEUE_BUTTON))

#define MEX_QUEUE_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_QUEUE_BUTTON, MexQueueButtonClass))

typedef struct _MexQueueButton MexQueueButton;
typedef struct _MexQueueButtonClass MexQueueButtonClass;
typedef struct _MexQueueButtonPrivate MexQueueButtonPrivate;

struct _MexQueueButton
{
  MexActionButton parent;

  MexQueueButtonPrivate *priv;
};

struct _MexQueueButtonClass
{
  MexActionButtonClass parent_class;
};

GType mex_queue_button_get_type (void) G_GNUC_CONST;

ClutterActor *mex_queue_button_new (void);

G_END_DECLS

#endif /* _MEX_QUEUE_BUTTON_H */
