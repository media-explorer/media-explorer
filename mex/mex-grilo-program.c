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
  GPid      pid;
};

/**/

/* static GHashTable *grl_to_mex; */
static GHashTable *mex_to_grl;

/**/

static void
_insert_grl_mex_link (GrlKeyID grl_key, MexContentMetadata mex_key)
{
  /* g_hash_table_insert (grl_to_mex, */
  /*                      GSIZE_TO_POINTER (grl_key), */
  /*                      GSIZE_TO_POINTER (mex_key)); */
  g_hash_table_insert (mex_to_grl,
                       GSIZE_TO_POINTER (mex_key),
                       GSIZE_TO_POINTER (grl_key));
}

static void
_setup_grl_mex_mapping (void)
{
  /* grl_to_mex = g_hash_table_new (g_direct_hash, g_direct_equal); */
  mex_to_grl = g_hash_table_new (g_direct_hash, g_direct_equal);

  _insert_grl_mex_link (GRL_METADATA_KEY_SHOW,
                        MEX_CONTENT_METADATA_SERIES_NAME);
  _insert_grl_mex_link (GRL_METADATA_KEY_DESCRIPTION,
                        MEX_CONTENT_METADATA_SYNOPSIS);
  _insert_grl_mex_link (GRL_METADATA_KEY_TITLE,
                        MEX_CONTENT_METADATA_TITLE);
  _insert_grl_mex_link (GRL_METADATA_KEY_SEASON,
                        MEX_CONTENT_METADATA_SEASON);
  _insert_grl_mex_link (GRL_METADATA_KEY_EPISODE,
                        MEX_CONTENT_METADATA_EPISODE);
  _insert_grl_mex_link (GRL_METADATA_KEY_EPISODE,
                        MEX_CONTENT_METADATA_EPISODE);
  _insert_grl_mex_link (GRL_METADATA_KEY_DURATION,
                        MEX_CONTENT_METADATA_DURATION);
  _insert_grl_mex_link (GRL_METADATA_KEY_URL,
                        MEX_CONTENT_METADATA_STREAM);
  _insert_grl_mex_link (GRL_METADATA_KEY_DATE,
                        MEX_CONTENT_METADATA_DATE);
  _insert_grl_mex_link (GRL_METADATA_KEY_MIME,
                        MEX_CONTENT_METADATA_MIMETYPE);
  _insert_grl_mex_link (GRL_METADATA_KEY_THUMBNAIL,
                        MEX_CONTENT_METADATA_STILL);
  _insert_grl_mex_link (GRL_METADATA_KEY_LAST_POSITION,
                        MEX_CONTENT_METADATA_LAST_POSITION);
  _insert_grl_mex_link (GRL_METADATA_KEY_PLAY_COUNT,
                        MEX_CONTENT_METADATA_PLAY_COUNT);
  _insert_grl_mex_link (GRL_METADATA_KEY_LAST_PLAYED,
                        MEX_CONTENT_METADATA_LAST_PLAYED_DATE);
  _insert_grl_mex_link (GRL_METADATA_KEY_WIDTH,
                        MEX_CONTENT_METADATA_WIDTH);
  _insert_grl_mex_link (GRL_METADATA_KEY_HEIGHT,
                        MEX_CONTENT_METADATA_HEIGHT);
  _insert_grl_mex_link (GRL_METADATA_KEY_CAMERA_MODEL,
                        MEX_CONTENT_METADATA_CAMERA_MODEL);
  _insert_grl_mex_link (GRL_METADATA_KEY_ORIENTATION,
                        MEX_CONTENT_METADATA_ORIENTATION);
  _insert_grl_mex_link (GRL_METADATA_KEY_FLASH_USED,
                        MEX_CONTENT_METADATA_FLASH_USED);
  _insert_grl_mex_link (GRL_METADATA_KEY_EXPOSURE_TIME,
                        MEX_CONTENT_METADATA_EXPOSURE_TIME);
  _insert_grl_mex_link (GRL_METADATA_KEY_ISO_SPEED,
                        MEX_CONTENT_METADATA_ISO_SPEED);
  _insert_grl_mex_link (GRL_METADATA_KEY_CREATION_DATE,
                        MEX_CONTENT_METADATA_CREATION_DATE);
  _insert_grl_mex_link (GRL_METADATA_KEY_ARTIST,
                        MEX_CONTENT_METADATA_ARTIST);
  _insert_grl_mex_link (GRL_METADATA_KEY_ALBUM,
                        MEX_CONTENT_METADATA_ALBUM);
}

