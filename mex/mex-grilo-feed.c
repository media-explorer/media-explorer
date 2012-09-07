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
  GrlSource *source;
  GrlMedia *root;

  MexGriloOperation *op;
  GList *query_keys;
  GList *metadata_keys;

  guint        completed : 1;

  MexGriloFeedOpenCb open_callback;

  GList *items_to_add;
};

#define BROWSE_LIMIT 100
#define BROWSE_FLAGS (GRL_RESOLVE_IDLE_RELAY | GRL_RESOLVE_FULL)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),           \
                                                       MEX_TYPE_GRILO_FEED, \
                                                       MexGriloFeedPrivate))

G_DEFINE_TYPE (MexGriloFeed, mex_grilo_feed, MEX_TYPE_FEED)

static void update_source (MexGriloFeed *feed, GrlSource *new_source);

static void mex_grilo_feed_stop_op (MexGriloFeed *feed);
static void mex_grilo_feed_start_op (MexGriloFeed *feed);
static void mex_grilo_feed_free_op (MexGriloFeed *feed);
static void mex_grilo_feed_init_op (MexGriloFeed *feed);

static guint _mex_grilo_feed_browse (MexGriloFeed      *feed,
                                     int                offset,
                                     int                limit,
                                     GrlSourceResultCb  callback);
static guint _mex_grilo_feed_query (MexGriloFeed      *feed,
                                    const char        *query,
                                    int                offset,
                                    int                limit,
                                    GrlSourceResultCb  callback);
static guint _mex_grilo_feed_search (MexGriloFeed      *feed,
                                     const char        *search_text,
                                     int                offset,
                                     int                limit,
                                     GrlSourceResultCb  callback);

static void _mex_grilo_feed_content_updated (GrlSource           *source,
                                             GPtrArray           *changed_medias,
                                             GrlSourceChangeType  change_type,
                                             gboolean             known_location,
                                             MexGriloFeed        *feed);

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

  if (G_OBJECT_CLASS (mex_grilo_feed_parent_class)->constructed)
    G_OBJECT_CLASS (mex_grilo_feed_parent_class)->constructed (object);

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
  if (!title && GRL_IS_SOURCE (priv->source))
    title = grl_source_get_name (GRL_SOURCE (priv->source));
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
                               GRL_TYPE_SOURCE,
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
}

static void
mex_grilo_feed_init (MexGriloFeed *self)
{
  MexGriloFeedPrivate *priv = GET_PRIVATE (self);

  self->priv = priv;

  priv->open_callback = mex_grilo_feed_open_default;
}

MexFeed *
mex_grilo_feed_new (GrlSource   *source,
                    const GList *query_keys,
                    const GList *metadata_keys,
                    GrlMedia    *root)
{
  return g_object_new (MEX_TYPE_GRILO_FEED,
                       "grilo-source", source,
                       "grilo-box", root,
                       "grilo-query-keys", query_keys,
                       "grilo-metadata-keys", metadata_keys,
                       NULL);
}

static gboolean
emit_media_added_finished (MexGriloFeed *feed)
{
  mex_model_add (MEX_MODEL (feed), feed->priv->items_to_add);

  g_list_free (feed->priv->items_to_add);
  feed->priv->items_to_add = NULL;

  g_object_unref (feed);

  return FALSE;
}

static void
emit_media_added (MexGriloFeed *feed, GrlMedia *media)
{
  MexProgram *program;

  /* collect items by waiting 250ms */
  if (!feed->priv->items_to_add)
    g_timeout_add (250, (GSourceFunc) emit_media_added_finished,
                   g_object_ref (feed));

  program = mex_grilo_program_new (feed, media);
  _mex_program_complete (program);
  feed->priv->items_to_add = g_list_prepend (feed->priv->items_to_add, program);
}

