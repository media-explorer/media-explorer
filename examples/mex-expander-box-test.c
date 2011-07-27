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

#if 1
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

      full_file = g_build_filename (directory, file, NULL);

      if (!g_file_test (full_file, G_FILE_TEST_IS_REGULAR))
        goto skip_file;

      if ((!g_str_has_suffix (full_file, ".png")) &&
          (!g_str_has_suffix (full_file, ".jpg")))
        goto skip_file;

      files = g_list_prepend (files, full_file);
      continue;

skip_file:
      g_free (full_file);
    }

  g_dir_close (dir);

  files = g_list_sort (files, (GCompareFunc)strcmp);

  if (files == NULL)
    {
      g_error ("This example expects some images (.png and .jpg) in %s",
               directory);
    }

  return files;
}

static gboolean
texture_enter_cb (ClutterActor         *button,
                  ClutterCrossingEvent *event,
                  MexExpanderBox        *drawer)
{
  mex_expander_box_set_important (drawer, TRUE);
  return FALSE;
}

static gboolean
texture_leave_cb (ClutterActor         *button,
                  ClutterCrossingEvent *event,
                  MexExpanderBox        *drawer)
{
  mex_expander_box_set_important (drawer, FALSE);
  return FALSE;
}

static void
texture_clicked_cb (ClutterActor       *button,
                    ClutterButtonEvent *event,
                    MexExpanderBox      *drawer)
{
  gboolean open = !mex_expander_box_get_open (drawer);
  mex_expander_box_set_open (drawer, open);
}

static void
sync_drawer2_cb (MexExpanderBox *box1,
                 GParamSpec    *pspec,
                 MexExpanderBox *box2)
{
  mex_expander_box_set_open (box2, mex_expander_box_get_open (box1));
}

static void
add_pictures (ClutterActor *box)
{
  GList *files = get_pictures ();

  while (files)
    {
      gint w, h, i;
      ClutterActor *drawer, *drawer2, *tile, *texture, *menu, *description;

      gchar *file = files->data;

      /* Create texture */
      texture = clutter_texture_new_from_file (file, NULL);
      clutter_texture_get_base_size (CLUTTER_TEXTURE (texture), &w, &h);
      clutter_actor_set_size (texture, 300, 300.0/w * h);

      /* Create menu */
      menu = mx_box_layout_new ();
      mx_box_layout_set_orientation (MX_BOX_LAYOUT (menu),
                                     MX_ORIENTATION_VERTICAL);
      for (i = 0; i < 4; i++)
        {
          ClutterActor *button, *layout, *icon, *label;

          button = mx_button_new ();

          layout = mx_box_layout_new ();
          icon = mx_icon_new ();
          label = mx_label_new ();

          mx_box_layout_set_spacing (MX_BOX_LAYOUT (layout), 8);

          mx_icon_set_icon_size (MX_ICON (icon), 16);
          clutter_actor_set_size (icon, 16, 16);

          clutter_container_add (CLUTTER_CONTAINER (layout),
                                 icon, label, NULL);
          mx_bin_set_child (MX_BIN (button), layout);
          mx_bin_set_alignment (MX_BIN (button),
                                MX_ALIGN_START,
                                MX_ALIGN_MIDDLE);

          clutter_container_add_actor (CLUTTER_CONTAINER (menu), button);
          mx_box_layout_child_set_x_fill (MX_BOX_LAYOUT (menu), button, TRUE);

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
      mex_expander_box_set_important_on_focus (MEX_EXPANDER_BOX (drawer), TRUE);
      drawer2 = mex_expander_box_new ();
      mex_expander_box_set_grow_direction (MEX_EXPANDER_BOX (drawer2),
                                          MEX_EXPANDER_BOX_RIGHT);
      mex_expander_box_set_important (MEX_EXPANDER_BOX (drawer2), TRUE);

      tile = mex_tile_new_with_label (file);
      mex_tile_set_important (MEX_TILE (tile), TRUE);
      mx_bin_set_child (MX_BIN (tile), texture);

      clutter_container_add (CLUTTER_CONTAINER (drawer2), tile, menu, NULL);
      clutter_container_add (CLUTTER_CONTAINER (drawer),
                             drawer2, description, NULL);

      g_signal_connect (drawer, "notify::open",
                        G_CALLBACK (sync_drawer2_cb), drawer2);

      clutter_container_add_actor (CLUTTER_CONTAINER (box), drawer);

      clutter_actor_set_reactive (texture, TRUE);
      g_signal_connect (texture, "enter-event",
                        G_CALLBACK (texture_enter_cb), drawer);
      g_signal_connect (texture, "leave-event",
                        G_CALLBACK (texture_leave_cb), drawer);
      g_signal_connect (texture, "button-press-event",
                        G_CALLBACK (texture_clicked_cb), drawer);

      g_free (file);
      files = g_list_delete_link (files, files);
    }
}

int
main (int argc, char **argv)
{
  const ClutterColor dark_gray = { 0x40, 0x40, 0x40, 0xff };

  ClutterActor *box, *stage;
  MxApplication *app;
  MxWindow *window;

  mex_init (&argc, &argv);
  app = mx_application_new (&argc, &argv, "test-mex-expander-box", 0);

  mex_style_load_default ();

  window = mx_application_create_window (app);
  stage = (ClutterActor *)mx_window_get_clutter_stage (window);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &dark_gray);

  box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box), MX_ORIENTATION_VERTICAL);

  add_pictures (box);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), box);

  mx_window_set_has_toolbar (window, FALSE);
  clutter_actor_set_size (stage, 640, 480);
  clutter_actor_show (stage);

  clutter_main ();

  return 0;
}
#else

static gboolean
toggle_important (MexExpanderBox *box)
{
  mex_expander_box_set_important (box, !mex_expander_box_get_important (box));
  return TRUE;
}

int
main (int argc, char **argv)
{
  const ClutterColor red = { 0xff, 0x00, 0x00, 0xff };
  ClutterActor *stage, *box, *tile, *rectangle;

  clutter_init (&argc, &argv);

  mex_style_load_default ();

  stage = clutter_stage_get_default ();
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);

  tile = mex_tile_new_with_label ("Red rectangle");
  mex_tile_set_important (MEX_TILE (tile), TRUE);
  rectangle = clutter_rectangle_new_with_color (&red);
  clutter_actor_set_size (rectangle, 300, 300);
  mx_bin_set_child (MX_BIN (tile), rectangle);

  box = mex_expander_box_new ();
  clutter_container_add (CLUTTER_CONTAINER (box), tile,
                         clutter_text_new_with_text ("Sans 16", "Hello"),
                         NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), box);

  clutter_actor_set_reactive (rectangle, TRUE);
  g_signal_connect_swapped (rectangle, "enter-event",
                            G_CALLBACK (toggle_important), box);

  clutter_actor_show (stage);

  clutter_main ();

  return 0;
}

#endif
