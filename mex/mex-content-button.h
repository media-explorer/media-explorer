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

#ifndef __MEX_CONTENT_BUTTON_H__
#define __MEX_CONTENT_BUTTON_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_CONTENT_BUTTON mex_content_button_get_type()

#define MEX_CONTENT_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_CONTENT_BUTTON, MexContentButton))

#define MEX_CONTENT_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_CONTENT_BUTTON, MexContentButtonClass))

#define MEX_IS_CONTENT_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_CONTENT_BUTTON))

#define MEX_IS_CONTENT_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_CONTENT_BUTTON))

#define MEX_CONTENT_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_CONTENT_BUTTON, MexContentButtonClass))

typedef struct _MexContentButton MexContentButton;
typedef struct _MexContentButtonClass MexContentButtonClass;
typedef struct _MexContentButtonPrivate MexContentButtonPrivate;

struct _MexContentButton
{
  MxButton parent;

  MexContentButtonPrivate *priv;
};

struct _MexContentButtonClass
{
  MxButtonClass parent_class;
};

GType mex_content_button_get_type (void) G_GNUC_CONST;

ClutterActor *mex_content_button_new (void);

G_END_DECLS

#endif /* __MEX_CONTENT_BUTTON_H__ */
