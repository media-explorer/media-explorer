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


#include "mex-scroll-view.h"
#include "mex-scroll-indicator.h"
#include "mex-resizing-hbox.h"
#include "mex-scrollable-container.h"

/* This widget is mostly derived from MxScrollView */

static void clutter_container_iface_init (ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexScrollView, mex_scroll_view,
                         MX_TYPE_KINETIC_SCROLL_VIEW,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init))

#define SCROLL_VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SCROLL_VIEW, MexScrollViewPrivate))

enum
{
  PROP_0,

  PROP_INDICATORS_HIDDEN,
  PROP_FOLLOW_RECURSE,
  PROP_SCROLL_DELAY,
  PROP_SCROLL_GRAVITY,
  PROP_INTERPOLATE
};

struct _MexScrollViewPrivate
{
  guint           indicators_hidden : 1;
  guint           hscroll_hidden    : 1;
  guint           vscroll_hidden    : 1;
  guint           follow_recurse    : 1;
  guint           interpolate       : 1;

  ClutterGravity  scroll_gravity;

  ClutterActor   *child;
  ClutterActor   *hscroll;
  ClutterActor   *vscroll;

  ClutterActor   *focus;
  gulong          keep_visible_id;
  guint           keep_visible_timeout;
  guint           scroll_delay;

  gdouble         htarget;
  gdouble         vtarget;

  guint           scroll_indicator_timeout;
};

static void
mex_scroll_view_focus_allocation_cb (ClutterActor           *focus,
                                     ClutterActorBox        *box,
                                     ClutterAllocationFlags  flags,
                                     MexScrollView          *self);
static void
mex_scroll_view_delay_focus_allocation_cb (ClutterActor           *focus,
                                           ClutterActorBox        *box,
                                           ClutterAllocationFlags  flags,
                                           MexScrollView          *self);

/* ClutterContainerIface implementation */

static void
mex_scroll_view_update_visibility (MexScrollView *self,
                                   ClutterActor  *scroll,
                                   gboolean      *hidden)
{
  MxAdjustment *adjustment;
  gdouble value, lower, upper, page_size;

  MexScrollViewPrivate *priv = self->priv;

  adjustment = mex_scroll_indicator_get_adjustment (
                 MEX_SCROLL_INDICATOR (scroll));

  if (adjustment)
    mx_adjustment_get_values (adjustment,
                              &value,
                              &lower,
                              &upper,
                              NULL,
                              NULL,
                              &page_size);

  if (!adjustment || (upper - lower <= page_size))
    {
      *hidden = TRUE;
      clutter_actor_animate (scroll, CLUTTER_EASE_OUT_QUAD, 100,
                             "opacity", 0x00,
                             NULL);
    }
  else
    {
      *hidden = FALSE;
      if (!priv->indicators_hidden)
        clutter_actor_animate (scroll, CLUTTER_EASE_OUT_QUAD, 100,
                               "opacity", 0xff,
                               NULL);
    }
}

static gboolean
hide_indicators_timeout (MexScrollView *self)
{
  MexScrollViewPrivate *priv = self->priv;

  if (!priv->hscroll_hidden)
    clutter_actor_animate (priv->hscroll, CLUTTER_EASE_OUT_QUAD, 100,
                           "opacity", 0x00, NULL);

  if (!priv->vscroll_hidden)
    clutter_actor_animate (priv->vscroll, CLUTTER_EASE_OUT_QUAD, 100,
                           "opacity", 0x00, NULL);


  return FALSE;
}

static void
mex_scroll_view_adjustment_changed (MexScrollView *self)
{
  MexScrollViewPrivate *priv = self->priv;

  if (priv->hscroll)
    {
      gboolean hidden;
      mex_scroll_view_update_visibility (self, priv->hscroll, &hidden);
      priv->hscroll_hidden = hidden;
    }

  if (priv->vscroll)
    {
      gboolean hidden;
      mex_scroll_view_update_visibility (self, priv->vscroll, &hidden);
      priv->vscroll_hidden = hidden;
    }

  /* Make sure the focused actor remains in view */
  if (priv->focus)
    {
      ClutterActorBox box;
      clutter_actor_get_allocation_box (priv->focus, &box);
      if (priv->scroll_delay)
        mex_scroll_view_delay_focus_allocation_cb (priv->focus, &box, 0, self);
      else
        mex_scroll_view_focus_allocation_cb (priv->focus, &box, 0, self);
    }


  if (priv->scroll_indicator_timeout)
    g_source_remove (priv->scroll_indicator_timeout);

  if (!priv->indicators_hidden)
    priv->scroll_indicator_timeout =
      g_timeout_add_seconds (1, (GSourceFunc) hide_indicators_timeout, self);
}

