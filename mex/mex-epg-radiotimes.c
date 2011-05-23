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


/*
 * This documentation does not seem to be on the radiotimes web site, I've
 * found the original mail/post from the author else where.
 *
 * I have developed a solution that will still supply you with all the data
 * XMLTV needs but will not cause us any more problems.
 *
 * This solution is based around a channel list file and multiple channels
 * files containing 14 days of listings. These will be produced by 08:00 every
 * morning.
 *
 * 1. Channel List file - http://xmltv.radiotimes.com/xmltv/channels.dat
 *
 * This file lists all the the TV channels, the fields are CHANNEL_ID and
 * CHANNEL_NAME and are separated by a pipe "|", and ended by a new line e.g.
 *
 * 22|Adventure One
 * 24|ITV1 Anglia
 * 25|ITV1 Border
 *
 * 2. Channel files - http://xmltv.radiotimes.com/xmltv/<CHANNEL_ID.dat
 *
 * Each channel file (identified by it's ID) called CHANNEL_ID.dat (e.g.
 * 24.dat) contains 14 days of listings for that channel. The fields are
 * separated by a tilda "~", and ended by a new line. The fields that are
 * supplied are :
 *
 *   - Programme Title
 *   - Sub-Title
 *   - Episode
 *   - Year
 *   - Director
 *   - Performers (Cast) - This will be either a string containing the Actors
 *     names or be made up of Character name and Actor name pairs which are
 *     separated by an asterix "*" and each pair by pipe "|"
 *       e.g. Rocky*Sylvester Stallone|Terminator*Arnold Schwarzenegger.
 *   - Premiere
 *   - Film
 *   - Repear
 *   - Subtitles
 *   - Widescreen
 *   - New series
 *   - Deaf signed
 *   - Black and White
 *   - Film star rating
 *   - Film certificate
 *   - Genre
 *   - Description
 *   - Radio Times Choice - This means that the Radio Times editorial team
 *     have marked it as a choice
 *   - Date
 *   - Start Time
 *   - End Time
 *   - Duration (Minutes)
 *
 * Notes :
 *
 * 1. Can you make sure that there is a notice alongside the instructions that
 *    all
 *    - data is the copyright of the Radio Times website
 *      (http://www.radiotimes.com)
 *    - and that the use of this data is restricted to personal use only.
 *
 * 2. Also, to give you and your users some warning, we may have to look at
 *    * implementing this as a paid-for service in the future. Of course, we
 *      will keep you up-to-date with any such developments.
 *
 * 3. Finally, as you have mentioned in the past can you code your applications
 *    to have a useful User-Agent so we can identify the different application
 *    using the data.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include "mex-content.h"
#include "mex-debug.h"
#include "mex-download-queue.h"
#include "mex-epg-event.h"
#include "mex-program.h"

#include "mex-epg-radiotimes.h"

static void mex_epg_provider_iface_init (MexEpgProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexEpgRadiotimes,
                         mex_epg_radiotimes,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_EPG_PROVIDER,
                                                mex_epg_provider_iface_init))

#define EPG_RADIOTIMES_PRIVATE(o)                           \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o),                      \
                                  MEX_TYPE_EPG_RADIOTIMES,  \
                                  MexEpgRadiotimesPrivate))

#define RADIOTIMES_BASE_URL   "http://xmltv.radiotimes.com/xmltv"

typedef enum {
  MEX_RT_KEY_TITLE,
  MEX_RT_KEY_SUB_TITLE,
  MEX_RT_KEY_EPISODE,
  MEX_RT_KEY_YEAR,
  MEX_RT_KEY_DIRECTORS,
  MEX_RT_KEY_CAST,
  MEX_RT_KEY_PREMIERE,           /* true | false */
  MEX_RT_KEY_FILM,               /* true | false */
  MEX_RT_KEY_REPEAR,             /* true | false */
  MEX_RT_KEY_SUBTITLES,          /* true | false */
  MEX_RT_KEY_WIDESCREEN,         /* true | false */
  MEX_RT_KEY_NEW_SERIES,         /* true | false */
  MEX_RT_KEY_DEAF_SIGNED,        /* true | false */
  MEX_RT_KEY_BLACK_AND_WHITE,    /* true | false */
  MEX_RT_KEY_FILM_STAR_RATING,
  MEX_RT_KEY_FILM_CERTIFICATE,
  MEX_RT_KEY_GENRE,
  MEX_RT_KEY_DESCRIPTION,
  MEX_RT_KEY_RADIO_TIMES_CHOICE,
  MEX_RT_KEY_DATE,
  MEX_RT_KEY_START_TIME,
  MEX_RT_KEY_END_TIME,
  MEX_RT_KEY_DURATION,           /* minutes */
  MEX_RT_KEY_LAST
} MexRTKey;

