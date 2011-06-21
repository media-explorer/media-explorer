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

#define _GNU_SOURCE

#include <string.h>

#include "mex-content-view.h"
#include "mex-grilo-feed.h"
#include "mex-player.h"

enum {
  PROP_0,
  PROP_SOURCE,
  PROP_ROOT,
  PROP_QUERY_KEYS,
  PROP_METADATA_KEYS,
  PROP_COMPLETED
};

struct _MexGriloFeedPrivate {
  GrlMediaSource *source;
  GrlMedia *root;

  MexGriloOperation *op;
  GList *query_keys;
  GList *metadata_keys;

  guint        completed : 1;

  GHashTable *programs;

  MexGriloFeedOpenCb open_callback;
};

#define BROWSE_LIMIT 100
#define BROWSE_FLAGS (GRL_RESOLVE_IDLE_RELAY | GRL_RESOLVE_FAST_ONLY)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),           \
                                                       MEX_TYPE_GRILO_FEED, \
                                                       MexGriloFeedPrivate))

G_DEFINE_TYPE (MexGriloFeed, mex_grilo_feed, MEX_TYPE_FEED)

static GQuark mex_grilo_feed_aggregate_model_quark = 0;

static void update_source (MexGriloFeed *feed, GrlMediaSource *new_source);

static void mex_grilo_feed_stop_op (MexGriloFeed *feed);
static void mex_grilo_feed_start_op (MexGriloFeed *feed);
static void mex_grilo_feed_free_op (MexGriloFeed *feed);
static void mex_grilo_feed_init_op (MexGriloFeed *feed);

static guint _mex_grilo_feed_browse (MexGriloFeed           *feed,
                                     int                     offset,
                                     int                     limit,
                                     GrlMediaSourceResultCb  callback);
static guint _mex_grilo_feed_query (MexGriloFeed           *feed,
                                    const char             *query,
                                    int                     offset,
                                    int                     limit,
                                    GrlMediaSourceResultCb  callback);
static guint _mex_grilo_feed_search (MexGriloFeed           *feed,
                                     const char             *search_text,
                                     int                     offset,
                                     int                     limit,
                                     GrlMediaSourceResultCb  callback);

static void _mex_grilo_feed_content_updated (GrlMediaSource           *source,
                                             GPtrArray                *changed_medias,
                                             GrlMediaSourceChangeType  change_type,
                                             gboolean                  known_location,
                                             MexGriloFeed             *feed);

static void mex_grilo_feed_open_default (MexGriloProgram *program,
                                         MexGriloFeed    *feed);

static void
mex_grilo_feed_finalize (GObject *object)
{
  MexGriloFeed *self = (MexGriloFeed *) object;
  MexGriloFeedPrivate *priv = self->priv;

  if (priv->op) {
    g_slice_free (MexGriloOperation, priv->op);
    priv->op = NULL;
  }

  if (priv->query_keys) {
    g_list_free (priv->query_keys);
    priv->query_keys = NULL;
  }

  if (priv->metadata_keys) {
    g_list_free (priv->metadata_keys);
    priv->metadata_keys = NULL;
  }

  G_OBJECT_CLASS (mex_grilo_feed_parent_class)->finalize (object);
}

static void
mex_grilo_feed_dispose (GObject *object)
{
  MexGriloFeed *self = (MexGriloFeed *) object;
  MexGriloFeedPrivate *priv = self->priv;

  mex_grilo_feed_free_op (self);

  if (priv->programs) {
    g_hash_table_destroy (priv->programs);
    priv->programs = NULL;
  }

  if (priv->source) {
    update_source (self, NULL);
  }

  if (priv->root) {
    g_object_unref (priv->root);
    priv->root = NULL;
  }

  G_OBJECT_CLASS (mex_grilo_feed_parent_class)->dispose (object);
}

