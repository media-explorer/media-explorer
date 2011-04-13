/*
 * Mex - a media explorer
 *
 * Copyright © , 2010, 2011 Intel Corporation.
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


#include "rebinder-debug.h"

#ifdef REBINDER_ENABLE_DEBUG

guint rebinder_debug_flags = 0;  /* global mex debug flag */

static GTimer *rebinder_timer;

static const GDebugKey rebinder_debug_keys[] =
{
  { "remapping", MEX_DEBUG_REMAPPING },
  { "evdev",     MEX_DEBUG_EVDEV },
  { "fake",      MEX_DEBUG_FAKE },
  { "state",     MEX_DEBUG_STATE },
};

/*
 * rebinder_get_timestamp:
 *
 * Returns the approximate number of microseconds passed since Clutter-Gst was
 * intialized.
 *
 * Return value: Number of microseconds since mex_init() was called.
 */
gulong
rebinder_get_timestamp (void)
{
  gdouble seconds;

  seconds = g_timer_elapsed (rebinder_timer, NULL);

  return (gulong)(seconds / 1.0e-6);
}

gboolean rebinder_debug_init (void)
{
  const char *env_string;

  env_string = g_getenv ("REBINDER_DEBUG");

  rebinder_timer = g_timer_new ();
  g_timer_start (rebinder_timer);

  if (env_string == NULL)
    return TRUE;

  rebinder_debug_flags =
    g_parse_debug_string (env_string,
                          rebinder_debug_keys,
                          G_N_ELEMENTS (rebinder_debug_keys));

  return TRUE;
}

#endif /* MEX_ENABLE_DEBUG */
