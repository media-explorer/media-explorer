/*
 * Mex - Media Explorer
 *
 * Copyright © 2012 Intel Corporation.
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

#include <gst/gst.h>
#include <glib.h>
#include <mex/mex.h>

#define PIPELINE "dvbsrc name=dvbsrc pids=0:16:17:18 stats-reporting-interval=0 ! mpegtsparse ! fakesink silent=true"

static gboolean
bus_func (GstBus     *bus,
          GstMessage *message,
          gpointer    data)
{
  const GValue *value_list;

  if (message->type != GST_MESSAGE_ELEMENT)
    return TRUE;

  if (g_strcmp0 (gst_structure_get_name (message->structure), "eit"))
    return TRUE;

  value_list = gst_structure_get_value (message->structure, "events");

  if (value_list)
    {
      gint i;

      for (i = 0; i < gst_value_list_get_size (value_list); i++)
        {
          guint year, month, day, hour, minute, second, duration, event_id;
          gchar *name = NULL, *description = NULL;
          const GValue* value = gst_value_list_get_value (value_list, i);
          const GstStructure* event;

          if (!value)
            continue;
          event = gst_value_get_structure (value);
          if (!event)
            continue;

          if (!gst_structure_get (event,
                                  "event-id", G_TYPE_UINT, &event_id,
                                  "year", G_TYPE_UINT, &year,
                                  "month", G_TYPE_UINT, &month,
                                  "day", G_TYPE_UINT, &day,
                                  "hour", G_TYPE_UINT, &hour,
                                  "minute", G_TYPE_UINT, &minute,
                                  "second", G_TYPE_UINT, &second,
                                  "duration", G_TYPE_UINT, &duration,
                                  NULL))
            g_error ("error getting struct data");

          if (gst_structure_has_field (event, "name"))
            {
              gst_structure_get (event,
                                 "name", G_TYPE_STRING, &name,
                                 NULL);
            }


          if (gst_structure_has_field (event, "description"))
            {
              gst_structure_get (event,
                                 "description", G_TYPE_STRING, &description,
                                 NULL);
            }

          printf ("Event %d = %02d/%02d/%02d %02d:%02d %s \n\t(%s)\n", event_id,
                   day, month, year, hour, minute, name, description);

          g_free (name);
          g_free (description);
        }
    }

  return TRUE;
}

int
main (int argc, char **argv)
{
  GstPipeline *pipeline;
  GMainLoop *loop;
  GError *err = NULL;
  GstBus *bus;
  MexChannelManager *manager;
  MexPluginManager *pmanager;
  const GPtrArray *array;
  gint i;

  mex_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  pipeline = (GstPipeline*) gst_parse_launch (PIPELINE, &err);

  if (!pipeline)
    g_error ("Error creating pipeline: %s", err->message);

  bus = gst_pipeline_get_bus (pipeline);
  gst_bus_add_watch (bus, bus_func, NULL);

  pmanager = mex_plugin_manager_get_default ();
  mex_plugin_manager_refresh (pmanager);

  manager = mex_channel_manager_get_default ();
  array = mex_channel_manager_get_channels (manager);

  if (array->len < 1)
    g_error ("No Channels");

  for (i = 0; i < array->len; i++)
    {
      MexChannel *channel = array->pdata[i];

      /* find a dvb-t channel to scan from */
      if (MEX_IS_DVBT_CHANNEL (channel))
        {
          guint frequency;
          MexDvbInversion inversion;
          MexDvbBandwidth bandwidth;
          MexDvbCodeRate code_rate_hp, code_rate_lp;
          MexDvbModulation modulation;
          MexDvbTransmissionMode transmission_mode;
          MexDvbGuard guard;
          MexDvbHierarchy hierarchy;
          GstElement *element;

          printf ("Tuning to %s\n", mex_channel_get_name (channel));

          g_object_get (channel,
                        "modulation", &modulation,
                        "transmission-mode", &transmission_mode,
                        "code-rate-hp", &code_rate_hp,
                        "code-rate-lp", &code_rate_lp,
                        "guard", &guard,
                        "bandwidth", &bandwidth,
                        "frequency", &frequency,
                        "hierarchy", &hierarchy,
                        "inversion", &inversion,
                        NULL);

          element = gst_bin_get_by_name (GST_BIN (pipeline), "dvbsrc");


          g_object_set (element,
                        "modulation", modulation,
                        "trans-mode", transmission_mode,
                        "code-rate-hp", code_rate_hp,
                        "code-rate-lp", code_rate_lp,
                        "guard", guard,
                        "bandwidth", bandwidth,
                        "frequency", frequency,
                        "hierarchy", hierarchy,
                        "inversion", inversion,
                        NULL);

          g_object_unref (element);

          break;
        }
    }

  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

  g_main_loop_run (loop);

  return 0;
}