static GrlKeyID
_get_grl_key_from_mex (MexContentMetadata mex_key)
{
  return (GrlKeyID) g_hash_table_lookup (mex_to_grl,
                                         GSIZE_TO_POINTER (mex_key));
}

/* static MexContentMetadata */
/* _get_mex_key_from_grl (GrlKeyID grl_key) */
/* { */
/*   return (MexContentMetadata) g_hash_table_lookup (grl_to_mex, */
/*                                                    GSIZE_TO_POINTER (grl_key)); */
/* } */

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

static void
mex_grilo_program_set_metadata (MexContent *content,
                                MexContentMetadata key,
                                const gchar *value)
{
  MexContentIface        *iface, *parent_iface;

  iface = MEX_CONTENT_GET_IFACE (content);
  parent_iface = g_type_interface_peek_parent (iface);
  parent_iface->set_metadata (content, key, value);
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
    keys = grl_metadata_key_list_new (GRL_METADATA_KEY_URL);
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
  char *thumb_path;

  content = MEX_CONTENT (user_data);

  thumb_path = mex_get_thumbnail_path_for_uri (uri);

  if (g_file_test (thumb_path, G_FILE_TEST_EXISTS))
    {
      gchar *thumb_uri = g_filename_to_uri (thumb_path, NULL, NULL);
      mex_grilo_program_set_metadata (content,
                                      MEX_CONTENT_METADATA_STILL,
                                      thumb_uri);
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
  const char *url;
  char *thumb_path;

  /* If the media isn't local, then we'll ignore it for now */
  url = grl_media_get_url (media);
  if (url == NULL || !g_str_has_prefix (url, "file:///"))
    return;

  if (GRL_IS_MEDIA_BOX (media))
    {
      gchar *tmp;

      tmp = g_build_filename (mex_get_data_dir (), "common", "folder-tile.png",
                              NULL);
      mex_grilo_program_set_metadata (content, MEX_CONTENT_METADATA_STILL, tmp);
      g_free (tmp);
      return;
    }

  thumb_path = mex_get_thumbnail_path_for_uri (url);

  if (g_file_test (thumb_path, G_FILE_TEST_EXISTS))
    {
      gchar *thumb_uri = g_filename_to_uri (thumb_path, NULL, NULL);
      mex_grilo_program_set_metadata (content, MEX_CONTENT_METADATA_STILL,
                                      thumb_uri);
      g_free (thumb_uri);
    }
  else
    {
      mex_thumbnailer_generate (url, grl_media_get_mime (media),
                                thumbnail_cb, content);
    }
  g_free (thumb_path);
}

/* Returns whether to free the value parameter or not. */
static void
set_metadata_from_media (MexContent          *content,
                         GrlMedia            *media,
                         MexContentMetadata   mex_key)
{
  gchar       *string;
  const gchar *cstring;
  GrlKeyID     grl_key = _get_grl_key_from_mex (mex_key);
  gint n;

  if (!grl_key)
    return;

  switch (G_PARAM_SPEC (grl_key)->value_type) {
  case G_TYPE_STRING:
    cstring = grl_data_get_string (GRL_DATA (media), grl_key);

    if (cstring)
      {
        if (mex_key == MEX_CONTENT_METADATA_TITLE)
          {
            GRegex *regex;
            gchar *replacement;

            /* strip off any file extensions */

            regex = g_regex_new ("\\.....?$", 0, 0, NULL);
            replacement = g_regex_replace (regex, cstring, -1, 0, "", 0, NULL);

            g_regex_unref (regex);

            if (!replacement)
              replacement = g_strdup ("");

            mex_grilo_program_set_metadata (content, mex_key, replacement);
          }
        else
          mex_grilo_program_set_metadata (content, mex_key, cstring);
      }
    break;

  case G_TYPE_INT:
    n = grl_data_get_int (GRL_DATA (media), grl_key);

    if (n > 0)
      {
        string = g_strdup_printf ("%i", n);
        mex_grilo_program_set_metadata (content, mex_key, string);
        g_free (string);
      }
    break;

  case G_TYPE_FLOAT:
    string = g_strdup_printf ("%f", grl_data_get_float (GRL_DATA (media),
                                                        grl_key));
    mex_grilo_program_set_metadata (content, mex_key, string);
    g_free (string);
    break;
  }
}

static void
set_metadata_to_media (GrlMedia           *media,
                       MexContentMetadata  mex_key,
                       const gchar        *value)
{
  int      ival;
  float    fval;
  GrlKeyID grl_key = _get_grl_key_from_mex (mex_key);

  if (!grl_key) {
    g_warning ("No grilo key to handle %s",
               mex_content_metadata_key_to_string (mex_key));
    return;
  }

  switch (G_PARAM_SPEC (grl_key)->value_type) {
  case G_TYPE_STRING:
    grl_data_set_string (GRL_DATA (media), grl_key, value);
    break;

  case G_TYPE_INT:
    ival = atoi (value);
    grl_data_set_int (GRL_DATA (media), grl_key, ival);
    break;

  case G_TYPE_FLOAT:
    fval = atof (value);
    grl_data_set_float (GRL_DATA (media), grl_key, fval);
    break;
  }
}

static void
set_metadatas_from_media (MexContent *content,
                          GrlMedia   *media)
{
  /* FIXME: This list is just hard-coded and needs to be the same as
   *        the default set of keys in MexGriloFeed... Grilo is likely
   *        to add an API to retrieve all setted keys, we might want
   *        to use that.
   */
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_TITLE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_SYNOPSIS);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_MIMETYPE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_STILL);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_STREAM);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_WIDTH);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_HEIGHT);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_DATE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_DURATION);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_LAST_POSITION);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_PLAY_COUNT);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_LAST_PLAYED_DATE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_CAMERA_MODEL);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_ORIENTATION);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_FLASH_USED);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_EXPOSURE_TIME);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_ISO_SPEED);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_CREATION_DATE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_ALBUM);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_ARTIST);

}

