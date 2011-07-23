/*
 * Mex - Media Explorer
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

#include <mex/mex.h>

int
main (int argc, char **argv)
{
  const ClutterColor grey = { 0x40, 0x40, 0x40, 0xff };

  ClutterActor *stage, *info_bar;
  ClutterConstraint *constraint;
  MxApplication *app;
  MxWindow *window;

  mex_init (&argc, &argv);

  app = mx_application_new (&argc, &argv, "mex-media-controls-test", 0);
  mex_style_load_default ();

  window = mx_application_create_window (app);
  stage = (ClutterActor *)mx_window_get_clutter_stage (window);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &grey);
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);

  info_bar = mex_info_bar_get_default ();
  constraint = clutter_bind_constraint_new (stage, CLUTTER_BIND_WIDTH, 0.0);
  clutter_actor_add_constraint (info_bar, constraint);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), info_bar);

  clutter_actor_set_size (stage, 1024, 768);
  clutter_actor_show (stage);

  clutter_main ();

  return 0;
}