static void
mex_scroll_view_hadjust_notify_cb (ClutterActor  *child,
                                   GParamSpec    *pspec,
                                   MexScrollView *self)
{
  MexScrollViewPrivate *priv = self->priv;

  if (priv->hscroll)
    {
      MxAdjustment *adjustment;

      /* Disconnect from old adjustment */
      adjustment = mex_scroll_indicator_get_adjustment (MEX_SCROLL_INDICATOR (
                                                        priv->hscroll));
      if (adjustment)
        g_signal_handlers_disconnect_by_func (adjustment,
                                           mex_scroll_view_adjustment_changed,
                                           self);

      /* Set new adjustment */
      mx_scrollable_get_adjustments (MX_SCROLLABLE (child), &adjustment, NULL);
      mex_scroll_indicator_set_adjustment (MEX_SCROLL_INDICATOR (priv->hscroll),
                                           adjustment);

      /* Connect to new adjustment changing so we can hide/show the indicators
       * if necessary.
       */
      if (adjustment)
        g_signal_connect_swapped (adjustment, "changed",
                              G_CALLBACK (mex_scroll_view_adjustment_changed),
                              self);

      mex_scroll_view_adjustment_changed (self);
    }
}

static void
mex_scroll_view_vadjust_notify_cb (ClutterActor  *child,
                                   GParamSpec    *pspec,
                                   MexScrollView *self)
{
  MexScrollViewPrivate *priv = self->priv;

  if (priv->vscroll)
    {
      MxAdjustment *adjustment;

      /* Disconnect from old adjustment */
      adjustment = mex_scroll_indicator_get_adjustment (MEX_SCROLL_INDICATOR (
                                                        priv->vscroll));
      if (adjustment)
        g_signal_handlers_disconnect_by_func (adjustment,
                                           mex_scroll_view_adjustment_changed,
                                           self);

      /* Set new adjustment */
      mx_scrollable_get_adjustments (MX_SCROLLABLE (child), NULL, &adjustment);
      mex_scroll_indicator_set_adjustment (MEX_SCROLL_INDICATOR (priv->vscroll),
                                           adjustment);

      /* Connect to new adjustment changing so we can hide/show the indicators
       * if necessary.
       */
      if (adjustment)
        g_signal_connect_swapped (adjustment, "changed",
                              G_CALLBACK (mex_scroll_view_adjustment_changed),
                              self);

      mex_scroll_view_adjustment_changed (self);
    }
}

static void
mex_scroll_view_notify_child (MexScrollView *self)
{
  ClutterActor *child;
  MexScrollViewPrivate *priv = self->priv;

  child = mx_bin_get_child (MX_BIN (self));
  if (child == priv->child)
    return;

  if (priv->child)
    {
      if (priv->focus)
        {
          g_signal_handler_disconnect (priv->focus, priv->keep_visible_id);
          g_object_remove_weak_pointer (G_OBJECT (priv->focus),
                                        (gpointer *)&priv->focus);
          priv->focus = NULL;
        }

      g_signal_handlers_disconnect_by_func (priv->child,
                                            mex_scroll_view_hadjust_notify_cb,
                                            self);
      g_signal_handlers_disconnect_by_func (priv->child,
                                            mex_scroll_view_vadjust_notify_cb,
                                            self);

      priv->child = NULL;

      if (priv->hscroll)
        {
          clutter_actor_animate (CLUTTER_ACTOR (priv->hscroll),
                                 CLUTTER_EASE_OUT_QUAD, 100,
                                 "opacity", 0x00,
                                 NULL);
          mex_scroll_indicator_set_adjustment (MEX_SCROLL_INDICATOR (
                                               priv->hscroll), NULL);
        }
      if (priv->vscroll)
        {
          clutter_actor_animate (CLUTTER_ACTOR (priv->vscroll),
                                 CLUTTER_EASE_OUT_QUAD, 100,
                                 "opacity", 0x00,
                                 NULL);
          mex_scroll_indicator_set_adjustment (MEX_SCROLL_INDICATOR (
                                               priv->vscroll), NULL);
        }
    }

  if (MX_IS_SCROLLABLE (child))
    {
      priv->child = child;

      /* Get adjustments */
      g_signal_connect (child, "notify::horizontal-adjustment",
                        G_CALLBACK (mex_scroll_view_hadjust_notify_cb), self);
      g_signal_connect (child, "notify::vertical-adjustment",
                        G_CALLBACK (mex_scroll_view_vadjust_notify_cb), self);
      mex_scroll_view_hadjust_notify_cb (child, NULL, self);
      mex_scroll_view_vadjust_notify_cb (child, NULL, self);
    }
  else if (child)
    g_warning (G_STRLOC ": An actor of type %s has been added to "
               "a MexScrollView, but the actor does not implement "
               "MxScrollable.", g_type_name (G_OBJECT_TYPE (child)));
}

