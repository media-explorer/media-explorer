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

#include "mex-visibility-manager.h"

G_DEFINE_TYPE (MexVisibilityManager, mex_visibility_manager, G_TYPE_OBJECT)

#define VISIBILITY_MANAGER_PRIVATE(o)                                   \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MEX_TYPE_VISIBILITY_MANAGER,            \
                                MexVisibilityManagerPrivate))

enum
{
  PROP_0,

  PROP_MAX_VISIBLE_ITEMS,
  PROP_TIME_SLICE
};

struct _MexVisibilityManagerPrivate
{
  GHashTable *view_to_list; /* MexContentView -> GList(MexContentView) */
  GQueue     *queue;

  GHashTable *view_to_ops;
  GQueue     *ops;

  guint   source;
  GTimer *timer;

  guint max_visible_items;
  guint time_slice;
};

/**/

static void
mex_visibility_manager_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  MexVisibilityManagerPrivate *priv = VISIBILITY_MANAGER_PRIVATE (object);

  switch (property_id)
    {
    case PROP_MAX_VISIBLE_ITEMS:
      g_value_set_uint (value, priv->max_visible_items);
      break;

    case PROP_TIME_SLICE:
      g_value_set_uint (value, priv->time_slice);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_visibility_manager_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  MexVisibilityManagerPrivate *priv = VISIBILITY_MANAGER_PRIVATE (object);

  switch (property_id)
    {
    case PROP_MAX_VISIBLE_ITEMS:
      priv->max_visible_items = g_value_get_uint (value);
      break;

    case PROP_TIME_SLICE:
      priv->time_slice = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_visibility_manager_dispose (GObject *object)
{
  MexVisibilityManagerPrivate *priv = VISIBILITY_MANAGER_PRIVATE (object);

  if (priv->timer)
    {
      g_timer_destroy (priv->timer);
      priv->timer = NULL;
    }

  if (priv->source)
    {
      g_source_remove (priv->source);
      priv->source = 0;
    }

  if (priv->view_to_ops)
    {
      g_hash_table_destroy (priv->view_to_ops);
      priv->view_to_ops = NULL;
    }

  if (priv->ops)
    {
      g_queue_free (priv->ops);
      priv->ops = NULL;
    }

  if (priv->view_to_list)
    {
      g_hash_table_destroy (priv->view_to_list);
      priv->view_to_list = NULL;
    }

  if (priv->queue)
    {
      g_queue_free (priv->queue);
      priv->queue = NULL;
    }

  G_OBJECT_CLASS (mex_visibility_manager_parent_class)->dispose (object);
}

static void
mex_visibility_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_visibility_manager_parent_class)->finalize (object);
}

static void
mex_visibility_manager_class_init (MexVisibilityManagerClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexVisibilityManagerPrivate));

  object_class->get_property = mex_visibility_manager_get_property;
  object_class->set_property = mex_visibility_manager_set_property;
  object_class->dispose = mex_visibility_manager_dispose;
  object_class->finalize = mex_visibility_manager_finalize;

  pspec = g_param_spec_uint ("max-visible-items",
                             "Max visible items",
                             "Amount of visible items.",
                             0, G_MAXUINT, 50,
                             G_PARAM_READWRITE |
                             G_PARAM_STATIC_STRINGS |
                             G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_MAX_VISIBLE_ITEMS, pspec);

  pspec = g_param_spec_uint ("time-slice",
                             "Time slice",
                             "The amount of time to spend performing "
                             "operations, per frame, in ms",
                             0, G_MAXUINT, 5,
                             G_PARAM_READWRITE |
                             G_PARAM_STATIC_STRINGS |
                             G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_TIME_SLICE, pspec);
}

static void
mex_visibility_manager_init (MexVisibilityManager *self)
{
  MexVisibilityManagerPrivate *priv = VISIBILITY_MANAGER_PRIVATE (self);

  self->priv = priv;

  priv->view_to_list = g_hash_table_new (g_direct_hash, g_direct_equal);
  priv->queue        = g_queue_new ();

  priv->view_to_ops = g_hash_table_new (g_direct_hash, g_direct_equal);
  priv->ops         = g_queue_new ();

  priv->timer = g_timer_new ();
}

MexVisibilityManager *
mex_visibility_manager_new (void)
{
  return g_object_new (MEX_TYPE_VISIBILITY_MANAGER, NULL);
}

