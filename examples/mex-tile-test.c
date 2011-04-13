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

static gboolean
focus_in_cb (gpointer cat, gpointer dog, gpointer bob)
{
  g_debug ("focus in");

  return TRUE;
}


int
main (int argc, char **argv)
{
  const ClutterColor grey = { 0x40, 0x40, 0x40, 0xff };

  ClutterActor *stage, *tile, *tile2, *image, *image2, *dialog, *tiles;
  MxApplication *app;
  ClutterConstraint *constraint;

  mex_init (&argc, &argv);

  app = mx_application_new (&argc, &argv, "mex-tile-controls-test", 0);
  mex_style_load_default ();

  stage = clutter_stage_get_default ();
  clutter_stage_set_color (CLUTTER_STAGE (stage), &grey);
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);

  image = mx_image_new ();
  mx_image_set_from_file (MX_IMAGE (image), "/home/michael/dev/mex-info-bar/graphic-mapButtons.png", NULL);

  image2 = mx_image_new ();
  mx_image_set_from_file (MX_IMAGE (image2), "/home/michael/dev/mex-info-bar/graphic-network.png", NULL);

  tile = mex_tile_new ();
  mex_tile_set_label (tile, "Remote mapping");
  mex_tile_set_important (tile, TRUE);

  ClutterActor *button;
  button = mx_button_new ();

  mx_bin_set_child (MX_BIN (tile), button);
  mx_bin_set_child (MX_BIN (button), image);

  tile2 = mex_tile_new ();
  mex_tile_set_label (tile2, "Network");
  mex_tile_set_important (tile2, TRUE);

  ClutterActor *button2;
  button2 = mx_button_new ();

  mx_bin_set_child (MX_BIN (tile2), button2);
  mx_bin_set_child (MX_BIN (button2), image2);

  tiles = mx_box_layout_new ();
  mx_box_layout_set_spacing (tiles, 10);
  mx_box_layout_set_orientation (tiles, MX_ORIENTATION_HORIZONTAL);
  mx_box_layout_add_actor (tiles, tile, 0);
  mx_box_layout_add_actor (tiles, tile2, 1);


  g_print (clutter_actor_get_reactive (tile) ? "reactive" : "notreactive");
  dialog = mx_dialog_new ();
  mx_dialog_set_transient_parent (dialog, stage);

  g_signal_connect (button2, "clicked", G_CALLBACK (focus_in_cb), NULL);
  g_signal_connect (button, "clicked", G_CALLBACK (focus_in_cb), NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (dialog), tiles);

  clutter_actor_show (dialog);

  clutter_actor_set_size (stage, 1024, 768);
  clutter_actor_show (stage);

  clutter_main ();

  return 0;
}