static MexContentMetadata field2key[MEX_RT_KEY_LAST] =
{
  MEX_CONTENT_METADATA_TITLE,
  MEX_CONTENT_METADATA_SUB_TITLE,
  MEX_CONTENT_METADATA_EPISODE,
  MEX_CONTENT_METADATA_YEAR,
  MEX_CONTENT_METADATA_DIRECTOR,
  MEX_CONTENT_METADATA_NONE,        /* cast */
  MEX_CONTENT_METADATA_NONE,        /* premiere */
  MEX_CONTENT_METADATA_NONE,        /* film */
  MEX_CONTENT_METADATA_NONE,        /* repear */
  MEX_CONTENT_METADATA_NONE,        /* subtitles */
  MEX_CONTENT_METADATA_NONE,        /* widescreen */
  MEX_CONTENT_METADATA_NONE,        /* new series */
  MEX_CONTENT_METADATA_NONE,        /* deaf signed */
  MEX_CONTENT_METADATA_NONE,        /* black and white */
  MEX_CONTENT_METADATA_NONE,        /* rating */
  MEX_CONTENT_METADATA_NONE,        /* film certificate */
  MEX_CONTENT_METADATA_NONE,        /* genre */
  MEX_CONTENT_METADATA_SYNOPSIS,
  MEX_CONTENT_METADATA_NONE,        /* radio times choice */
  MEX_CONTENT_METADATA_NONE,        /* date */
  MEX_CONTENT_METADATA_NONE,        /* start time */
  MEX_CONTENT_METADATA_NONE,        /* end time */
  MEX_CONTENT_METADATA_DURATION
};

enum
{
  PROP_0,

  PROP_BASE_URL
};

struct _MexEpgRadiotimesPrivate
{
  gchar *base_url;
  GHashTable *channel2id;   /* exists when we've parsed channels.dat */
};

typedef struct
{
  MexEpgProvider *provider;
  MexChannel *channel;
  GDateTime *start_date, *end_date;
  MexEpgProviderReply callback;
  gpointer user_data;
} Request;

/*
 * mex_epg_radiotimes_set_base_url:
 * @radiotimes: a #MexEpgRadiotimes
 * @base_url: the base URL
 *
 * Sets the base URL the Radiotome epg fetcher should use to retrieve
 * data. This is mainly for test purposes where we want to use local data
 * to check against.
 */
static void
mex_epg_radiotimes_set_base_url (MexEpgRadiotimes *radiotimes,
                                 const gchar      *base_url)
{
  MexEpgRadiotimesPrivate *priv = radiotimes->priv;

  g_free (priv->base_url);
  priv->base_url = g_strdup (base_url);
}

static gboolean
parse_channels_dat_line (MexEpgRadiotimes *provider,
                         const gchar      *line)
{
  MexEpgRadiotimesPrivate *priv = provider->priv;
  gchar **fields, *id, *name;

  fields = g_strsplit (line, "|", 0);
  if (!(fields[0] && fields[1]))
    {
      g_warning ("Invalid channel definition in channels.dat: %s", line);
      g_strfreev (fields);
      return FALSE;
    }

  id = fields[0];
  name = fields[1];

  g_hash_table_insert (priv->channel2id, name, id);

  /* we only free the array here as we keep the fields in the hash table */
  g_free (fields);

  return TRUE;
}

