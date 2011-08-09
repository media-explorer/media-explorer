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

/* Thumbnail/Still photo by Tomi Tapio licenced under CC-BY 2.0
 * http://creativecommons.org/licenses/by/2.0/
 * http://www.flickr.com/photos/tomitapio/
 */

#include <mex/mex.h>

int
main (int argc, char **argv)
{
  const ClutterColor grey = { 0x40, 0x40, 0x40, 0xff };

  ClutterActor *stage, *info_panel, *align;
  MxApplication *app;
  MxWindow *window;
  MexFeed *feed;
  MexProgram *program;

  mex_init (&argc, &argv);

  app = mx_application_new (&argc, &argv, "mex-info-panel-test", 0);
  mex_style_load_default ();

  window = mx_application_create_window (app);
  stage = (ClutterActor *) mx_window_get_clutter_stage (window);
  clutter_stage_set_color ((ClutterStage *) stage, &grey);

  align = g_object_new (MX_TYPE_FRAME, "x-align", MX_ALIGN_MIDDLE,
                        "y-align", MX_ALIGN_END, NULL);

  mx_window_set_child (window, align);

  info_panel = mex_info_panel_new (MEX_INFO_PANEL_MODE_FULL);
  mx_bin_set_child (MX_BIN (align), info_panel);

  mx_window_set_has_toolbar (window, FALSE);
  clutter_actor_set_size (stage, 1024, 768);

  feed = mex_feed_new ("source", "title");
  program =  mex_program_new (feed);

  mex_content_set_metadata (MEX_CONTENT (program),
                            MEX_CONTENT_METADATA_MIMETYPE,
                            "video/mp4");

  mex_content_set_metadata (MEX_CONTENT (program),
                            MEX_CONTENT_METADATA_TITLE,
                            "The cats on the moon");
  mex_content_set_metadata (MEX_CONTENT (program),
                            MEX_CONTENT_METADATA_SYNOPSIS,
                            "An original title where cats are sent to the moon to catch all the mice which are naturally attracted there due to the large consistency of cheese, this results in a space race between NASA and CATSA, leading to war on the moon over territory claimed by cats");
  mex_content_set_metadata (MEX_CONTENT (program),
                            MEX_CONTENT_METADATA_STILL,
                            "http://farm5.static.flickr.com/4013/4305303148_5cbc986a44_m.jpg");

  mex_content_view_set_content (MEX_CONTENT_VIEW (info_panel),
                                MEX_CONTENT (program));


  clutter_actor_show (stage);

  clutter_main ();

  return 0;
}
