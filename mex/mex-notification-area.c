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

/* mex-notification-area.c */

#include "mex-notification-area.h"
#include "mex-dummy-notification-source.h"
#include "mex-gio-notification-source.h"
#include "mex-network-notification-source.h"

G_DEFINE_TYPE (MexNotificationArea, mex_notification_area, MX_TYPE_STACK)

#define NOTIFICATION_AREA_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_NOTIFICATION_AREA, MexNotificationAreaPrivate))

#define GET_PRIVATE(o) MEX_NOTIFICATION_AREA(o)->priv

struct _MexNotificationAreaPrivate
{
  MexNotificationSource **sources;
  GHashTable *notification_to_actor, *notification_to_timeout_id;

  GQueue *stack;
};

static void
mex_notification_area_get_property (GObject    *object,
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
mex_notification_area_set_property (GObject      *object,
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
mex_notification_area_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_notification_area_parent_class)->dispose (object);
}

static void
mex_notification_area_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_notification_area_parent_class)->finalize (object);
}

static void
mex_notification_area_class_init (MexNotificationAreaClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexNotificationAreaPrivate));

  object_class->get_property = mex_notification_area_get_property;
  object_class->set_property = mex_notification_area_set_property;
  object_class->dispose = mex_notification_area_dispose;
  object_class->finalize = mex_notification_area_finalize;
}

static ClutterActor *
_make_notification_actor (MexNotification *notification)
{
  ClutterActor *box;
  ClutterActor *label, *icon;

  box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box),
                                 MX_ORIENTATION_HORIZONTAL);

  if (notification->icon)
    {
      icon = mx_icon_new ();
      clutter_actor_set_size (icon, 26, 26);
      mx_icon_set_icon_name (MX_ICON (icon), notification->icon);
      clutter_container_add_actor (CLUTTER_CONTAINER (box), icon);
    }

  label = mx_label_new_with_text (notification->message);

  mx_label_set_y_align (MX_LABEL (label), MX_ALIGN_MIDDLE);

  clutter_container_add_actor (CLUTTER_CONTAINER (box), label);

  return box;
}

static void
_animation_completed_cb (ClutterAnimation *animation,
                         ClutterActor     *actor)
{
  clutter_actor_destroy (actor);
}

static void
_expire_notification (MexNotificationArea *area,
                      MexNotification     *notification,
                      ClutterActor        *actor)
{
  MexNotificationAreaPrivate *priv = GET_PRIVATE (area);
  ClutterAnimation *animation;
  ClutterActor *last_top_actor;

  g_hash_table_remove (priv->notification_to_timeout_id, notification);
  g_hash_table_remove (priv->notification_to_actor, notification);

  g_queue_remove_all (priv->stack, actor);

  animation = clutter_actor_animate (actor,
                                     CLUTTER_EASE_OUT_QUAD,
                                     350,
                                     "opacity", 0x00,
                                     NULL);

  g_signal_connect_after (animation,
                          "completed",
                          (GCallback)_animation_completed_cb,
                          actor);

  /* Check if there is already something else in the stack. If so fade that
   * back in ... */
  last_top_actor = g_queue_peek_head (priv->stack);

  if (last_top_actor)
    {
      ClutterTimeline *timeline;

      animation = clutter_actor_animate (last_top_actor,
                                         CLUTTER_EASE_OUT_QUAD,
                                         350,
                                         "opacity", 0xff,
                                         NULL);

      timeline = clutter_animation_get_timeline (animation);

      clutter_timeline_set_delay (timeline, 450);
    }
}

static gboolean
_notification_timeout_cb (ClutterActor *actor)
{
  MexNotificationArea *area;
  MexNotification *notification;

  area = (MexNotificationArea *)g_object_get_data (G_OBJECT (actor),
                                                   "notification-area");
  notification = (MexNotification *)g_object_get_data (G_OBJECT (actor),
                                                       "notification");

  _expire_notification (area, notification, actor);

  return FALSE;
}

