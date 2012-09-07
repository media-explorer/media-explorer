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

#ifndef __MEX_GRILO_TRACKER_FEED_H__
#define __MEX_GRILO_TRACKER_FEED_H__

#include <glib-object.h>

#include <mex/mex-grilo-feed.h>

G_BEGIN_DECLS

#define MEX_TYPE_GRILO_TRACKER_FEED             \
  (mex_grilo_tracker_feed_get_type())
#define MEX_GRILO_TRACKER_FEED(obj)                             \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               MEX_TYPE_GRILO_TRACKER_FEED,     \
                               MexGriloTrackerFeed))
#define MEX_GRILO_TRACKER_FEED_CLASS(klass)                     \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                            \
                            MEX_TYPE_GRILO_TRACKER_FEED,        \
                            MexGriloTrackerFeedClass))
#define MEX_IS_GRILO_TRACKER_FEED(obj)                          \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               MEX_TYPE_GRILO_TRACKER_FEED))
#define MEX_IS_GRILO_TRACKER_FEED_CLASS(klass)                  \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                            \
                            MEX_TYPE_GRILO_TRACKER_FEED))
#define MEX_GRILO_TRACKER_FEED_GET_CLASS(obj)                   \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              MEX_TYPE_GRILO_TRACKER_FEED,      \
                              MexGriloTrackerFeedClass))

typedef struct _MexGriloTrackerFeedPrivate MexGriloTrackerFeedPrivate;
typedef struct _MexGriloTrackerFeed        MexGriloTrackerFeed;
typedef struct _MexGriloTrackerFeedClass   MexGriloTrackerFeedClass;

struct _MexGriloTrackerFeed
{
  MexGriloFeed parent;

  MexGriloTrackerFeedPrivate *priv;
};

struct _MexGriloTrackerFeedClass
{
  MexGriloFeedClass parent_class;
};

GType mex_grilo_tracker_feed_get_type (void) G_GNUC_CONST;

MexFeed *mex_grilo_tracker_feed_new (GrlSource   *source,
                                     const GList *query_keys,
                                     const GList *metadata_keys,
                                     const gchar *filter,
                                     GrlMedia    *root);

G_END_DECLS

#endif /* __MEX_GRILO_TRACKER_FEED_H__ */
