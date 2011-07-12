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

#ifndef __MEX_FEED_H__
#define __MEX_FEED_H__

#include <glib-object.h>

#include <mex/mex-generic-model.h>
#include <mex/mex-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_FEED                           \
  (mex_feed_get_type())
#define MEX_FEED(obj)                           \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),           \
                               MEX_TYPE_FEED,   \
                               MexFeed))
#define MEX_FEED_CLASS(klass)                   \
  (G_TYPE_CHECK_CLASS_CAST ((klass),            \
                            MEX_TYPE_FEED,      \
                            MexFeedClass))
#define MEX_IS_FEED(obj)                        \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),           \
                               MEX_TYPE_FEED))
#define MEX_IS_FEED_CLASS(klass)                \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),            \
                            MEX_TYPE_FEED))
#define MEX_FEED_GET_CLASS(obj)                 \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),            \
                              MEX_TYPE_FEED,    \
                              MexFeedClass))

typedef enum
{
  MEX_FEED_SEARCH_MODE_OR,
  MEX_FEED_SEARCH_MODE_AND
} MexFeedSearchMode;

typedef struct _MexFeedPrivate MexFeedPrivate;
typedef struct _MexFeed      MexFeed;
typedef struct _MexFeedClass MexFeedClass;

/* This is defined here to avoid an include dependency loop */
typedef struct _MexProgram      MexProgram;

struct _MexFeed
{
  MexGenericModel parent;

  MexFeedPrivate *priv;
};

struct _MexFeedClass
{
  MexGenericModelClass parent_class;

  void (*refresh) (MexFeed *feed);
};

GType mex_feed_get_type (void) G_GNUC_CONST;
MexFeed *mex_feed_new (const char *title,
                       const char *source);

void mex_feed_search (MexFeed            *feed,
                      const char        **search,
                      MexFeedSearchMode   mode,
                      MexModel           *results_model);

MexProgram *mex_feed_lookup (MexFeed    *feed,
                             const char *id);

guint mex_feed_get_default_nb_results (MexFeed *feed);

G_END_DECLS

#endif /* __MEX_FEED_H__ */