static void
mex_grilo_feed_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MexGriloFeed *self = (MexGriloFeed *) object;
  MexGriloFeedPrivate *priv = self->priv;

  switch (prop_id) {
  case PROP_SOURCE:
    update_source (self, g_value_get_object (value));
    break;

  case PROP_QUERY_KEYS:
    priv->query_keys = g_list_copy (g_value_get_pointer (value));
    break;

  case PROP_METADATA_KEYS:
    priv->metadata_keys = g_list_copy (g_value_get_pointer (value));
    break;

  case PROP_ROOT:
    priv->root = g_value_dup_object (value);
    break;

  default:
    break;
  }
}

static void
mex_grilo_feed_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MexGriloFeed *self = (MexGriloFeed *) object;
  MexGriloFeedPrivate *priv = self->priv;

  switch (prop_id) {
  case PROP_SOURCE:
    g_value_set_object (value, priv->source);
    break;

  case PROP_ROOT:
    g_value_set_object (value, priv->root);
    break;

  case PROP_QUERY_KEYS:
    g_value_set_pointer (value, g_list_copy (priv->query_keys));
    break;

  case PROP_METADATA_KEYS:
    g_value_set_pointer (value, g_list_copy (priv->metadata_keys));
    break;

  case PROP_COMPLETED:
    g_value_set_boolean (value, priv->completed);
    break;

  default:
    break;
  }
}

static void
mex_grilo_feed_constructed (GObject *object)
{
  const gchar *title;
  MexGriloFeed *self = (MexGriloFeed *) object;
  MexGriloFeedPrivate *priv = self->priv;
  MexGriloFeedClass *klass = MEX_GRILO_FEED_GET_CLASS (object);

  if (priv->source == NULL) {
    g_warning ("No source supplied");
    return;
  }

  /* Fill keys in case it's not already done at creation. */
  if (priv->query_keys == NULL) {
    priv->query_keys = mex_grilo_program_get_default_keys ();
  }

  if (priv->metadata_keys == NULL) {
    priv->metadata_keys = g_list_copy (priv->query_keys);
  }

  title = NULL;
  if (priv->root)
    title = grl_media_get_title (priv->root);
  if (!title && GRL_IS_MEDIA_PLUGIN (priv->source))
    title = grl_metadata_source_get_name (GRL_METADATA_SOURCE (priv->source));
  if (title)
    g_object_set (object, "title", title, NULL);

  if (priv->source != NULL) {
    g_signal_handlers_disconnect_by_func (priv->source,
                                          G_CALLBACK (klass->content_updated),
                                          self);
    g_signal_connect (priv->source,
                      "content-changed",
                      G_CALLBACK (klass->content_updated),
                      self);
  }
}

static void
mex_grilo_feed_class_init (MexGriloFeedClass *klass)
{
  GObjectClass *o_class = (GObjectClass *) klass;
  GParamSpec *pspec;

  o_class->dispose = mex_grilo_feed_dispose;
  o_class->finalize = mex_grilo_feed_finalize;
  o_class->set_property = mex_grilo_feed_set_property;
  o_class->get_property = mex_grilo_feed_get_property;
  o_class->constructed = mex_grilo_feed_constructed;

  klass->browse = _mex_grilo_feed_browse;
  klass->query = _mex_grilo_feed_query;
  klass->search = _mex_grilo_feed_search;
  klass->content_updated = _mex_grilo_feed_content_updated;

  g_type_class_add_private (klass, sizeof (MexGriloFeedPrivate));

  pspec = g_param_spec_object ("grilo-source", "Grilo source",
                               "Grilo source for this feed",
                               GRL_TYPE_MEDIA_SOURCE,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_SOURCE, pspec);

  pspec = g_param_spec_object ("grilo-box", "Grilo box",
                               "Grilo box that represents the root",
                               GRL_TYPE_MEDIA,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_ROOT, pspec);

  pspec = g_param_spec_pointer ("grilo-query-keys", "Grilo query keys",
                                "The Grilo metadata keys that the feed "
                                "tries to retrieve when querying.",
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                                G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_QUERY_KEYS, pspec);

  pspec = g_param_spec_pointer ("grilo-metadata-keys", "Grilo metadata keys",
                                "The Grilo metadata keys that the feed "
                                "tries to retrieve when fully exploring the medias.",
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                                G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_METADATA_KEYS, pspec);

  pspec = g_param_spec_boolean ("completed", "Completed",
                                "Whether the current query has completed.",
                                FALSE,
                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_COMPLETED, pspec);

  mex_grilo_feed_aggregate_model_quark =
    g_quark_from_static_string ("mex-grilo-feed");
}

