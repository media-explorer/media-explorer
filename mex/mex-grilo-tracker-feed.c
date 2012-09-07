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

#include "mex-grilo-tracker-feed.h"
#include "mex-grilo-program.h"

enum {
  PROP_0,
  PROP_FILTER
};

struct _MexGriloTrackerFeedPrivate {
  GrlMedia  *root;
  GrlSource *source;
  gchar     *filter;
  GList     *keys;
};

#define BROWSE_FLAGS (GRL_RESOLVE_IDLE_RELAY | GRL_RESOLVE_FULL)

#define GET_PRIVATE(obj)                                                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj),                                  \
                                MEX_TYPE_GRILO_TRACKER_FEED,            \
                                MexGriloTrackerFeedPrivate))

G_DEFINE_TYPE (MexGriloTrackerFeed, mex_grilo_tracker_feed, MEX_TYPE_GRILO_FEED)

static guint _mex_grilo_tracker_feed_browse (MexGriloFeed      *feed,
                                             int                offset,
                                             int                limit,
                                             GrlSourceResultCb  callback);
static guint _mex_grilo_tracker_feed_query (MexGriloFeed      *feed,
                                            const char        *query,
                                            int                offset,
                                            int                limit,
                                            GrlSourceResultCb  callback);
static guint _mex_grilo_tracker_feed_search (MexGriloFeed     *feed,
                                             const char        *search_text,
                                             int                offset,
                                             int                limit,
                                             GrlSourceResultCb  callback);
static void _mex_grilo_tracker_feed_content_updated (GrlSource           *source,
                                                     GPtrArray           *changed_medias,
                                                     GrlSourceChangeType  change_type,
                                                     gboolean             known_location,
                                                     MexGriloFeed        *feed);

static void
mex_grilo_tracker_feed_finalize (GObject *object)
{
  MexGriloTrackerFeed *self = MEX_GRILO_TRACKER_FEED (object);
  MexGriloTrackerFeedPrivate *priv = self->priv;

  if (priv->filter) {
    g_free (priv->filter);
    priv->filter = NULL;
  }

  if (priv->keys)
    {
      g_list_free (priv->keys);
      priv->keys = NULL;
    }

  G_OBJECT_CLASS (mex_grilo_tracker_feed_parent_class)->finalize (object);
}

static void
mex_grilo_tracker_feed_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_grilo_tracker_feed_parent_class)->dispose (object);
}

static void
mex_grilo_tracker_feed_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  MexGriloTrackerFeed *self = MEX_GRILO_TRACKER_FEED (object);
  MexGriloTrackerFeedPrivate *priv = self->priv;

  switch (prop_id) {
  case PROP_FILTER:
    if (priv->filter)
      g_free (priv->filter);
    priv->filter = g_value_dup_string (value);
    break;

  default:
    break;
  }
}

static void
mex_grilo_tracker_feed_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  MexGriloTrackerFeed *self = MEX_GRILO_TRACKER_FEED (object);
  MexGriloTrackerFeedPrivate *priv = self->priv;

  switch (prop_id) {
  case PROP_FILTER:
    g_value_set_string (value, priv->filter);
    break;

  default:
    break;
  }
}

static void
mex_grilo_tracker_feed_constructed (GObject *object)
{
  MexGriloTrackerFeed *self = MEX_GRILO_TRACKER_FEED (object);
  MexGriloTrackerFeedPrivate *priv = self->priv;

  if (G_OBJECT_CLASS (mex_grilo_tracker_feed_parent_class)->constructed)
    G_OBJECT_CLASS (mex_grilo_tracker_feed_parent_class)->constructed (object);

  g_object_get (object,
                "grilo-box", &priv->root,
                "grilo-source", &priv->source,
                "grilo-query-keys", &priv->keys,
                NULL);
}

static void
mex_grilo_tracker_feed_class_init (MexGriloTrackerFeedClass *klass)
{
  GObjectClass *o_class = (GObjectClass *) klass;
  MexGriloFeedClass *f_class = (MexGriloFeedClass *)klass;
  GParamSpec *pspec;

  o_class->dispose = mex_grilo_tracker_feed_dispose;
  o_class->finalize = mex_grilo_tracker_feed_finalize;
  o_class->set_property = mex_grilo_tracker_feed_set_property;
  o_class->get_property = mex_grilo_tracker_feed_get_property;
  o_class->constructed = mex_grilo_tracker_feed_constructed;

  f_class->browse = _mex_grilo_tracker_feed_browse;
  f_class->query = _mex_grilo_tracker_feed_query;
  f_class->search = _mex_grilo_tracker_feed_search;
  f_class->content_updated = _mex_grilo_tracker_feed_content_updated;

  pspec = g_param_spec_string ("tracker-filter", "Tracker filter",
                               "Tracker filter to apply on contents "
                               "(SparQL format).",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_FILTER, pspec);

  g_type_class_add_private (klass, sizeof (MexGriloTrackerFeedPrivate));
}

