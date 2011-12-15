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


#include "mex-content-tile.h"

#include "mex-download-queue.h"
#include "mex-program.h"

#include "mex-utils.h"
#include <string.h>

#include <clutter-gst/clutter-gst.h>

static void mex_content_view_iface_init (MexContentViewIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexContentTile, mex_content_tile, MEX_TYPE_TILE,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW, mex_content_view_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE, mx_focusable_iface_init))

#define CONTENT_TILE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_CONTENT_TILE, MexContentTilePrivate))

enum
{
  PROP_THUMB_WIDTH = 1,
  PROP_THUMB_HEIGHT
};

struct _MexContentTilePrivate
{
  MexContent *content;
  guint changed_id;

  MexModel *model;

  ClutterActor *image;
  ClutterActor *video_preview;

  gint thumb_height;
  gint thumb_width;

  guint start_video_preview;
  guint stop_video_preview;

  gpointer download_id;

  guint thumbnail_loaded : 1;
  guint image_set        : 1;
};

enum
{
  FOCUS_IN,
  FOCUS_OUT,

  LAST_SIGNAL
};

static gulong signals[LAST_SIGNAL] = { 0, };

static gboolean
_stop_video_preview (MexContentTile *self)
{
  MexContentTilePrivate *priv = MEX_CONTENT_TILE (self)->priv;

  if (priv->start_video_preview > 0)
    g_source_remove (priv->start_video_preview);

  /* video never started */
  if (mx_bin_get_child (MX_BIN (self)) == priv->image)
    return FALSE;

  if (!priv->video_preview)
    return FALSE;

  clutter_media_set_playing (CLUTTER_MEDIA (priv->video_preview), FALSE);
  mx_bin_set_child (MX_BIN (self), priv->image);
  /* After setting the child the old child is killed off so NULL this to
   *  help with checks
   */
  priv->video_preview = NULL;


  return FALSE;
}

static void
_stop_video_eos (ClutterMedia *media, MexContentTile *self)
{
  _stop_video_preview (self);
}

static gboolean
_start_video_preview (MexContentTile *self)
{
  MexContentTilePrivate *priv = self->priv;
  GstElement *pipeline;
  gint gst_flags;

  const gchar *mimetype, *uri;
  mimetype = mex_content_get_metadata (priv->content,
                                       MEX_CONTENT_METADATA_MIMETYPE);

  if ((mimetype) && strncmp (mimetype, "video/", 6) != 0)
    return FALSE;

  if (!(uri = mex_content_get_metadata (priv->content,
                                        MEX_CONTENT_METADATA_STREAM)))
    return FALSE;

  priv->video_preview = clutter_gst_video_texture_new ();

  pipeline = clutter_gst_video_texture_get_pipeline (priv->video_preview);
  g_object_get (G_OBJECT (pipeline), "flags", &gst_flags, NULL);

  gst_flags = 1;//GST_PLAY_FLAG_VIDEO;

  g_object_set (G_OBJECT (pipeline), "flags", gst_flags, NULL);


  clutter_gst_video_texture_set_idle_material (CLUTTER_GST_VIDEO_TEXTURE (priv->video_preview),
                                               NULL);
  g_signal_connect (priv->video_preview, "eos",
                    G_CALLBACK (_stop_video_eos),
                    self);

  clutter_actor_set_opacity (priv->video_preview, 0);

  g_object_ref (priv->image);
  mx_bin_set_child (MX_BIN (self), priv->video_preview);


  clutter_actor_animate (priv->video_preview, CLUTTER_LINEAR, 500,
                         "opacity", 0xff, NULL);

  clutter_actor_set_size (priv->video_preview,
                          (gfloat)priv->thumb_width,
                          (gfloat)priv->thumb_height);

  clutter_media_set_uri (CLUTTER_MEDIA (priv->video_preview), uri);
  clutter_media_set_playing (CLUTTER_MEDIA (priv->video_preview), TRUE);

  if (priv->stop_video_preview <= 0)
    priv->stop_video_preview =
      g_timeout_add_seconds (180, (GSourceFunc)_stop_video_preview, self);

  return FALSE;
}

