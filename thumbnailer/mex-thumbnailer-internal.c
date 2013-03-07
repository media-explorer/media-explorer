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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gst/gst.h>
#include <string.h>
#include <stdlib.h>

#include <mex/mex-settings.h>
#include <mex/mex-utils.h>

#define THUMBNAIL_SIZE 512

static void mex_internal_thumbnail_image (const gchar *uri, const gchar *path);
static void mex_internal_thumbnail_video (const gchar *uri, const gchar *path);

int
main (int argc, char **argv)
{
  gchar *mime, *settings;
  GstRegistry *registry;
  GKeyFile *key_file;

  if (argc != 4)
    return 1;

  g_type_init ();
  gst_init (&argc, &argv);

  settings = mex_settings_find_config_file (mex_settings_get_default (),
                                            "mex.conf");
  if (settings)
    {
      key_file = g_key_file_new ();

      if (g_key_file_load_from_file (key_file, settings,
                                     G_KEY_FILE_NONE, NULL))
        {
          gint i;
          gchar **denied_plugins;

          denied_plugins = g_key_file_get_string_list (key_file,
                                                       "gstreamer-plugins",
                                                       "denied",
                                                       NULL,
                                                       NULL);

          registry = gst_registry_get ();

          for (i = 0; denied_plugins && denied_plugins[i]; i++)
            {
              GstPlugin *plugin = gst_registry_find_plugin (registry,
                                                            denied_plugins[i]);
              if (plugin)
                gst_registry_remove_plugin (registry, plugin);
            }
        }
      g_key_file_free (key_file);
      g_free (settings);
    }

  mime = argv[1];

  if (g_str_has_prefix (mime, "image/"))
    mex_internal_thumbnail_image (argv[2], argv[3]);
  else if (g_str_has_prefix (mime, "video/"))
    {
      gst_init (&argc, &argv);

      mex_internal_thumbnail_video (argv[2], argv[3]);
    }
  else
    return 1;

  return 0;
}

/* image thumbnailer */
static void
mex_internal_thumbnail_image (const gchar *uri,
                              const gchar *thumbnail_path)
{
  GError *err = NULL;
  gchar *filename;
  GdkPixbuf *pixbuf;

  filename = g_filename_from_uri (uri, NULL, NULL);
  pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, THUMBNAIL_SIZE,
                                              THUMBNAIL_SIZE, TRUE, &err);
  g_free (filename);

  if (err)
    {
      g_warning (G_STRLOC ": %s", err->message);
      g_clear_error (&err);
    }
  else
    {
      gdk_pixbuf_save (pixbuf, thumbnail_path, "jpeg", &err, NULL);

      if (err)
        {
          g_warning (G_STRLOC ": %s", err->message);
          g_clear_error (&err);
        }
    }
}

/* video thumbnailer */
/*
 * The following functions where based on code from Bickley:
 *
 * Bickley - a meta data management framework.
 * Copyright © 2008 - 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */
static GdkPixbuf *
get_shot (const char *uri)
{
  GstElement *playbin, *audio_sink, *video_sink;
  GstStateChangeReturn state;
  GdkPixbuf *shot = NULL;
  int count = 0;

  playbin = gst_element_factory_make ("playbin", "playbin");
  audio_sink = gst_element_factory_make ("fakesink", "audiosink");
  video_sink = gst_element_factory_make ("gdkpixbufsink", "videosink");

  g_object_set (playbin,
                "uri", uri,
                "audio-sink", audio_sink,
                "video-sink", video_sink,
                NULL);
  g_object_set (video_sink,
                "sync", TRUE,
                NULL);

  state = gst_element_set_state (playbin, GST_STATE_PAUSED);
  while (state == GST_STATE_CHANGE_ASYNC && count < 5)
    {
      state = gst_element_get_state (playbin, NULL, 0, 1 * GST_SECOND);
      count++;

      while (g_main_context_pending (NULL))
        {
          g_main_context_iteration (NULL, FALSE);
        }
    }

  if (state != GST_STATE_CHANGE_FAILURE &&
      state != GST_STATE_CHANGE_ASYNC)
    {
      GstFormat format = GST_FORMAT_TIME;
      gint64 duration;

      if (gst_element_query_duration (playbin, format, &duration))
        {
          gint64 seekpos;

          if (duration > 0)
            {
              if (duration / (3 * GST_SECOND) > 90) {
                  seekpos = (rand () % (duration / (3 * GST_SECOND))) * GST_SECOND;
              }
              else
                {
                  seekpos = (rand () % (duration / (GST_SECOND))) * GST_SECOND;
                }
            }
          else
            {
              seekpos = 5 * GST_SECOND;
            }

          gst_element_seek_simple (playbin, GST_FORMAT_TIME,
                                   GST_SEEK_FLAG_FLUSH |
                                   GST_SEEK_FLAG_ACCURATE, seekpos);

          /* Wait for seek to complete */
          count = 0;
          state = gst_element_get_state (playbin, NULL, 0,
                                         0.2 * GST_SECOND);
          while (state == GST_STATE_CHANGE_ASYNC && count < 3)
            {
              state = gst_element_get_state (playbin, NULL, 0, 1 * GST_SECOND);
              count++;

              while (g_main_context_pending (NULL))
                {
                  g_main_context_iteration (NULL, FALSE);
                }
            }

          g_object_get (video_sink, "last-pixbuf", &shot, NULL);

          if (shot == NULL)
            {
              g_warning ("No frame for %s", uri);
              return NULL;
            }
        }

    }

  gst_element_set_state (playbin, GST_STATE_NULL);
  g_object_unref (playbin);

  return shot;
}

static gboolean
is_interesting (GdkPixbuf *pixbuf)
{
  int width, height, r, rowstride;
  gboolean has_alpha;
  guint32 histogram[4][4][4] = {{{0,},},};
  guchar *pixels;
  int pxl_count = 0, count, i;

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);

  pixels = gdk_pixbuf_get_pixels (pixbuf);
  for (r = 0; r < height; r++)
    {
      guchar *row = pixels + (r * rowstride);
      int c;

      for (c = 0; c < width; c++)
        {
          guchar red, green, blue;

          red = row[0];
          green = row[1];
          blue = row[2];

          histogram[red / 64][green / 64][blue / 64]++;

          if (has_alpha)
            {
              row += 4;
            }
          else
            {
              row += 3;
            }

          pxl_count++;
        }
    }

  count = 0;
  for (i = 0; i < 4; i++)
    {
      int j;
      for (j = 0; j < 4; j++)
        {
          int k;

          for (k = 0; k < 4; k++)
            {
              /* Count how many bins have more than
                 1% of the pixels in the histogram */
              if (histogram[i][j][k] > pxl_count / 100)
                {
                  count++;
                }
            }
        }
    }

  /* Image is boring if there is only 1 bin with > 1% of pixels */
  return count > 1;
}

void
mex_internal_thumbnail_video (const gchar *uri,
                              const gchar *thumbnail_path)
{
  GdkPixbuf *shot = NULL;
  gboolean interesting = FALSE;
  int count = 0;

  while (interesting == FALSE && count < 5)
    {
      shot = get_shot (uri);
      if (shot == NULL)
        return;

      count++;
      interesting = is_interesting (shot);
    }

  if (shot)
    {
      GdkPixbuf *output;
      int dw, dh, width, height;
      GError *error = NULL;

      width = gdk_pixbuf_get_width (shot);
      height = gdk_pixbuf_get_height (shot);

      if (width > height)
        {
          double ratio;

          ratio = (double) THUMBNAIL_SIZE / (double) width;
          dw = THUMBNAIL_SIZE;
          dh = height * ratio;
        }
      else
        {
          double ratio;

          ratio = (double) THUMBNAIL_SIZE / (double) height;
          dh = THUMBNAIL_SIZE;
          dw = width * ratio;
        }

      output = gdk_pixbuf_scale_simple (shot, dw, dh, GDK_INTERP_HYPER);

      if (gdk_pixbuf_save (output, thumbnail_path, "jpeg", &error, NULL) == FALSE)
        {
          g_warning ("Error writing file %s for %s: %s", thumbnail_path, uri,
                     error->message);
          g_error_free (error);
        }

      g_object_unref (output);
      g_object_unref (shot);
    }
}

