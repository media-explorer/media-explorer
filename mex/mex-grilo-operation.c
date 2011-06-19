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

#include "mex-grilo-operation.h"
#include "mex-grilo.h"
#include "mex-marshal.h"

#include <stdlib.h>
#include <grilo.h>

G_DEFINE_TYPE (MexGriloOperation, mex_grilo_operation, G_TYPE_OBJECT)

#define GRILO_OPERATION_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GRILO_OPERATION, MexGriloOperationPrivate))

enum
{
  CONTENT_CREATED,
  CONTENT_UPDATED,

  LAST_SIGNAL
};

struct _MexGriloOperationPrivate
{
  GAsyncQueue *op_queue;
  GAsyncQueue *ret_queue;

  GMainContext *context;


  /* user side */
  MexCreateContentCb create_callback;
  gpointer           create_data;

  /* thread side */
  guint current_id;
};

static guint signals[LAST_SIGNAL] = { 0, };

static GHashTable *mex_to_grl = NULL;

/**/

static void
_insert_grl_mex_link (GrlKeyID grl_key, MexContentMetadata mex_key)
{
  g_hash_table_insert (mex_to_grl,
                       GSIZE_TO_POINTER (mex_key),
                       GSIZE_TO_POINTER (grl_key));
}

static GrlKeyID
_get_grl_key_from_mex (MexContentMetadata mex_key)
{
  return (GrlKeyID) g_hash_table_lookup (mex_to_grl,
                                         GSIZE_TO_POINTER (mex_key));
}

static void
_setup_grl_mex_mapping (void)
{
  mex_to_grl = g_hash_table_new (g_direct_hash, g_direct_equal);

  _insert_grl_mex_link (GRL_METADATA_KEY_ID,
                        MEX_CONTENT_METADATA_ID);
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

static GList *
mex_to_grilo_keys (GList *mex_metadatas)
{
  GList *grl_metadatas = NULL;
  GrlKeyID grl_key;

  while (mex_metadatas)
    {
      grl_key = _get_grl_key_from_mex (GPOINTER_TO_SIZE (mex_metadatas->data));
      grl_metadatas = g_list_append (grl_metadatas, GSIZE_TO_POINTER (grl_key));
      mex_metadatas = mex_metadatas->next;
    }

  return grl_metadatas;
}

/**/

static void
mex_grilo_operation_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_grilo_operation_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_grilo_operation_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_grilo_operation_parent_class)->dispose (object);
}

static void
mex_grilo_operation_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_grilo_operation_parent_class)->finalize (object);
}

static void
mex_grilo_operation_class_init (MexGriloOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexGriloOperationPrivate));

  object_class->get_property = mex_grilo_operation_get_property;
  object_class->set_property = mex_grilo_operation_set_property;
  object_class->dispose = mex_grilo_operation_dispose;
  object_class->finalize = mex_grilo_operation_finalize;

  signals[CONTENT_CREATED] =
    g_signal_new ("content-created",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  mex_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, G_TYPE_OBJECT);

  signals[CONTENT_UPDATED] =
    g_signal_new ("content-updated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  mex_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, G_TYPE_OBJECT);

  mex_grilo_init ();
  _setup_grl_mex_mapping ();
}

static void
mex_grilo_operation_init (MexGriloOperation *self)
{
  MexGriloOperationPrivate *priv = GRILO_OPERATION_PRIVATE (self);

  self->priv = priv;

  priv->op_queue = g_async_queue_new ();
  priv->ret_queue = g_async_queue_new ();
  priv->context = g_main_context_get_thread_default ();
}

MexGriloOperation *
mex_grilo_operation_new (void)
{
  return g_object_new (MEX_TYPE_GRILO_OPERATION, NULL);
}

typedef struct _GriloOp GriloOp;

/* Generic handling */
typedef void (*GriloOpCb) (GriloOp *op);

struct _GriloOp
{
  MexGriloOperation *operation;

  GriloOpCb          start_callback;
  GriloOpCb          free_callback;

  MexCreateContentCb create_callback;
  gpointer           create_data;

  MexMetadatas *metadatas;
};


