/*
 * mex-networks - Connection Manager UI for Media Explorer 
 * Copyright © 2011, Intel Corporation.
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
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St 
 * - Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef _MTN_BUTTON_BOX
#define _MTN_BUTTON_BOX

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MTN_TYPE_BUTTON_BOX mtn_button_box_get_type()

#define MTN_BUTTON_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTN_TYPE_BUTTON_BOX, MtnButtonBox))

#define MTN_BUTTON_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MTN_TYPE_BUTTON_BOX, MtnButtonBoxClass))

#define MTN_IS_BUTTON_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTN_TYPE_BUTTON_BOX))

#define MTN_IS_BUTTON_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MTN_TYPE_BUTTON_BOX))

#define MTN_BUTTON_BOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MTN_TYPE_BUTTON_BOX, MtnButtonBoxClass))

typedef struct {
  MxBoxLayout parent;
} MtnButtonBox;

typedef struct {
  MxBoxLayoutClass parent_class;
} MtnButtonBoxClass;

GType mtn_button_box_get_type (void);

ClutterActor* mtn_button_box_new (void);

G_END_DECLS

#endif
