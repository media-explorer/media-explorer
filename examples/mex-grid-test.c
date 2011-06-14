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
button_clicked_cb (ClutterActor       *button,
                   MexExpanderBox      *drawer)
{
  gboolean open = !mex_expander_box_get_open (drawer);

  mex_expander_box_set_open (drawer, open);

  if (open)
    {
      MxFocusManager *manager =
        mx_focus_manager_get_for_stage ((ClutterStage *)
                                        clutter_actor_get_stage (button));
      if ((ClutterActor *)mx_focus_manager_get_focused (manager) == button)
        mx_focus_manager_move_focus (manager, MX_FOCUS_DIRECTION_RIGHT);
    }
}

static void
add_pictures (MexGrid *grid)
{
  GList *files = get_pictures ();
  MxTextureCache *cache = mx_texture_cache_get_default ();
  const ClutterColor shadow_color = { 0x00, 0x00, 0x00, 0xb0 };

  while (files)
    {
      gint i;
      gchar *basename;
      MexShadow *shadow;
      ClutterActor *drawer, *tile, *button, *texture, *menu, *description;

      gchar *file = files->data;

      /* Create texture */
      button = mx_button_new ();
      texture = (ClutterActor *)mx_texture_cache_get_texture (cache, file);
      mx_bin_set_child (MX_BIN (button), texture);
      mx_bin_set_fill (MX_BIN (button), TRUE, TRUE);

      /* Create menu */
      menu = mx_box_layout_new ();
      mx_box_layout_set_orientation (MX_BOX_LAYOUT (menu),
                                     MX_ORIENTATION_VERTICAL);
      for (i = 0; i < 4; i++)
        {
          ClutterActor *item, *layout, *icon, *label;

          item = mx_button_new ();

          layout = mx_box_layout_new ();
          icon = mx_icon_new ();
          label = mx_label_new ();

          mx_box_layout_set_spacing (MX_BOX_LAYOUT (layout), 8);

          mx_icon_set_icon_size (MX_ICON (icon), 16);
          clutter_actor_set_size (icon, 16, 16);

          clutter_container_add (CLUTTER_CONTAINER (layout),
                                 icon, label, NULL);
          mx_bin_set_child (MX_BIN (item), layout);
          mx_bin_set_alignment (MX_BIN (item),
                                MX_ALIGN_START,
                                MX_ALIGN_MIDDLE);

          clutter_container_add_actor (CLUTTER_CONTAINER (menu), item);
          mx_box_layout_child_set_x_fill (MX_BOX_LAYOUT (menu), item, TRUE);

          switch (i)
            {
            case 0:
              mx_icon_set_icon_name (MX_ICON (icon), "dialog-information");
              mx_label_set_text (MX_LABEL (label), "This");
              break;

            case 1:
              mx_icon_set_icon_name (MX_ICON (icon), "dialog-question");
              mx_label_set_text (MX_LABEL (label), "is");
              break;

            case 2:
              mx_icon_set_icon_name (MX_ICON (icon), "dialog-warning");
              mx_label_set_text (MX_LABEL (label), "a");
              break;

            case 3:
              mx_icon_set_icon_name (MX_ICON (icon), "dialog-error");
              mx_label_set_text (MX_LABEL (label), "menu");
              break;
            }
        }

      /* Create description */
      description = mx_label_new_with_text ("Here you could put a very "
                                            "long description of whatever "
                                            "is above it. Or you could put "
                                            "another focusable widget here "
                                            "and it'd be navigable, like "
                                            "the menu on the right. Whoo!");
      clutter_text_set_line_wrap ((ClutterText *)mx_label_get_clutter_text (
                                    MX_LABEL (description)), TRUE);

      drawer = mex_expander_box_new ();
      tiles ++;
      basename = g_filename_display_basename (file);
      tile = mex_tile_new_with_label (basename);
      g_free (basename);
      mx_bin_set_child (MX_BIN (tile), button);

      clutter_container_add (CLUTTER_CONTAINER (drawer),
                             tile, menu, description, NULL);
      mex_expander_box_set_important (MEX_EXPANDER_BOX (drawer), TRUE);

      clutter_container_add_actor (CLUTTER_CONTAINER (grid), drawer);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (button_clicked_cb), drawer);

      g_free (file);
      files = g_list_delete_link (files, files);

      shadow = mex_shadow_new ();
      mex_shadow_set_radius_y (shadow, 24);
      mex_shadow_set_radius_x (shadow, 0);
      mex_shadow_set_color (shadow, &shadow_color);
      clutter_actor_add_effect (drawer, CLUTTER_EFFECT (shadow));
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