static void
mex_scroll_view_foreach_with_internals (ClutterContainer *container,
                                        ClutterCallback   callback,
                                        gpointer          user_data)
{
  MexScrollViewPrivate *priv = MEX_SCROLL_VIEW (container)->priv;

  if (priv->child)
    callback (priv->child, user_data);
  if (priv->hscroll)
    callback (priv->hscroll, user_data);
  if (priv->vscroll)
    callback (priv->vscroll, user_data);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->foreach_with_internals = mex_scroll_view_foreach_with_internals;
}

/* Actor implementation */

static void
mex_scroll_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MexScrollView *self = MEX_SCROLL_VIEW (object);

  switch (property_id)
    {
    case PROP_INDICATORS_HIDDEN:
      g_value_set_boolean (value, mex_scroll_view_get_indicators_hidden (self));
      break;

    case PROP_FOLLOW_RECURSE:
      g_value_set_boolean (value, mex_scroll_view_get_follow_recurse (self));
      break;

    case PROP_SCROLL_DELAY:
      g_value_set_uint (value, mex_scroll_view_get_scroll_delay (self));
      break;

    case PROP_SCROLL_GRAVITY:
      g_value_set_enum (value, mex_scroll_view_get_scroll_gravity (self));
      break;

    case PROP_INTERPOLATE:
      g_value_set_boolean (value, mex_scroll_view_get_interpolate (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_scroll_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MexScrollView *self = MEX_SCROLL_VIEW (object);

  switch (property_id)
    {
    case PROP_INDICATORS_HIDDEN:
      mex_scroll_view_set_indicators_hidden (self, g_value_get_boolean (value));
      break;

    case PROP_FOLLOW_RECURSE:
      mex_scroll_view_set_follow_recurse (self, g_value_get_boolean (value));
      break;

    case PROP_SCROLL_DELAY:
      mex_scroll_view_set_scroll_delay (self, g_value_get_uint (value));
      break;

    case PROP_SCROLL_GRAVITY:
      mex_scroll_view_set_scroll_gravity (self, g_value_get_enum (value));
      break;

    case PROP_INTERPOLATE:
      mex_scroll_view_set_interpolate (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_scroll_view_dispose (GObject *object)
{
  MexScrollViewPrivate *priv = MEX_SCROLL_VIEW (object)->priv;
  MxAdjustment *adjustment;

  if (priv->scroll_indicator_timeout)
    {
      g_source_remove (priv->scroll_indicator_timeout);
      priv->scroll_indicator_timeout = 0;
    }

  if (priv->vscroll)
    {
      adjustment = mex_scroll_indicator_get_adjustment (MEX_SCROLL_INDICATOR (
                                                               priv->vscroll));
      if (adjustment)
        g_signal_handlers_disconnect_by_func (adjustment,
                                              mex_scroll_view_adjustment_changed,
                                              object);
      clutter_actor_unparent (priv->vscroll);
      priv->vscroll = NULL;
    }

  if (priv->hscroll)
    {
      adjustment = mex_scroll_indicator_get_adjustment (MEX_SCROLL_INDICATOR (
                                                               priv->hscroll));
      if (adjustment)
        g_signal_handlers_disconnect_by_func (adjustment,
                                              mex_scroll_view_adjustment_changed,
                                              object);
      clutter_actor_unparent (priv->hscroll);
      priv->hscroll = NULL;
    }

  if (priv->keep_visible_timeout)
    {
      g_source_remove (priv->keep_visible_timeout);
      priv->keep_visible_timeout = 0;
    }

  G_OBJECT_CLASS (mex_scroll_view_parent_class)->dispose (object);
}

static void
mex_scroll_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_scroll_view_parent_class)->finalize (object);
}

static void
mex_scroll_view_get_preferred_width (ClutterActor *actor,
                                     gfloat        for_height,
                                     gfloat       *min_width_p,
                                     gfloat       *nat_width_p)
{
  MxPadding padding;
  /*MxScrollPolicy scroll_policy;*/
  gfloat child_min_width, child_nat_width, scroll_w;

  MexScrollViewPrivate *priv = MEX_SCROLL_VIEW (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (!priv->child)
    {
      if (min_width_p)
        *min_width_p = padding.left + padding.right;
      if (nat_width_p)
        *nat_width_p = padding.left + padding.right;

      return;
    }

  if (priv->vscroll_hidden)
    scroll_w = 0;
  else
    clutter_actor_get_preferred_width (priv->vscroll,
                                       -1,
                                       NULL,
                                       &scroll_w);

  clutter_actor_get_preferred_width (priv->child,
                                     for_height,
                                     &child_min_width,
                                     &child_nat_width);

  if (nat_width_p)
    *nat_width_p = MAX (child_nat_width, scroll_w) +
                   padding.left + padding.right;

  /*scroll_policy =
    mx_kinetic_scroll_view_get_scroll_policy (MX_KINETIC_SCROLL_VIEW (actor));

  if ((scroll_policy == MX_SCROLL_POLICY_BOTH) ||
      (scroll_policy == MX_SCROLL_POLICY_HORIZONTAL))*/
    child_min_width = 0;

  if (min_width_p)
    *min_width_p = MAX (child_min_width, scroll_w) +
                   padding.left + padding.right;
}

static void
mex_scroll_view_get_preferred_height (ClutterActor *actor,
                                      gfloat        for_width,
                                      gfloat       *min_height_p,
                                      gfloat       *nat_height_p)
{
  MxPadding padding;
  /*MxScrollPolicy scroll_policy;*/
  gfloat child_min_height, child_nat_height, scroll_h;

  MexScrollViewPrivate *priv = MEX_SCROLL_VIEW (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (!priv->child)
    {
      if (min_height_p)
        *min_height_p = padding.top + padding.bottom;
      if (nat_height_p)
        *nat_height_p = padding.top + padding.bottom;

      return;
    }

  /* Note, we get the width here because this widget is rotated */
  if (priv->hscroll_hidden)
    scroll_h = 0;
  else
    clutter_actor_get_preferred_width (priv->hscroll,
                                       -1,
                                       NULL,
                                       &scroll_h);

  clutter_actor_get_preferred_height (priv->child,
                                      for_width,
                                      &child_min_height,
                                      &child_nat_height);

  if (nat_height_p)
    *nat_height_p = MAX (child_nat_height, scroll_h) +
                    padding.top + padding.bottom;

  /*scroll_policy =
    mx_kinetic_scroll_view_get_scroll_policy (MX_KINETIC_SCROLL_VIEW (actor));

  if ((scroll_policy == MX_SCROLL_POLICY_BOTH) ||
      (scroll_policy == MX_SCROLL_POLICY_VERTICAL))*/
    child_min_height = 0;

  if (min_height_p)
    *min_height_p = MAX (child_min_height, scroll_h) +
                    padding.top + padding.bottom;
}

static void
mex_scroll_view_allocate (ClutterActor           *actor,
                          const ClutterActorBox  *box,
                          ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;

  MexScrollViewPrivate *priv = MEX_SCROLL_VIEW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_scroll_view_parent_class)->
    allocate (actor, box, flags);

  /* Allocate child */
  if (CLUTTER_ACTOR_IS_VISIBLE (actor))
    mx_bin_allocate_child (MX_BIN (actor), box, flags);

  /* Allocate scroll-bars */
  /* Note, purposefully ignoring padding */

  /* Allocate hscroll */
  /* Note, the horizontal scroll-bar is rotated 90 degrees counter-clockwise */
  if (priv->hscroll)
    {
      child_box.y1 = 0;
      child_box.y2 = box->x2 - box->x1;
      clutter_actor_get_preferred_width (priv->hscroll,
                                         child_box.y2,
                                         NULL,
                                         &child_box.x2);
      child_box.x1 = 0;

      clutter_actor_allocate (priv->hscroll, &child_box, flags);
    }

  /* Allocate vscroll */
  if (priv->vscroll)
    {
      child_box.y1 = 0;
      child_box.y2 = box->y2 - box->y1;
      clutter_actor_get_preferred_width (priv->vscroll,
                                         child_box.y2,
                                         NULL,
                                         &child_box.x2);

      child_box.x1 = box->x2 - box->x1 - child_box.x2;
      child_box.x2 += child_box.x1;

      clutter_actor_allocate (priv->vscroll, &child_box, flags);
    }
}

static void
mex_scroll_view_paint (ClutterActor *actor)
{
  MexScrollViewPrivate *priv = MEX_SCROLL_VIEW (actor)->priv;

  /* Chain up for background and child */
  CLUTTER_ACTOR_CLASS (mex_scroll_view_parent_class)->paint (actor);

  /* Paint scroll-bars */
  if (priv->hscroll && clutter_actor_get_opacity (priv->hscroll) > 0)
    clutter_actor_paint (priv->hscroll);
  if (priv->vscroll && clutter_actor_get_opacity (priv->vscroll) > 0)
    clutter_actor_paint (priv->vscroll);
}

static void
mex_scroll_view_focus_allocation_cb (ClutterActor           *focus,
                                     ClutterActorBox        *box,
                                     ClutterAllocationFlags  flags,
                                     MexScrollView          *self)
{
  ClutterActor *parent;
  ClutterGeometry geom;
  ClutterActorBox child_box = { 0, };

  MexScrollViewPrivate *priv = self->priv;

  /* HACK: Override the child-box for resizing-hboxes... Would be good
   *       to figure out a way to do this in a generic way in the
   *       scrollable interface, perhaps.
   */
  if (MEX_IS_SCROLLABLE_CONTAINER (priv->child))
    mex_scrollable_container_get_allocation (
      MEX_SCROLLABLE_CONTAINER (priv->child), focus, &child_box);
  else
    child_box = *box;

  geom.x = child_box.x1;
  geom.y = child_box.y1;
  geom.width = child_box.x2 - child_box.x1;
  geom.height = child_box.y2 - child_box.y1;

  /* Find the relative position of the focused child */
  parent = clutter_actor_get_parent (focus);
  while (parent)
    {
      ClutterActorBox parent_box;

      if (parent == priv->child)
        {
          mex_scroll_view_ensure_visible (self, &geom);
          return;
        }

      clutter_actor_get_allocation_box (parent, &parent_box);
      geom.x += parent_box.x1;
      geom.y += parent_box.y1;

      parent = clutter_actor_get_parent (parent);
    }

  g_warning (G_STRLOC ": Focused child is no longer our descendant");

  g_signal_handler_disconnect (priv->focus, priv->keep_visible_id);
  g_object_remove_weak_pointer (G_OBJECT (priv->focus),
                                (gpointer *)priv->focus);
  priv->focus = NULL;
}

static gboolean
mex_scroll_view_keep_visible_cb (MexScrollView *self)
{
  ClutterActorBox box;

  MexScrollViewPrivate *priv = self->priv;

  priv->keep_visible_timeout = 0;

  if (!priv->focus)
    return FALSE;

  clutter_actor_get_allocation_box (priv->focus, &box);
  mex_scroll_view_focus_allocation_cb (priv->focus, &box, 0, self);

  return FALSE;
}

static void
mex_scroll_view_delay_focus_allocation_cb (ClutterActor           *focus,
                                           ClutterActorBox        *box,
                                           ClutterAllocationFlags  flags,
                                           MexScrollView          *self)
{
  MexScrollViewPrivate *priv = self->priv;

  if (priv->keep_visible_timeout)
    g_source_remove (priv->keep_visible_timeout);

  priv->keep_visible_timeout =
    g_timeout_add (priv->scroll_delay,
                   (GSourceFunc)mex_scroll_view_keep_visible_cb,
                   self);
}

static void
mex_scroll_view_focused_cb (MxFocusManager *manager,
                            GParamSpec     *pspec,
                            MexScrollView  *self)
{
  ClutterActor *focus, *parent;
  MexScrollViewPrivate *priv = self->priv;

  if (priv->focus)
    {
      g_signal_handler_disconnect (priv->focus, priv->keep_visible_id);
      g_object_remove_weak_pointer (G_OBJECT (priv->focus),
                                    (gpointer *)&priv->focus);
      priv->focus = NULL;
    }

  if (priv->keep_visible_timeout)
    {
      g_source_remove (priv->keep_visible_timeout);
      priv->keep_visible_timeout = 0;
    }

  if (!priv->child)
    return;

  focus = (ClutterActor *)mx_focus_manager_get_focused (manager);

  if (!focus)
    return;

  /* If the focused actor is a descendant of our child, make sure to keep
   * it visible.
   */
  priv->focus = focus;
  parent = clutter_actor_get_parent (focus);
  while (parent)
    {
      if (parent == priv->child)
        {
          GCallback callback;
          ClutterActorBox box;

          /* Whether our focus-following should recurse to the outer-most
           * focused child, or the inner-most.
           */
          if (!priv->follow_recurse)
            priv->focus = focus;

          g_object_add_weak_pointer (G_OBJECT (priv->focus),
                                     (gpointer *)&priv->focus);

          clutter_actor_get_allocation_box (priv->focus, &box);

          if (priv->scroll_delay)
            {
              callback = G_CALLBACK (mex_scroll_view_delay_focus_allocation_cb);
              mex_scroll_view_delay_focus_allocation_cb (priv->focus, &box,
                                                         0, self);
            }
          else
            {
              callback = G_CALLBACK (mex_scroll_view_focus_allocation_cb);
              mex_scroll_view_focus_allocation_cb (priv->focus, &box, 0, self);
            }

          priv->keep_visible_id =
            g_signal_connect (priv->focus, "allocation-changed",
                              callback, self);

          return;
        }

      focus = parent;
      parent = clutter_actor_get_parent (focus);
    }

  priv->focus = NULL;
}

static void
mex_scroll_view_map (ClutterActor *actor)
{
  MxFocusManager *manager;

  MexScrollViewPrivate *priv = MEX_SCROLL_VIEW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_scroll_view_parent_class)->map (actor);

  if (priv->hscroll)
    clutter_actor_map (priv->hscroll);
  if (priv->vscroll)
    clutter_actor_map (priv->vscroll);

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    {
      g_signal_connect (manager, "notify::focused",
                        G_CALLBACK (mex_scroll_view_focused_cb), actor);
      mex_scroll_view_focused_cb (manager, NULL, (MexScrollView *)actor);
    }
}

static void
mex_scroll_view_unmap (ClutterActor *actor)
{
  MxFocusManager *manager;

  MexScrollViewPrivate *priv = MEX_SCROLL_VIEW (actor)->priv;

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  g_signal_handlers_disconnect_by_func (manager, mex_scroll_view_focused_cb,
                                        actor);

  if (priv->hscroll)
    clutter_actor_unmap (priv->hscroll);
  if (priv->vscroll)
    clutter_actor_unmap (priv->vscroll);

  CLUTTER_ACTOR_CLASS (mex_scroll_view_parent_class)->unmap (actor);
}

static gboolean
mx_scroll_view_get_paint_volume (ClutterActor       *self,
                                 ClutterPaintVolume *volume)
{
  ClutterGeometry geometry = { 0, };

  /* calling clutter_actor_get_allocation_* can potentially be very
   * expensive, as it can result in a synchronous full stage relayout
   * and redraw
   */
  if (!clutter_actor_has_allocation (self))
    return FALSE;

  clutter_actor_get_allocation_geometry (self, &geometry);

  clutter_paint_volume_set_width (volume, geometry.width);
  clutter_paint_volume_set_height (volume, geometry.height);

  return TRUE;
}


static void
mex_scroll_view_class_init (MexScrollViewClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexScrollViewPrivate));

  object_class->get_property = mex_scroll_view_get_property;
  object_class->set_property = mex_scroll_view_set_property;
  object_class->dispose = mex_scroll_view_dispose;
  object_class->finalize = mex_scroll_view_finalize;

  actor_class->get_preferred_width = mex_scroll_view_get_preferred_width;
  actor_class->get_preferred_height = mex_scroll_view_get_preferred_height;
  actor_class->allocate = mex_scroll_view_allocate;
  actor_class->paint = mex_scroll_view_paint;
  actor_class->map = mex_scroll_view_map;
  actor_class->unmap = mex_scroll_view_unmap;
  actor_class->get_paint_volume = mx_scroll_view_get_paint_volume;

  pspec = g_param_spec_boolean ("indicators-hidden",
                                "Indicators hidden",
                                "Whether the scroll indicators are hidden.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INDICATORS_HIDDEN, pspec);

  pspec = g_param_spec_boolean ("follow-recurse",
                                "Follow recurse",
                                "Whether focus-following recurses to the "
                                "outer-most child.",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_FOLLOW_RECURSE, pspec);

  pspec = g_param_spec_uint ("scroll-delay",
                             "Scroll delay",
                             "Delay before scrolling.",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SCROLL_DELAY, pspec);

  pspec = g_param_spec_enum ("scroll-gravity",
                             "Scroll gravity",
                             "The scroll gravity",
                             CLUTTER_TYPE_GRAVITY,
                             CLUTTER_GRAVITY_NONE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SCROLL_GRAVITY, pspec);

  pspec = g_param_spec_boolean ("interpolate",
                                "Interpolate",
                                "Interpolate adjustment changes.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INTERPOLATE, pspec);
}

static void
mex_scroll_view_style_changed_cb (MexScrollView       *self,
                                  MxStyleChangedFlags  flags)
{
  MexScrollViewPrivate *priv = self->priv;

  if (priv->hscroll)
    mx_stylable_style_changed (MX_STYLABLE (priv->hscroll), flags);
  if (priv->vscroll)
    mx_stylable_style_changed (MX_STYLABLE (priv->vscroll), flags);
}

static void
mex_scroll_view_init (MexScrollView *self)
{
  MexScrollViewPrivate *priv = self->priv = SCROLL_VIEW_PRIVATE (self);

  priv->interpolate = TRUE;

  /* Set x-fill and y-fill to true to get the simple allocation */
  mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);

  /* Create the scroll-bars */
  priv->hscroll = mex_scroll_indicator_new ();
  priv->vscroll = mex_scroll_indicator_new ();

  clutter_actor_set_parent (priv->hscroll, CLUTTER_ACTOR (self));
  clutter_actor_set_parent (priv->vscroll, CLUTTER_ACTOR (self));

  clutter_actor_set_opacity (priv->hscroll, 0x00);
  clutter_actor_set_opacity (priv->vscroll, 0x00);

  clutter_actor_set_rotation (priv->hscroll,
                              CLUTTER_Z_AXIS,
                              270.0,
                              0, 0, 0);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mex_scroll_view_style_changed_cb), NULL);
  g_signal_connect (self, "notify::child",
                    G_CALLBACK (mex_scroll_view_notify_child), NULL);

  /* Queue a relayout when the scroll-policy changes */
  g_signal_connect (self, "notify::scroll-policy",
                    G_CALLBACK (clutter_actor_queue_relayout), NULL);

  priv->indicators_hidden = TRUE;
}