static void
op_free (GriloOp *op)
{
  g_object_unref (op->operation);
  g_object_unref (op->metadatas);

  g_free (op);
}

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

            mex_content_set_metadata (content, mex_key, replacement);

            g_free (replacement);
          }
        else
          mex_content_set_metadata (content, mex_key, cstring);
      }
    break;

  case G_TYPE_INT:
    n = grl_data_get_int (GRL_DATA (media), grl_key);

    if (n > 0)
      {
        string = g_strdup_printf ("%i", n);
        mex_content_set_metadata (content, mex_key, string);
        g_free (string);
      }
    break;

  case G_TYPE_FLOAT:
    string = g_strdup_printf ("%f", grl_data_get_float (GRL_DATA (media),
                                                        grl_key));
    mex_content_set_metadata (content, mex_key, string);
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

  if (!grl_key)
    {
      g_warning ("No grilo key to handle %s",
                 mex_content_metadata_key_to_string (mex_key));
      return;
    }

  switch (G_PARAM_SPEC (grl_key)->value_type)
    {
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

/* Search handling */
typedef struct
{
  GriloOp base;

  guint  id;
  gchar *text;
  gchar *source_id;
} GriloSearchOp;

static void
search_op_free_cb (GriloSearchOp *op)
{
  if (op->text)
    {
      g_free (op->text);
      op->text = NULL;
    }
  if (op->source_id)
    {
      g_free (op->source_id);
      op->source_id = NULL;
    }
  op_free ((GriloOp *) op);
}

static gboolean
search_op_notify_cb (MexGriloOperation *operation)
{
  MexGriloOperationPrivate *priv = operation->priv;
  MexContent *content;
  gint i;

  g_async_queue_lock (priv->ret_queue);

  for (i = 0;
       (i < 10) && (g_async_queue_length_unlocked (priv->ret_queue) > 0);
       i++)
    {
      content = (MexContent *) g_async_queue_pop_unlocked (priv->ret_queue);
      g_signal_emit (operation,
                     signals[CONTENT_CREATED], 0, content);
      g_object_unref (content);
    }

  if (g_async_queue_length_unlocked (priv->ret_queue) > 0)
    {
      g_async_queue_unlock (priv->ret_queue);
      return TRUE;
    }

  g_async_queue_unlock (priv->ret_queue);
  return FALSE;
}

static void
search_op_grilo_result_cb (GrlMediaSource *source,
                           guint           id,
                           GrlMedia       *media,
                           guint           remaining,
                           gpointer        userdata,
                           const GError   *error)
{
  GriloSearchOp     *op        = (GriloSearchOp *) userdata;
  MexGriloOperation *operation = op->base.operation;
  MexGriloOperationPrivate *priv = operation->priv;

  if (error)
    {
      g_warning ("Error searching grilo: %s", error->message);
      return;
    }

  if (operation->priv->current_id != id)
    {
      /* TODO: remove warning */
      g_warning ("Removed request id %u", id);
      return;
    }

  if (media)
    {
      gconstpointer foo = grl_media_get_id (media);
      MexContent *content;
      GList *mex_keys;

      if (!foo)
        {
          const gchar *source_name;

          source_name =
            grl_metadata_source_get_name (GRL_METADATA_SOURCE (source));
          g_warning ("FIXME: oh no, a grilo bug! (on the '%s' source)",
                     source_name);
          return;
        }

      content = op->base.create_callback (op->base.create_data);

      /* Setup content */
      mex_keys = mex_metadatas_get_metadata_list (op->base.metadatas);
      while (mex_keys)
        {
          set_metadata_from_media (content, media,
                                   GPOINTER_TO_SIZE (mex_keys->data));
          mex_keys = mex_keys->next;
        }

      /* Queue content for processing in main thread */
      g_async_queue_lock (priv->ret_queue);

      g_async_queue_push_unlocked (priv->ret_queue, content);

      if (g_async_queue_length_unlocked (priv->ret_queue) <= 1)
        {
          GSource *gsource = g_idle_source_new ();
          g_source_set_callback (gsource,
                                 (GSourceFunc) search_op_notify_cb,
                                 operation, NULL);
          g_source_attach (gsource, priv->context);
        }

      g_async_queue_unlock (priv->ret_queue);
    }

  if (remaining == 0)
    {
      operation->priv->current_id = 0;
      op->base.free_callback ((GriloOp *) op);
    }
}

static void
search_op_start_cb (GriloSearchOp *op)
{
  MexGriloOperation *operation = op->base.operation;
  GrlMediaPlugin *plugin =
    grl_plugin_registry_lookup_source (grl_plugin_registry_get_default (),
                                       op->source_id);
  GList *keys;

  if (!plugin)
    {
      g_warning ("No source for id '%s'", op->source_id);
      op->base.free_callback ((GriloOp *) op);
      return;
    }

  keys =
    mex_to_grilo_keys (mex_metadatas_get_metadata_list (op->base.metadatas));

  op->id = grl_media_source_search (GRL_MEDIA_SOURCE (plugin),
                                         op->text,
                                         keys,
                                         0,
                                         G_MAXINT,
                                         /* GRL_RESOLVE_IDLE_RELAY | */
                                         GRL_RESOLVE_FAST_ONLY,
                                         search_op_grilo_result_cb,
                                         op);

  operation->priv->current_id = op->id;

  g_list_free (keys);
}

void
mex_grilo_operation_search (MexGriloOperation *operation,
                            const gchar       *source_id,
                            const gchar       *text,
                            MexMetadatas      *metadatas)
{
  MexGriloOperationPrivate *priv;
  GriloSearchOp *op;

  g_return_if_fail (MEX_IS_GRILO_OPERATION (operation));
  g_return_if_fail (source_id != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (MEX_IS_METADATAS (metadatas));

  priv = operation->priv;

  op = g_new0 (GriloSearchOp, 1);

  op->base.operation       = g_object_ref (operation);
  op->base.metadatas       = g_object_ref (metadatas);
  op->base.start_callback  = (GriloOpCb) search_op_start_cb;
  op->base.free_callback   = (GriloOpCb) search_op_free_cb;
  op->base.create_callback = priv->create_callback;
  op->base.create_data     = priv->create_data;

  op->text      = g_strdup (text);
  op->source_id = g_strdup (source_id);

  g_async_queue_push (priv->op_queue, op);
  mex_grilo_schedule (operation);
}

/* Search all */
static void
search_all_op_start_cb (GriloSearchOp *op)
{
  MexGriloOperation *operation = op->base.operation;
  GList *keys;

  keys =
    mex_to_grilo_keys (mex_metadatas_get_metadata_list (op->base.metadatas));

  op->id = grl_multiple_search (NULL,
                                     op->text,
                                     keys,
                                     G_MAXINT,
                                     /* GRL_RESOLVE_IDLE_RELAY | */
                                     GRL_RESOLVE_FAST_ONLY,
                                     search_op_grilo_result_cb,
                                     op);

  operation->priv->current_id = op->id;

  g_list_free (keys);
}

void
mex_grilo_operation_search_all (MexGriloOperation *operation,
                                const gchar       *text,
                                MexMetadatas      *metadatas)
{
  MexGriloOperationPrivate *priv;
  GriloSearchOp *op;

  g_return_if_fail (MEX_IS_GRILO_OPERATION (operation));
  g_return_if_fail (text != NULL);
  g_return_if_fail (MEX_IS_METADATAS (metadatas));

  priv = operation->priv;

  op = g_new0 (GriloSearchOp, 1);

  op->base.operation       = g_object_ref (operation);
  op->base.metadatas       = g_object_ref (metadatas);
  op->base.start_callback  = (GriloOpCb) search_all_op_start_cb;
  op->base.free_callback   = (GriloOpCb) search_op_free_cb;
  op->base.create_callback = priv->create_callback;
  op->base.create_data     = priv->create_data;

  op->text = g_strdup (text);

  g_async_queue_push (priv->op_queue, op);
  mex_grilo_schedule (operation);
}

/* Metadata handling */
void
mex_grilo_operation_metadata (MexGriloOperation *operation,
                              MexContent        *content,
                              MexMetadatas      *metadatas)
{
  g_return_if_fail (MEX_IS_GRILO_OPERATION (operation));
  g_return_if_fail (MEX_IS_CONTENT (content));
  g_return_if_fail (MEX_IS_METADATAS (metadatas));


}

/* Cancelling operation handling */
typedef struct
{
  GriloOp base;

  guint id;
} GriloCancelOp;

static void
cancel_op_start_cb (GriloCancelOp *op)
{
  grl_operation_cancel (op->id);

  op->base.free_callback ((GriloOp *) op);
}

void
mex_grilo_operation_stop (MexGriloOperation *operation)
{
  MexGriloOperationPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_OPERATION (operation));

  priv = operation->priv;

  if (!priv->current_id)
    return;

  op = g_new0 (GriloCancelOp, 1);

  op->id = priv->current_id;
  op->base.start_callback  = (GriloOpCb) cancel_op_start_cb;
  op->base.free_callback   = (GriloOpCb) op_free;

  g_async_queue_push (priv->op_queue, op);
  mex_grilo_schedule (operation);
}

/**/
void
mex_grilo_operation_set_create_content_cb (MexGriloOperation  *operation,
                                           MexCreateContentCb  callback,
                                           gpointer            data)
{
  MexGriloOperationPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_OPERATION (operation));
  g_return_if_fail (callback != NULL);

  priv = operation->priv;

  priv->create_callback = callback;
  priv->create_data     = data;
}

/* Likely to be called from another thread. */
void
mex_grilo_operation_start (MexGriloOperation *operation)
{
  MexGriloOperationPrivate *priv = operation->priv;
  GriloOp *op;

  op = (GriloOp *) g_async_queue_pop (priv->op_queue);
  op->start_callback (op);
}