static MxFocusable*
mex_content_tile_accept_focus (MxFocusable *focusable,
                               MxFocusHint  hint)
{
  MexContentTilePrivate *priv = MEX_CONTENT_TILE (focusable)->priv;

  clutter_actor_grab_key_focus (CLUTTER_ACTOR (focusable));

  priv->start_video_preview =
    g_timeout_add_seconds (1, (GSourceFunc)_start_video_preview,
                           MEX_CONTENT_TILE (focusable));

  g_signal_emit (focusable, signals[FOCUS_IN], 0);

  return focusable;
}

static MxFocusable*
mex_content_tile_move_focus (MxFocusable      *focusable,
                             MxFocusDirection  direction,
                             MxFocusable      *old_focus)
{
  g_signal_emit (focusable, signals[FOCUS_OUT], 0);

  _stop_video_preview (MEX_CONTENT_TILE (old_focus));

  return NULL;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = mex_content_tile_accept_focus;
  iface->move_focus = mex_content_tile_move_focus;
}

static void
download_queue_completed (MexDownloadQueue *queue,
                          const gchar      *uri,
                          const gchar      *buffer,
                          gsize             count,
                          const GError     *error,
                          gpointer          user_data)
{
  MexContentTile *tile = MEX_CONTENT_TILE (user_data);
  MexContentTilePrivate *priv = tile->priv;
  GError *suberror = NULL;

  priv->download_id = NULL;
  priv->thumbnail_loaded = TRUE;

  if (error)
    {
      g_warning ("Error loading %s: %s", uri, error->message);
      return;
    }

  /* TODO: Find a way of not having to do a g_memdup here */
  if (!mx_image_set_from_buffer_at_size (MX_IMAGE (priv->image),
                                         g_memdup (buffer, count), count,
                                         (GDestroyNotify)g_free,
                                         priv->thumb_width,
                                         priv->thumb_height,
                                         &suberror))
    {
      g_warning ("Error loading %s: %s", uri, suberror->message);
      g_error_free (suberror);

      /* TODO: Maybe set a broken-image tile? */

      return;
    }

  priv->image_set = TRUE;
  clutter_actor_set_size (priv->image,
                          priv->thumb_width, priv->thumb_height);
}

static void
_update_thumbnail_from_image (MexContentTile *tile,
                              const gchar    *file)
{
  GError *error = NULL;

  MexContentTilePrivate *priv = tile->priv;

  if (!mx_image_set_from_file_at_size (MX_IMAGE (priv->image),
                                       file,
                                       -1,
                                       -1,
                                       &error))
    {
      g_warning ("Error loading %s: %s", file, error->message);
      g_error_free (error);

      return;
    }

  priv->image_set = TRUE;
}

