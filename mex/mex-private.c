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


#include "mex-private.h"
#include "mex-content.h"

gboolean
mex_content_title_fallback_cb (GBinding     *binding,
                               const GValue *source_value,
                               GValue       *target_value,
                               gpointer      user_data)
{
  gchar *target;
  const gchar *showname;

  MexContent *content = MEX_CONTENT (user_data);
  const gchar *source_str = g_value_get_string (source_value);

  if ((!source_str) || (*source_str == '\0'))
    {
      showname = mex_content_get_metadata (content,
                                           MEX_CONTENT_METADATA_SERIES_NAME);
      if (!showname)
        {
          const gchar *url;
          gchar *basename;

          url = mex_content_get_metadata (content, MEX_CONTENT_METADATA_URL);
          basename = g_path_get_basename (url);
          target = g_uri_unescape_string (basename, NULL);
          g_free (basename);
        }
      else
        {
          const gchar *episode, *season;

          episode = mex_content_get_metadata (content,
                                              MEX_CONTENT_METADATA_EPISODE);
          season = mex_content_get_metadata (content,
                                             MEX_CONTENT_METADATA_SEASON);
          if (episode && season)
            target = g_strdup_printf ("%s: Season %s, Episode %s",
                                      showname, season, episode);
          else if (episode)
            target = g_strdup_printf ("%s: Episode %s",
                                      showname, episode);
          else if (season)
            target = g_strdup_printf ("%s: Season %s",
                                      showname, season);
          else
            target = g_strdup (showname);
        }
    }
  else
    target = g_strdup (source_str);

  g_value_take_string (target_value, target);

  return TRUE;
}

void
_mex_print_date (GDateTime *date)
{
  gchar *str;

  str = g_date_time_format (date, "%d/%m/%y %H:%M");
  g_debug ("Date: %s", str);
  g_free (str);
}