ClutterActor *
mex_scroll_view_new (void)
{
  return g_object_new (MEX_TYPE_SCROLL_VIEW, NULL);
}

void
mex_scroll_view_set_indicators_hidden (MexScrollView *view,
                                       gboolean       hidden)
{
  MexScrollViewPrivate *priv;

  g_return_if_fail (MEX_IS_SCROLL_VIEW (view));

  priv = view->priv;

  if (priv->indicators_hidden != hidden)
    {
      priv->indicators_hidden = hidden;

      if (hidden)
        {
          if (priv->hscroll)
            clutter_actor_animate (priv->hscroll, CLUTTER_EASE_OUT_QUAD, 100,
                                   "opacity", 0x00,
                                   NULL);
          if (priv->vscroll)
            clutter_actor_animate (priv->vscroll, CLUTTER_EASE_OUT_QUAD, 100,
                                   "opacity", 0x00,
                                   NULL);
        }
      else
        {
          if (priv->hscroll && !priv->hscroll_hidden)
            clutter_actor_animate (priv->hscroll, CLUTTER_EASE_OUT_QUAD, 100,
                                   "opacity", 0xff,
                                   NULL);
          if (priv->vscroll && !priv->vscroll_hidden)
            clutter_actor_animate (priv->vscroll, CLUTTER_EASE_OUT_QUAD, 100,
                                   "opacity", 0xff,
                                   NULL);
        }
    }
}