static void
_reset_thumbnail (MexContentTile *tile)
{
  MexContentTilePrivate *priv = tile->priv;
  MexDownloadQueue *queue = mex_download_queue_get_default ();
  const gchar *mime = NULL;
  gchar *placeholder_filename = NULL;

  queue = mex_download_queue_get_default ();

  /* cancel any download already in progress */
  if (priv->download_id)
    {
      mex_download_queue_cancel (queue, priv->download_id);
      priv->download_id = NULL;
    }

  priv->thumbnail_loaded = FALSE;

  /* Load placeholder image */
  if (priv->content)
    mime = mex_content_get_metadata (priv->content,
                                     MEX_CONTENT_METADATA_MIMETYPE);

  if (mime && g_str_has_prefix (mime, "image/"))
    {
      placeholder_filename = "thumb-image.png";
    }
  else if (mime && g_str_equal (mime, "x-mex/tv"))
    {
      placeholder_filename = "thumb-tv.png";
    }
  else if (mime && g_str_equal (mime, "video/dvd"))
    {
      placeholder_filename = "thumb-disc.png";
    }
  else if (mime && (g_str_has_prefix (mime, "video/") ||
                    g_str_equal (mime, "x-mex/media")))
    {
      placeholder_filename = "thumb-video.png";
    }
  else if (mime && (g_str_has_prefix (mime, "audio/")))
    {
      placeholder_filename = "thumb-music.png";
    }
  else if (mime && g_str_equal (mime, "x-grl/box"))
    {
      placeholder_filename = "folder-tile.png";
    }
  else if (mime && g_str_equal (mime, "x-mex/group"))
    {
      placeholder_filename = "folder-tile.png";
    }

  if (placeholder_filename)
    {
      gchar *tmp;
      const gchar *dir = mex_get_data_dir ();

      tmp = g_build_filename (dir, "style", placeholder_filename, NULL);
      _update_thumbnail_from_image (tile, tmp);
      g_free (tmp);
    }
  else
    {
      mx_image_clear (MX_IMAGE (priv->image));

      /* Reset the height - really, we ought to reset the width and height,
       * but for all our use-cases, we want to keep the set width.
       */
      clutter_actor_set_height (priv->image, -1);
      priv->image_set = FALSE;

      return;
    }

  clutter_actor_set_size (priv->image,
                          priv->thumb_width, priv->thumb_height);
}

static void
_update_thumbnail (MexContentTile *tile)
{
  MexContentTilePrivate *priv = tile->priv;
  MexDownloadQueue *queue;
  const gchar *uri;
  GFile *file;

  queue = mex_download_queue_get_default ();

  /* cancel any download already in progress */
  if (priv->download_id)
    {
      mex_download_queue_cancel (queue, priv->download_id);
      priv->download_id = NULL;
    }

  /* update thumbnail */
  uri = mex_content_get_metadata (priv->content,
                                  MEX_CONTENT_METADATA_STILL);
  if (uri)
    {
      file = g_file_new_for_uri (uri);

      /* TODO: Display a spinner? */
      if (file)
        {
          gchar *path = g_file_get_path (file);

          if (path)
            {
              mx_image_set_from_file_at_size (MX_IMAGE (priv->image), path,
                                              priv->thumb_width,
                                              priv->thumb_height,
                                              NULL);
              priv->thumbnail_loaded = TRUE;
              priv->image_set = TRUE;
              clutter_actor_set_size (priv->image,
                                      priv->thumb_width, priv->thumb_height);
              g_free (path);
            }
          else
            {
              priv->download_id =
                mex_download_queue_enqueue (queue, uri,
                                            download_queue_completed,
                                            tile);
            }

          g_object_unref (file);
        }
    }
  else
    priv->thumbnail_loaded = TRUE;
}

static void
_update_logo (MexContentTile *tile)
{
  ClutterActor *image;
  GError *err = NULL;
  const gchar *logo_url;

  logo_url = mex_content_get_metadata (tile->priv->content,
                                       MEX_CONTENT_METADATA_STATION_LOGO);
  if (!logo_url)
    {
      mex_tile_set_primary_icon (MEX_TILE (tile), NULL);
      return;
    }

  image = mx_image_new ();

  if (g_str_has_prefix (logo_url, "file://"))
    logo_url = logo_url + 7;

  mx_image_set_from_file_at_size (MX_IMAGE (image), logo_url, 26, 26, &err);

  if (err)
    {
      g_warning ("Could not load station logo: %s", err->message);
      g_clear_error (&err);
      return;
    }

  mex_tile_set_primary_icon (MEX_TILE (tile), image);
}

