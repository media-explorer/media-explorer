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

#include "mex-tracker-cache.h"

G_DEFINE_TYPE (MexTrackerCache, mex_tracker_cache, G_TYPE_OBJECT)

#define TRACKER_CACHE_PRIVATE(o)                                        \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MEX_TYPE_TRACKER_CACHE,                 \
                                MexTrackerCachePrivate))

typedef struct
{
  MexContent *content;
  gchar      *urn;
} MexTrackerCacheItem;

struct _MexTrackerCachePrivate
{
  GHashTable *urn_to_item;      /* urn -> MexTrackerCacheItem */
  GHashTable *content_to_item;  /* content -> MexTrackerCacheItem */
};


static void
mex_tracker_cache_get_property (GObject    *object,
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
mex_tracker_cache_set_property (GObject      *object,
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
mex_tracker_cache_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_cache_parent_class)->dispose (object);
}

static void
mex_tracker_cache_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_cache_parent_class)->finalize (object);
}

static void
mex_tracker_cache_class_init (MexTrackerCacheClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexTrackerCachePrivate));

  object_class->get_property = mex_tracker_cache_get_property;
  object_class->set_property = mex_tracker_cache_set_property;
  object_class->dispose = mex_tracker_cache_dispose;
  object_class->finalize = mex_tracker_cache_finalize;
}

static void
mex_tracker_cache_init (MexTrackerCache *self)
{
  MexTrackerCachePrivate *priv;

  priv = TRACKER_CACHE_PRIVATE (self);
  self->priv = priv;

  priv->urn_to_item     = g_hash_table_new (g_str_hash, g_str_equal);
  priv->content_to_item = g_hash_table_new (g_direct_hash, g_direct_equal);
}

MexTrackerCache *
mex_tracker_cache_new (void)
{
  return g_object_new (MEX_TYPE_TRACKER_CACHE, NULL);
}

MexContent *
mex_tracker_cache_lookup (MexTrackerCache *cache, const gchar *urn)
{
  MexTrackerCachePrivate *priv;
  MexTrackerCacheItem *item;

  g_return_val_if_fail (cache != NULL, NULL);
  g_return_val_if_fail (urn != NULL, NULL);

  priv = cache->priv;

  item = g_hash_table_lookup (priv->urn_to_item, urn);
  if (item)
    return item->content;

  return NULL;
}

static void
mex_tracker_cache_weak_notify (MexTrackerCache *cache,
                               GObject *freed_object)
{
  MexTrackerCachePrivate *priv = cache->priv;
  MexTrackerCacheItem *item;

  item = g_hash_table_lookup (priv->content_to_item, freed_object);

  g_hash_table_remove (priv->content_to_item, freed_object);
  g_hash_table_remove (priv->urn_to_item, item->urn);

  g_slice_free (MexTrackerCacheItem, item);
}

void
mex_tracker_cache_add (MexTrackerCache *cache, MexContent *content)
{
  MexTrackerCachePrivate *priv;
  MexTrackerCacheItem *item;

  g_return_if_fail (cache != NULL);
  g_return_if_fail (content != NULL);

  priv = cache->priv;

  item = g_slice_new0 (MexTrackerCacheItem);

  item->content = content;
  item->urn =
    g_strdup (mex_content_get_metadata (content,
                                        MEX_CONTENT_METADATA_PRIVATE_ID));

  g_object_weak_ref (G_OBJECT (content),
                     (GWeakNotify) mex_tracker_cache_weak_notify,
                     cache);

  g_hash_table_insert (priv->urn_to_item, item->urn, item);
  g_hash_table_insert (priv->content_to_item, content, item);
}