static void
mex_grilo_tracker_feed_init (MexGriloTrackerFeed *self)
{
  MexGriloTrackerFeedPrivate *priv;

  self->priv = priv = GET_PRIVATE (self);
}

MexFeed *
mex_grilo_tracker_feed_new (GrlSource   *source,
                            const GList *query_keys,
                            const GList *metadata_keys,
                            const gchar *filter,
                            GrlMedia    *root)
{
  return g_object_new (MEX_TYPE_GRILO_TRACKER_FEED,
                       "grilo-source", source,
                       "grilo-box", root,
                       "grilo-query-keys", query_keys,
                       "grilo-metadata-keys", metadata_keys,
                       "tracker-filter", filter,
                       NULL);
}

static void
item_cb (GrlSource    *source,
         guint         id,
         GrlMedia     *media,
         guint         remaining,
         gpointer      userdata,
         const GError *error)
{
  MexGriloTrackerFeed *feed = (MexGriloTrackerFeed *) userdata;
  MexGriloTrackerFeedPrivate *priv = feed->priv;
  MexProgram *program;

  if (error) {
    g_warning ("Error browsing: %s", error->message);
    return;
  }

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

    program = mex_feed_lookup (MEX_FEED (feed), grl_media_get_id (media));
    if (program != NULL) {
      mex_grilo_program_set_grilo_media (MEX_GRILO_PROGRAM (program), media);
    } else {
      program = mex_grilo_program_new (MEX_GRILO_FEED (feed), media);
      mex_model_add_content (MEX_MODEL (feed), (MexContent *) program);
    }
    g_object_unref (media);
  }
}

static gchar *
get_filter_from_operation (MexGriloTrackerFeed *feed,
                           const gchar *input_text,
                           MexGriloOperationType operation)
{
  MexGriloTrackerFeedPrivate *priv = feed->priv;
  gchar *text = NULL;

  switch (operation) {
  case MEX_GRILO_FEED_OPERATION_NONE:
    break;

  case MEX_GRILO_FEED_OPERATION_BROWSE:
    if (!priv->root) {
      if (priv->filter)
        text = g_strdup_printf ("{ ?urn a nfo:Folder } UNION "
                                "{ %s } . "
                                "FILTER(!bound(nfo:belongsToContainer(?urn)))",
                                priv->filter);
      else
        text = g_strdup_printf ("{ ?urn a nfo:Folder } UNION "
                                "{ ?urn a nfo:Media } UNION "
                                "{ ?urn a nfo:Document } . "
                                "FILTER(!bound(nfo:belongsToContainer(?urn)))");

    } else {
      if (priv->filter)
        text = g_strdup_printf ("{ ?urn a nfo:Folder } UNION "
                                "{ %s } . "
                                "FILTER(tracker:id(nfo:belongsToContainer(?urn)) = %s)",
                                priv->filter,
                                grl_media_get_id (priv->root));
      else
        text = g_strdup_printf ("{ ?urn a nfo:Folder } UNION "
                                "{ ?urn a nfo:Media } UNION "
                                "{ ?urn a nfo:Document } . "
                                "FILTER(tracker:id(nfo:belongsToContainer(?urn)) = %s)",
                                grl_media_get_id (priv->root));

    }
    break;

  case MEX_GRILO_FEED_OPERATION_QUERY:
    if (priv->filter)
      text = g_strdup_printf ("%s . %s", input_text, priv->filter);
    else
      text = g_strdup (input_text);
    break;

  case MEX_GRILO_FEED_OPERATION_SEARCH:
    if (priv->filter)
      text = g_strdup_printf ("?urn a nfo:Media . "
                              "?urn tracker:available true . "
                              "?urn fts:match '*%s*' . "
                              "%s",
                              input_text, priv->filter);
    else
      text = g_strdup_printf ("?urn a nfo:Media . "
                              "?urn tracker:available true . "
                              "?urn fts:match '*%s*'",
                              input_text);
    break;
  }

  return text;
}

static void
filter_media (MexGriloTrackerFeed *feed, GrlMedia *media)
{
  MexGriloTrackerFeedPrivate *priv = feed->priv;
  const MexGriloOperation *op;
  gchar *query_text = NULL, *query_final = NULL;
  const gchar *str_id = grl_media_get_id (media);
  GrlOperationOptions *options;

  if (!str_id) {
    g_warning ("Cannot filter media without id");
    return;
  }

  op = mex_grilo_feed_get_operation (MEX_GRILO_FEED (feed));

  if (op->type == MEX_GRILO_FEED_OPERATION_NONE)
    return;

  query_text = get_filter_from_operation (feed, op->text, op->type);
  query_final = g_strdup_printf ("%s . FILTER(tracker:id(?urn) = %s)",
                                 query_text, str_id);

  options = grl_operation_options_new (NULL);
  grl_operation_options_set_flags (options, BROWSE_FLAGS);
  grl_operation_options_set_skip (options, 0);
  grl_operation_options_set_count (options, 1);

  grl_source_query (priv->source, query_final,
                          priv->keys,
                          options,
                          item_cb, feed);

	g_object_unref (options);
  g_free (query_final);
  g_free (query_text);
}