static void
mex_grilo_feed_init (MexGriloFeed *self)
{
  MexGriloFeedPrivate *priv = GET_PRIVATE (self);

  self->priv = priv;

  priv->programs = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          NULL, g_object_unref);

  priv->open_callback = mex_grilo_feed_open_default;
}

MexFeed *
mex_grilo_feed_new (GrlMediaSource *source,
                    const GList    *query_keys,
                    const GList    *metadata_keys,
                    GrlMedia       *root)
{
  return g_object_new (MEX_TYPE_GRILO_FEED,
                       "grilo-source", source,
                       "grilo-box", root,
                       "grilo-query-keys", query_keys,
                       "grilo-metadata-keys", metadata_keys,
                       NULL);
}

static void
emit_media_added (MexGriloFeed *feed,
                  GrlMedia     *media)
{
  MexProgram *program;

  program = mex_grilo_program_new (feed, media);
  mex_model_add_content (MEX_MODEL (feed), (MexContent *) program);
}

static void
browse_cb (GrlMediaSource *source,
           guint           browse_id,
           GrlMedia       *media,
           guint           remaining,
           gpointer        userdata,
           const GError   *error)
{
  MexGriloFeed *feed = (MexGriloFeed *) userdata;
  MexGriloFeedPrivate *priv = feed->priv;
  MexGriloProgram *program;

  if (error) {
    g_warning ("Error browsing: %s", error->message);
    return;
  }

  if (priv->op == NULL) {
    g_warning ("No operation found");
    return;
  }

  if (priv->op->op_id != browse_id)
    return;

  if (media) {
    /*
     * FIXME: talk to thomas/lionel/grilo guys about that crasher. We are
     * being called with what seems to be an invalid media when cancelled.
     * this is obviously temporary to enable people to work in the meantime
     */
    gconstpointer foo = grl_media_get_id (media);
    if (!foo) {
      const gchar *source_name;

      source_name =
        grl_metadata_source_get_name (GRL_METADATA_SOURCE (priv->source));
      g_warning ("FIXME: oh no, a grilo bug! (on the '%s' source)",
                 source_name);
      return;
    }

    program = g_hash_table_lookup (priv->programs,
                                   grl_media_get_id (media));
    if (program != NULL) {
      mex_grilo_program_set_grilo_media (program, media);
      return;
    } else {
      emit_media_added (feed, media);
      g_object_unref (media);
    }
  }

  priv->op->count++;

  if (remaining == 0) {
    priv->op->op_id = 0;

    /* Emit completed signal */
    priv->completed = TRUE;
    g_object_notify (G_OBJECT (feed), "completed");
  }
}

static void
mex_grilo_feed_stop_op (MexGriloFeed *feed)
{
  MexGriloFeedPrivate *priv = feed->priv;

  if (!priv->op)
    return;

  if (!priv->op->op_id)
    return;

  grl_metadata_source_cancel (GRL_METADATA_SOURCE (priv->source),
                              priv->op->op_id);
  priv->op->op_id = 0;

  if (priv->completed) {
    priv->completed = FALSE;
    g_object_notify (G_OBJECT (feed), "completed");
  }
}