gboolean
mex_scroll_view_get_indicators_hidden (MexScrollView *view)
{
  g_return_val_if_fail (MEX_IS_SCROLL_VIEW (view), FALSE);
  return view->priv->indicators_hidden;
}

static void
mex_scroll_view_adjustment_set_value (MexScrollView *view,
                                      MxAdjustment  *adjustment,
                                      gdouble        value)
{
  MxAdjustment *hadjust, *vadjust;
  MexScrollViewPrivate *priv = view->priv;

  mx_scrollable_get_adjustments (MX_SCROLLABLE (priv->child),
                                 &hadjust, &vadjust);

  if (adjustment == hadjust)
    {
      if (value == priv->htarget)
        return;

      priv->htarget = value;
    }
  else
    {
      if (value == priv->vtarget)
        return;

      priv->vtarget = value;
    }

  if (priv->interpolate)
    mx_adjustment_interpolate (adjustment, value, 200, CLUTTER_EASE_OUT_QUAD);
  else
    mx_adjustment_set_value (adjustment, value);
}

/* Following two functions copied and modified from MxScrollView
 * (verified written by myself, Chris Lord) */
static void
_mex_scroll_view_ensure_visible_axis (MexScrollView *view,
                                      MxAdjustment  *adjust,
                                      gdouble        lower,
                                      gdouble        upper)
{
  gdouble new_value, adjust_lower, adjust_upper, adjust_page_size;

  gboolean changed = FALSE;

  mx_adjustment_get_values (adjust,
                            &new_value,
                            &adjust_lower,
                            &adjust_upper,
                            NULL,
                            NULL,
                            &adjust_page_size);

  /* Sanitise input values */
  lower = CLAMP (lower, adjust_lower, adjust_upper);
  upper = CLAMP (upper, adjust_lower, adjust_upper);

  /* Ensure the bottom is visible */
  if (new_value + adjust_page_size < upper)
    {
      new_value = upper - adjust_page_size;
      changed = TRUE;
    }

  /* Ensure the top is visible */
  if (lower < new_value)
    {
      new_value = lower;
      changed = TRUE;
    }

  if (changed)
    mex_scroll_view_adjustment_set_value (view, adjust, new_value);
}

