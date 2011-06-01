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

#ifndef _MEX_TRACKER_CACHE_H
#define _MEX_TRACKER_CACHE_H

#include <glib-object.h>

#include <mex-content.h>

G_BEGIN_DECLS

#define MEX_TYPE_TRACKER_CACHE mex_tracker_cache_get_type()

#define MEX_TRACKER_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_TRACKER_CACHE, MexTrackerCache))

#define MEX_TRACKER_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_TRACKER_CACHE, MexTrackerCacheClass))

#define MEX_IS_TRACKER_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_TRACKER_CACHE))

#define MEX_IS_TRACKER_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_TRACKER_CACHE))

#define MEX_TRACKER_CACHE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_TRACKER_CACHE, MexTrackerCacheClass))

typedef struct _MexTrackerCache MexTrackerCache;
typedef struct _MexTrackerCacheClass MexTrackerCacheClass;
typedef struct _MexTrackerCachePrivate MexTrackerCachePrivate;

struct _MexTrackerCache
{
  GObject parent;

  MexTrackerCachePrivate *priv;
};

struct _MexTrackerCacheClass
{
  GObjectClass parent_class;
};

GType mex_tracker_cache_get_type (void) G_GNUC_CONST;

MexTrackerCache *mex_tracker_cache_new (void);

MexContent *mex_tracker_cache_lookup (MexTrackerCache *cache, const gchar *urn);

void mex_tracker_cache_add (MexTrackerCache *cache, MexContent *content);

G_END_DECLS

#endif /* _MEX_TRACKER_CACHE_H */
