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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif

#include "mex-log.h"
#include "mex-download-queue.h"

#define MEX_LOG_DOMAIN_DEFAULT  download_queue_log_domain
MEX_LOG_DOMAIN(download_queue_log_domain);

enum
{
  PROP_0,

  PROP_THROTTLE,
  PROP_QUEUE_LENGTH
};

enum
{
  LAST_SIGNAL,
};

struct _MexDownloadQueuePrivate
{
  GQueue *queue;
  GList  *last_local;
  guint   max_transfers;
  guint   in_progress;

  SoupSession *session;

  guint    throttle;
  GTimeVal last_process;
  guint    process_timeout;

  GHashTable *cache;
  gsize cache_size;
  guint current_age;
};

typedef enum
{
  MEX_DQ_TYPE_NONE,
  MEX_DQ_TYPE_GIO,
  MEX_DQ_TYPE_SOUP,
  MEX_DQ_TYPE_CACHED
} MexDownloadQueueTaskType;

struct _DQTaskAny
{
  MexDownloadQueueTaskType  type;
  MexDownloadQueue         *queue;
  char                     *uri;

  MexDownloadQueueCompletedReply callback;
  gpointer                       userdata;
};

#define MAX_CACHE_SIZE (6 * 1024 * 1024)
typedef struct
{
  gchar *data;
  gsize  length;
  gint   age;
} DQCacheItem;

#define BUFFER_SIZE 4096
struct _DQTaskGIO
{
  MexDownloadQueueTaskType  type;
  MexDownloadQueue         *queue;
  char                     *uri;

  MexDownloadQueueCompletedReply callback;
  gpointer                       userdata;

  GCancellable *cancellable;
  GFile        *file;
};

struct _DQTaskSoup
{
  MexDownloadQueueTaskType  type;
  MexDownloadQueue         *queue;
  char                     *uri;

  MexDownloadQueueCompletedReply callback;
  gpointer                       userdata;

  SoupMessage *message;
};

struct _DQTaskCached
{
  MexDownloadQueueTaskType  type;
  MexDownloadQueue         *queue;
  char                     *uri;

  MexDownloadQueueCompletedReply callback;
  gpointer                       userdata;

  guint source_id;
};

typedef union _DQTask
{
  MexDownloadQueueTaskType type;

  struct _DQTaskAny  any;
  struct _DQTaskGIO  gio;
  struct _DQTaskSoup soup;
  struct _DQTaskCached cached;
} DQTask;

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                                       MEX_TYPE_DOWNLOAD_QUEUE, \
                                                       MexDownloadQueuePrivate))
G_DEFINE_TYPE (MexDownloadQueue, mex_download_queue, G_TYPE_OBJECT);

static void process_queue (MexDownloadQueue *self);

static void
mex_download_queue_cache_item_free (DQCacheItem *item)
{
  g_free (item->data);
  g_slice_free (DQCacheItem, item);
}

static void
mex_download_queue_cache_insert (MexDownloadQueue *queue,
                                 const gchar      *uri,
                                 gchar            *data,
                                 gsize             length)
{
  DQCacheItem *item;
  guint iterations = 0;

  item = g_slice_new (DQCacheItem);
  item->length = length;
  item->data = data;
  item->age = queue->priv->current_age++;

  g_hash_table_insert (queue->priv->cache, g_strdup (uri), item);

  queue->priv->cache_size += item->length;

  MEX_DEBUG ("cache (%" G_GSSIZE_FORMAT "): added: %s",
             queue->priv->cache_size, uri);

  /* keep cache size under MAX_CACHE_SIZE */
  while (queue->priv->cache_size > MAX_CACHE_SIZE && iterations < 3)
    {
      DQCacheItem *value, *value_to_remove = NULL;
      gchar *key_to_remove = NULL, *key;
      guint lowest = G_MAXINT;
      GHashTableIter iter;

      g_hash_table_iter_init (&iter, queue->priv->cache);
      while (g_hash_table_iter_next (&iter, (gpointer*) &key,
                                     (gpointer*) &value))
        {
          /* find the item with the lowest age to evict from the cache */
          if (value->age < lowest)
            {
              lowest = value->age;
              key_to_remove = key;
              value_to_remove = value;
            }
        }

      if (value_to_remove)
        {
          queue->priv->cache_size -= value_to_remove->length;

          MEX_DEBUG ("cache (%" G_GSSIZE_FORMAT "): removed: %s",
                     queue->priv->cache_size, key_to_remove);

          g_hash_table_remove (queue->priv->cache, key_to_remove);
        }
      else
        {
          /* nothing to remove, hash table must be empty */
          break;
        }

      iterations++;
    }
}