void
mex_scroll_view_ensure_visible (MexScrollView         *scroll,
                                const ClutterGeometry *geometry)
{
  MxAdjustment *hadjust, *vadjust;
  MxScrollPolicy scroll_policy;
  MexScrollViewPrivate *priv;

  g_return_if_fail (MEX_IS_SCROLL_VIEW (scroll));

  priv = scroll->priv;

  if (!priv->child)
    return;

  mx_scrollable_get_adjustments (MX_SCROLLABLE (priv->child),
                                 &hadjust, &vadjust);

  scroll_policy =
    mx_kinetic_scroll_view_get_scroll_policy (MX_KINETIC_SCROLL_VIEW (scroll));

  if (hadjust &&
      ((scroll_policy == MX_SCROLL_POLICY_BOTH) ||
       (scroll_policy == MX_SCROLL_POLICY_HORIZONTAL)))
    {
      gdouble new_value;
      gdouble value = mx_adjustment_get_value (hadjust);
      gdouble width = mx_adjustment_get_page_size (hadjust);

      switch ((geometry->width < width) ?
              priv->scroll_gravity : CLUTTER_GRAVITY_CENTER)
        {
        /* Align left */
        case CLUTTER_GRAVITY_NORTH_WEST:
        case CLUTTER_GRAVITY_WEST:
        case CLUTTER_GRAVITY_SOUTH_WEST:
          if (value != geometry->x)
            mex_scroll_view_adjustment_set_value (scroll, hadjust, geometry->x);
          break;

        /* Align right */
        case CLUTTER_GRAVITY_NORTH_EAST:
        case CLUTTER_GRAVITY_EAST:
        case CLUTTER_GRAVITY_SOUTH_EAST:
          new_value = geometry->x + (width - geometry->width);
          if (value != new_value)
            mex_scroll_view_adjustment_set_value (scroll, hadjust, new_value);
          break;

        /* Align middle */
        case CLUTTER_GRAVITY_NORTH:
        case CLUTTER_GRAVITY_CENTER:
        case CLUTTER_GRAVITY_SOUTH:
          new_value = geometry->x - (width - geometry->width) / 2.0;
          if (value != new_value)
            mex_scroll_view_adjustment_set_value (scroll, hadjust, new_value);
          break;

        default:
          _mex_scroll_view_ensure_visible_axis (scroll, hadjust, geometry->x,
                                                geometry->x + geometry->width);
          break;
        }
    }

  if (vadjust &&
      ((scroll_policy == MX_SCROLL_POLICY_BOTH) ||
       (scroll_policy == MX_SCROLL_POLICY_VERTICAL)))
    {
      gdouble new_value;
      gdouble value = mx_adjustment_get_value (vadjust);
      gdouble height = mx_adjustment_get_page_size (vadjust);

      switch ((geometry->height < height) ?
              priv->scroll_gravity : CLUTTER_GRAVITY_CENTER)
        {
        /* Align top */
        case CLUTTER_GRAVITY_NORTH_WEST:
        case CLUTTER_GRAVITY_NORTH:
        case CLUTTER_GRAVITY_NORTH_EAST:
          if (value != geometry->y)
            mex_scroll_view_adjustment_set_value (scroll, vadjust, geometry->y);
          break;

        /* Align bottom */
        case CLUTTER_GRAVITY_SOUTH_WEST:
        case CLUTTER_GRAVITY_SOUTH:
        case CLUTTER_GRAVITY_SOUTH_EAST:
          new_value = geometry->y + (height - geometry->height);
          if (value != new_value)
            mex_scroll_view_adjustment_set_value (scroll, vadjust, new_value);
          break;

        /* Align middle */
        case CLUTTER_GRAVITY_WEST:
        case CLUTTER_GRAVITY_CENTER:
        case CLUTTER_GRAVITY_EAST:
          new_value = geometry->y - (height - geometry->height) / 2.0;
          if (value != new_value)
            mex_scroll_view_adjustment_set_value (scroll, vadjust, new_value);
          break;

        default:
          _mex_scroll_view_ensure_visible_axis (scroll, vadjust, geometry->y,
                                                geometry->y + geometry->height);
          break;
        }
    }
}

