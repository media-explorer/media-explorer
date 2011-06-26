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

/* required by glibc for strptime from time.h */
#define _XOPEN_SOURCE 600
#include <time.h>
#include <math.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "mex-metadata-utils.h"

#define TV_REGEX "(?<showname>.*)\\.(?<season>(?:\\d{1,2})|(?:[sS]\\K\\d{1,2}))(?<episode>(?:\\d{2})|(?:[eE]\\K\\d{1,2}))\\.?(?<name>.*)?"
#define MOVIE_REGEX "(?<name>.*)\\.?[\\(\\[](?<year>[12][90]\\d{2})[\\)\\]]"

const gchar *blacklisted_prefix[] = {
    "tpz-", NULL
};

/* Blacklisted are words that we ignore everything after */
const char *blacklist[] = {
    "720p", "1080p",
    "ws", "WS", "proper", "PROPER",
    "repack", "real.repack",
    "hdtv", "HDTV", "pdtv", "PDTV", "notv", "NOTV",
    "dsr", "DSR", "DVDRip", "divx", "DIVX", "xvid", "Xvid",
    NULL
};


static gchar *
sanitise_string (const gchar *str)
{
    int i;
    gchar *line;

    line = (gchar *) str;
    for (i = 0; blacklisted_prefix[i]; i++) {
        if (g_str_has_prefix (str, blacklisted_prefix[i])) {
            int len = strlen (blacklisted_prefix[i]);

            line = (gchar *) str + len;
        }
    }

    for (i = 0; blacklist[i]; i++) {
        gchar *end;

        end = strstr (line, blacklist[i]);
        if (end) {
            return g_strndup (line, end - line);
        }
    }
    return g_strdup (line);
}

/* tidies strings before we run them through the regexes */
static gchar *
uri_to_metadata (const gchar *uri)
{
    gchar *ext, *base_name, *name, *whitelisted;

    base_name = g_path_get_basename (uri);
    ext = strrchr (base_name, '.');
    if (ext) {
        name = g_strndup (base_name, ext - base_name);
        g_free (base_name);
    } else {
        name = base_name;
    }

    /* Replace _ <space> with . */
    g_strdelimit (name, "_ ", '.');
    whitelisted = sanitise_string (name);
    g_free (name);

    return whitelisted;
}

void
mex_metadata_from_uri (const gchar *uri,
                       gchar      **title,
                       gchar      **showname,
                       GDate      **date,
                       gint        *season,
                       gint        *episode)
{
    gchar *metadata;
    GRegex *regex;
    GMatchInfo *info;

    metadata = uri_to_metadata (uri);

    regex = g_regex_new (MOVIE_REGEX, 0, 0, NULL);
    g_regex_match (regex, metadata, 0, &info);
    if (g_match_info_matches (info)) {
        if (title) {
            *title= g_match_info_fetch_named (info, "name");
            /* Replace "." with <space> */
            g_strdelimit (*title, ".", ' ');
        }

        if (date) {
            gchar *year = g_match_info_fetch_named (info, "year");

            *date = g_date_new ();
            g_date_set_year (*date, atoi (year));
            g_free (year);
        }

        if (showname) {
            *showname = NULL;
        }

        if (season) {
            *season = 0;
        }

        if (episode) {
            *episode = 0;
        }

        g_regex_unref (regex);
        g_match_info_free (info);
        g_free (metadata);

        return;
    }

    g_regex_unref (regex);
    g_match_info_free (info);

    regex = g_regex_new (TV_REGEX, 0, 0, NULL);
    g_regex_match (regex, metadata, 0, &info);

    if (g_match_info_matches (info)) {
        if (title) {
            *title = g_match_info_fetch_named (info, "name");
            g_strdelimit (*title, ".", ' ');
        }

        if (showname) {
            *showname = g_match_info_fetch_named (info, "showname");
            g_strdelimit (*showname, ".", ' ');
        }

        if (season) {
            gchar *s = g_match_info_fetch_named (info, "season");
            if (s) {
                if (*s == 's' || *s == 'S') {
                    *season = atoi (s + 1);
                } else {
                    *season = atoi (s);
                }
            } else {
                *season = 0;
            }

            g_free (s);
        }

        if (episode) {
            gchar *e = g_match_info_fetch_named (info, "episode");
            if (e) {
                if (*e == 'e' || *e == 'E') {
                    *episode = atoi (e + 1);
                } else {
                    *episode = atoi (e);
                }
            } else {
                *episode = 0;
            }

            g_free (e);
        }

        if (date) {
            *date = NULL;
        }

        g_regex_unref (regex);
        g_match_info_free (info);
        g_free (metadata);
        return;
    }

    g_regex_unref (regex);
    g_match_info_free (info);

    /* The filename doesn't look like a movie or a TV show, just use the
       filename without extension as the title */
    if (title) {
        *title = g_strdelimit (metadata, ".", ' ');
    }

    if (showname) {
        *showname = NULL;
    }

    if (date) {
        *date = NULL;
    }

    if (season) {
        *season = 0;
    }
    if (episode) {
        *episode = 0;
    }

    return;
}