static void
on_channel_dat_received (MexDownloadQueue *queue,
                         const char       *uri,
                         const char       *buffer,
                         gsize             count,
                         const GError     *dq_error,
                         gpointer          userdata)
{
  MexEpgRadiotimes *provider = MEX_EPG_RADIOTIMES (userdata);
  MexEpgRadiotimesPrivate *priv = provider->priv;
  GInputStream *input;
  GDataInputStream *data;
  GError *error = NULL;
  gchar *line;

  MEX_NOTE (EPG, "received %s, size %"G_GSIZE_FORMAT, uri, count);

  if (dq_error)
    {
      g_warning ("Could not download %s: %s", uri, dq_error->message);
      return;
    }

  /* prepare channel2id hash table */
  if (priv->channel2id)
    g_hash_table_unref (priv->channel2id);

  priv->channel2id = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, g_free);

  /* parse the date line by line */
  input = g_memory_input_stream_new_from_data (buffer, count, NULL);
  data = g_data_input_stream_new (input);

  /* The first line is empty */
  line = g_data_input_stream_read_line (data, NULL, NULL, &error);
  g_free (line);

  /* The second line is the disclamer*/
  line = g_data_input_stream_read_line (data, NULL, NULL, &error);
  g_free (line);

  line = g_data_input_stream_read_line (data, NULL, NULL, &error);
  while (line)
    {
      parse_channels_dat_line (provider, line);
      g_free (line);
      line = g_data_input_stream_read_line (data, NULL, NULL, &error);
    }
  if (G_UNLIKELY (error))
    {
      g_warning ("Could not read line: %s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (data);
  g_object_unref (input);

  g_signal_emit_by_name (provider, "epg-provider-ready", 0);
}

static void
mex_epg_radiotimes_grab_channel_list (MexEpgRadiotimes *provider)
{
  MexEpgRadiotimesPrivate *priv = provider->priv;
  gchar *channels_dat_url;
  MexDownloadQueue *dq;

  dq = mex_download_queue_get_default ();
  channels_dat_url = g_strconcat (priv->base_url, "/channels.dat", NULL);
  mex_download_queue_enqueue (dq, channels_dat_url,
                              on_channel_dat_received, provider);
  g_free (channels_dat_url);
}

/*
 * MexEpgProvider interface
 */

static gboolean
mex_epg_radiotimes_is_ready (MexEpgProvider *provider)
{
  MexEpgRadiotimes *radiotimes = MEX_EPG_RADIOTIMES (provider);

  return radiotimes->priv->channel2id != NULL;
}

static gchar *
cut_last_field_out (const gchar *line)
{
  gchar *field;

  field = strrchr (line, '~');
  if (field == NULL)
    return NULL;

  *field = '\0';
  field++;

  return field;
}

static gchar *
cut_first_field_out (gchar **linep)
{
  gchar *field = *linep, *end;

  end = strchr (field, '~');
  if (end == NULL)
    {
      /* nothing left to cut, return the whole string */
      *linep = NULL;
      return field;
    }

  *end = '\0';
  end++;
  *linep = end;

  return field;
}

static MexProgram *
parse_program (gchar *line)
{
  MexProgram *program;
  gchar *field;
  guint i;

  program = mex_program_new (NULL);
  /* n - 4 because we've already parsed date, start, end, duration */
  for (i = 0; i < G_N_ELEMENTS (field2key) - 4; i++)
    {
      field = cut_first_field_out (&line);
      if (field == NULL)
        {
          g_object_unref (program);
          return NULL;
        }

      if (strcmp (field, "") == 0)
        continue;

      if (field2key[i] == MEX_CONTENT_METADATA_NONE)
        continue;

      MEX_NOTE (EPG, "metadata %s: %s",
                 mex_content_metadata_key_to_string (field2key[i]), field);

      mex_content_set_metadata (MEX_CONTENT (program), field2key[i], field);
    }

  return program;
}

static gboolean
parse_epg_dat_line (Request   *req,
                    gchar     *line,
                    GPtrArray *events)
{
  gchar *duration_s, *start_time_s, *end_time_s, *date_s;
  gboolean start_in_between, end_in_between, englobing;
  gint year, month, day, hours, minutes, duration;
  GDateTime *start_date, *end_date;
  gint n_parsed;

  duration_s = cut_last_field_out (line);
  if (duration_s == NULL)
    goto no_duration;

  end_time_s = cut_last_field_out (line);
  if (end_time_s == NULL)
    goto no_end_time;

  start_time_s = cut_last_field_out (line);
  if (start_time_s == NULL)
    goto no_start_time;

  date_s = cut_last_field_out (line);
  if (date_s == NULL)
    goto no_date;

  n_parsed = sscanf (date_s, "%d/%d/%d", &day, &month, &year);
  if (n_parsed != 3)
    goto scanf_failed;

  n_parsed = sscanf (start_time_s, "%d:%d", &hours, &minutes);
  if (n_parsed != 2)
    goto scanf_failed;

  duration = atoi (duration_s);
  if (duration == 0)
    return TRUE;

  /* duration is always is seconds in Mex, minutes in the data files */
  duration *= 60;

  start_date = g_date_time_new_local (year, month, day, hours, minutes, 0);
  end_date = g_date_time_add_seconds (start_date, duration);

  start_in_between = g_date_time_compare (start_date, req->start_date) >= 0
		     && g_date_time_compare (start_date, req->end_date) <= 0;
  end_in_between = g_date_time_compare (end_date, req->start_date) >= 0 &&
		   g_date_time_compare (end_date, req->end_date) <= 0;
  englobing = g_date_time_compare (start_date, req->start_date) <= 0 &&
              g_date_time_compare (end_date, req->end_date) >= 0;

  if (start_in_between || end_in_between || englobing)
    {
      MexEpgEvent *event;
      MexProgram *program;

      event = mex_epg_event_new_with_date_time (start_date, duration);

      program = parse_program (line);
      if (program == NULL)
        goto program_failed;

      /* we add the duration here as parse_program don't do it and that we
       * need the duration in seconds instead of minutes */
      duration_s = g_strdup_printf ("%d", duration);
      mex_content_set_metadata (MEX_CONTENT (program),
                                MEX_CONTENT_METADATA_DURATION,
                                duration_s);
      g_free (duration_s);

      mex_epg_event_set_program (event, program);
      g_object_unref (program);

      mex_epg_event_set_channel (event, req->channel);

      g_ptr_array_add (events, event);
    }

  return TRUE;

program_failed:
  MEX_WARN (EPG, "could not create the program: %s", line);
  return FALSE;
scanf_failed:
  MEX_WARN (EPG, "could not parse date or time: %s", line);
  return FALSE;
no_date:
  MEX_WARN (EPG, "could not find the date: %s", line);
  return FALSE;
no_start_time:
  MEX_WARN (EPG, "could not find the start time: %s", line);
  return FALSE;
no_end_time:
  MEX_WARN (EPG, "could not find the end time: %s", line);
  return FALSE;
no_duration:
  MEX_WARN (EPG, "could not find the duration: %s", line);
  return FALSE;
}

static void
on_epg_dat_received (MexDownloadQueue *queue,
                     const char       *uri,
                     const char       *buffer,
                     gsize             count,
                     const GError     *dq_error,
                     gpointer          user_data)
{
  Request *req = user_data;
  GInputStream *input;
  GDataInputStream *data;
  GError *error = NULL;
  GPtrArray *events;
  gchar *line;

  if (dq_error)
    {
      g_warning ("Could not download %s: %s", uri, dq_error->message);
      return;
    }

  MEX_NOTE (EPG, "received %s, size %"G_GSIZE_FORMAT, uri, count);

  events = g_ptr_array_new_with_free_func (g_object_unref);

  /* parse the date line by line */
  input = g_memory_input_stream_new_from_data (buffer, count, NULL);
  data = g_data_input_stream_new (input);
  g_data_input_stream_set_newline_type (data, G_DATA_STREAM_NEWLINE_TYPE_CR_LF);

  /* The first line is empty */
  line = g_data_input_stream_read_line (data, NULL, NULL, &error);
  g_free (line);

  /* The second line is the disclamer*/
  line = g_data_input_stream_read_line (data, NULL, NULL, &error);
  g_free (line);

  line = g_data_input_stream_read_line (data, NULL, NULL, &error);
  while (line)
    {
      parse_epg_dat_line (req, line, events);
      g_free (line);
      line = g_data_input_stream_read_line (data, NULL, NULL, &error);
    }
  if (G_UNLIKELY (error))
    {
      g_warning ("Could not read line: %s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (data);
  g_object_unref (input);

  req->callback (req->provider, req->channel, events, req->user_data);

  g_date_time_unref (req->start_date);
  g_date_time_unref (req->end_date);
  g_ptr_array_unref (events);
  g_slice_free (Request, req);
}

static void
mex_epg_radiotimes_get_events (MexEpgProvider      *provider,
                               MexChannel          *channel,
                               GDateTime           *start_date,
                               GDateTime           *end_date,
                               MexEpgProviderReply  reply,
                               gpointer             user_data)
{
  MexEpgRadiotimes *radiotimes = MEX_EPG_RADIOTIMES (provider);
  MexEpgRadiotimesPrivate *priv = radiotimes->priv;
  const gchar *name, *id;
  MexDownloadQueue *dq;
  gchar *data_url;
  Request *req;

  name = mex_channel_get_name (channel);
  id = g_hash_table_lookup (priv->channel2id, name);
  if (id == NULL)
    reply (provider, channel, NULL, user_data);

  req = g_slice_new (Request);
  req->provider = provider;
  req->channel = channel;
  req->start_date = g_date_time_ref (start_date);
  req->end_date = g_date_time_ref (end_date);
  req->callback = reply;
  req->user_data = user_data;

  dq = mex_download_queue_get_default ();

  data_url = g_strconcat (priv->base_url, "/", id, ".dat", NULL);
  mex_download_queue_enqueue (dq, data_url, on_epg_dat_received, req);
  g_free (data_url);
}

static void
mex_epg_provider_iface_init (MexEpgProviderInterface *iface)
{
  iface->is_ready = mex_epg_radiotimes_is_ready;
  iface->get_events = mex_epg_radiotimes_get_events;
}

/*
 * GObject implementation
 */

static void
mex_epg_radiotimes_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MexEpgRadiotimes *radiotimes = MEX_EPG_RADIOTIMES (object);
  MexEpgRadiotimesPrivate *priv = radiotimes->priv;

  switch (property_id)
    {
    case PROP_BASE_URL:
      g_value_set_string (value, priv->base_url);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_radiotimes_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MexEpgRadiotimes *radiotimes = MEX_EPG_RADIOTIMES (object);

  switch (property_id)
    {
    case PROP_BASE_URL:
      mex_epg_radiotimes_set_base_url (radiotimes, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_radiotimes_constructed (GObject *object)
{
  MexEpgRadiotimes *radiotimes = MEX_EPG_RADIOTIMES (object);

  mex_epg_radiotimes_grab_channel_list (radiotimes);
}

static void
mex_epg_radiotimes_finalize (GObject *object)
{
  MexEpgRadiotimes *provider = MEX_EPG_RADIOTIMES (object);
  MexEpgRadiotimesPrivate *priv = provider->priv;

  g_free (priv->base_url);
  if (priv->channel2id)
    g_hash_table_unref (priv->channel2id);

  G_OBJECT_CLASS (mex_epg_radiotimes_parent_class)->finalize (object);
}

static void
mex_epg_radiotimes_class_init (MexEpgRadiotimesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexEpgRadiotimesPrivate));

  object_class->get_property = mex_epg_radiotimes_get_property;
  object_class->set_property = mex_epg_radiotimes_set_property;
  object_class->finalize = mex_epg_radiotimes_finalize;
  object_class->constructed = mex_epg_radiotimes_constructed;

  pspec = g_param_spec_string ("base-url",
			       "Base URL",
			       "The base URL to use when fetching data",
			       RADIOTIMES_BASE_URL,
			       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_BASE_URL, pspec);
}

static void
mex_epg_radiotimes_init (MexEpgRadiotimes *self)
{
  self->priv = EPG_RADIOTIMES_PRIVATE (self);
}

MexEpgProvider *
mex_epg_radiotimes_new (void)
{
  return g_object_new (MEX_TYPE_EPG_RADIOTIMES, NULL);
}

/**
 * mex_epg_radiotimes_get_channel_ids:
 * @self: a #MexEpgRadiotimes provider
 *
 * This functions retrieves a somewhat internal hash table that maps the
 * channel name to the it's ID in channels.dat. This function should be
 * mainly used by the unit tests.
 *
 * Return value: A channels to id #GHashTable.
 *
 * Since: 0.2
 */
GHashTable *
mex_epg_radiotimes_get_channel_ids (MexEpgRadiotimes *self)
{
  g_return_val_if_fail (MEX_IS_EPG_RADIOTIMES (self), NULL);

  return self->priv->channel2id;
}