void
mex_scroll_view_set_follow_recurse (MexScrollView *view,
                                    gboolean       recurse)
{
  MexScrollViewPrivate *priv;

  g_return_if_fail (MEX_IS_SCROLL_VIEW (view));

  priv = view->priv;
  if (priv->follow_recurse != recurse)
    {
      priv->follow_recurse = recurse;

      g_object_notify (G_OBJECT (view), "follow-recurse");
    }
}

gboolean
mex_scroll_view_get_follow_recurse (MexScrollView *view)
{
  g_return_val_if_fail (MEX_IS_SCROLL_VIEW (view), FALSE);
  return view->priv->follow_recurse;
}

void
mex_scroll_view_set_scroll_delay (MexScrollView *view,
                                  guint          delay)
{
  MexScrollViewPrivate *priv;

  g_return_if_fail (MEX_IS_SCROLL_VIEW (view));

  priv = view->priv;
  if (priv->scroll_delay != delay)
    {
      priv->scroll_delay = delay;
      g_object_notify (G_OBJECT (view), "scroll-delay");
    }
}

guint
mex_scroll_view_get_scroll_delay (MexScrollView *view)
{
  g_return_val_if_fail (MEX_IS_SCROLL_VIEW (view), 0);
  return view->priv->scroll_delay;
}

