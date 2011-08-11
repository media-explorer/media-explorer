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
#include <mx/mx.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>

static gint tiles = 0;

static GList *
get_pictures ()
{
  GDir *dir;
  const gchar *file;

  GList *files = NULL;
  GError *error = NULL;
  const gchar *directory = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);

  /* Compile list of images */
  if (!(dir = g_dir_open (directory, 0, &error)))
    {
      g_critical ("Error opening directory: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  while ((file = g_dir_read_name (dir)))
    {
      gchar *full_file;
      struct stat buf;

      full_file = g_build_filename (directory, file, NULL);

      if (!g_file_test (full_file, G_FILE_TEST_IS_REGULAR))
        goto skip_file;

      if ((!g_str_has_suffix (full_file, ".png")) &&
          (!g_str_has_suffix (full_file, ".jpg")))
        goto skip_file;

      /* Skip big files */
      g_stat (full_file, &buf);
      if (buf.st_size > 1024*1024*1)
        goto skip_file;

      files = g_list_prepend (files, full_file);
      continue;

skip_file:
      g_free (full_file);
    }

  g_dir_close (dir);

  files = g_list_sort (files, (GCompareFunc)strcmp);

  return files;
}

static void
add_pictures (MexGrid *grid)
{
  GList *files = get_pictures ();

  while (files)
    {
      gchar *basename, *url;
      ClutterActor *tile;
      MexContent *content;

      gchar *file = files->data;

      tile = mex_tile_new ();
      tiles ++;

      url = g_strconcat ("file://", file, NULL);
      basename = g_filename_display_basename (file);

      content = g_object_new (MEX_TYPE_GENERIC_CONTENT, NULL);
      mex_content_set_metadata (content, MEX_CONTENT_METADATA_STILL, url);
      mex_content_set_metadata (content, MEX_CONTENT_METADATA_TITLE, basename);

      tile = mex_content_box_new ();
      mex_content_view_set_content (MEX_CONTENT_VIEW (tile), content);

      mex_content_box_set_important (MEX_CONTENT_BOX (tile), TRUE);
      g_object_bind_property (grid, "tile-width",
                              tile, "thumb-width", G_BINDING_SYNC_CREATE);

      clutter_container_add_actor (CLUTTER_CONTAINER (grid), tile);

      files = g_list_delete_link (files, files);
      g_free (basename);
      g_free (file);
      g_free (url);
    }
}

int
main (int argc, char **argv)
{
  const ClutterColor dark_gray = { 0x40, 0x40, 0x40, 0xff };

  ClutterActor *stage, *scroll, *grid;
  MxApplication *app;
  MxWindow *window;

  app = mx_application_new (&argc, &argv, "test-mex-grid", 0);

  mex_style_load_default ();

  window = mx_application_create_window (app);
  stage = (ClutterActor *)mx_window_get_clutter_stage (window);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &dark_gray);

  grid = mex_grid_new ();

  while (tiles < 500)
    add_pictures (MEX_GRID (grid));

  g_message ("Packed %d actors into grid", tiles);

  scroll = mex_scroll_view_new ();

  clutter_container_add_actor (CLUTTER_CONTAINER (scroll), grid);
  mx_window_set_child (window, scroll);

  mx_window_set_has_toolbar (window, FALSE);
  clutter_actor_set_size (stage, 940, 768);
  clutter_actor_show (stage);

  clutter_main ();

  return 0;
}
