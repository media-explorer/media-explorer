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

#include <glib.h>

#include <mex.h>

static const gchar *uri_provider_channels[3 * 2] =
{
  "BBC1", "file:///dev/null",
  "TF1", "udp://224.1.2.3:4567",
  "E!", "file:///home/meego/Videos/e.ts"
};

static void
test_uri_channel_provider (void)
{
  MexChannelProvider *provider;
  MexUriChannelProvider *uri_provider;
  gchar *config_file;
  const GPtrArray *channels;
  guint i;

  config_file = g_build_filename (TESTDATADIR, "channels-uri.conf", NULL);

  /* construction */
  provider = mex_uri_channel_provider_new (config_file);
  g_assert (provider);

  uri_provider = MEX_URI_CHANNEL_PROVIDER (provider);
  g_assert_cmpstr (config_file,
                   ==,
                   mex_uri_channel_provider_get_config_file (uri_provider));

  /* MexChannelProvider */
  g_assert_cmpuint (mex_channel_provider_get_n_channels (provider),
                    ==,
                    3);

  channels = mex_channel_provider_get_channels (provider);
  g_assert_cmpuint (channels->len, ==, 3);
  for (i = 0; i < channels->len; i++)
    {
      MexChannel *channel = g_ptr_array_index (channels, i);

      g_assert_cmpstr (mex_channel_get_name (channel),
                       ==,
                       uri_provider_channels[2 * i]);
      g_assert_cmpstr (mex_channel_get_uri (channel), ==, uri_provider_channels[2 * i + 1]);
    }

  g_free (config_file);
}

static void
test_manager_simple (void)
{
  MexChannelProvider *provider;
  MexChannelManager *manager;
  const GPtrArray *channels;
  gchar *config_file;
  guint i;

  config_file = g_build_filename (TESTDATADIR, "channels-uri.conf", NULL);
  provider = mex_uri_channel_provider_new (config_file);
  manager = mex_channel_manager_get_default ();

  /* Add a URI provider to the known channels */
  mex_channel_manager_add_provider (manager, provider);

  /* now we should be able to free the provider, the manager is the class
   * having all the knownledge about channels at the end */
  g_object_unref (provider);

  channels = mex_channel_manager_get_channels (manager);
  g_assert_cmpuint (channels->len, ==, 3);
  for (i = 0; i < channels->len; i++)
    {
      MexChannel *channel = g_ptr_array_index (channels, i);

      g_assert_cmpstr (mex_channel_get_name (channel),
                       ==,
                       uri_provider_channels[2 * i]);
      g_assert_cmpstr (mex_channel_get_uri (channel),
                       ==,
                       uri_provider_channels[2 * i + 1]);
    }

  g_free (config_file);
}

/*
 * The goal of this test is to ensure the various defines we have to tune
 * the dvb card map to the dvbsrc ones (The numeric values have been
 * obtained by running gst-inspect dvbsrc).
 */
static void
test_gst_defines (void)
{
  g_assert_cmpuint (MEX_DVB_BANDWIDTH_8M,   ==, 0);
  g_assert_cmpuint (MEX_DVB_BANDWIDTH_7M,   ==, 1);
  g_assert_cmpuint (MEX_DVB_BANDWIDTH_6M,   ==, 2);
  g_assert_cmpuint (MEX_DVB_BANDWIDTH_AUTO, ==, 3);

  g_assert_cmpuint (MEX_DVB_CODE_RATE_NONE, ==, 0);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_1_2,  ==, 1);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_2_3,  ==, 2);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_3_4,  ==, 3);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_4_5,  ==, 4);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_5_6,  ==, 5);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_6_7,  ==, 6);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_7_8,  ==, 7);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_8_9,  ==, 8);
  g_assert_cmpuint (MEX_DVB_CODE_RATE_AUTO, ==, 9);

  g_assert_cmpuint (MEX_DVB_GUARD_32,   ==, 0);
  g_assert_cmpuint (MEX_DVB_GUARD_16,   ==, 1);
  g_assert_cmpuint (MEX_DVB_GUARD_8,    ==, 2);
  g_assert_cmpuint (MEX_DVB_GUARD_4,    ==, 3);
  g_assert_cmpuint (MEX_DVB_GUARD_AUTO, ==, 4);

  g_assert_cmpuint (MEX_DVB_MODULATION_QPSK,   ==, 0);
  g_assert_cmpuint (MEX_DVB_MODULATION_QAM16,  ==, 1);
  g_assert_cmpuint (MEX_DVB_MODULATION_QAM32,  ==, 2);
  g_assert_cmpuint (MEX_DVB_MODULATION_QAM64,  ==, 3);
  g_assert_cmpuint (MEX_DVB_MODULATION_QAM128, ==, 4);
  g_assert_cmpuint (MEX_DVB_MODULATION_QAM256, ==, 5);
  g_assert_cmpuint (MEX_DVB_MODULATION_AUTO,   ==, 6);
  g_assert_cmpuint (MEX_DVB_MODULATION_8VSB,   ==, 7);
  g_assert_cmpuint (MEX_DVB_MODULATION_16VSB,  ==, 8);

  g_assert_cmpuint (MEX_DVB_TRANSMISSION_MODE_2K,   ==, 0);
  g_assert_cmpuint (MEX_DVB_TRANSMISSION_MODE_8K,   ==, 1);
  g_assert_cmpuint (MEX_DVB_TRANSMISSION_MODE_AUTO, ==, 2);

  g_assert_cmpuint (MEX_DVB_HIERARCHY_NONE, ==, 0);
  g_assert_cmpuint (MEX_DVB_HIERARCHY_1,    ==, 1);
  g_assert_cmpuint (MEX_DVB_HIERARCHY_2,    ==, 2);
  g_assert_cmpuint (MEX_DVB_HIERARCHY_4,    ==, 3);
  g_assert_cmpuint (MEX_DVB_HIERARCHY_AUTO, ==, 4);

  g_assert_cmpuint (MEX_DVB_INVERSION_OFF,  ==, 0);
  g_assert_cmpuint (MEX_DVB_INVERSION_ON,   ==, 1);
  g_assert_cmpuint (MEX_DVB_INVERSION_AUTO, ==, 2);
}

int
main (int   argc,
      char *argv[])
{
    g_type_init ();
    g_test_init (&argc, &argv, NULL);
    mex_init (&argc, &argv);

    g_test_add_func ("/dvb/channel-provider/uri", test_uri_channel_provider);
    g_test_add_func ("/dvb/channel-manager/creation", test_manager_simple);
    g_test_add_func ("/dvb/gst-defines", test_gst_defines);

    return g_test_run ();
}