void
mex_scroll_view_set_scroll_gravity (MexScrollView  *view,
                                    ClutterGravity  gravity)
{
  MexScrollViewPrivate *priv;

  g_return_if_fail (MEX_IS_SCROLL_VIEW (view));

  priv = view->priv;
  if (priv->scroll_gravity != gravity)
    {
      priv->scroll_gravity = gravity;
      g_object_notify (G_OBJECT (view), "scroll-gravity");
    }
}

ClutterGravity
mex_scroll_view_get_scroll_gravity (MexScrollView *view)
{
  g_return_val_if_fail (MEX_IS_SCROLL_VIEW (view), CLUTTER_GRAVITY_NONE);
  return view->priv->scroll_gravity;
}

void
mex_scroll_view_set_interpolate (MexScrollView *view,
                                 gboolean       interpolate)
{
  MexScrollViewPrivate *priv;

  g_return_if_fail (MEX_IS_SCROLL_VIEW (view));

  priv = view->priv;
  if (priv->interpolate)
    {
      priv->interpolate = interpolate;
      g_object_notify (G_OBJECT (view), "interpolate");
    }
}

gboolean
mex_scroll_view_get_interpolate (MexScrollView *view)
{
  g_return_val_if_fail (MEX_IS_SCROLL_VIEW (view), FALSE);
  return view->priv->interpolate;
}
