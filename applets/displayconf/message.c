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

#include <config.h>
#include <clutter/clutter.h>

void
show_message (void)
{
  ClutterActor *stage, *label;
  ClutterColor black = { 0, 0, 0, 255 };
  ClutterColor white = { 255, 255, 255, 255 };

  clutter_init (NULL, NULL);

  stage = clutter_stage_get_default ();
  clutter_stage_set_color (CLUTTER_STAGE (stage), &black);
  clutter_stage_set_fullscreen (CLUTTER_STAGE (stage), TRUE);

  label = clutter_text_new_with_text ("Sans 20",
                                      "Cannot find suitable screen mode\n"
                                      "Media Explorer requires a 720p screen");
  clutter_text_set_line_wrap (CLUTTER_TEXT (label), TRUE);
  clutter_text_set_color (CLUTTER_TEXT (label), &white);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), label);

  /* Put the label in the middle */
  clutter_actor_add_constraint
    (label, clutter_align_constraint_new (stage, CLUTTER_ALIGN_X_AXIS, 0.5));
  clutter_actor_add_constraint
    (label, clutter_align_constraint_new (stage, CLUTTER_ALIGN_Y_AXIS, 0.5));

  clutter_actor_show (stage);
  clutter_main ();
}

#ifdef STANDALONE
int
main (int argc, char **argv)
{
  clutter_init (&argc, &argv);
  show_message ();
  return 0;
}
#endif
