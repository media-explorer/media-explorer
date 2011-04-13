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

  ClutterActor *stage, *controls, *align;
  MxApplication *app;
  MxWindow *window;

  mex_init (&argc, &argv);

  app = mx_application_new (&argc, &argv, "mex-media-controls-test", 0);
  mex_style_load_default ();

  window = mx_application_create_window (app);
  stage = (ClutterActor *) mx_window_get_clutter_stage (window);
  clutter_stage_set_color ((ClutterStage *) stage, &grey);

  align = g_object_new (MX_TYPE_FRAME, "x-align", MX_ALIGN_MIDDLE,
                        "y-align", MX_ALIGN_END, NULL);

  mx_window_set_child (window, align);


  controls = mex_media_controls_new ();
  mx_bin_set_child (MX_BIN (align), controls);

  g_signal_connect_swapped (controls, "stopped", G_CALLBACK (printf),
                            "Stopped signal received\n");

  mx_window_set_has_toolbar (window, FALSE);
  clutter_actor_set_size (stage, 1024, 768);
  clutter_actor_show (stage);

  clutter_main ();

  return 0;
}
