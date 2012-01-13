/*
 * Mex - a media explorer
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
 *
 * Authors:
 *   Damien Lespiau <damien.lespiau@intel.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gio/gio.h>

#include <mex/mex-plugin.h>
#include <mex/mex-dvbt-channel.h>
#include <mex/mex-settings.h>
#include <mex/mex-channel-manager.h>

#include "mex-dvb-plugin.h"

#define MEX_LOG_DOMAIN_DEFAULT  dvb_log_domain
MEX_LOG_DOMAIN_STATIC (dvb_log_domain);

G_DEFINE_TYPE (MexDvbPlugin, mex_dvb_plugin, MEX_TYPE_PLUGIN)

#define DVB_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_DVB_PLUGIN, MexDvbPluginPrivate))

struct _MexDvbPluginPrivate
{
  gint dummy;
};

static gint
get_n_fields (gchar **fields)
{
  gint i = 0;

  if (fields == NULL)
    return 0;

  while (fields[i++] != NULL);

  return i - 1;
}

static const gboolean
parse_service_id (MexChannel *channel,
                  gchar      *field)
{
  gchar *semi_colon;

  if (field == NULL)
    return FALSE;

  semi_colon = strchr (field, ';');
  if (semi_colon == NULL)
    return FALSE;

  *semi_colon = '\0';

  mex_channel_set_name (channel, field);
  mex_dvbt_channel_set_service_id (channel, field);

  return TRUE;
}

static gboolean
parse_frequency (MexChannel  *channel,
                 const gchar *field)
{
  gint frequency;

  frequency = atoi (field);
  if (frequency <= 0)
    return FALSE;

  mex_dvbt_channel_set_frequency (MEX_DVBT_CHANNEL (channel), frequency);

  return TRUE;
}

static MexDvbInversion
vdr_inversion_to_enum (gint in)
{
  switch (in)
    {
    case 0:
      return MEX_DVB_INVERSION_OFF;
    case 1:
      return MEX_DVB_INVERSION_ON;
    default:
      return MEX_DVB_INVERSION_AUTO;
    }
}

static MexDvbBandwidth
vdr_bandwidth_to_enum (gint in)
{
  switch (in)
    {
    case 8:
      return MEX_DVB_BANDWIDTH_8M;
    case 7:
      return MEX_DVB_BANDWIDTH_7M;
    case 6:
      return MEX_DVB_BANDWIDTH_6M;
    default:
      return MEX_DVB_BANDWIDTH_AUTO;
    }
}

static MexDvbCodeRate
vdr_code_rate_to_enum (gint in)
{
  switch (in)
    {
    case 0:
      return MEX_DVB_CODE_RATE_NONE;
    case 12:
      return MEX_DVB_CODE_RATE_1_2;
    case 23:
      return MEX_DVB_CODE_RATE_2_3;
    case 34:
      return MEX_DVB_CODE_RATE_3_4;
    case 45:
      return MEX_DVB_CODE_RATE_4_5;
    case 56:
      return MEX_DVB_CODE_RATE_5_6;
    case 67:
      return MEX_DVB_CODE_RATE_6_7;
    case 78:
      return MEX_DVB_CODE_RATE_7_8;
    case 89:
      return MEX_DVB_CODE_RATE_8_9;
    case 35:
    case 910:
      MEX_WARNING ("Unsupported code rate %d", in);
      return MEX_DVB_CODE_RATE_AUTO;
    default:
      return MEX_DVB_CODE_RATE_AUTO;
    }
}

static MexDvbModulation
vdr_modulation_to_enum (gint in)
{
  switch (in)
    {
    case 2:
      return MEX_DVB_MODULATION_QPSK;
    case 5:
    case 6:
    case 7:
      MEX_WARNING ("Unsupported modulation %d", in);
      return MEX_DVB_MODULATION_AUTO;
    case 10:
      return MEX_DVB_MODULATION_8VSB;
    case 11:
      return MEX_DVB_MODULATION_16VSB;
    case 16:
      return MEX_DVB_MODULATION_QAM16;
    case 32:
      return MEX_DVB_MODULATION_QAM32;
    case 64:
      return MEX_DVB_MODULATION_QAM64;
    case 128:
      return MEX_DVB_MODULATION_QAM128;
    case 256:
      return MEX_DVB_MODULATION_QAM256;
    default:
      return MEX_DVB_MODULATION_AUTO;
    }
}

static MexDvbTransmissionMode
vdr_transmission_mode_to_enum (gint in)
{
  switch (in)
    {
    case 2:
      return MEX_DVB_TRANSMISSION_MODE_2K;
    case 4:
      MEX_WARNING ("Unsupported transmission mode %d", in);
      return -1;
    case 8:
      return MEX_DVB_TRANSMISSION_MODE_8K;
    default:
      return MEX_DVB_TRANSMISSION_MODE_AUTO;
    }
}

static MexDvbGuard
vdr_guard_to_enum (gint in)
{
  switch (in)
    {
    case 4:
      return MEX_DVB_GUARD_4;
    case 8:
      return MEX_DVB_GUARD_8;
    case 16:
      return MEX_DVB_GUARD_16;
    case 32:
      return MEX_DVB_GUARD_32;
    default:
      return MEX_DVB_GUARD_AUTO;
    }
}

static MexDvbHierarchy
vdr_hierarchy_to_enum (gint in)
{
  switch (in)
    {
    case 0:
      return MEX_DVB_HIERARCHY_NONE;
    case 1:
      return MEX_DVB_HIERARCHY_1;
    case 2:
      return MEX_DVB_HIERARCHY_2;
    case 4:
      return MEX_DVB_HIERARCHY_4;
    default:
      return MEX_DVB_HIERARCHY_AUTO;
    }
}

static gboolean
parse_tuning_parameters (MexChannel  *channel,
                         const gchar *field)
{
  gint inversion_vdr, bandwidth_vdr, code_rate_hp_vdr, code_rate_lp_vdr,
       modulation_vdr, trans_mode_vdr, guard_vdr, hierarchy_vdr;
  MexDvbInversion inversion;
  MexDvbBandwidth bandwidth;
  MexDvbCodeRate code_rate_lp, code_rate_hp;
  MexDvbModulation modulation;
  MexDvbTransmissionMode trans_mode;
  MexDvbGuard guard;
  MexDvbHierarchy hierarchy;
  gint n;

  n = sscanf (field, "I%dB%dC%dD%dM%dT%dG%dY%d", &inversion_vdr, &bandwidth_vdr,
              &code_rate_hp_vdr, &code_rate_lp_vdr, &modulation_vdr,
              &trans_mode_vdr, &guard_vdr, &hierarchy_vdr);
  if (n != 8)
    return FALSE;

  inversion = vdr_inversion_to_enum (inversion_vdr);
  bandwidth = vdr_bandwidth_to_enum (bandwidth_vdr);
  code_rate_hp = vdr_code_rate_to_enum (code_rate_hp_vdr);
  code_rate_lp = vdr_code_rate_to_enum (code_rate_lp_vdr);
  modulation = vdr_modulation_to_enum (modulation_vdr);
  trans_mode = vdr_transmission_mode_to_enum (trans_mode_vdr);
  guard = vdr_guard_to_enum (guard_vdr);
  hierarchy = vdr_hierarchy_to_enum (hierarchy_vdr);

  if (inversion == -1 || bandwidth == -1 || code_rate_hp == -1 ||
      code_rate_lp == -1 || modulation == -1 || trans_mode == -1 ||
      guard == -1 || hierarchy == -1)
    {
      return FALSE;
    }

  mex_dvbt_channel_set_inversion (MEX_DVBT_CHANNEL (channel), inversion);
  mex_dvbt_channel_set_bandwidth (MEX_DVBT_CHANNEL (channel), bandwidth);
  mex_dvbt_channel_set_code_rate_hp (MEX_DVBT_CHANNEL (channel), code_rate_hp);
  mex_dvbt_channel_set_code_rate_lp (MEX_DVBT_CHANNEL (channel), code_rate_lp);
  mex_dvbt_channel_set_modulation (MEX_DVBT_CHANNEL (channel), modulation);
  mex_dvbt_channel_set_transmission_mode (MEX_DVBT_CHANNEL (channel),
                                          trans_mode);
  mex_dvbt_channel_set_guard (MEX_DVBT_CHANNEL (channel), guard);
  mex_dvbt_channel_set_hierarchy (MEX_DVBT_CHANNEL (channel), hierarchy);

  return TRUE;
}

static gboolean
parse_line (MexDvbPlugin  *plugin,
            const gchar   *line,
            MexChannel   **channel_out)
{
  gchar **fields;
  MexChannel *channel = NULL;
  gchar *error_message = "";

  fields = g_strsplit (line, ":", 0);
  if (get_n_fields (fields) != 14)
    {
      MEX_WARNING ("Expecting 14 fields, found %d", get_n_fields (fields));
      goto parse_error;
    }

  channel = mex_dvbt_channel_new ();

  if (parse_service_id (channel, fields[0]) == FALSE)
    {
      error_message = "Invalid service name";
      goto parse_error;
    }

  if (parse_frequency (channel, fields[1]) == FALSE)
    {
      error_message = "Invalid frequency";
      goto parse_error;
    }

  if (parse_tuning_parameters (channel, fields[2]) == FALSE)
    {
      error_message = "Invalid tuning parameters";
      goto parse_error;
    }

  if (g_strcmp0 (fields[3], "T") != 0)
    {
      error_message = "Expected 'T' field not found";
      goto parse_error;
    }

  if (g_strcmp0 (fields[4], "27500") != 0)
    {
      error_message = "Expected '27500' field not found";
      goto parse_error;
    }

  if (channel_out)
    *channel_out = channel;

  g_strfreev (fields);
  return TRUE;

parse_error:
  MEX_WARNING ("Could not parse %s%s", line, error_message);
  if (channel)
    g_object_unref (channel);
  g_strfreev (fields);
  return FALSE;
}

static GPtrArray *
load_channels_from_file (MexDvbPlugin *plugin)
{
  MexSettings *settings;
  GPtrArray *channels;
  gchar *channels_conf_path;
  GFile *channels_conf;
  GFileInputStream *input;
  GDataInputStream *data;
  GError *error = NULL;
  gchar *line;

  settings = mex_settings_get_default ();
  channels_conf_path = g_build_filename (mex_settings_get_config_dir (settings),
                                         "dvb", "channels.conf", NULL);
  channels_conf = g_file_new_for_path (channels_conf_path);

  input = g_file_read (channels_conf, NULL, &error);
  if (error && error->code == G_IO_ERROR_NOT_FOUND )
    {
      /* Obviously this will have to trigger the scanning UI */
      MEX_INFO ("channels.conf missing, need to run the scanner (%s)",
                channels_conf_path);
      g_clear_error (&error);
      goto read_error;
    }
  else if (error)
    {
      MEX_WARNING ("Could not read config file %s: %s", channels_conf_path,
                   error->message);
      g_clear_error (&error);
      goto read_error;
    }

  channels = g_ptr_array_new_with_free_func (g_object_unref);

  data = g_data_input_stream_new (G_INPUT_STREAM (input));

  line = g_data_input_stream_read_line (data, NULL, NULL, &error);
  while (line)
    {
      gboolean channel_parsed;
      MexChannel *channel;

      channel_parsed = parse_line (plugin, line, &channel);
      if (channel_parsed)
        g_ptr_array_add (channels, channel);
      g_free (line);

      line = g_data_input_stream_read_line (data, NULL, NULL, &error);
    }
  if (G_UNLIKELY (error))
    {
      MEX_WARNING ("Could not read line: %s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (data);
  g_object_unref (input);
read_error:
  g_free (channels_conf_path);
  g_object_unref (channels_conf);

  return channels;
}

/*
 * MexPlugin implementation
 */

static void
mex_dvb_plugin_start (MexPlugin *self)
{
  MexDvbPlugin *plugin = MEX_DVB_PLUGIN (self);
  GPtrArray *channels;

  MEX_INFO ("DVB plugin started");

  channels = load_channels_from_file (plugin);
  if (channels && channels->len > 0)
    {
      MexChannelManager *manager;

      manager = mex_channel_manager_get_default ();
      mex_channel_manager_add_channels (manager, channels);
    }

  if (channels)
    g_ptr_array_unref (channels);
}

static void
mex_dvb_plugin_stop (MexPlugin *plugin)
{
  MEX_WARNING ("Not implemented");
}

/*
 * GObject implementation
 */

static void
mex_dvb_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_dvb_plugin_parent_class)->finalize (object);
}