static const DQCacheItem*
mex_download_queue_cache_lookup (MexDownloadQueue *queue,
                                 const gchar      *uri)
{
  DQCacheItem *item;

  item = g_hash_table_lookup (queue->priv->cache, uri);

  if (item)
    item->age = queue->priv->current_age++;

  return item;
}

static void
mex_download_queue_free (DQTask *task)
{
  MexDownloadQueue *self = task->any.queue;
  MexDownloadQueuePrivate *priv = self->priv;

  switch (task->any.type)
    {
    case MEX_DQ_TYPE_GIO:
      if (task->gio.cancellable)
        {
          g_cancellable_cancel (task->gio.cancellable);

          /* Return, cancelling the task will run the file load callback,
            * which will unref the cancellable and call this free function
            * again.
            */
          return;
        }

      if (task->gio.file)
        g_object_unref (task->gio.file);

      break;

    case MEX_DQ_TYPE_SOUP:
      if (task->soup.message)
        {
          soup_session_cancel_message (priv->session,
                                       task->soup.message,
                                       SOUP_STATUS_CANCELLED);

          /* Return, the callback will call this function again
            * after setting the message to NULL.
            */
          return;
        }

      break;

    default:
      break;
    }

  if (task->any.type != MEX_DQ_TYPE_NONE)
    {
      priv->in_progress--;
      process_queue (self);
      g_object_notify (G_OBJECT (self), "queue-length");
    }

  g_slice_free (DQTask, task);
}

static void
mex_download_queue_finalize (GObject *object)
{
  MexDownloadQueuePrivate *priv = MEX_DOWNLOAD_QUEUE (object)->priv;

  if (priv->cache)
    {
      g_hash_table_destroy (priv->cache);
      priv->cache = NULL;
    }

  G_OBJECT_CLASS (mex_download_queue_parent_class)->finalize (object);
}

static void
mex_download_queue_dispose (GObject *object)
{
  MexDownloadQueue *self = (MexDownloadQueue *)object;
  MexDownloadQueuePrivate *priv = self->priv;

  if (priv->process_timeout)
    {
      g_source_remove (priv->process_timeout);
      priv->process_timeout = 0;
    }

  if (priv->queue)
    {
      g_queue_foreach (priv->queue, (GFunc)mex_download_queue_free, NULL);
      g_queue_free (priv->queue);
      priv->queue = NULL;
    }

  G_OBJECT_CLASS (mex_download_queue_parent_class)->dispose (object);
}