/**
 * mex_metadata_humanise_duration:
 * @duration: duration in seconds
 *
 * Gives a human readable version of the duration
 *
 * Return value: Human friendly version of duration or %NULL
 */
gchar *
mex_metadata_humanise_duration (const gchar *duration)
{
  gchar *humanised;
  gfloat minutes;

  if (duration)
    {
      minutes = atof (duration)/60;

      if (minutes == 0)
        return NULL;

      if (minutes < 1.0)
        return g_strdup (_("Less than a minute"));


      minutes = roundf (minutes);
      humanised = g_strdup_printf ("%.0f %s", minutes,
                                   g_dngettext (NULL,
                                                _("minute"),
                                                _("minutes"),
                                                (gulong) minutes));
      return humanised;
    }
  return NULL;
}

/**
 * mex_metadata_humanise_date:
 * @iso8601_date: iso8601 date string dd-mm-yyyyThh:mm:ssZ
 *
 * Parses the date string given and outputs human friendly version
 *
 * Return value: Human friendly version of date or %NULL
 */

gchar *
mex_metadata_humanise_date (const gchar *iso8601_date)
{
  if (iso8601_date)
    {
      gchar humanised[255];
      struct tm tm_parsed;
      gchar *valid;

      /* clear tm_parsed */
      memset (&tm_parsed, '\0', sizeof (struct tm));

      valid = strptime (iso8601_date, "%FT%TZ%z", &tm_parsed);

      /* valid populates with characters that can't be parsed therefore it
         is null when a full parse has happened */
      if (valid == NULL)
        {
          /* arg1 NULL to measure desired allocation for humanised */
          strftime (humanised, 255, "%e %b %Y", &tm_parsed);
          return g_strdup (humanised);
        }
      g_free (valid);
    }
  return NULL;
}

gchar *
mex_metadata_humanise_time (const gchar *time_str)
{
  if (time_str)
    {
      gchar *humanised;
      gint len_h, len_m, len_s, length;

      length = atoi (time_str);

      len_h = length / 3600;
      len_m = (length - (len_h * 3600)) / 60;
      len_s = (length - (len_h * 3600) - (len_m * 60));

      humanised = g_strdup_printf ("%02d:%02d:%02d",
                                   len_h, len_m, len_s);
      return humanised;
    }
  return NULL;
}

/**
 * mex_metadata_info_new:
 * @key: Metadata key
 * @key_string: A field label for the metadata value
 * @priority: How important this metadata is. 0 = High
 *
 * Container to pass through mex_metadata_get_metadata
 *
 * Return value: a new MexMetadataInfo.
 */
MexMetadataInfo *
mex_metadata_info_new (MexContentMetadata key,
                      const  gchar *key_string,
                      gint priority)
{
  MexMetadataInfo *info;

  info = g_new0 (MexMetadataInfo, 1);

  info->key = key;
  info->key_string = key_string;
  info->priority = priority;
  info->value = NULL;

  return info;
}

/**
 * mex_metadata_info_free:
 * @info: MexMetadaInfo
 *
 * Free/0 the values of MexMetadataInfo
 *
 * Return value: a new MexMetadataInfo.
 */
void
mex_metadata_info_free (MexMetadataInfo *info)
{
  if (!info)
    return;

  info->key = 0;
  info->priority = 0;
  info->value = NULL;
  info->key_string = NULL;

  g_free (info);
}

/**
 * mex_metadata_get_metadata:
 * @metadata_template: A GList with MexMetadataInfo data
 *
 * Fills in the values for the GList of MexMetadataInfo using the priority
 * value to provide fall back metadata. Where the highest priority is 0.
 *
 */
void
mex_metadata_get_metadata (GList **metadata_template, MexContent *content)
{
  GList *current;

  for (current = *metadata_template; current != NULL; current = current->next)
    {
      MexMetadataInfo *info_n;
      info_n = current->data;

      /* remove any values left from a previous call */
      info_n->value = NULL;

      /* if there is no previous item (i.e. this is the start) then set the
       * value. OR if the priority of the item is 0.
       */
      if (current->prev == NULL || info_n->priority == 0)
        {
          info_n->value = mex_content_get_metadata (content, info_n->key);
        }
      /* If the priority of the current item is greater than the previous item
       * and the previous item wasn't filled in then we're filling in a
       * fallback item.
       */
      else if (info_n->priority >
               ((MexMetadataInfo *)current->prev->data)->priority &&
               ((MexMetadataInfo *)current->prev->data)->value == NULL)
        {
          info_n->value = mex_content_get_metadata (content, info_n->key);
        }
    }
}