static void
_content_notify (MexContent     *content,
                 GParamSpec     *pspec,
                 MexContentTile *tile)
{
  MexContentTilePrivate *priv = tile->priv;
  const gchar *still_prop_name, *logo_prop_name;

  still_prop_name = mex_content_get_property_name (MEX_CONTENT (priv->content),
                                                   MEX_CONTENT_METADATA_STILL);
  logo_prop_name = mex_content_get_property_name (MEX_CONTENT (priv->content),
                                                  MEX_CONTENT_METADATA_STATION_LOGO);

  if (!g_strcmp0 (pspec->name, still_prop_name))
    {
      _reset_thumbnail (tile);
    }
  else if (!g_strcmp0 (pspec->name, logo_prop_name))
    {
      _update_logo (tile);
    }

}

static void
mex_content_tile_set_content (MexContentView *view,
                              MexContent     *content)
{
  MexContentTile *tile = MEX_CONTENT_TILE (view);
  MexContentTilePrivate *priv = tile->priv;
  const gchar *label_prop_name, *secondary_label_prop_name;

  if (priv->content == content)
    return;

  if (priv->changed_id)
    {
      g_signal_handler_disconnect (priv->content, priv->changed_id);
      priv->changed_id = 0;
    }

  if (priv->content)
    {
      g_object_unref (priv->content);
      priv->content = NULL;
    }

  if (!content)
    return;

  priv->content = g_object_ref_sink (content);

  /* Update title/thumbnail display */

  label_prop_name = mex_content_get_property_name (MEX_CONTENT (priv->content),
                                                   MEX_CONTENT_METADATA_ARTIST);
  secondary_label_prop_name = mex_content_get_property_name (MEX_CONTENT (priv->content),
                                                   MEX_CONTENT_METADATA_TITLE);
  g_object_bind_property (content, label_prop_name, tile, "secondary-label",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (content, secondary_label_prop_name, tile, "label",
                          G_BINDING_SYNC_CREATE);



  _update_logo (tile);
  _reset_thumbnail (tile);

  /* TODO: use g_object_bind_property */
  priv->changed_id = g_signal_connect (priv->content,
                                       "notify",
                                       G_CALLBACK (_content_notify),
                                       view);
}

static MexContent*
mex_content_tile_get_content (MexContentView *view)
{
  MexContentTile *tile = MEX_CONTENT_TILE (view);
  MexContentTilePrivate *priv = tile->priv;

  return priv->content;
}

static void
mex_content_tile_set_context (MexContentView *view,
                              MexModel       *model)
{
  MexContentTile *tile = MEX_CONTENT_TILE (view);
  MexContentTilePrivate *priv = tile->priv;

  if (priv->model)
    g_object_unref (priv->model);

  priv->model = g_object_ref (model);
}

static MexModel*
mex_content_tile_get_context (MexContentView *view)
{
  MexContentTile *tile = MEX_CONTENT_TILE (view);
  MexContentTilePrivate *priv = tile->priv;

  return priv->model;
}

static void
mex_content_view_iface_init (MexContentViewIface *iface)
{
  iface->set_content = mex_content_tile_set_content;
  iface->get_content = mex_content_tile_get_content;

  iface->set_context = mex_content_tile_set_context;
  iface->get_context = mex_content_tile_get_context;
}


static void
mex_content_tile_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MexContentTilePrivate *priv = MEX_CONTENT_TILE (object)->priv;

  switch (property_id)
    {
    case PROP_THUMB_WIDTH:
      g_value_set_int (value, priv->thumb_width);
      break;

    case PROP_THUMB_HEIGHT:
      g_value_set_int (value, priv->thumb_height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_content_tile_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MexContentTilePrivate *priv = MEX_CONTENT_TILE (object)->priv;

  switch (property_id)
    {
    case PROP_THUMB_WIDTH:
      priv->thumb_width = g_value_get_int (value);

      /* Ideally we'd use the image_set variable to determine if we set this,
       * but for all our use-cases, we always want the set thumbnail width.
       */
      clutter_actor_set_width (priv->image, priv->thumb_width);
      break;

    case PROP_THUMB_HEIGHT:
      priv->thumb_height = g_value_get_int (value);
      if (priv->image_set)
        clutter_actor_set_height (priv->image, priv->thumb_height);
      break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }

}

static void
mex_content_tile_dispose (GObject *object)
{
  MexContentTilePrivate *priv = MEX_CONTENT_TILE (object)->priv;

  if (priv->content)
    {
      /* remove the reference to the MexContent and also disconnect the signal
       * handlers */
      mex_content_tile_set_content (MEX_CONTENT_VIEW (object), NULL);
    }

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->download_id)
    {
      MexDownloadQueue *dl_queue = mex_download_queue_get_default ();
      mex_download_queue_cancel (dl_queue, priv->download_id);
      priv->download_id = NULL;
    }

  if (priv->start_video_preview > 0)
    g_source_remove (priv->start_video_preview);

  if (priv->stop_video_preview > 0)
    g_source_remove (priv->stop_video_preview);

  /* This may or may not be parented so explicitly mark for destroying */
  if (priv->video_preview)
    {
      clutter_actor_destroy (CLUTTER_ACTOR (priv->video_preview));
      priv->video_preview = NULL;
    }

  G_OBJECT_CLASS (mex_content_tile_parent_class)->dispose (object);
}

static void
mex_content_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_content_tile_parent_class)->finalize (object);
}