static void
mex_download_queue_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MexDownloadQueue *self = MEX_DOWNLOAD_QUEUE (object);

  switch (prop_id)
    {
    case PROP_THROTTLE:
      mex_download_queue_set_throttle (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
mex_download_queue_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MexDownloadQueue *self = MEX_DOWNLOAD_QUEUE (object);

  switch (prop_id)
    {
    case PROP_THROTTLE:
      g_value_set_uint (value, mex_download_queue_get_throttle (self));
      break;

    case PROP_QUEUE_LENGTH:
      g_value_set_uint (value, mex_download_queue_get_queue_length (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
mex_download_queue_class_init (MexDownloadQueueClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *o_class = (GObjectClass *)klass;

  o_class->dispose = mex_download_queue_dispose;
  o_class->finalize = mex_download_queue_finalize;
  o_class->set_property = mex_download_queue_set_property;
  o_class->get_property = mex_download_queue_get_property;

  g_type_class_add_private (klass, sizeof (MexDownloadQueuePrivate));

  pspec = g_param_spec_uint ("throttle",
                             "Throttle",
                             "The minimum time to wait between new requests",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_THROTTLE, pspec);

  pspec = g_param_spec_uint ("queue-length",
                             "Queue length",
                             "The number of items in the "
                             "queue to be downloaded",
                             0, G_MAXUINT, 3,
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_QUEUE_LENGTH, pspec);
}

static void
file_load_cb (GObject      *source_object,
              GAsyncResult *res,
              gpointer      user_data)
{
  GError *error = NULL;
  DQTask *task = user_data;
  gchar *contents = NULL;
  gsize length = 0;

  g_file_load_contents_finish (task->gio.file,
                               res,
                               &contents,
                               &length,
                               NULL,
                               &error);

  if (!g_cancellable_is_cancelled (task->gio.cancellable))
    {
      if (error)
        {
          task->any.callback (task->any.queue, task->any.uri,
                              NULL, 0,
                              error,
                              task->any.userdata);
        }
      else
        {
          task->any.callback (task->any.queue, task->any.uri,
                              contents, length,
                              NULL,
                              task->any.userdata);

          mex_download_queue_cache_insert (task->any.queue, task->any.uri,
                                           contents, length);
        }
    }

  if (error)
    g_error_free (error);

  g_object_unref (task->gio.cancellable);
  task->gio.cancellable = NULL;

  mex_download_queue_free (task);
}

static void
process_gio (MexDownloadQueue *queue,
             DQTask           *task)
{
  task->gio.file = g_file_new_for_uri (task->any.uri);
  task->gio.cancellable = g_cancellable_new ();

  g_file_load_contents_async (task->gio.file,
                              task->gio.cancellable,
                              file_load_cb,
                              task);
}

static void
soup_session_cb (SoupSession *session,
                 SoupMessage *msg,
                 gpointer     user_data)
{
  DQTask *task = user_data;

  if (SOUP_STATUS_IS_REDIRECTION (msg->status_code))
    {
      const char *header =
        soup_message_headers_get_one (msg->response_headers, "Location");

      if (header)
        {
          SoupURI *uri;

          uri = soup_uri_new_with_base (soup_message_get_uri (msg), header);
          soup_message_set_uri (msg, uri);
          soup_uri_free (uri);

          soup_session_requeue_message (session, msg);

          return;
        }
    }
  else if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
    {
      void *cache;

      task->any.callback (task->any.queue, task->any.uri,
                          msg->response_body->data,
                          msg->response_body->length,
                          NULL,
                          task->any.userdata);

      /* add the contents to the cache */
      cache = g_memdup (msg->response_body->data, msg->response_body->length);
      mex_download_queue_cache_insert (task->any.queue, task->any.uri, cache,
                                       msg->response_body->length);

    }
  else if (msg->status_code != SOUP_STATUS_CANCELLED)
    {
      /* FIXME: Also create an error on failure */
      task->any.callback (task->any.queue, task->any.uri,
                          NULL, 0, NULL,
                          task->any.userdata);
    }

  /* The message is unref'd by the session */
  task->soup.message = NULL;

  mex_download_queue_free (task);
}

static void
process_soup (MexDownloadQueue *queue,
              DQTask           *task)
{
  MexDownloadQueuePrivate *priv = queue->priv;

  task->soup.message = soup_message_new (SOUP_METHOD_GET, task->any.uri);

  if (!task->soup.message)
    {
      /* FIXME: Another situation in which to create a GError */
      task->any.callback (task->any.queue, task->any.uri,
                          NULL, 0, NULL,
                          task->any.userdata);
      mex_download_queue_free (task);

      return;
    }

  soup_session_queue_message (priv->session,
                              task->soup.message,
                              soup_session_cb,
                              task);
}

static gboolean
process_queue_timeout_cb (MexDownloadQueue *self)
{
  MexDownloadQueuePrivate *priv = self->priv;

  priv->process_timeout = 0;
  process_queue (self);

  return FALSE;
}

static gboolean
run_cached_callback (DQTask *task)
{
  const DQCacheItem *cached;

  cached = mex_download_queue_cache_lookup (task->any.queue, task->any.uri);

  if (cached)
    task->any.callback (task->any.queue, task->any.uri,
                        cached->data, cached->length,
                        NULL, task->any.userdata);
  else
    task->any.callback (task->any.queue, task->any.uri,
                        NULL, 0, NULL,
                        task->any.userdata);


  mex_download_queue_free (task);

  return FALSE;
}

static void
process_cached (MexDownloadQueue *self,
                DQTask           *task)
{
  task->cached.source_id = g_idle_add ((GSourceFunc) run_cached_callback,
                                       task);
}

static void
process_queue (MexDownloadQueue *self)
{
  MexDownloadQueuePrivate *priv = self->priv;

  if (priv->in_progress >= priv->max_transfers)
    return;

  /* If we already have a timeout set, wait for that to happen */
  if (priv->process_timeout)
    return;

  /* Check if there's been enough time since the last request */
  if (priv->throttle &&
      (priv->last_process.tv_sec || priv->last_process.tv_usec))
    {
      GTimeVal current;
      guint difference;

      g_get_current_time (&current);

      difference = (current.tv_sec - priv->last_process.tv_sec) * 1000;
      difference += (current.tv_usec - priv->last_process.tv_usec) / 1000;

      if (difference < priv->throttle)
        {
          priv->process_timeout = g_timeout_add (priv->throttle - difference,
                                                 (GSourceFunc)
                                                 process_queue_timeout_cb,
                                                 self);
          return;
        }
    }

  /* Queue up new requests. If we have a throttle set, only
   * queue one request.
   */
  while ((priv->in_progress < priv->max_transfers) &&
         (g_queue_get_length (priv->queue) > 0))
    {
      DQTask *task = g_queue_peek_head (priv->queue);
      gboolean is_http = g_str_has_prefix (task->any.uri, "http://");
      const DQCacheItem *cached =
        mex_download_queue_cache_lookup (self, task->any.uri);

      /* Make sure to reserve one slot for local/cached content */
      if (!cached && is_http &&
          (priv->in_progress >= priv->max_transfers - 1))
        break;
      else
        {
          /* We've reached the last local download, wipe our
           * pointer.
           */
          if (priv->queue->head == priv->last_local)
            priv->last_local = NULL;

          g_queue_pop_head (priv->queue);
        }

      if (cached)
        {
          MEX_DEBUG ("cache: hit: %s", task->any.uri);

          task->type = MEX_DQ_TYPE_CACHED;
          process_cached (self, task);
        }
      else if (is_http)
        {
          MEX_DEBUG ("cache miss, using soup: %s", task->any.uri);

          task->type = MEX_DQ_TYPE_SOUP;
          process_soup (self, task);
        }
      else
        {
          MEX_DEBUG ("cache miss, using gio: %s", task->any.uri);

          task->type = MEX_DQ_TYPE_GIO;
          process_gio (self, task);
        }

      priv->in_progress++;

      if (priv->throttle)
        break;
    }

  /* Set a timeout to handle another request if there are more in
   * the queue and we're throttling requests.
   */
  g_get_current_time (&priv->last_process);
  if (priv->throttle && (g_queue_get_length (priv->queue) > 0))
    priv->process_timeout = g_timeout_add (priv->throttle,
                                           (GSourceFunc)
                                           process_queue_timeout_cb,
                                           self);
}

static void
mex_download_queue_init (MexDownloadQueue *self)
{
  MexDownloadQueuePrivate *priv = GET_PRIVATE (self);

  self->priv = priv;
  priv->queue = g_queue_new ();
  priv->max_transfers = 3;

  priv->session = soup_session_async_new_with_options (
#ifdef HAVE_LIBSOUP_GNOME
    SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_GNOME_FEATURES_2_26,
#endif
    SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
    SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_COOKIE_JAR,
    SOUP_SESSION_ACCEPT_LANGUAGE_AUTO, TRUE,
    NULL);

  priv->cache = g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free,
                                       (GDestroyNotify) mex_download_queue_cache_item_free);
}

MexDownloadQueue *
mex_download_queue_get_default (void)
{
  static MexDownloadQueue *queue = NULL;

  if (G_UNLIKELY (queue == NULL))
    queue = g_object_new (MEX_TYPE_DOWNLOAD_QUEUE, NULL);

  return queue;
}

gpointer
mex_download_queue_enqueue (MexDownloadQueue               *queue,
                            const char                     *uri,
                            MexDownloadQueueCompletedReply  reply,
                            gpointer                        userdata)
{
  MexDownloadQueuePrivate *priv;
  DQTask *task;

  g_return_val_if_fail (MEX_IS_DOWNLOAD_QUEUE (queue), NULL);
  g_return_val_if_fail (uri, NULL);

  priv = queue->priv;

  task = g_slice_new0 (DQTask);
  task->any.uri = g_strdup (uri);
  task->any.queue = queue;
  task->any.callback = reply;
  task->any.userdata = userdata;

  MEX_DEBUG ("queueing download: %s", uri);

  if (g_str_has_prefix (uri, "http://"))
    g_queue_push_tail (priv->queue, task);
  else
    {
      /* Push local requests before web requests */
      if (!priv->last_local)
        {
          g_queue_push_head (priv->queue, task);
          priv->last_local = priv->queue->head;
        }
      else
        {
          g_queue_insert_after (priv->queue, priv->last_local, task);
          priv->last_local = priv->last_local->next;
        }
    }

  process_queue (queue);

  g_object_notify (G_OBJECT (queue), "queue-length");

  return task;
}

void
mex_download_queue_cancel (MexDownloadQueue *queue,
                           gpointer          id)
{
  MexDownloadQueuePrivate *priv;
  DQTask *task = id;
  GList *l;

  g_return_if_fail (MEX_IS_DOWNLOAD_QUEUE (queue));
  g_return_if_fail (id);

  priv = queue->priv;

  MEX_DEBUG ("cancelling download: %s", task->any.uri);

  l = g_queue_find (priv->queue, task);
  if (l)
    {
      /* Make sure our last-local link stays valid */
      if (priv->last_local == l)
        priv->last_local = l->prev;

      mex_download_queue_free (task);
      g_queue_delete_link (priv->queue, l);

      g_object_notify (G_OBJECT (queue), "queue-length");

      return;
    }

  switch (task->type)
    {
    case MEX_DQ_TYPE_SOUP:
      soup_session_cancel_message (priv->session,
                                   task->soup.message,
                                   SOUP_STATUS_CANCELLED);
      break;

    case MEX_DQ_TYPE_GIO:
      g_cancellable_cancel (task->gio.cancellable);
      break;

    case MEX_DQ_TYPE_CACHED:
      if (task->cached.source_id)
        g_source_remove (task->cached.source_id);
      task->cached.source_id = 0;

      mex_download_queue_free (task);
      break;

    default:
      g_warning ("Unknown download type cancelled! %d", task->type);
      break;
    }
}

void
mex_download_queue_set_throttle (MexDownloadQueue *queue,
                                 guint             throttle)
{
  MexDownloadQueuePrivate *priv;

  g_return_if_fail (MEX_IS_DOWNLOAD_QUEUE (queue));

  priv = queue->priv;
  if (priv->throttle != throttle)
    {
      priv->throttle = throttle;
      g_object_notify (G_OBJECT (queue), "throttle");
    }
}

guint
mex_download_queue_get_throttle (MexDownloadQueue *queue)
{
  g_return_val_if_fail (MEX_IS_DOWNLOAD_QUEUE (queue), 0);
  return queue->priv->throttle;
}

guint
mex_download_queue_get_queue_length (MexDownloadQueue *queue)
{
  g_return_val_if_fail (MEX_IS_DOWNLOAD_QUEUE (queue), 0);
  return g_queue_get_length (queue->priv->queue) + queue->priv->in_progress;
}
