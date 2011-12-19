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

/* mex-grilo-program.c */

#include <string.h>
#include <stdlib.h>

#include "mex-grilo.h"
#include "mex-grilo-program.h"
#include "mex-thumbnailer.h"
#include "mex-utils.h"

static void mex_content_iface_init (MexContentIface *iface);

G_DEFINE_TYPE_EXTENDED (MexGriloProgram,
                        mex_grilo_program,
                        MEX_TYPE_PROGRAM,
                        0,
                        G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                               mex_content_iface_init))

#define GRILO_PROGRAM_PRIVATE(o)                                        \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GRILO_PROGRAM, MexGriloProgramPrivate))

enum
{
  PROP_0,

  PROP_GRILO_MEDIA
};

struct _MexGriloProgramPrivate
{
  GrlMedia *media;
  guint     completed : 1;
  guint     in_update : 1;
  GPid      pid;
};

/**/

static void
mex_grilo_program_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MexGriloProgram *self = MEX_GRILO_PROGRAM (object);

  switch (property_id) {
  case PROP_GRILO_MEDIA:
    g_value_set_object (value, mex_grilo_program_get_grilo_media (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mex_grilo_program_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MexGriloProgram *program = MEX_GRILO_PROGRAM (object);

  switch (property_id) {
  case PROP_GRILO_MEDIA:
    mex_grilo_program_set_grilo_media (program, g_value_get_object (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mex_grilo_program_dispose (GObject *object)
{
  MexGriloProgramPrivate *priv = MEX_GRILO_PROGRAM (object)->priv;

  if (priv->media) {
    g_object_unref (priv->media);
    priv->media = NULL;
  }

  if (priv->pid) {
    g_spawn_close_pid (priv->pid);
    priv->pid = 0;
  }

  G_OBJECT_CLASS (mex_grilo_program_parent_class)->dispose (object);
}

static void
mex_grilo_program_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_grilo_program_parent_class)->finalize (object);
}

typedef struct
{
  MexGriloProgram   *self;
  MexGetStreamReply  reply;
  gpointer           userdata;
} MexGriloProgramClosure;

static void
mex_grilo_program_get_stream_cb (GrlMediaSource *source,
                                 guint           operation_id,
                                 GrlMedia       *media,
                                 gpointer        userdata,
                                 const GError   *error)
{
  MexGriloProgramClosure *closure = userdata;
  MexContent *content = MEX_CONTENT (closure->self);
  const gchar *url = grl_media_get_url (media);

  MEX_CONTENT_IFACE (mex_grilo_program_parent_class)->set_metadata (content,
                                                                    MEX_CONTENT_METADATA_STREAM,
                                                                    url);

  closure->reply (MEX_PROGRAM (content),
                  url, error, closure->userdata);

  g_object_unref (content);
  g_object_unref (source);
  g_slice_free (MexGriloProgramClosure, closure);
}

static gboolean
mex_grilo_program_no_get_stream_cb (gpointer userdata)
{
  MexGriloProgramClosure *closure = userdata;
  MexProgram *program = MEX_PROGRAM (closure->self);

  closure->reply (program, NULL, NULL, closure->userdata);

  g_slice_free (MexGriloProgramClosure, closure);
  g_object_unref (program);

  return FALSE;
}

static void
mex_grilo_program_get_stream (MexProgram        *program,
                              MexGetStreamReply  reply,
                              gpointer           userdata)
{
  GList *keys;
  GrlMediaSource *source;
  MexGriloProgramClosure *closure;
  MexGriloProgram *self = MEX_GRILO_PROGRAM (program);
  MexGriloProgramPrivate *priv = self->priv;

  closure = g_slice_new0 (MexGriloProgramClosure);
  closure->self = self;
  closure->reply = reply;
  closure->userdata = userdata;

  /* We have to add a reference around ourselves, as you can't cancel
   * the metadata callback and we need to be around when the reply
   * comes.
   */
  g_object_ref (self);

  g_object_get (G_OBJECT (mex_program_get_feed (program)),
                "grilo-source", &source,
                NULL);
  if (GRL_IS_METADATA_SOURCE (source) &&
      grl_metadata_source_supported_operations (
                                                GRL_METADATA_SOURCE (source)) & GRL_OP_METADATA) {
    keys = grl_metadata_key_list_new (GRL_METADATA_KEY_URL, NULL);
    grl_media_source_metadata (source,
                               priv->media,
                               keys,
                               GRL_RESOLVE_IDLE_RELAY | GRL_RESOLVE_FULL,
                               mex_grilo_program_get_stream_cb,
                               closure);
    g_list_free (keys);
  }
  else
    g_idle_add (mex_grilo_program_no_get_stream_cb, closure);
}

static gchar *
mex_grilo_program_get_id (MexProgram *program)
{
  MexGriloProgram *self = MEX_GRILO_PROGRAM (program);
  MexGriloProgramPrivate *priv = self->priv;
  const gchar *id = grl_media_get_id (priv->media);

  if (!id)
    return NULL;

  return g_strdup (id);
}

/*
 * Callback from mex_thumbnailer_generate, when a thumbnail has been generated.
 */
static void
thumbnail_cb (const char *uri, gpointer user_data)
{
  MexContent *content;
  MexGriloProgramPrivate *priv;
  char *thumb_path;

  content = MEX_CONTENT (user_data);
  priv = GRILO_PROGRAM_PRIVATE (user_data);

  thumb_path = mex_get_thumbnail_path_for_uri (uri);

  if (g_file_test (thumb_path, G_FILE_TEST_EXISTS))
    {
      gchar *thumb_uri = g_filename_to_uri (thumb_path, NULL, NULL);

      priv->in_update = TRUE;

      mex_content_set_metadata (content,
                                MEX_CONTENT_METADATA_STILL,
                                thumb_uri);

      priv->in_update = FALSE;

      g_free (thumb_uri);
    }

  g_free (thumb_path);
}

/*
 * If we need to, generate a thumbnail.
 *
 * If the URL is not file:/// then we assume it's okay to use.
 *
 */
static void
mex_grilo_program_thumbnail (MexContent *content, GrlMedia *media)
{
  const char *url, *old_thumb_url;
  char *thumb_path;
  static gchar *folder_thumb_uri = NULL;

  /* If the media isn't local, then we'll ignore it for now */
  url = grl_media_get_url (media);
  if (url == NULL || !g_str_has_prefix (url, "file:///"))
    return;

  old_thumb_url = mex_content_get_metadata (content,
                                            MEX_CONTENT_METADATA_STILL);

  if (GRL_IS_MEDIA_BOX (media))
    {
      if (G_UNLIKELY (folder_thumb_uri == NULL))
        {
          thumb_path = g_build_filename (mex_get_data_dir (),
                                         "common", "folder-tile.png",
                                         NULL);
          folder_thumb_uri = g_filename_to_uri (thumb_path, NULL, NULL);
          g_free (thumb_path);
        }

      mex_content_set_metadata (content, MEX_CONTENT_METADATA_STILL,
                                folder_thumb_uri);
      return;
    }

  thumb_path = mex_get_thumbnail_path_for_uri (url);

  if (g_file_test (thumb_path, G_FILE_TEST_EXISTS))
    {
      gchar *thumb_url = g_filename_to_uri (thumb_path, NULL, NULL);
      if (!old_thumb_url || strcmp (thumb_url, old_thumb_url) != 0)
        mex_content_set_metadata (content, MEX_CONTENT_METADATA_STILL,
                                  thumb_url);
      g_free (thumb_url);
    }
  else
    {
      mex_thumbnailer_generate (url, grl_media_get_mime (media),
                                thumbnail_cb, content);
    }
  g_free (thumb_path);
}

static void
program_complete_cb (GrlMediaSource *source,
                     guint          operation_id,
                     GrlMedia       *media,
                     gpointer        userdata,
                     const GError   *error)
{
  MexGriloProgram *self = MEX_GRILO_PROGRAM (userdata);
  MexGriloProgramPrivate *priv = self->priv;
  MexContent *content = MEX_CONTENT (self);

  priv->in_update = TRUE;

  mex_grilo_update_content_from_media (content, media);
  mex_grilo_program_thumbnail (content, media);

  priv->in_update = FALSE;

  g_object_unref (self);
  g_object_unref (source);
}

static void
mex_grilo_program_complete (MexProgram *program)
{
  GList *keys = NULL;
  GrlMediaSource *source = NULL;

  MexGriloProgram *self = MEX_GRILO_PROGRAM (program);
  MexGriloProgramPrivate *priv = self->priv;

  if (priv->completed)
    return;

  priv->completed = TRUE;

  g_object_get (G_OBJECT (mex_program_get_feed (program)),
                "grilo-source", &source,
                "grilo-metadata-keys", &keys,
                NULL);

  if (!source)
    return;

  if (!GRL_IS_METADATA_SOURCE (source))
    return;

  if (!(grl_metadata_source_supported_operations (GRL_METADATA_SOURCE (source))
        & GRL_OP_METADATA))
    return;

  /* FIXME: Currently just adding a ref, but we should keep the operation ID
   *        and cancel it on dispose instead.
   */
  g_object_ref (self);
  grl_media_source_metadata (source,
                             priv->media,
                             keys,
                             GRL_RESOLVE_IDLE_RELAY | GRL_RESOLVE_FULL,
                             program_complete_cb,
                             self);

  /* We don't unref the source here, it is done in
     mex_grilo_program_complete_cb */
}

static void
mex_grilo_program_set_metadata (MexContent         *content,
                                 MexContentMetadata  key,
                                 const gchar        *value)
{
  MexGriloProgram        *program = MEX_GRILO_PROGRAM (content);
  MexGriloProgramPrivate *priv    = program->priv;
  MexContentIface        *iface, *parent_iface;

  if (!priv->in_update && key != MEX_CONTENT_METADATA_QUEUED)
    mex_grilo_set_media_content_metadata (priv->media, key, value);


  iface = MEX_CONTENT_GET_IFACE (content);
  parent_iface = g_type_interface_peek_parent (iface);
  parent_iface->set_metadata (content, key, value);
  /* mex_grilo_program_set_metadata (content, key, value); */
}

static void
mex_grilo_program_save_metadata (MexContent *content)
{
  MexGriloProgram        *program = MEX_GRILO_PROGRAM (content);
  MexGriloProgramPrivate *priv    = program->priv;
  GrlMediaSource         *source;
  const GList *ckeys;
  GList *keys;

  g_object_get (G_OBJECT (mex_program_get_feed (MEX_PROGRAM (program))),
                "grilo-source", &source,
                NULL);

  if (!(grl_metadata_source_supported_operations (GRL_METADATA_SOURCE (source))
        & GRL_OP_SET_METADATA))
    goto goout;

  ckeys = grl_metadata_source_writable_keys (GRL_METADATA_SOURCE (source));
  keys = g_list_copy ((GList *) ckeys);

  grl_metadata_source_set_metadata (GRL_METADATA_SOURCE (source),
                                    priv->media,
                                    keys,
                                    GRL_WRITE_NORMAL,
                                    NULL, NULL);

  g_list_free (keys);

 goout:
  g_object_unref (source);
}

static void
_mex_grilo_program_open (MexContent *content, MexModel *context)
{
  MexGriloFeed *feed =
    (MexGriloFeed *) mex_program_get_feed ((MexProgram *) content);

  mex_grilo_feed_open (feed, (MexGriloProgram *) content);
}

static void
mex_grilo_program_class_init (MexGriloProgramClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MexProgramClass *program_class = MEX_PROGRAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexGriloProgramPrivate));

  object_class->get_property = mex_grilo_program_get_property;
  object_class->set_property = mex_grilo_program_set_property;
  object_class->dispose = mex_grilo_program_dispose;
  object_class->finalize = mex_grilo_program_finalize;

  program_class->get_stream = mex_grilo_program_get_stream;
  program_class->complete = mex_grilo_program_complete;
  program_class->get_id = mex_grilo_program_get_id;

  pspec = g_param_spec_object ("grilo-media", "Grilo media",
                               "GrlMedia object for this program",
                               GRL_TYPE_MEDIA,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_GRILO_MEDIA, pspec);
}

static void
mex_grilo_program_init (MexGriloProgram *self)
{
  self->priv = GRILO_PROGRAM_PRIVATE (self);
}

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->set_metadata = mex_grilo_program_set_metadata;
  iface->save_metadata = mex_grilo_program_save_metadata;

  iface->open = _mex_grilo_program_open;
}

MexProgram *
mex_grilo_program_new (MexGriloFeed *feed, GrlMedia *media)
{
  g_return_val_if_fail (MEX_IS_GRILO_FEED (feed), NULL);
  g_return_val_if_fail (GRL_IS_MEDIA (media), NULL);

  return g_object_new (MEX_TYPE_GRILO_PROGRAM,
                       "feed", feed,
                       "grilo-media", media,
                       NULL);
}

GrlMedia *
mex_grilo_program_get_grilo_media (MexGriloProgram *program)
{
  g_return_val_if_fail (MEX_IS_GRILO_PROGRAM (program), NULL);
  return program->priv->media;
}

void
mex_grilo_program_set_grilo_media (MexGriloProgram *program,
                                   GrlMedia        *media)
{
  MexGriloProgramPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_PROGRAM (program));
  g_return_if_fail (GRL_IS_MEDIA (media));

  priv = program->priv;

  if (priv->media == media)
    return;

  if (priv->media != NULL)
    g_object_unref (priv->media);
  priv->media = g_object_ref (media);

  priv->in_update = TRUE;

  mex_grilo_update_content_from_media (MEX_CONTENT (program), media);

  priv->in_update = FALSE;

  /* Unset 'completed' so that the next time completed is called, all data
   * on this Grilo media is re-resolved.
   */
  priv->completed = FALSE;
}

GList *
mex_grilo_program_get_default_keys (void)
{
  return grl_metadata_key_list_new (GRL_METADATA_KEY_ID,
                                    GRL_METADATA_KEY_TITLE,
                                    GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_MIME,
                                    GRL_METADATA_KEY_URL,
                                    GRL_METADATA_KEY_THUMBNAIL,
                                    GRL_METADATA_KEY_DATE,
                                    GRL_METADATA_KEY_DURATION,
                                    GRL_METADATA_KEY_WIDTH,
                                    GRL_METADATA_KEY_HEIGHT,
                                    GRL_METADATA_KEY_LAST_POSITION,
                                    GRL_METADATA_KEY_PLAY_COUNT,
                                    GRL_METADATA_KEY_LAST_PLAYED,
                                    NULL);
}
