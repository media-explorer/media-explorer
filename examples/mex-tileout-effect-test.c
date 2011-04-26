/*
 * Mex - Media Explorer
 *
 * Copyright © 2011 Intel Corporation.
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
#include <stdlib.h>

static const ClutterColor dark_gray = { 0x40, 0x40, 0x40, 0xff };

static ClutterAnimation *
animate_progress (ClutterEffect *effect,
                  gdouble        to)
{
  ClutterAnimation *animation;
  ClutterTimeline *timeline;
  gdouble initial_progress;
  GValue from_value = { 0, };
  GValue to_value = { 0, };
  ClutterInterval *interval;

  animation = clutter_animation_new ();
  clutter_animation_set_duration (animation, 1500);
  clutter_animation_set_object (animation, G_OBJECT (effect));
  clutter_animation_set_mode (animation, CLUTTER_LINEAR);

  g_value_init (&from_value, G_TYPE_DOUBLE);
  g_object_get (effect, "progress", &initial_progress, NULL);
  g_value_set_double (&from_value, initial_progress);

  g_value_init (&to_value, G_TYPE_DOUBLE);
  g_value_set_double (&to_value, to);

  interval = clutter_interval_new_with_values (G_TYPE_DOUBLE,
                                               &from_value,
                                               &to_value);

  clutter_animation_bind_interval (animation, "progress", interval);

  timeline = clutter_animation_get_timeline (animation);
  clutter_timeline_start (timeline);

  return animation;
}

int
main (int argc, char **argv)
{
  ClutterActor *stage, *texture, *frame;
  ClutterEffect *tileout_effect;
  ClutterAnimation *animation;
  MxApplication *app;
  MxWindow *window;
  GError *error = NULL;

  mex_init (&argc, &argv);
  app = mx_application_new (&argc, &argv, "test-mex-tileout-effect", 0);
  clutter_init (&argc, &argv);

  mex_style_load_default ();

  window = mx_application_create_window (app);
  stage = (ClutterActor *)mx_window_get_clutter_stage (window);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &dark_gray);
  clutter_actor_set_size (stage, 640, 480);

  texture = clutter_texture_new_from_file (argv[1], &error);
  if (texture == NULL)
    {
      static const gchar message[] = "Could not load texture";

      if (error)
        g_message ("%s %s: %s", message, argv[1], error->message);
      else
        g_message ("%s %s", message, argv[1]);

      return EXIT_FAILURE;
    }

  tileout_effect = mex_tileout_effect_new ();
  clutter_actor_add_effect (texture, tileout_effect);

  frame = mx_frame_new ();
#if 0
  mx_bin_set_alignment (MX_BIN (frame), MX_ALIGN_MIDDLE, MX_ALIGN_MIDDLE);
#endif
  clutter_actor_set_size (texture,
                          clutter_actor_get_width (stage),
                          clutter_actor_get_height (stage));
  clutter_actor_set_position (texture, 0.f, 0.f);

  //clutter_container_add_actor (CLUTTER_CONTAINER (frame), texture);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), texture);

  mx_window_set_has_toolbar (window, FALSE);
  clutter_actor_show (stage);

  animation = animate_progress (tileout_effect, 1.0);

  clutter_main ();

  g_object_unref (animation);

  return EXIT_SUCCESS;
}
