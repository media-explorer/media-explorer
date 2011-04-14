/*
 * Mex - a media explorer
 *
 * Copyright © , 2010, 2011 Intel Corporation.
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

#include <linux/input.h>

#include <clutter/clutter.h>

#include "rebinder.h"

/*
 * This array is used to map evdev KEY events with keycodes > 255 to friendly
 * X keysyms or ClutterKeyEvents which is virtually the same things as
 * ClutterKeyEvents use the same keysyms as X.
 *
 * Note: We use C99's designated and named initializers
 */

#define BINDING(keycode_, keysym_) \
  [keycode_] = { .name = NULL, .keysym = keysym_, .keycode = keycode_ }

static Binding evdev_keymap [] =
{
  BINDING (KEY_OK, CLUTTER_KEY_Return),
  BINDING (KEY_INFO, CLUTTER_KEY_Menu), /* can't find a proper info keysym,
                                           we've used Menu so far for the info
                                           button, keep with that */

  BINDING (KEY_RED, CLUTTER_KEY_Red),
  BINDING (KEY_GREEN, CLUTTER_KEY_Green),
  BINDING (KEY_YELLOW, CLUTTER_KEY_Yellow),
  BINDING (KEY_BLUE, CLUTTER_KEY_Blue),

  BINDING (KEY_NUMERIC_0, CLUTTER_KEY_0),
  BINDING (KEY_NUMERIC_1, CLUTTER_KEY_1),
  BINDING (KEY_NUMERIC_2, CLUTTER_KEY_2),
  BINDING (KEY_NUMERIC_3, CLUTTER_KEY_3),
  BINDING (KEY_NUMERIC_4, CLUTTER_KEY_4),
  BINDING (KEY_NUMERIC_5, CLUTTER_KEY_5),
  BINDING (KEY_NUMERIC_6, CLUTTER_KEY_6),
  BINDING (KEY_NUMERIC_7, CLUTTER_KEY_7),
  BINDING (KEY_NUMERIC_8, CLUTTER_KEY_8),
  BINDING (KEY_NUMERIC_9, CLUTTER_KEY_9),

  BINDING (KEY_NUMERIC_STAR, CLUTTER_KEY_asterisk),
  BINDING (KEY_NUMERIC_POUND, CLUTTER_KEY_numbersign),

  BINDING (KEY_PREVIOUS, CLUTTER_KEY_AudioPrev),
  BINDING (KEY_NEXT, CLUTTER_KEY_AudioNext),

  /* This is the last KEY_ define in the input.h I was looking at (and we
   * don't want to use it) */
#ifndef KEY_WPS_BUTTON
#define KEY_WPS_BUTTON 0x211
#endif
#define MAX_KEY KEY_WPS_BUTTON
  BINDING (KEY_WPS_BUTTON, 0)
};

#undef BINDING

Binding *
find_evdev_binding_by_keycode (guint32 key)
{
  /* KEY_OK (0x160) is the first interesting KEY_ define > 255 */
  if (key < KEY_OK || key > MAX_KEY)
    return NULL;

  /* only return non empty binding */
  if (evdev_keymap[key].keysym == 0 || evdev_keymap[key].keycode == 0)
    return NULL;

  return &evdev_keymap[key];
}