static void
mex_grilo_feed_start_op (MexGriloFeed *feed)
{
  MexGriloFeedPrivate *priv = feed->priv;
  MexGriloFeedClass *klass = MEX_GRILO_FEED_GET_CLASS (feed);

  if (!priv->op)
    return;

  if (priv->op->op_id) {
    mex_grilo_feed_stop_op (feed);
  }

  switch (priv->op->type) {
  case MEX_GRILO_FEED_OPERATION_NONE:
    g_assert_not_reached ();
    break;

  case MEX_GRILO_FEED_OPERATION_BROWSE:
    priv->op->op_id = klass->browse (feed,
                                     priv->op->offset,
                                     priv->op->limit,
                                     browse_cb);
    break;

  case MEX_GRILO_FEED_OPERATION_QUERY:
    priv->op->op_id = klass->query (feed,
                                    priv->op->text,
                                    priv->op->offset,
                                    priv->op->limit,
                                    browse_cb);
    break;

  case MEX_GRILO_FEED_OPERATION_SEARCH:
    priv->op->op_id = klass->search (feed,
                                     priv->op->text,
                                     priv->op->offset,
                                     priv->op->limit,
                                     browse_cb);
    break;
  }
}

static void
mex_grilo_feed_free_op (MexGriloFeed *feed)
{
  MexGriloFeedPrivate *priv = feed->priv;

  if (!priv->op)
    return;

  mex_grilo_feed_stop_op (feed);

  if (priv->op->text != NULL) {
    g_free (priv->op->text);
  }

  priv->op = g_slice_new0 (MexGriloOperation);
}

static void
mex_grilo_feed_init_op (MexGriloFeed *feed)
{
  MexGriloFeedPrivate *priv = feed->priv;

  /* Cancel any previous operation */
  if (!priv->op) {
    priv->op = g_slice_new0 (MexGriloOperation);
  } else if (priv->op->op_id) {
    mex_grilo_feed_stop_op (feed);
  }

  if (priv->op->text != NULL) {
    g_free (priv->op->text);
  }

  if (priv->completed) {
    priv->completed = FALSE;
    g_object_notify (G_OBJECT (feed), "completed");
  }
}

static guint
_mex_grilo_feed_browse (MexGriloFeed           *feed,
                        int                     offset,
                        int                     limit,
                        GrlMediaSourceResultCb  callback)
{
  MexGriloFeedPrivate *priv = feed->priv;

  return grl_media_source_browse (priv->source, priv->root,
                                  priv->query_keys,
                                  priv->op->offset,
                                  priv->op->limit,
                                  BROWSE_FLAGS,
                                  callback, feed);
}

static guint
_mex_grilo_feed_query (MexGriloFeed           *feed,
                       const char             *query,
                       int                     offset,
                       int                     limit,
                       GrlMediaSourceResultCb  callback)
{
  MexGriloFeedPrivate *priv = feed->priv;

  return grl_media_source_query (priv->source, priv->op->text,
                                 priv->query_keys,
                                 priv->op->offset,
                                 priv->op->limit,
                                 BROWSE_FLAGS,
                                 callback, feed);
}

static guint
_mex_grilo_feed_search (MexGriloFeed           *feed,
                        const char             *search_text,
                        int                     offset,
                        int                     limit,
                        GrlMediaSourceResultCb  callback)
{
  MexGriloFeedPrivate *priv = feed->priv;

  return grl_media_source_search (priv->source, priv->op->text,
                                  priv->query_keys,
                                  priv->op->offset,
                                  priv->op->limit,
                                  BROWSE_FLAGS,
                                  callback, feed);
}

void
mex_grilo_feed_browse (MexGriloFeed *feed,
                       int           offset,
                       int           limit)
{
  MexGriloFeedPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_FEED (feed));

  priv = feed->priv;

  mex_grilo_feed_init_op (feed);
  mex_model_clear (MEX_MODEL (feed));

  priv->op->type = MEX_GRILO_FEED_OPERATION_BROWSE;
  priv->op->offset = offset;
  priv->op->limit = limit;
  priv->op->count = 0;

  mex_grilo_feed_start_op (feed);
}

