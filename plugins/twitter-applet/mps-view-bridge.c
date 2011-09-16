/*
 * Copyright (C) 2010 Intel Corporation.
 * Derived from meego-panel-status
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "mps-view-bridge.h"
#include <mx/mx.h>

G_DEFINE_TYPE (MpsViewBridge, mps_view_bridge, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPS_TYPE_VIEW_BRIDGE, MpsViewBridgePrivate))

typedef struct _MpsViewBridgePrivate MpsViewBridgePrivate;

struct _MpsViewBridgePrivate {
  SwClientItemView *view;
  ClutterContainer *container;

  GHashTable *item_uid_to_actor;
  ClutterScore *score;

  GList *actors_to_animate;
  GQueue *pending_items_queue;
  ClutterTimeline *current_timeline;

  MpsViewBridgeFactoryFunc func;
  gpointer userdata;

  gboolean active_transition;
  gboolean paused;

  guint delay_timeout_id;
};

enum
{
  PROP_0,
  PROP_VIEW,
  PROP_CONTAINER
};

#define THRESHOLD 5
#define MAX_LENGTH 20
#define CARD_WIDTH 268.0

static void
mps_view_bridge_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MpsViewBridge *bridge = (MpsViewBridge *)object;

  switch (property_id) {
    case PROP_VIEW:
      g_value_set_object (value, mps_view_bridge_get_view (bridge));
      break;
    case PROP_CONTAINER:
      g_value_set_object (value, mps_view_bridge_get_container (bridge));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_view_bridge_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  MpsViewBridge *bridge = (MpsViewBridge *)object;

  switch (property_id) {
    case PROP_VIEW:
      mps_view_bridge_set_view (bridge,
                                (SwClientItemView *)g_value_get_object (value));
      break;
    case PROP_CONTAINER:
      mps_view_bridge_set_container (bridge,
                                     (ClutterContainer *)g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_view_bridge_dispose (GObject *object)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (object);

  if (priv->score)
  {
    g_object_unref (priv->score);
    priv->score = NULL;
  }

  if (priv->item_uid_to_actor)
  {
    g_hash_table_unref (priv->item_uid_to_actor);
    priv->item_uid_to_actor = NULL;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  if (priv->delay_timeout_id)
  {
    g_source_remove (priv->delay_timeout_id);
  }

  G_OBJECT_CLASS (mps_view_bridge_parent_class)->dispose (object);
}

static void
mps_view_bridge_finalize (GObject *object)
{
  G_OBJECT_CLASS (mps_view_bridge_parent_class)->finalize (object);
}

static void
mps_view_bridge_class_init (MpsViewBridgeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpsViewBridgePrivate));

  object_class->get_property = mps_view_bridge_get_property;
  object_class->set_property = mps_view_bridge_set_property;
  object_class->dispose = mps_view_bridge_dispose;
  object_class->finalize = mps_view_bridge_finalize;
}

static void
mps_view_bridge_init (MpsViewBridge *self)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (self);

  priv->item_uid_to_actor = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   (GDestroyNotify)clutter_actor_destroy);

  priv->pending_items_queue = g_queue_new ();
}

MpsViewBridge *
mps_view_bridge_new (void)
{
  return g_object_new (MPS_TYPE_VIEW_BRIDGE, NULL);
}

static gint
_sw_item_sort_compare_func (SwItem *a,
                            SwItem *b)
{
  if (a->date.tv_sec < b->date.tv_sec)
  {
    return -1;
  } else if (a->date.tv_sec == b->date.tv_sec) {
    return 0;
  } else {
    return 1;
  }
}

#if 0
static void _do_next_card_animation (MpsViewBridge *bridge);

static void
_animation_completed_cb (ClutterAnimation *animation,
                         MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  priv->current_timeline = NULL;

  _do_next_card_animation (bridge);
}

static void
_do_next_card_animation (MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  ClutterActor *actor;
  ClutterAnimation *animation;
  ClutterTimeline *timeline;
  ClutterTimeline *opacity_timeline;
  ClutterAlpha *alpha;
  ClutterBehaviour *behave;

  if (!priv->actors_to_animate)
  {
    return;
  }

  /* Get current actor and update head of list */
  actor = (ClutterActor *)priv->actors_to_animate->data;
  priv->actors_to_animate = g_list_remove (priv->actors_to_animate,
                                           actor);

  animation = clutter_actor_animate (actor,
                                     CLUTTER_LINEAR,
                                     400,
                                     "width", CARD_WIDTH,
                                     NULL);
  timeline = clutter_animation_get_timeline (animation);
  clutter_timeline_stop (timeline);
  clutter_timeline_set_delay (timeline, 600);
  clutter_timeline_start (timeline);
  priv->current_timeline = timeline;

  opacity_timeline = clutter_timeline_new (150);
  clutter_timeline_set_delay (opacity_timeline, 850);
  alpha = clutter_alpha_new_full (opacity_timeline, CLUTTER_EASE_OUT_QUAD);
  g_object_unref (opacity_timeline);
  behave = clutter_behaviour_opacity_new (alpha,
                                          0,
                                          255);
  clutter_behaviour_apply (behave, actor);
  g_signal_connect_swapped (opacity_timeline,
                            "completed",
                            (GCallback)g_object_unref,
                            behave);
  clutter_timeline_start (opacity_timeline);

  g_signal_connect_after (animation,
                          "completed",
                          (GCallback)_animation_completed_cb,
                          bridge);
}