static void
mex_dvb_plugin_class_init (MexDvbPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MexPluginClass *plugin_class = MEX_PLUGIN_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexDvbPluginPrivate));

  object_class->finalize = mex_dvb_plugin_finalize;

  plugin_class->start = mex_dvb_plugin_start;
  plugin_class->stop = mex_dvb_plugin_stop;
}

static void
mex_dvb_plugin_init (MexDvbPlugin *self)
{
  self->priv = DVB_PLUGIN_PRIVATE (self);

  MEX_LOG_DOMAIN_INIT (dvb_log_domain, "dvb-plugin");
}

static GType
mex_dvb_get_type (void)
{
  return MEX_TYPE_DVB_PLUGIN;
}

MEX_DEFINE_PLUGIN ("DVB",
                   "linux-dvb integration",
                   PACKAGE_VERSION,
                   "LGPLv2.1+",
                   "Damien Lespiau <damien.lespiau@intel.com>",
                   MEX_API_MAJOR, MEX_API_MINOR,
                   mex_dvb_get_type,
                   MEX_PLUGIN_PRIORITY_NORMAL)

/*
 * Unit tests
 */

#if defined (ENABLE_TESTS)

static MexDvbPlugin *
load_plugin (void)
{
  return g_object_new (MEX_TYPE_DVB_PLUGIN, NULL);
}

