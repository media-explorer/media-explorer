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

#ifndef __MEX_CONTENT_BOX_H__
#define __MEX_CONTENT_BOX_H__

#include <glib-object.h>
#include <mx/mx.h>
#include <mex/mex-content.h>

G_BEGIN_DECLS

#define MEX_TYPE_CONTENT_BOX mex_content_box_get_type()

#define MEX_CONTENT_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_CONTENT_BOX, MexContentBox))

#define MEX_CONTENT_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_CONTENT_BOX, MexContentBoxClass))

#define MEX_IS_CONTENT_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_CONTENT_BOX))

#define MEX_IS_CONTENT_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_CONTENT_BOX))

#define MEX_CONTENT_BOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_CONTENT_BOX, MexContentBoxClass))

typedef struct _MexContentBox MexContentBox;
typedef struct _MexContentBoxClass MexContentBoxClass;
typedef struct _MexContentBoxPrivate MexContentBoxPrivate;

struct _MexContentBox
{
  MxWidget parent;

  MexContentBoxPrivate *priv;
};

struct _MexContentBoxClass
{
  MxWidgetClass parent_class;
};

GType mex_content_box_get_type (void) G_GNUC_CONST;

ClutterActor *mex_content_box_new (void);

gboolean mex_content_box_get_open (MexContentBox *box);

void     mex_content_box_set_important (MexContentBox *box, gboolean important);
gboolean mex_content_box_get_important (MexContentBox *box);

G_END_DECLS

#endif /* __MEX_CONTENT_BOX_H__ */
