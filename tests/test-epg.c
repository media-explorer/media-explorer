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

#include <mex/mex.h>

static GMainLoop *main_loop;

static void
on_provider_ready_channels (MexEpgProvider *provider,
			    gpointer        user_data)
{
  GHashTable *channel2id;
  const gchar *id;

  channel2id =
    mex_epg_radiotimes_get_channel_ids (MEX_EPG_RADIOTIMES (provider));

  g_assert_cmpint (g_hash_table_size (channel2id), ==, 386);
  id = g_hash_table_lookup (channel2id, "BBC1 London");
  g_assert (id);
  g_assert_cmpstr (id, ==, "94");

  if (main_loop)
    {
      g_main_loop_quit (main_loop);
      g_main_loop_unref (main_loop);
      main_loop = NULL;
    }
}

static void
test_radiotimes_channels (void)
{
  MexEpgProvider *provider;

  provider = g_object_new (MEX_TYPE_EPG_RADIOTIMES,
			   "base-url", "file://" ABS_TESTDATADIR,
			   NULL);

  if (mex_epg_provider_is_ready (provider))
    {
      on_provider_ready_channels (provider, NULL);
    }
  else
    {
      g_signal_connect (provider, "epg-provider-ready",
                        G_CALLBACK (on_provider_ready_channels), NULL);
      main_loop = g_main_loop_new (NULL, TRUE);
      g_main_loop_run (main_loop);
    }
}

typedef struct
{
  MexChannel *channel;
  GDateTime *start_date;
  GDateTime *end_date;
} TestData;

TestData test_data;

typedef struct
{
  const gchar *start_date, *end_date;
  const gchar *metadata[7];
} Event;

MexContentMetadata index2key[7] =
{
  MEX_CONTENT_METADATA_TITLE,
  MEX_CONTENT_METADATA_SUB_TITLE,
  MEX_CONTENT_METADATA_EPISODE,
  MEX_CONTENT_METADATA_YEAR,
  MEX_CONTENT_METADATA_DIRECTOR,
  MEX_CONTENT_METADATA_SYNOPSIS,
  MEX_CONTENT_METADATA_DURATION
};

Event test_events[3] =
{
  {
    "23/09/2010 22:35",
    "23/09/2010 23:35",
    {
      "Question Time",
      NULL,
      NULL,
      NULL,
      NULL,
      "David Dimbleby chairs a debate from the Liberal Democrats conference "
      "in Liverpool, as panellists Business Secretary Vince Cable, Private "
      "Eye editor Ian Hislop, Labour MP Caroline Flint, Conservative MP John "
      "Redwood, and the New Statesman's Senior Editor Mehdi Hasan face "
      "topical questions from the audience.",
      "3600"
    }
  },
  {
    "23/09/2010 23:35",
    "24/09/2010 00:25",
    {
      "This Week",
      NULL,
      NULL,
      NULL,
      NULL,
      "Overview of the leading political stories from the past seven days, "
      "with Andrew Neil, Michael Portillo and Diane Abbott.",
      "3000"
    }
  },
  {
    "24/09/2010 00:25",
    "24/09/2010 00:55",
    {
      "Panorama",
      NULL,
      "Because We're Worth It - The Taxpayers' Rich List",
      NULL,
      NULL,
      "Vivian White reports on the findings of an investigation into the "
      "salaries of people in the public sector, from council chiefs to head "
      "teachers, police officers and BBC bosses. Using the Freedom of "
      "Information Act, 2,400 organisations were contacted to obtain details "
      "of employees' pay, and the programme examines whether some positions "
      "are deserving of higher incomes than the prime minister.",
      "1800"
    }
  }
};

static void
verify_event (MexEpgEvent *event,
	      Event *expected,
	      TestData *verify)
{
  MexProgram *program;
  gchar *show_start_s, *show_end_s;
  GDateTime *show_start, *show_end;
  gboolean start_in_between, end_in_between;
  guint i;

  /* first verify if we've parsed the right values for start and end dates
   * of the program */
  show_start = mex_epg_event_get_start_date (event);
  show_start_s = g_date_time_format (show_start, "%d/%m/%Y %H:%M");
  g_assert_cmpstr (expected->start_date, ==, show_start_s);

  show_end = mex_epg_event_get_end_date (event);
  show_end_s = g_date_time_format (show_end, "%d/%m/%Y %H:%M");
  g_assert_cmpstr (expected->end_date, ==, show_end_s);

  /* Then test if the results are actually in the right range */
  start_in_between = g_date_time_compare (show_start, verify->start_date) >= 0
		     && g_date_time_compare (show_start, verify->end_date) < 0;
  end_in_between = g_date_time_compare (show_end, verify->start_date) >= 0 &&
		   g_date_time_compare (show_end, verify->end_date) < 0;
  g_assert (start_in_between || end_in_between);

  /* finally check the metadata of the program associated with the event */
  program = mex_epg_event_get_program (event);
  for (i = 0; i < G_N_ELEMENTS (index2key); i++)
    {
      const gchar *metadata;

      metadata = mex_content_get_metadata (MEX_CONTENT (program), index2key[i]);
      g_assert_cmpstr (expected->metadata[i], ==, metadata);
    }

  g_date_time_unref (show_start);
  g_date_time_unref (show_end);
  g_free (show_start_s);
  g_free (show_end_s);
}

static void
on_epg_data_received_full (MexEpgProvider *provider,
			   MexChannel     *channel,
			   GPtrArray      *events,
			   gpointer        user_data)
{
  TestData *verify = user_data;
  guint i;

  g_assert (verify == &test_data);
  g_assert (channel == verify->channel);
  g_assert_cmpstr (mex_channel_get_name (channel), ==, "BBC1 London");

  g_assert_cmpint (events->len, ==, G_N_ELEMENTS (test_events));
  for (i = 0; i < G_N_ELEMENTS (test_events); i++)
    verify_event (g_ptr_array_index (events, i), &test_events[i], verify);

  g_main_loop_quit (main_loop);
  g_main_loop_unref (main_loop);
  main_loop = NULL;
}

static void
on_provider_ready_full (MexEpgProvider *provider,
			gpointer        user_data)
{
  MexChannel *bbc1;
  GDateTime *start_date, *end_date;

  bbc1 = mex_channel_new ();
  mex_channel_set_name (bbc1, "BBC1 London");
  start_date = g_date_time_new_local (2010, 9, 23, 22, 45, 0.0);
  end_date = g_date_time_new_local (2010, 9, 24, 0, 50, 0.0);

  test_data.channel = bbc1;
  test_data.start_date = start_date;
  test_data.end_date = end_date;

  mex_epg_provider_get_events (provider, bbc1, start_date, end_date,
			       on_epg_data_received_full, &test_data);
}

static void
test_radiotimes_full (void)
{
  MexEpgProvider *provider;

  provider = g_object_new (MEX_TYPE_EPG_RADIOTIMES,
			   "base-url", "file://" ABS_TESTDATADIR,
			   NULL);

  if (mex_epg_provider_is_ready (provider))
    {
      on_provider_ready_full (provider, NULL);
    }
  else
    {
      g_signal_connect (provider, "epg-provider-ready",
                        G_CALLBACK (on_provider_ready_full), NULL);
    }

  main_loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (main_loop);
}

int
main (int   argc,
      char *argv[])
{
    g_type_init ();
    g_test_init (&argc, &argv, NULL);
    mex_init (&argc, &argv);

    g_test_add_func ("/epg/radiotimes/channels", test_radiotimes_channels);
    g_test_add_func ("/epg/radiotimes", test_radiotimes_full);

    return g_test_run ();
}
