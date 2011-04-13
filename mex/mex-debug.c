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


#include "mex-debug.h"

#ifdef MEX_ENABLE_DEBUG

guint _mex_debug_flags = 0;  /* global mex debug flag */

static GTimer *mex_timer;

static const GDebugKey mex_debug_keys[] =
{
  { "misc", MEX_DEBUG_MISC },
  { "epg",  MEX_DEBUG_EPG },
  { "applet-manager", MEX_DEBUG_APPLET_MANAGER },
  { "channel", MEX_DEBUG_CHANNEL },
  { "download-queue", MEX_DEBUG_DOWNLOAD_QUEUE },
};

/*
 * _mex_get_timestamp:
 *
 * Returns the approximate number of microseconds passed since Clutter-Gst was
 * intialized.
 *
 * Return value: Number of microseconds since mex_init() was called.
 */
gulong
_mex_get_timestamp (void)
{
  gdouble seconds;

  seconds = g_timer_elapsed (mex_timer, NULL);

  return (gulong)(seconds / 1.0e-6);
}

gboolean _mex_debug_init (void)
{
  const char *env_string;

  env_string = g_getenv ("MEX_DEBUG");

  mex_timer = g_timer_new ();
  g_timer_start (mex_timer);

  if (env_string == NULL)
    return TRUE;

  _mex_debug_flags =
    g_parse_debug_string (env_string,
                          mex_debug_keys,
                          G_N_ELEMENTS (mex_debug_keys));

  return TRUE;
}

#endif /* MEX_ENABLE_DEBUG */