static const char bbc_def[] = "BBC ONE;BBC:505833:I999B8C34D0M16T2G32Y0:T:"
                              "27500:600:601,602=eng:0:0:4164:9018:4100:0:4164";

static void
test_dvbt_parse_config (void)
{
  MexChannel *channel = NULL;
  MexDVBTChannel *dvbt_channel;
  MexDvbPlugin *plugin;
  gboolean parsed;

  plugin = load_plugin ();

  parsed = parse_line (plugin, bbc_def, &channel);
  g_assert (parsed);
  g_assert (channel);

  g_assert_cmpstr (mex_channel_get_name (channel), ==, "BBC ONE");

  g_assert (MEX_IS_DVBT_CHANNEL (channel));
  dvbt_channel = MEX_DVBT_CHANNEL (channel);

  g_assert_cmpuint (mex_dvbt_channel_get_frequency (dvbt_channel), ==, 505833);
  g_assert_cmpint (mex_dvbt_channel_get_inversion (dvbt_channel),
                   ==,
                   MEX_DVB_INVERSION_AUTO);
  g_assert_cmpint (mex_dvbt_channel_get_bandwidth (dvbt_channel),
                   ==,
                   MEX_DVB_BANDWIDTH_8M);
  g_assert_cmpint (mex_dvbt_channel_get_code_rate_hp (dvbt_channel),
                   ==,
                   MEX_DVB_CODE_RATE_3_4);
  g_assert_cmpint (mex_dvbt_channel_get_code_rate_lp (dvbt_channel),
                   ==,
                   MEX_DVB_CODE_RATE_NONE);
  g_assert_cmpint (mex_dvbt_channel_get_modulation (dvbt_channel),
                   ==,
                   MEX_DVB_MODULATION_QAM16);
  g_assert_cmpint (mex_dvbt_channel_get_transmission_mode (dvbt_channel),
                   ==,
                   MEX_DVB_TRANSMISSION_MODE_2K);
  g_assert_cmpint (mex_dvbt_channel_get_guard (dvbt_channel),
                   ==,
                   MEX_DVB_GUARD_32);
  g_assert_cmpint (mex_dvbt_channel_get_hierarchy (dvbt_channel),
                   ==,
                   MEX_DVB_HIERARCHY_NONE);
}

int
main (int   argc,
      char *argv[])
{
    g_type_init ();
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/dvb/channel/dvbt-parse-config", test_dvbt_parse_config);

    return g_test_run ();
}

#endif