static void
_view_items_added_cb (SwClientItemView *view,
                      GList            *items,
                      MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  gint item_count = 0;
  GList *l;
  gint i = 0;
  static ClutterActor *bottom_actor = NULL;

  g_debug (G_STRLOC ": %s called", G_STRFUNC);

  /* Oldest first */
  items = g_list_sort (items,
                       (GCompareFunc)_sw_item_sort_compare_func);

  item_count = g_list_length (items);


  for (l = items; l; l = l->next)
  {
    SwItem *item = (SwItem *)l->data;
    ClutterActor *actor;

    if (priv->func)
    {
      actor = priv->func (bridge, item, priv->userdata);
    }

    clutter_actor_set_height (actor, CARD_HEIGHT);

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->container),
                                 actor);

    g_hash_table_insert (priv->item_uid_to_actor,
                         g_strdup (item->uuid),
                         actor);

    /* Position it at the top */
    clutter_container_lower_child (priv->container, actor, NULL);

    clutter_container_child_set (CLUTTER_CONTAINER (priv->container),
                                 actor,
                                 "x-fill", FALSE,
                                 "y-fill", FALSE,
                                 "expand", FALSE,
                                 NULL);

    if (i < item_count - THRESHOLD)
    {
      clutter_actor_set_width (actor, CARD_WIDTH);
    } else {
      clutter_actor_set_width (actor, 0);
      priv->actors_to_animate = g_list_append (priv->actors_to_animate,
                                               actor);
      clutter_actor_set_opacity (actor, 0);
    }

    if (!bottom_actor)
    {
      bottom_actor = actor;
      mx_stylable_set_style_class (MX_STYLABLE (actor), "mps-tweet-card-last");
    }

    i++;
  }

  /* Deal with the overflow on the pending items */
  while (g_list_length (priv->actors_to_animate) > THRESHOLD)
  {
    ClutterActor *actor = (ClutterActor *)priv->actors_to_animate->data;

    clutter_actor_set_height (actor, CARD_WIDTH);
    clutter_actor_set_opacity (actor, 255);

    priv->actors_to_animate = g_list_remove (priv->actors_to_animate, actor);
  }

  /* We have a started chain of animations */
  if (!priv->current_timeline)
    _do_next_card_animation (bridge);
}
#endif


static void _populate_with_actors (MpsViewBridge *bridge);


static gboolean
_transition_delay_cb (MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  priv->active_transition = FALSE;

  _populate_with_actors (bridge);

  return FALSE;
}

static void
_timeline_completed_cb (ClutterTimeline *timeline,
                        MpsViewBridge   *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  priv->delay_timeout_id = g_timeout_add_seconds (1,
                                                  (GSourceFunc)_transition_delay_cb,
                                                  bridge);
}

static void
_populate_with_actors (MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  ClutterActor *actor;
  SwItem *item;

  if (priv->paused)
  {
    return;
  }

  /* Put actors into the container straight away if we have more than the
   * threshold in the the queue */
  while (g_queue_get_length (priv->pending_items_queue) > THRESHOLD)
  {
    item = (SwItem *)g_queue_pop_tail (priv->pending_items_queue);

    if (priv->func)
    {
      actor = priv->func (bridge, item, priv->userdata);
    }

    clutter_actor_set_width (actor, CARD_WIDTH);

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->container),
                                 actor);

    g_hash_table_insert (priv->item_uid_to_actor,
                         g_strdup (item->uuid),
                         actor);

    /* Position it at the top */
    clutter_container_lower_child (priv->container, actor, NULL);

    clutter_container_child_set (CLUTTER_CONTAINER (priv->container),
                                 actor,
                                 "x-fill", FALSE,
                                 "y-fill", TRUE,
                                 "expand", TRUE,
                                 NULL);
  }

  /* First post-threshold item */
  item = g_queue_pop_tail (priv->pending_items_queue);

  if (item)
  {
    ClutterAnimator *a;

    priv->active_transition = TRUE;

    if (priv->func)
    {
      actor = priv->func (bridge, item, priv->userdata);
    }

    clutter_actor_set_width (actor, 0);
    clutter_actor_set_opacity (actor, 0);
    a = clutter_animator_new ();
    clutter_animator_set (a,
                          actor, "width",   CLUTTER_EASE_OUT_QUAD, 0.0, 0.0,
                          actor, "width",   CLUTTER_EASE_OUT_QUAD, 0.8, CARD_WIDTH,
                          actor, "opacity", CLUTTER_EASE_OUT_QUAD, 0.0, 0x00,
                          actor, "opacity", CLUTTER_EASE_OUT_QUAD, 0.8, 0x00,
                          actor, "opacity", CLUTTER_EASE_OUT_QUAD, 1.0, 0xff,
                          NULL);

    clutter_animator_set_duration (a, 600);
    clutter_animator_start (a);
    g_signal_connect_after (clutter_animator_get_timeline (a),
                           "completed",
                           (GCallback)_timeline_completed_cb,
                           bridge);


    clutter_container_add_actor (CLUTTER_CONTAINER (priv->container),
                                 actor);

    g_hash_table_insert (priv->item_uid_to_actor,
                         g_strdup (item->uuid),
                         actor);

    /* Position it at the top */
    clutter_container_lower_child (priv->container, actor, NULL);

    clutter_container_child_set (CLUTTER_CONTAINER (priv->container),
                                 actor,
                                 "x-fill", FALSE,
                                 "y-fill", TRUE,
                                 "expand", TRUE,
                                 NULL);
  }

  /* FIXME: I don't think this code is any good, its obviously inefficient */
  {
    GList *children, *l;
    gint count = 0;

    children = clutter_container_get_children (CLUTTER_CONTAINER (priv->container));
    l = children;

    /* Currently we expect that the children are in order; newest first */
    if (g_list_length (children) > MAX_LENGTH)
      {
        while (l)
          {
            if (count > MAX_LENGTH)
              {
                ClutterActor *actor;
                SwItem *item;

                actor = CLUTTER_ACTOR (l->data);

                g_object_get (actor,
                              "item", &item,
                              NULL);
                /* Calls _destroy which triggers unparent */
                g_hash_table_remove (priv->item_uid_to_actor,
                                     item->uuid);
              }

            l = l->next;
            count++;
          }
      }
    g_list_free (children);
  }
}

