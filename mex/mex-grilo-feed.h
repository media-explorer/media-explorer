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

#ifndef __MEX_GRILO_FEED_H__
#define __MEX_GRILO_FEED_H__

#include <glib-object.h>

#include <grilo.h>

#include <mex/mex-aggregate-model.h>
#include <mex/mex-feed.h>

G_BEGIN_DECLS

#define MEX_TYPE_GRILO_FEED                     \
  (mex_grilo_feed_get_type())
#define MEX_GRILO_FEED(obj)                             \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               MEX_TYPE_GRILO_FEED,     \
                               MexGriloFeed))
#define MEX_GRILO_FEED_CLASS(klass)                     \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                    \
                            MEX_TYPE_GRILO_FEED,        \
                            MexGriloFeedClass))
#define MEX_IS_GRILO_FEED(obj)                          \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MEX_TYPE_GRILO_FEED))
#define MEX_IS_GRILO_FEED_CLASS(klass)                  \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                    \
                            MEX_TYPE_GRILO_FEED))
#define MEX_GRILO_FEED_GET_CLASS(obj)                   \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              MEX_TYPE_GRILO_FEED,      \
                              MexGriloFeedClass))

typedef struct _MexGriloFeedPrivate MexGriloFeedPrivate;
typedef struct _MexGriloFeed        MexGriloFeed;
typedef struct _MexGriloFeedClass   MexGriloFeedClass;

#include <mex/mex-grilo-program.h>

typedef enum {
  MEX_GRILO_FEED_OPERATION_NONE,
  MEX_GRILO_FEED_OPERATION_BROWSE,
  MEX_GRILO_FEED_OPERATION_QUERY,
  MEX_GRILO_FEED_OPERATION_SEARCH
} MexGriloOperationType;

typedef void (*MexGriloFeedOpenCb) (MexGriloProgram *content,
                                    MexGriloFeed    *feed);

typedef struct {
  MexGriloOperationType type;

  char    *text;
  guint32  limit;
  guint32  count;
  guint32  offset;

  guint32 op_id;
} MexGriloOperation;

struct _MexGriloFeed
{
  MexFeed parent;

  MexGriloFeedPrivate *priv;
};

struct _MexGriloFeedClass
{
  MexFeedClass parent_class;

  guint (*browse) (MexGriloFeed           *feed,
                   int                     offset,
                   int                     limit,
                   GrlMediaSourceResultCb  callback);
  guint (*query) (MexGriloFeed            *feed,
                  const char              *query,
                  int                      offset,
                  int                      limit,
                  GrlMediaSourceResultCb   callback);
  guint (*search) (MexGriloFeed           *feed,
                   const char             *search_text,
                   int                     offset,
                   int                     limit,
                   GrlMediaSourceResultCb  callback);

  void (*content_updated) (GrlMediaSource           *source,
                           GPtrArray                *changed_medias,
                           GrlMediaSourceChangeType  change_type,
                           gboolean                  known_location,
                           MexGriloFeed             *feed);
};

GType mex_grilo_feed_get_type (void) G_GNUC_CONST;

MexFeed *mex_grilo_feed_new (GrlMediaSource *source,
                             const GList    *query_keys,
                             const GList    *metadata_keys,
                             GrlMedia       *root);

void mex_grilo_feed_browse (MexGriloFeed *feed,
                            int            offset,
                            int            limit);

void mex_grilo_feed_search (MexGriloFeed   *feed,
                            const char     *search_text,
                            int             offset,
                            int             limit);

void mex_grilo_feed_query (MexGriloFeed   *feed,
                           const char     *query,
                           int             offset,
                           int             limit);

const MexGriloOperation *mex_grilo_feed_get_operation (MexGriloFeed *feed);

gboolean mex_grilo_feed_get_completed (MexGriloFeed *feed);

void mex_grilo_feed_set_open_callback (MexGriloFeed       *feed,
                                       MexGriloFeedOpenCb  callback);

void mex_grilo_feed_open (MexGriloFeed    *feed,
                          MexGriloProgram *program);

G_END_DECLS

#endif /* __MEX_GRILO_FEED_H__ */