void
mex_grilo_feed_search (MexGriloFeed *feed,
                       const char   *search_text,
                       int           offset,
                       int           limit)
{
  MexGriloFeedPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_FEED (feed));

  priv = feed->priv;

  mex_grilo_feed_init_op (feed);
  mex_model_clear (MEX_MODEL (feed));

  priv->op->type = MEX_GRILO_FEED_OPERATION_SEARCH;
  priv->op->offset = offset;
  priv->op->limit = limit;
  priv->op->count = 0;
  priv->op->text = g_strdup (search_text);

  mex_grilo_feed_start_op (feed);
}

void
mex_grilo_feed_query (MexGriloFeed *feed,
                      const char   *query,
                      int           offset,
                      int           limit)
{
  MexGriloFeedPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_FEED (feed));

  priv = feed->priv;

  mex_grilo_feed_init_op (feed);
  mex_model_clear (MEX_MODEL (feed));

  priv->op->type = MEX_GRILO_FEED_OPERATION_QUERY;
  priv->op->offset = offset;
  priv->op->limit = limit;
  priv->op->count = 0;
  priv->op->text = g_strdup (query);

  mex_grilo_feed_start_op (feed);
}

const MexGriloOperation *
mex_grilo_feed_get_operation (MexGriloFeed *feed)
{
  MexGriloFeedPrivate *priv;

  g_return_val_if_fail (MEX_IS_GRILO_FEED (feed), NULL);

  priv = feed->priv;

  return priv->op;
}

gboolean
mex_grilo_feed_get_completed (MexGriloFeed *feed)
{
  g_return_val_if_fail (MEX_IS_GRILO_FEED (feed), FALSE);
  return feed->priv->completed;
}

static void
_mex_grilo_feed_content_updated (GrlMediaSource *source,
                                 GPtrArray *changed_medias,
                                 GrlMediaSourceChangeType change_type,
                                 gboolean known_location,
                                 MexGriloFeed *feed)
{
  gint i;
  GrlMedia *media;
  const gchar *id;
  MexGriloProgram *program;
  MexGriloFeedPrivate *priv = feed->priv;

  for (i = 0 ; i < changed_medias->len ; i++)
    {
      media =  g_ptr_array_index (changed_medias, i);
      id = grl_media_get_id (media);
      switch (change_type)
        {
        case GRL_CONTENT_CHANGED:
          program = MEX_GRILO_PROGRAM (g_hash_table_lookup (priv->programs, id));
          /* The policy might be slightly different here... */
          if (program != NULL) {
            mex_grilo_program_set_grilo_media (program, media);
          }
          break;

        case GRL_CONTENT_ADDED:
          program = MEX_GRILO_PROGRAM (g_hash_table_lookup (priv->programs, id));
          if (program != NULL) {
            mex_grilo_program_set_grilo_media (program, media);
          } else {
            program = (MexGriloProgram *) mex_grilo_program_new (feed, media);
            mex_model_add_content (MEX_MODEL (feed), MEX_CONTENT (program));
          }
          break;

        case GRL_CONTENT_REMOVED:
          program = MEX_GRILO_PROGRAM (g_hash_table_lookup (priv->programs, id));
          if (program != NULL) {
            mex_model_remove_content (MEX_MODEL (feed), MEX_CONTENT (program));
          }
          break;
        }
    }
}