static void
_view_items_added_cb (SwClientItemView *view,
                      GList            *items,
                      MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  gint item_count = 0;
  GList *l;
  gint i = 0;

  /* Oldest first */
  items = g_list_sort (items,
                       (GCompareFunc)_sw_item_sort_compare_func);

  for (l = items; l; l = l->next)
  {
    SwItem *item = (SwItem *)l->data;

    g_queue_push_head (priv->pending_items_queue,
                       sw_item_ref (item));
  }

  if (!priv->active_transition)
    _populate_with_actors (bridge);
}

static void
_view_items_removed_cb (SwClientItemView *view,
                        GList            *items,
                        MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

}

static void
_view_items_changed_cb (SwClientItemView *view,
                        GList            *items,
                        MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  GList *l;

  for (l = items; l; l = l->next)
  {
    SwItem *item = (SwItem *)l->data;
    ClutterActor *actor;

    actor = g_hash_table_lookup (priv->item_uid_to_actor,
                                 item->uuid);

    if (actor)
    {
      g_object_set (actor,
                    "item", item,
                    NULL);
    }
  }
}


void
mps_view_bridge_set_view (MpsViewBridge    *bridge,
                          SwClientItemView *view)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  if (view != priv->view && priv->view != NULL)
  {
    g_signal_handlers_disconnect_by_func (priv->view,
                                          _view_items_added_cb,
                                          bridge);
    g_signal_handlers_disconnect_by_func (priv->view,
                                          _view_items_removed_cb,
                                          bridge);
    g_signal_handlers_disconnect_by_func (priv->view,
                                          _view_items_changed_cb,
                                          bridge);
    g_object_unref (priv->view);
    priv->view = NULL;

    g_hash_table_remove_all (priv->item_uid_to_actor);
    clutter_container_foreach (priv->container,
                               (ClutterCallback)clutter_actor_unparent,
                               NULL);
    g_queue_clear (priv->pending_items_queue);
  }

  /* Can only be called once */
  priv->view = g_object_ref (view);

  g_signal_connect (priv->view,
                    "items-added",
                    (GCallback)_view_items_added_cb,
                    bridge);
  g_signal_connect (priv->view,
                    "items-removed",
                    (GCallback)_view_items_removed_cb,
                    bridge);
  g_signal_connect (priv->view,
                    "items-changed",
                    (GCallback)_view_items_changed_cb,
                    bridge);
}

void
mps_view_bridge_set_factory_func (MpsViewBridge            *bridge,
                                  MpsViewBridgeFactoryFunc  func,
                                  gpointer                  userdata)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  priv->func = func;
  priv->userdata = userdata;
}

void
mps_view_bridge_set_container (MpsViewBridge    *bridge,
                               ClutterContainer *container)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  /* Can only be called once */
  g_assert (!priv->container);
  priv->container = g_object_ref (container);
}

SwClientItemView *
mps_view_bridge_get_view (MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  return priv->view;
}

ClutterContainer *
mps_view_bridge_get_container (MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  return priv->container;
}


void
mps_view_bridge_set_paused (MpsViewBridge *bridge,
                            gboolean       paused)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  if (priv->paused != paused)
  {
    /* If we're unpaused and have no active transition then attempt population
     * of the container
     */
    if (priv->paused && !paused && !priv->active_transition)
    {
      priv->paused = paused;
      _populate_with_actors (bridge);
    }

    /* If there is an active transition then the animation completed will fall
     * through and call into _populate_with_actors
     */

    priv->paused = paused;
  }
}