static gboolean
mex_visibility_manager_process_operations (MexVisibilityManager *manager)
{
  MexVisibilityManagerPrivate *priv = manager->priv;

  g_timer_start (priv->timer);

  while (!g_queue_is_empty (priv->ops))
    {
      MexContentView *view = (MexContentView *) g_queue_pop_head (priv->ops);

      if (g_hash_table_lookup (priv->view_to_list, view))
        mex_content_view_set_visible (view, TRUE);
      else
        mex_content_view_set_visible (view, FALSE);
      g_hash_table_remove (priv->view_to_ops, view);

      if (g_timer_elapsed (priv->timer, NULL) * 1000 >= priv->time_slice)
        break;
    }

  g_timer_stop (priv->timer);

  if (!g_queue_is_empty (priv->ops))
    return TRUE;

  priv->source = 0;
  return FALSE;
}

void
mex_visibility_manager_push (MexVisibilityManager *manager,
                             MexContentView       *view)
{
  MexVisibilityManagerPrivate *priv;
  GList *item;
  MexContentView *ditem;
  gboolean schedule_ops = FALSE;

  g_return_if_fail (MEX_IS_VISIBILITY_MANAGER (manager));
  g_return_if_fail (MEX_IS_CONTENT_VIEW (view));

  priv = manager->priv;

  item = g_hash_table_lookup (priv->view_to_list, view);

  if (item)
    {
      g_queue_unlink (priv->queue, item);
      g_queue_push_head_link (priv->queue, item);
    }
  else
    {
      g_queue_push_head (priv->queue, view);
      item = g_queue_peek_head_link (priv->queue);
      g_hash_table_insert (priv->view_to_list, view, item);

      g_queue_push_head (priv->ops, view);
      item = g_queue_peek_head_link (priv->ops);
      g_hash_table_insert (priv->view_to_ops, view, item);

      schedule_ops = TRUE;
    }

  if (g_hash_table_size (priv->view_to_list) > priv->max_visible_items)
    {
      ditem = g_queue_pop_tail (priv->queue);
      g_hash_table_remove (priv->view_to_list, ditem);

      g_queue_push_tail (priv->ops, ditem);
      item = g_queue_peek_tail_link (priv->ops);
      g_hash_table_insert (priv->view_to_ops, ditem, item);

      schedule_ops = TRUE;
    }

  if (schedule_ops && !priv->source)
    priv->source =
      g_idle_add_full (G_PRIORITY_HIGH,
                       (GSourceFunc) mex_visibility_manager_process_operations,
                       manager,
                       NULL);
}

void
mex_visibility_manager_pop (MexVisibilityManager *manager,
                            MexContentView       *view)
{
  MexVisibilityManagerPrivate *priv;
  GList *item;

  g_return_if_fail (MEX_IS_VISIBILITY_MANAGER (manager));
  g_return_if_fail (MEX_IS_CONTENT_VIEW (view));

  priv = manager->priv;

  item = g_hash_table_lookup (priv->view_to_list, view);

  if (item)
    {
      g_hash_table_remove (priv->view_to_list, view);
      g_queue_delete_link (priv->queue, item);
    }

  item = g_hash_table_lookup (priv->view_to_ops, view);

  if (item)
    {
      g_hash_table_remove (priv->view_to_ops, view);
      g_queue_delete_link (priv->ops, item);
    }
}

void
mex_visibility_manager_set_max_visible_items (MexVisibilityManager *manager,
                                              guint                 max)
{
  MexVisibilityManagerPrivate *priv;

  g_return_if_fail (MEX_IS_VISIBILITY_MANAGER (manager));

  priv = manager->priv;

  priv->max_visible_items = max;
}

guint
mex_visibility_manager_get_max_visible_items (MexVisibilityManager *manager)
{
  MexVisibilityManagerPrivate *priv;

  g_return_val_if_fail (MEX_IS_VISIBILITY_MANAGER (manager), 0);

  priv = manager->priv;

  return priv->max_visible_items;
}

void
mex_visibility_manager_set_time_slice (MexVisibilityManager *manager,
                                       guint                 msec)
{
  MexVisibilityManagerPrivate *priv;

  g_return_if_fail (MEX_IS_VISIBILITY_MANAGER (manager));

  priv = manager->priv;

  priv->time_slice = msec;
}

guint
mex_visibility_manager_get_time_slice (MexVisibilityManager *manager)
{
  MexVisibilityManagerPrivate *priv;

  g_return_val_if_fail (MEX_IS_VISIBILITY_MANAGER (manager), 0);

  priv = manager->priv;

  return priv->time_slice;
}