static void
_source_notification_removed_cb (MexNotificationSource *source,
                                 MexNotification       *notification,
                                 MexNotificationArea   *area)
{
  MexNotificationAreaPrivate *priv = GET_PRIVATE (area);
  ClutterActor *actor;

  actor = g_hash_table_lookup (priv->notification_to_actor, notification);
  _expire_notification (area, notification, actor);
}

static void
_source_notification_added_cb (MexNotificationSource *source,
                               MexNotification       *notification,
                               MexNotificationArea   *area)
{
  MexNotificationAreaPrivate *priv = GET_PRIVATE (area);
  ClutterActor *actor;
  ClutterActor *last_top_actor;
  ClutterAnimation *animation;

  actor = _make_notification_actor (notification);

  g_hash_table_insert (priv->notification_to_actor,
                       notification,
                       actor);

  clutter_container_add_actor (CLUTTER_CONTAINER (area),
                               actor);
  mx_stack_child_set_x_fill (MX_STACK (area), actor, FALSE);
  mx_stack_child_set_y_fill (MX_STACK (area), actor, FALSE);
  mx_stack_child_set_x_align (MX_STACK (area), actor, MX_ALIGN_MIDDLE);
  mx_stack_child_set_y_align (MX_STACK (area), actor, MX_ALIGN_MIDDLE);

  /* Get the last notification since we want to fade that out */
  last_top_actor = g_queue_peek_head (priv->stack);

  g_queue_push_head (priv->stack, actor);

  clutter_container_raise_child (CLUTTER_CONTAINER (area),
                                 actor,
                                 last_top_actor);



  /* Fade out old notification */
  if (last_top_actor)
    {
      clutter_actor_animate (last_top_actor,
                             CLUTTER_EASE_OUT_QUAD,
                             350,
                             "opacity", 0x00,
                             NULL);
    }

  clutter_actor_set_opacity (actor, 0);
  animation = clutter_actor_animate (actor,
                                     CLUTTER_EASE_OUT_QUAD,
                                     350,
                                     "opacity", 0xff,
                                     NULL);

  /* Delay new notification fade in if we had an old one */
  if (last_top_actor)
    {
      ClutterTimeline *timeline;

      timeline = clutter_animation_get_timeline (animation);

      clutter_timeline_set_delay (timeline, 450);
    }

  g_object_set_data (G_OBJECT (actor),
                     "notification-area",
                     area);
  g_object_set_data (G_OBJECT (actor),
                     "notification",
                     notification);

  if (notification->duration > 0)
    {
      guint timeout_id = 
        g_timeout_add_seconds (notification->duration,
                               (GSourceFunc)_notification_timeout_cb,
                               actor);

      g_hash_table_insert (priv->notification_to_timeout_id,
                           notification,
                           GINT_TO_POINTER (timeout_id));
    }
}

static void
mex_notification_area_init (MexNotificationArea *self)
{
  MexNotificationAreaPrivate *priv;
  gint i;
  GType source_types[] = { MEX_TYPE_NETWORK_NOTIFICATION_SOURCE,
                           MEX_TYPE_GIO_NOTIFICATION_SOURCE };

  self->priv = NOTIFICATION_AREA_PRIVATE (self);
  priv = self->priv;

  /* notification pointer -> timeout_id */
  priv->notification_to_timeout_id = g_hash_table_new (NULL, NULL);

  /* notification pointer -> actor */
  priv->notification_to_actor = g_hash_table_new (NULL, NULL);

  priv->sources = g_new0 (MexNotificationSource *, G_N_ELEMENTS (source_types));

  for (i = 0; i < G_N_ELEMENTS (source_types); i++)
    {
      priv->sources[i] = g_object_new (source_types[i],
                                       NULL);
      g_signal_connect (priv->sources[i],
                        "notification-added",
                        (GCallback)_source_notification_added_cb,
                        self);

      g_signal_connect (priv->sources[i],
                        "notification-removed",
                        (GCallback)_source_notification_removed_cb,
                        self);
    }
  priv->stack = g_queue_new ();
}

ClutterActor *
mex_notification_area_new (void)
{
  return g_object_new (MEX_TYPE_NOTIFICATION_AREA, NULL);
}