static void
mex_content_tile_paint (ClutterActor *actor)
{
  MexContentTilePrivate *priv = MEX_CONTENT_TILE (actor)->priv;

  if (priv->content && MEX_IS_PROGRAM (priv->content))
    _mex_program_complete (MEX_PROGRAM (priv->content));

  if (!priv->thumbnail_loaded && !priv->download_id)
    _update_thumbnail (MEX_CONTENT_TILE (actor));

  CLUTTER_ACTOR_CLASS (mex_content_tile_parent_class)->paint (actor);
}

static void
mex_content_tile_class_init (MexContentTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexContentTilePrivate));

  object_class->get_property = mex_content_tile_get_property;
  object_class->set_property = mex_content_tile_set_property;
  object_class->dispose = mex_content_tile_dispose;
  object_class->finalize = mex_content_tile_finalize;

  actor_class->paint = mex_content_tile_paint;

  pspec = g_param_spec_int ("thumb-width",
                            "Thumbnail width",
                            "Scale the width of the thumbnail while loading.",
                            -1, G_MAXINT, -1,
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_THUMB_WIDTH, pspec);

  pspec = g_param_spec_int ("thumb-height",
                            "Thumbnail height",
                            "Scale the height of the thumbnail while loading.",
                            -1, G_MAXINT, -1,
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_THUMB_HEIGHT, pspec);

  signals[FOCUS_IN] = g_signal_new ("focus-in",
                                    MEX_TYPE_CONTENT_TILE,
                                    G_SIGNAL_RUN_FIRST,
                                    0,
                                    NULL,
                                    NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE,
                                    0);

  signals[FOCUS_OUT] = g_signal_new ("focus-out",
                                     MEX_TYPE_CONTENT_TILE,
                                     G_SIGNAL_RUN_FIRST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);
}

static void
mex_content_tile_init (MexContentTile *self)
{
  MexContentTilePrivate *priv;

  self->priv = priv = CONTENT_TILE_PRIVATE (self);

  priv->thumb_width = -1;
  priv->thumb_height = -1;

  priv->image = mx_image_new ();
  mx_image_set_load_async (MX_IMAGE (priv->image), TRUE);
  mx_image_set_scale_width_threshold (MX_IMAGE (priv->image), 128);
  mx_image_set_scale_height_threshold (MX_IMAGE (priv->image), 128);
  mx_image_set_scale_mode (MX_IMAGE (priv->image), MX_IMAGE_SCALE_CROP);

  mx_bin_set_child (MX_BIN (self), priv->image);
}

ClutterActor *
mex_content_tile_new (void)
{
  return g_object_new (MEX_TYPE_CONTENT_TILE, NULL);
}