static void
browse_cb (GrlSource    *source,
           guint         browse_id,
           GrlMedia     *media,
           guint         remaining,
           gpointer      userdata,
           const GError *error)
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
        grl_source_get_name (GRL_SOURCE (priv->source));
      g_warning ("FIXME: oh no, a grilo bug! (on the '%s' source)",
                 source_name);
      return;
    }

    program = MEX_GRILO_PROGRAM (mex_feed_lookup (MEX_FEED (feed),
                                                  grl_media_get_id (media)));
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

  grl_operation_cancel (priv->op->op_id);
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
_mex_grilo_feed_browse (MexGriloFeed      *feed,
                        int                offset,
                        int                limit,
                        GrlSourceResultCb  callback)
{
  MexGriloFeedPrivate *priv = feed->priv;
  GrlOperationOptions *options;
	int op_id;

  options = grl_operation_options_new (NULL);
  grl_operation_options_set_flags (options, BROWSE_FLAGS);
  grl_operation_options_set_skip (options, priv->op->offset);
  grl_operation_options_set_count (options, priv->op->limit);


  op_id = grl_source_browse (priv->source, priv->root,
                             priv->query_keys,
                             options,
                             callback, feed);

	g_object_unref (options);

	return op_id;
}

static guint
_mex_grilo_feed_query (MexGriloFeed      *feed,
                       const char        *query,
                       int                offset,
                       int                limit,
                       GrlSourceResultCb  callback)
{
  MexGriloFeedPrivate *priv = feed->priv;
  GrlOperationOptions *options;
	int op_id;

  options = grl_operation_options_new (NULL);
  grl_operation_options_set_flags (options, BROWSE_FLAGS);
  grl_operation_options_set_skip (options, priv->op->offset);
  grl_operation_options_set_count (options, priv->op->limit);

  op_id = grl_source_query (priv->source, priv->op->text,
                            priv->query_keys,
                            options,
                            callback, feed);

	g_object_unref (options);

	return op_id;
}

static guint
_mex_grilo_feed_search (MexGriloFeed      *feed,
                        const char        *search_text,
                        int                offset,
                        int                limit,
                        GrlSourceResultCb  callback)
{
  MexGriloFeedPrivate *priv = feed->priv;
  GrlOperationOptions *options;
	int op_id;

  options = grl_operation_options_new (NULL);
  grl_operation_options_set_flags (options, BROWSE_FLAGS);
  grl_operation_options_set_skip (options, priv->op->offset);
  grl_operation_options_set_count (options, priv->op->limit);

  op_id = grl_source_search (priv->source, priv->op->text,
                             priv->query_keys,
                             options,
                             callback, feed);

	g_object_unref (options);

	return op_id;
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
_mex_grilo_feed_content_updated (GrlSource *source,
                                 GPtrArray *changed_medias,
                                 GrlSourceChangeType change_type,
                                 gboolean known_location,
                                 MexGriloFeed *feed)
{
  gint i;
  GrlMedia *media;
  const gchar *id;
  MexGriloProgram *program;

  for (i = 0 ; i < changed_medias->len ; i++)
    {
      media =  g_ptr_array_index (changed_medias, i);
      id = grl_media_get_id (media);
      switch (change_type)
        {
        case GRL_CONTENT_CHANGED:
          program = MEX_GRILO_PROGRAM (mex_feed_lookup (MEX_FEED (feed), id));
          /* The policy might be slightly different here... */
          if (program != NULL) {
            mex_grilo_program_set_grilo_media (program, media);
          }
          break;

        case GRL_CONTENT_ADDED:
          program = MEX_GRILO_PROGRAM (mex_feed_lookup (MEX_FEED (feed), id));
          if (program != NULL) {
            mex_grilo_program_set_grilo_media (program, media);
          } else {
            emit_media_added (feed, media);
          }
          break;

        case GRL_CONTENT_REMOVED:
          program = MEX_GRILO_PROGRAM (mex_feed_lookup (MEX_FEED (feed), id));
          if (program != NULL) {
            mex_model_remove_content (MEX_MODEL (feed), MEX_CONTENT (program));
          }
          break;
        }
    }
}

static void
update_source (MexGriloFeed *feed, GrlSource *new_source)
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
    gchar *lower;
    const gchar *source_name =
      grl_source_get_name (GRL_SOURCE (new_source));

    priv->source = g_object_ref (new_source);
    g_signal_connect (priv->source,
                      "content-changed",
                      G_CALLBACK (klass->content_updated),
                      feed);

    lower = g_ascii_strdown (source_name, -1);
    if (strstr (lower, "removable")) {
      g_object_set (feed, "icon-name", "icon-panelheader-usb", NULL);
    } else {
      g_object_set (feed, "icon-name", "icon-panelheader-computer", NULL);
    }
    g_free (lower);
  }
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