static guint
_mex_grilo_tracker_feed_browse (MexGriloFeed           *feed,
                                int                     offset,
                                int                     limit,
                                GrlSourceResultCb  callback)
{
  MexGriloTrackerFeed *self = MEX_GRILO_TRACKER_FEED (feed);
  MexGriloTrackerFeedPrivate *priv = self->priv;
  GrlOperationOptions *options;
  gchar *text;
  guint id;

  text = get_filter_from_operation (MEX_GRILO_TRACKER_FEED (feed),
                                    NULL,
                                    MEX_GRILO_FEED_OPERATION_BROWSE);

  options = grl_operation_options_new (NULL);
  grl_operation_options_set_flags (options, BROWSE_FLAGS);
  grl_operation_options_set_skip (options, offset);
  grl_operation_options_set_count (options, limit);

  id = grl_source_query (priv->source, text, priv->keys,
  			                 options,
                         callback, feed);

	g_object_unref (options);
  g_free (text);

  return id;
}

static guint
_mex_grilo_tracker_feed_query (MexGriloFeed           *feed,
                               const char             *query,
                               int                     offset,
                               int                     limit,
                               GrlSourceResultCb  callback)
{
  MexGriloTrackerFeed *self = MEX_GRILO_TRACKER_FEED (feed);
  MexGriloTrackerFeedPrivate *priv = self->priv;
  GrlOperationOptions *options;
  gchar *text;
  guint id;

  text = get_filter_from_operation (MEX_GRILO_TRACKER_FEED (feed),
                                    query,
                                    MEX_GRILO_FEED_OPERATION_QUERY);

  options = grl_operation_options_new (NULL);
  grl_operation_options_set_flags (options, BROWSE_FLAGS);
  grl_operation_options_set_skip (options, offset);
  grl_operation_options_set_count (options, limit);

  id = grl_source_query (priv->source, text,  priv->keys,
  			                 options,
                         callback, feed);

	g_object_unref (options);
  g_free (text);

  return id;
}

static guint
_mex_grilo_tracker_feed_search (MexGriloFeed           *feed,
                                const char             *search_text,
                                int                     offset,
                                int                     limit,
                                GrlSourceResultCb  callback)
{
  MexGriloTrackerFeed *self = MEX_GRILO_TRACKER_FEED (feed);
  MexGriloTrackerFeedPrivate *priv = self->priv;
  GrlOperationOptions *options;
  gchar *text;
  guint id;

  text = get_filter_from_operation (MEX_GRILO_TRACKER_FEED (feed),
                                    search_text,
                                    MEX_GRILO_FEED_OPERATION_SEARCH);

  options = grl_operation_options_new (NULL);
  grl_operation_options_set_flags (options, BROWSE_FLAGS);
  grl_operation_options_set_skip (options, offset);
  grl_operation_options_set_count (options, limit);

  id = grl_source_query (priv->source, text, priv->keys,
  			                 options,
                         callback, feed);

	g_object_unref (options);
  g_free (text);

  return id;
}

static void
_mex_grilo_tracker_feed_content_updated (GrlSource           *source,
                                         GPtrArray           *changed_medias,
                                         GrlSourceChangeType  change_type,
                                         gboolean             known_location,
                                         MexGriloFeed        *feed)
{
  gint i;
  const gchar *id;
  GrlMedia *media;
  MexProgram *program;

  for (i = 0 ; i < changed_medias->len ; i++)
    {
      media =  g_ptr_array_index (changed_medias, i);
      id = grl_media_get_id (media);

      switch (change_type)
        {
        case GRL_CONTENT_CHANGED:
          program = mex_feed_lookup (MEX_FEED (feed), id);
          /* The policy might be slightly different here... */
          if (program != NULL) {
            mex_grilo_program_set_grilo_media (MEX_GRILO_PROGRAM (program),
                                               media);
          }
          break;

        case GRL_CONTENT_ADDED:
          program = mex_feed_lookup (MEX_FEED (feed), id);
          if (program != NULL) {
            mex_grilo_program_set_grilo_media (MEX_GRILO_PROGRAM (program),
                                               media);
          } else {
            filter_media (MEX_GRILO_TRACKER_FEED (feed), media);
          }
          break;

        case GRL_CONTENT_REMOVED:
          program = mex_feed_lookup (MEX_FEED (feed), id);
          if (program != NULL) {
            mex_model_remove_content (MEX_MODEL (feed), MEX_CONTENT (program));
          }
          break;
        }
    }
}
