#include "mtn-button-box.h"

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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

static void mtn_button_box_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MtnButtonBox, mtn_button_box, MX_TYPE_BOX_LAYOUT,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mtn_button_box_focusable_iface_init));

static MxFocusable*
mtn_button_box_accept_focus (MxFocusable *focusable, MxFocusHint hint)
{
  MxFocusableIface *class, *parent_class;
  
  class = G_TYPE_INSTANCE_GET_INTERFACE (focusable, MX_TYPE_FOCUSABLE, MxFocusableIface);
  parent_class = g_type_interface_peek_parent (class);

  if (parent_class)
    return parent_class->accept_focus (focusable, MX_FOCUS_HINT_LAST);
  else 
    return NULL;
}

static void
mtn_button_box_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = mtn_button_box_accept_focus;
}

static void
mtn_button_box_class_init (MtnButtonBoxClass *klass) {}

static void
mtn_button_box_init (MtnButtonBox *self) {}

ClutterActor*
mtn_button_box_new (void)
{
  return g_object_new (MTN_TYPE_BUTTON_BOX, NULL);
}