static void
program_complete_cb (GrlMediaSource *source,
                     guint          operation_id,
                     GrlMedia       *media,
                     gpointer        userdata,
                     const GError   *error)
{
  MexGriloProgram *self = userdata;
  MexContent *content = MEX_CONTENT (self);

  set_metadatas_from_media (content, media);

  mex_grilo_program_thumbnail (content, media);

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
_mex_grilo_program_set_metadata (MexContent         *content,
                                 MexContentMetadata  key,
                                 const gchar        *value)
{
  MexGriloProgram        *program = MEX_GRILO_PROGRAM (content);
  MexGriloProgramPrivate *priv    = program->priv;

  if (!value)
    return;

  set_metadata_to_media (priv->media, key, value);
  mex_grilo_program_set_metadata (content, key, value);
}

static void
_mex_grilo_program_save_metadata (MexContent *content)
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

  _setup_grl_mex_mapping ();
}

static void
mex_grilo_program_init (MexGriloProgram *self)
{
  self->priv = GRILO_PROGRAM_PRIVATE (self);
}

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->set_metadata = _mex_grilo_program_set_metadata;
  iface->save_metadata = _mex_grilo_program_save_metadata;
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

  set_metadatas_from_media (MEX_CONTENT (program), media);

  /* Unset 'completed' so that the next time completed is called, all data
   * on this Grilo media is re-resolved.
   */
  priv->completed = FALSE;
}

GList *
mex_grilo_program_get_default_keys ()
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
