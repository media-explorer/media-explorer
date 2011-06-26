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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnomeui/gnome-rr.h>
#include <libgnomeui/gnome-rr-config.h>
#include "message.h"

static GMainLoop *loop = NULL;

int
size_sorter (gconstpointer a, gconstpointer b)
{
  GnomeRRMode *m_a = (GnomeRRMode *)a, *m_b = (GnomeRRMode *)b;

  return (gnome_rr_mode_get_height (m_b) - 720) < (gnome_rr_mode_get_height (m_a) - 720);
}

static GnomeRROutput *
pick_output (GnomeRRScreen *screen, GnomeRRConfig *config)
{
  GnomeOutputInfo *oi;
  GnomeRROutput *output, *laptop_output = NULL, *external_output = NULL;
  int i;

  g_assert (config);

  for (i = 0; config->outputs[i] != NULL; i++) {
    oi = config->outputs[i];
    output = gnome_rr_screen_get_output_by_name (screen, oi->name);

    if (!gnome_rr_output_is_connected (output))
      continue;

    if (gnome_rr_output_is_laptop (output))
      laptop_output = output;
    else
      external_output = output;
    /* TODO: handle multiple external monitors */
  }

  /* TODO: don't assert, but display error message */
  g_assert (laptop_output || external_output);

  return external_output ? external_output : laptop_output;
}

static void
update_configuration (GnomeRRConfig *config, GnomeRROutput *output, GnomeRRMode *mode)
{
  int i;

  g_assert (config);
  g_assert (output);
  g_assert (mode);

  for (i = 0; config->outputs[i] != NULL; i++) {
    GnomeOutputInfo *oi = config->outputs[i];

    if (g_strcmp0 (oi->name, gnome_rr_output_get_name (output)) == 0) {
      oi->on = TRUE;
      oi->primary = TRUE;
      oi->width = gnome_rr_mode_get_width (mode);
      oi->height = gnome_rr_mode_get_height (mode);
      oi->rate = gnome_rr_mode_get_freq (mode);
      oi->x = oi->y = 0;
      oi->rotation = GNOME_RR_ROTATION_0;
    } else {
      oi->on = FALSE;
    }
  }
}

static void
screen_changed_cb (GnomeRRScreen *screen, gpointer user_data)
{
  g_debug ("Got screen change event");

  g_main_loop_quit (loop);
}

gboolean
timeout_cb (gpointer data)
{
  g_debug ("Change timed out");
  /* TODO: check current configuration */
  g_main_loop_quit (loop);

  show_message ();

  return FALSE;
}

int
main (int argc, char **argv)
{
  GList *modes = NULL;
  GnomeRRScreen *screen;
  GnomeRRConfig *config, *old_config;
  GnomeRROutput *output;
  GnomeRRMode **all_modes;
  int i;
  GError *error = NULL;

  gdk_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  screen = gnome_rr_screen_new (gdk_screen_get_default (), screen_changed_cb, NULL, NULL);
  /* Get the screen configuration twice so we can see if we've actually made changes later */
  old_config = gnome_rr_config_new_current (screen);
  config = gnome_rr_config_new_current (screen);

  output = pick_output (screen, config);
  g_assert (output);
  g_debug ("Picked output %s", gnome_rr_output_get_name (output));

  all_modes = gnome_rr_output_list_modes (output);

  /* First scan for widescreen modes */
  for (i = 0; all_modes[i] != NULL; i++) {
    GnomeRRMode *mode = all_modes[i];
    int ratio;
    gboolean widescreen = FALSE;

    /* Skip modes with a height less than 720 straight away */
    if (gnome_rr_mode_get_height (mode) < 720)
      continue;

    /*
     * Build a list of all modes that are widescreen.  Multiply by ten and
     * truncate into an integer to avoid annoying float comparisons.
     */
    ratio = ((double)gnome_rr_mode_get_width (mode) / gnome_rr_mode_get_height (mode)) * 10;
    switch (ratio) {
    case 13:
      /* 1.33, or 4:3 */
      widescreen = FALSE;
      break;
    case 16:
      /* 1.6, or 16:10 */
      widescreen = TRUE;
      break;
    case 17:
      /* 1.777, or 16;9 */
      widescreen = TRUE;
      break;
    case 23:
      /* 2.333, or 21:9 */
      widescreen = TRUE;
      break;
    default:
      g_warning ("Unknown ratio for %d x %d",
                 gnome_rr_mode_get_width (mode),
                 gnome_rr_mode_get_height (mode));
      widescreen = FALSE;
      break;
    }

    if (widescreen) {
      modes = g_list_prepend (modes, mode);
    }
  }

  if (modes == NULL) {
    g_print ("No usable modes detected!\n");
    return 1;
  }

  /* Now sort it so they are sorted in order of distance from 720 */
  modes = g_list_sort (modes, size_sorter);
  g_debug ("Chose %d x %d", gnome_rr_mode_get_width (modes->data), gnome_rr_mode_get_height (modes->data));

  /* Now update the configuration */
  update_configuration (config, output, modes->data);

  if (!gnome_rr_config_applicable (config, screen, &error)) {
    g_warning ("Generated configuration isn't valid: %s", error->message);
    show_message ();
    return 1;
  }

  if (!gnome_rr_config_equal (config, old_config)) {
    guint32 timestamp;
    gnome_rr_screen_get_timestamps (screen, &timestamp, NULL);

    if (!gnome_rr_config_apply_with_time (config, screen, timestamp, &error)) {
      g_warning ("Cannot apply: %s", error->message);
      show_message ();
      return 1;
    }

    g_timeout_add_seconds (3, timeout_cb, NULL);
    g_main_loop_run (loop);
  } else {
    g_debug ("Already in best mode");
  }

  return 0;
}
