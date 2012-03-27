/*
 * Mex - a media explorer
 *
 * Copyright © 2008, 2009, 2010, 2011 Intel Corporation.
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

#include <gio/gio.h>
#include <clutter/clutter.h>

#include "mex-thumbnailer.h"
#include "mex-os.h"
#include "mex-marshal.h"

#include <stdlib.h>

gchar *
mex_get_thumbnail_path_for_uri (const gchar *uri)
{
  char *basepath;
  char *md5;
  char *file;
  char *path;

  md5 = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);

  basepath = g_build_filename (g_get_user_cache_dir (), "mex", "thumbnails",
                               NULL);
  file = g_strconcat (md5, ".jpg", NULL);

  g_free (md5);

  g_mkdir_with_parents (basepath, 0777);

  path = g_build_filename (basepath, file, NULL);

  g_free (basepath);
  g_free (file);

  return path;
}

static GThreadPool *thumbnail_thread_pool = NULL;

static char * get_mime_type (const char *uri);

/* thumbnail data */
typedef struct
{
  gchar *uri;
  gchar *mime;
  gchar *thumbnail_path;
  MexThumbnailCallback finished;
  gpointer user_data;
} ThumbnailData;

static ThumbnailData*
thumbnail_data_new (const gchar          *uri,
                    MexThumbnailCallback  finished,
                    gpointer              user_data)
{
  ThumbnailData *data;

  data = g_slice_new (ThumbnailData);

  data->uri = g_strdup (uri);
  data->finished = finished;
  data->user_data = user_data;
  data->thumbnail_path = mex_get_thumbnail_path_for_uri (uri);
  data->mime = get_mime_type (uri);

  return data;
}

static void
thumbnail_data_free (ThumbnailData *data)
{
  g_free (data->uri);
  g_free (data->mime);
  g_free (data->thumbnail_path);
  g_slice_free (ThumbnailData, data);
}

static gboolean
mex_internal_thumbnail_finished (gpointer user_data)
{
  ThumbnailData *data = user_data;

  data->finished (data->uri, data->user_data);
  thumbnail_data_free (data);

  return FALSE;
}

static void
mex_internal_thumbnail_start (ThumbnailData *data,
                              gpointer       foo)

{
  int status;
  gchar *argv[5], *output;
  GError *err = NULL;

  if (!data->mime)
    return;

  if (g_str_has_prefix (data->mime, "image/")
      || g_str_has_prefix (data->mime, "video/"))
    {
      argv[0] = g_build_filename (LIBEXECDIR, "mex-thumbnailer", NULL);

      /* if mex-thumbnailer is not in LIBEXECDIR, search the PATH */
      if (!g_file_test (argv[0], G_FILE_TEST_EXISTS))
        {
          gchar *path = g_strdup (getenv ("PATH"));
          gchar **paths;
          int i;

          g_free (argv[0]);

          paths = g_strsplit (path, G_SEARCHPATH_SEPARATOR_S, -1);

          for (i = 0; paths[i]; i++)
            {
              argv[0] = g_build_filename (paths[i], "mex-thumbnailer", NULL);

              if (g_file_test (argv[0], G_FILE_TEST_EXISTS))
                break;

              g_free (argv[0]);
              argv[0] = NULL;
            }

          g_free (path);
          g_strfreev (paths);
        }

      if (!argv[0])
        {
          g_warning ("Could not locate mex-thumbnailer");
        }
      else
        {
          argv[1] = data->mime;
          argv[2] = data->uri;
          argv[3] = data->thumbnail_path;
          argv[4] = NULL;

          g_spawn_sync (NULL, argv, NULL, 0, NULL, NULL, NULL, &output, &status,
                        &err);
          if (err)
            {
              g_warning ("Error: %s", err->message);
              g_clear_error (&err);
            }

          g_free (argv[0]);
        }
    }

  clutter_threads_add_timeout (0, mex_internal_thumbnail_finished, data);
}

static void
mex_internal_thumbnail (const gchar          *uri,
                        MexThumbnailCallback  finished,
                        gpointer              data)
{
  GError *err = NULL;

  if (!thumbnail_thread_pool)
    thumbnail_thread_pool = g_thread_pool_new ((GFunc)mex_internal_thumbnail_start,
                                               NULL,
                                               mex_os_get_n_cores (),
                                               FALSE, &err);

  if (err)
    {
      g_warning (G_STRLOC ": %s", err->message);
      g_clear_error (&err);
      return;
    }

  g_thread_pool_push (thumbnail_thread_pool,
                      thumbnail_data_new (uri, finished, data), &err);
  if (err)
    {
      g_warning (G_STRLOC ": %s", err->message);
      g_clear_error (&err);
      return;
    }
}

static char *
get_mime_type (const char *uri)
{
  GFile *file;
  GFileInfo *info;
  GError *error = NULL;
  char *mime;

  g_assert (uri);

  file = g_file_new_for_uri (uri);
  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                            G_FILE_QUERY_INFO_NONE,
                            NULL, &error);
  if (error) {
    g_message ("Cannot query MIME type for %s: %s", uri, error->message);
    g_object_unref (file);
    return NULL;
  }

  mime = g_strdup (g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE));
  g_object_unref (info);
  g_object_unref (file);

  return mime;
}

/**
 * mex_thumbnailer_generate: (skip)
 * @url: the URL to thumbnail
 * @mime_type: the MIME type of the URL (will be sniffed if %NULL)
 * @callback: function to callback when thumbnailing is successfull
 * @user_data: data to pass to @callback
 *
 * Attempt to generate a thumbnail for @url asynchronously.  When thumbnailing
 * is complete, @callback is called.  If thumbnailing isn't possible or fails no
 * callback will be made, so it is recommended to set a fallback image before
 * calling this function.
 */
void
mex_thumbnailer_generate (const char           *url,
                          const char           *mime_type,
                          MexThumbnailCallback  callback,
                          gpointer              user_data)
{
  mex_internal_thumbnail (url, callback, user_data);
  return;
}