static void
update_source (MexGriloFeed *feed, GrlMediaSource *new_source)
{
  MexGriloFeedPrivate *priv = feed->priv;
  MexGriloFeedClass *klass = MEX_GRILO_FEED_GET_CLASS (feed);

  if (priv->source != NULL) {
    g_signal_handlers_disconnect_by_func (priv->source,
                                          G_CALLBACK (klass->content_updated),
                                          feed);
    g_object_unref (priv->source);
    priv->source = NULL;
  }

  if (new_source) {
    const gchar *source_name =
      grl_metadata_source_get_name (GRL_METADATA_SOURCE (new_source));

    priv->source = g_object_ref (new_source);
    g_signal_connect (priv->source,
                      "content-changed",
                      G_CALLBACK (klass->content_updated),
                      feed);

    if (strcasestr (source_name, "removable")) {
      g_object_set (feed, "icon-name", "icon-panelheader-usb", NULL);
    } else {
      g_object_set (feed, "icon-name", "icon-panelheader-computer", NULL);
    }
  }
}

MexAggregateModel *
mex_grilo_feed_to_aggregate_model (MexGriloFeed *feed,
                                   int           offset,
                                   int           limit)
{
  int i;
  MexContent *content;
  GrlMediaSource *source;
  MexModel *model, *ag_model;

  g_return_val_if_fail (MEX_IS_GRILO_FEED (feed), NULL);

  source = feed->priv->source;
  model = MEX_MODEL (feed);
  ag_model = NULL;
  i = 0;

  while ((content = mex_model_get_content (model, i))) {
    const char *mime =
      mex_content_get_metadata (content,
                                MEX_CONTENT_METADATA_MIMETYPE);
    if (mime && g_str_equal (mime, "x-grl/box")) {
      GrlMedia *media;
      MexFeed *sub_feed;
      GList *query_keys = NULL, *metadata_keys = NULL;

      if (!ag_model) {
        ag_model = mex_aggregate_model_new ();
        mex_aggregate_model_add_model (MEX_AGGREGATE_MODEL (ag_model),
                                       g_object_ref (model));
        g_object_set_qdata (G_OBJECT (ag_model),
                            mex_grilo_feed_aggregate_model_quark,
                            feed);
      }

      media =
        mex_grilo_program_get_grilo_media (MEX_GRILO_PROGRAM (content));

      g_object_get (G_OBJECT (feed),
                    "grilo-query-keys", &query_keys,
                    "grilo-metadata-keys", &metadata_keys,
                    NULL);

      sub_feed = mex_grilo_feed_new (source, query_keys, metadata_keys, media);
      g_list_free (query_keys);
      g_list_free (metadata_keys);
      mex_grilo_feed_browse (MEX_GRILO_FEED (sub_feed), offset, limit);

      mex_aggregate_model_add_model (MEX_AGGREGATE_MODEL (ag_model),
                                     MEX_MODEL (sub_feed));
    }
    i++;
  }

  return (MexAggregateModel *)ag_model;
}

MexGriloFeed *
mex_aggregate_model_get_grilo_feed (MexAggregateModel *model)
{
  g_return_val_if_fail (MEX_IS_AGGREGATE_MODEL (model), NULL);
  return g_object_get_qdata (G_OBJECT (model),
                             mex_grilo_feed_aggregate_model_quark);
}

void
mex_grilo_feed_set_open_callback (MexGriloFeed       *feed,
                                  MexGriloFeedOpenCb  callback)
{
  MexGriloFeedPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_FEED (feed));

  priv = feed->priv;

  priv->open_callback = callback;
}

static void
mex_grilo_feed_open_default (MexGriloProgram *program, MexGriloFeed *feed)
{
  MexPlayer *player = mex_player_get_default ();

  mex_content_view_set_context (MEX_CONTENT_VIEW (player),
                                MEX_MODEL (feed));
  mex_content_view_set_content (MEX_CONTENT_VIEW (player),
                                MEX_CONTENT (program));
}


void
mex_grilo_feed_open (MexGriloFeed    *feed,
                     MexGriloProgram *program)
{
  MexGriloFeedPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_FEED (feed));
  g_return_if_fail (MEX_IS_GRILO_PROGRAM (program));

  priv = feed->priv;

  if (priv->open_callback)
    priv->open_callback (program, feed);
}
