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


#include "mex-action-button.h"

G_DEFINE_TYPE (MexActionButton, mex_action_button, MX_TYPE_BUTTON)

#define ACTION_BUTTON_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_ACTION_BUTTON, MexActionButtonPrivate))

static void
mex_action_button_class_init (MexActionButtonClass *klass)
{
}

static void
mex_action_button_init (MexActionButton *self)
{
  mx_button_set_icon_position (MX_BUTTON (self), MX_POSITION_RIGHT);

  mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);
}

ClutterActor *
mex_action_button_new (MxAction *action)
{
  return g_object_new (MEX_TYPE_ACTION_BUTTON, "action", action, NULL);
}
