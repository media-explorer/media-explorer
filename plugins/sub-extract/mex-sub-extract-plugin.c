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
 */

#include "mex-sub-extract-plugin.h"

#include <gst/app/gstappsink.h>
#include <clutter-gst/clutter-gst.h>
#include <clutter-gst/clutter-gst-player.h>

#include <mex/mex-player.h>
#include <mex/mex-surface-player.h>

G_DEFINE_TYPE (MexSubExtractPlugin, mex_sub_extract_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o)                                                  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MEX_TYPE_SUB_EXTRACT_PLUGIN,            \
                                MexSubExtractPluginPrivate))

#define EXTRACT_DELAY (-2000) /* 2 seconds ahead of video/audio */

struct _MexSubExtractPluginPrivate
{
  GstPipeline *pipeline;
  GstAppSink  *text_sink;
};

/**/

static void
new_text_buffer (GstAppSink *sink, MexSubExtractPlugin *plugin)
{
  GstBuffer *buffer = gst_app_sink_pull_buffer (sink);

  g_print ("New buffer: %u -> %s\n", buffer->size, (gchar *) buffer->data);

  gst_buffer_unref (buffer);
}

/**/

static void
mex_sub_extract_plugin_dispose (GObject *object)
{
  MexSubExtractPluginPrivate *priv = MEX_SUB_EXTRACT_PLUGIN (object)->priv;

  if (priv->text_sink)
    {
      g_object_set (G_OBJECT (priv->pipeline),
                    "text-sink", NULL,
                    NULL);
      g_object_unref (priv->text_sink);
      priv->text_sink = NULL;
    }

  if (priv->pipeline)
    {
      g_object_unref (priv->pipeline);
      priv->pipeline = NULL;
    }

  G_OBJECT_CLASS (mex_sub_extract_plugin_parent_class)->finalize (object);
}

static void
mex_sub_extract_plugin_class_init (MexSubExtractPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = mex_sub_extract_plugin_dispose;

  g_type_class_add_private (klass, sizeof (MexSubExtractPluginPrivate));
}

static void
mex_sub_extract_plugin_init (MexSubExtractPlugin *self)
{
  MexSubExtractPluginPrivate *priv;
  ClutterMedia *media;
  GstElement *queue;

  self->priv = priv = GET_PRIVATE (self);

  priv->text_sink = g_object_new (GST_TYPE_APP_SINK, NULL);

  g_object_set (G_OBJECT (priv->text_sink),
                "ts-offset", (guint64) EXTRACT_DELAY * 1000 * 1000,
                NULL);

  g_object_ref_sink (priv->text_sink);

  g_signal_connect (priv->text_sink, "new-buffer",
                    G_CALLBACK (new_text_buffer), self);
  gst_app_sink_set_emit_signals (priv->text_sink, TRUE);

  media = mex_player_get_default_media_player ();

  /* That's BAD BAD !!! Need an interface here. */
  if (CLUTTER_GST_IS_PLAYER (media))
    {
      ClutterGstPlayer *player = CLUTTER_GST_PLAYER (media);

      priv->pipeline =
        GST_PIPELINE (clutter_gst_player_get_pipeline (player));
      g_object_ref_sink (priv->pipeline);

      g_object_set (G_OBJECT (priv->pipeline),
                    "text-sink", priv->text_sink,
                    NULL);
    }
  else
    g_warning ("Trying to hook the player which hasn't a known type");
}

MexSubExtractPlugin *
mex_sub_extract_plugin_new (void)
{
  return g_object_new (MEX_TYPE_SUB_EXTRACT_PLUGIN, NULL);
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_SUB_EXTRACT_PLUGIN;
}
