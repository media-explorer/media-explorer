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


#include "mex-column.h"
#include "mex-column-view.h"
#include "mex-content-box.h"
#include "mex-scroll-view.h"
#include "mex-tile.h"
#include "mex-shadow.h"
#include "mex-scrollable-container.h"

enum
{
  PROP_0,
  PROP_HADJUST,
  PROP_VADJUST,
  PROP_COLLAPSE_ON_FOCUS,
  PROP_OPENED
};

struct _MexColumnPrivate
{
  guint         has_focus_changed : 1;
  guint         has_focus : 1;
  guint         collapse : 1;

  ClutterActor *current_focus;

  ClutterTimeline *expand_timeline;

  GList            *children;
  guint             n_items;
  gint              open_boxes;

  MxAdjustment *adjustment;
  gdouble       adjustment_value;
};

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void mx_scrollable_iface_init (MxScrollableIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);
static void mex_scrollable_iface_init (MexScrollableContainerInterface *iface);

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MEX_TYPE_COLUMN, MexColumnPrivate))
G_DEFINE_TYPE_WITH_CODE (MexColumn, mex_column, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_SCROLLABLE,
                                                mx_scrollable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_SCROLLABLE_CONTAINER,
                                                mex_scrollable_iface_init))

static void
child_expand_complete_cb (ClutterAnimation *animation,
                          ClutterActor     *child)
{
  /* child should now be at it's natural height */
  clutter_actor_set_height (child, -1);
}

static void
mex_column_expand_child (ClutterActor *child)
{
  ClutterActorClass *actor_class;
  gfloat new_height, old_height;

  /* get the preferred directly from the class, to avoid getting any fixed
   * value */
  actor_class = CLUTTER_ACTOR_GET_CLASS (child);
  actor_class->get_preferred_height (child, -1, NULL, &new_height);

  old_height = clutter_actor_get_height (child);

  if (old_height == new_height)
    return;

  clutter_actor_animate (child, CLUTTER_EASE_OUT_CUBIC, 200,
                         "height", new_height,
                         "signal-after::completed", child_expand_complete_cb,
                         child,
                         NULL);
}

/* MexScrollableContainerInterface */
static void
mex_column_get_allocation (MexScrollableContainer *self,
                           ClutterActor           *child,
                           ClutterActorBox        *box)
{
  MexColumnPrivate *priv = MEX_COLUMN (self)->priv;
  gfloat width, height;
  gfloat child_height;
  gint i;

  box->y1 = 0;

  if (!priv->children)
    return;

  child_height = clutter_actor_get_height (priv->children->data);
  i = g_list_index (priv->children, child);

  if (i >= 0)
    box->y1 += child_height * i;

  clutter_actor_get_size (child, &width, &height);

  box->x1 = 0;
  box->x2 = width;
  box->y2 = box->y1 + height;
}


static void
mex_scrollable_iface_init (MexScrollableContainerInterface *iface)
{
  iface->get_allocation = mex_column_get_allocation;
}

static void
content_box_open_notify (MexContentBox *box,
                         GParamSpec    *pspec,
                         MexColumn     *column)
{
  MexColumnPrivate *priv = MEX_COLUMN (column)->priv;
  GList *l;
  ClutterActorMeta *shadow;

  if (mex_content_box_get_open (box))
    {
      for (l = priv->children; l; l = l->next)
        {
          if (l->data != box)
            clutter_actor_animate (l->data, CLUTTER_EASE_IN_OUT_QUAD, 200,
                                   "opacity", 56, NULL);
        }

      shadow = (ClutterActorMeta*) clutter_actor_get_effect (CLUTTER_ACTOR (box),
                                                             "shadow");
      clutter_actor_meta_set_enabled (shadow, TRUE);

      /* Restore the opened box to full opacity */
      clutter_actor_animate (CLUTTER_ACTOR (box), CLUTTER_EASE_IN_OUT_QUAD, 200,
                             "opacity", 255, NULL);

      priv->open_boxes ++;
    }
  else
    {
      priv->open_boxes --;
    }

  if (priv->open_boxes == 0)
    {
      /* restore all children to full opacity */
      for (l = priv->children; l; l = l->next)
        {
          clutter_actor_animate (l->data, CLUTTER_EASE_IN_OUT_QUAD, 200,
                                 "opacity", 255, NULL);
        }
      shadow = (ClutterActorMeta*) clutter_actor_get_effect (CLUTTER_ACTOR (box),
                                                             "shadow");
      clutter_actor_meta_set_enabled (shadow, FALSE);
    }

  g_object_notify (G_OBJECT (column), "opened");
}

/* ClutterContainerIface */

static void
mex_column_add (ClutterContainer *container,
                ClutterActor     *actor)
{
  MexColumn *self = MEX_COLUMN (container);
  MexColumnPrivate *priv = self->priv;

  priv->children = g_list_append (priv->children, actor);
  priv->n_items ++;

  /* Expand/collapse any drawer that gets added as appropriate */
  if (MEX_IS_CONTENT_BOX (actor))
    {
      MexShadow *shadow;
      ClutterColor shadow_color = { 0, 0, 0, 128 };

      shadow = mex_shadow_new ();
      mex_shadow_set_paint_flags (shadow,
                                  MEX_TEXTURE_FRAME_TOP
                                  | MEX_TEXTURE_FRAME_BOTTOM);
      mex_shadow_set_radius_y (shadow, 25);
      mex_shadow_set_color (shadow, &shadow_color);
      clutter_actor_add_effect_with_name (CLUTTER_ACTOR (actor), "shadow",
                                          CLUTTER_EFFECT (shadow));
      clutter_actor_meta_set_enabled (CLUTTER_ACTOR_META (shadow), FALSE);

      g_signal_connect (actor, "notify::open",
                        G_CALLBACK (content_box_open_notify), container);

      mex_content_box_set_important (MEX_CONTENT_BOX (actor), priv->has_focus);

    }

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (self));

  g_signal_emit_by_name (self, "actor-added", actor);
}

static void
mex_column_remove (ClutterContainer *container,
                   ClutterActor     *actor)
{
  GList *link_;

  MexColumn *self = MEX_COLUMN (container);
  MexColumnPrivate *priv = self->priv;

  link_ = g_list_find (priv->children, actor);

  if (!link_)
    {
      g_warning (G_STRLOC ": Trying to remove an unknown child");
      return;
    }

  if (priv->current_focus == actor)
    {
      priv->current_focus = link_->next ? link_->next->data :
        (link_->prev ? link_->prev->data : NULL);
      priv->has_focus = FALSE;
    }

  g_signal_handlers_disconnect_by_func (actor,
                                        content_box_open_notify,
                                        container);

  /* Remove the old actor */
  if (priv->expand_timeline)
    g_signal_handlers_disconnect_by_func (priv->expand_timeline,
                                          mex_column_expand_child,
                                          actor);
  g_object_ref (actor);

  clutter_actor_unparent (actor);
  priv->children = g_list_delete_link (priv->children, link_);
  priv->n_items --;

  /* children may not be at full opacity if the item removed was "open" */
  if (MEX_IS_CONTENT_BOX (actor)
      && mex_content_box_get_open (MEX_CONTENT_BOX (actor)))
    {
      GList *l;

      for (l = priv->children; l; l = l->next)
        clutter_actor_animate (l->data, CLUTTER_EASE_IN_OUT_QUAD, 200,
                               "opacity", 255, NULL);

      /* clutter_actor_animate (priv->header, CLUTTER_EASE_IN_OUT_QUAD, 200, */
      /*                        "opacity", 255, NULL); */
    }

  g_signal_emit_by_name (self, "actor-removed", actor);

  g_object_unref (actor);
}

static void
mex_column_foreach (ClutterContainer *container,
                    ClutterCallback   callback,
                    gpointer          user_data)
{
  MexColumn *self = MEX_COLUMN (container);
  MexColumnPrivate *priv = self->priv;

  g_list_foreach (priv->children, (GFunc)callback, user_data);
}

static void
mex_column_reorder (ClutterContainer *container,
                    ClutterActor     *actor,
                    ClutterActor     *sibling,
                    gboolean          after)
{
  GList *l, *sibling_link;
  MexColumnPrivate *priv = MEX_COLUMN (container)->priv;

  l = g_list_find (priv->children, actor);
  sibling_link = g_list_find (priv->children, sibling);

  if (!l || !sibling_link)
    {
      g_warning (G_STRLOC ": Children not found in internal child list");
      return;
    }

  if (after)
    sibling_link = g_list_next (sibling_link);

  if (l == sibling_link)
    return;

  priv->children = g_list_delete_link (priv->children, l);
  priv->children = g_list_insert_before (priv->children, sibling_link, actor);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));
}

static void
mex_column_raise (ClutterContainer *container,
                  ClutterActor     *actor,
                  ClutterActor     *sibling)
{
  mex_column_reorder (container, actor, sibling, TRUE);
}

static void
mex_column_lower (ClutterContainer *container,
                  ClutterActor     *actor,
                  ClutterActor     *sibling)
{
  mex_column_reorder (container, actor, sibling, FALSE);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = mex_column_add;
  iface->remove = mex_column_remove;
  iface->foreach = mex_column_foreach;
  iface->raise = mex_column_raise;
  iface->lower = mex_column_lower;
}

/* MxScrollableIface */

static void
mex_column_adjustment_changed_cb (MexColumn *self)
{
  MexColumnPrivate *priv = self->priv;

  priv->adjustment_value = mx_adjustment_get_value (priv->adjustment);
  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

static void
mex_column_set_adjustments (MxScrollable *scrollable,
                            MxAdjustment *hadjust,
                            MxAdjustment *vadjust)
{
  MexColumnPrivate *priv = MEX_COLUMN (scrollable)->priv;

  if (priv->adjustment == vadjust)
    return;

  if (priv->adjustment)
    {
      g_signal_handlers_disconnect_by_func (priv->adjustment,
                                            mex_column_adjustment_changed_cb,
                                            scrollable);
      g_object_unref (priv->adjustment);
      priv->adjustment_value = 0;
    }

  priv->adjustment = vadjust;

  if (priv->adjustment)
    {
      g_object_ref_sink (priv->adjustment);
      g_signal_connect_swapped (priv->adjustment, "notify::value",
                                G_CALLBACK (mex_column_adjustment_changed_cb),
                                scrollable);
      priv->adjustment_value = mx_adjustment_get_value (priv->adjustment);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (scrollable));
}

static void
mex_column_get_adjustments (MxScrollable  *scrollable,
                            MxAdjustment **hadjust,
                            MxAdjustment **vadjust)
{
  MexColumnPrivate *priv = MEX_COLUMN (scrollable)->priv;

  if (hadjust)
    *hadjust = NULL;

  if (!vadjust)
    return;

  if (!priv->adjustment)
    {
      *vadjust = mx_adjustment_new ();
      mx_scrollable_set_adjustments (scrollable, NULL, *vadjust);
      g_object_unref (*vadjust);
    }
  else
    *vadjust = priv->adjustment;
}

static void
mx_scrollable_iface_init (MxScrollableIface *iface)
{
  iface->set_adjustments = mex_column_set_adjustments;
  iface->get_adjustments = mex_column_get_adjustments;
}

/* MxFocusableIface */

static MxFocusable *
mex_column_move_focus (MxFocusable      *focusable,
                       MxFocusDirection  direction,
                       MxFocusable      *from)
{
  MxFocusHint hint;

  GList *link_ = NULL;
  MexColumn *self = MEX_COLUMN (focusable);
  MexColumnPrivate *priv = self->priv;

  focusable = NULL;

  link_ = g_list_find (priv->children, from);
  if (!link_)
    return NULL;

  switch (direction)
    {
    case MX_FOCUS_DIRECTION_PREVIOUS:
    case MX_FOCUS_DIRECTION_UP:
      hint = (direction == MX_FOCUS_DIRECTION_PREVIOUS) ?
        MX_FOCUS_HINT_LAST : MX_FOCUS_HINT_FROM_BELOW;
      link_ = g_list_previous (link_);
      if (link_)
        focusable = mx_focusable_accept_focus (
                       MX_FOCUSABLE (link_->data), hint);
      break;

    case MX_FOCUS_DIRECTION_NEXT:
    case MX_FOCUS_DIRECTION_DOWN:
      hint = (direction == MX_FOCUS_DIRECTION_NEXT) ?
        MX_FOCUS_HINT_FIRST : MX_FOCUS_HINT_FROM_ABOVE;
      link_ = g_list_next (link_);

      if (link_)
        focusable = mx_focusable_accept_focus (
                       MX_FOCUSABLE (link_->data), hint);
      break;

    case MX_FOCUS_DIRECTION_OUT:
      if (from &&
          (clutter_actor_get_parent (CLUTTER_ACTOR (from)) ==
           CLUTTER_ACTOR (self)))
        priv->current_focus = CLUTTER_ACTOR (from);
      break;

    default:
      break;
    }

  return focusable;
}

static MxFocusable *
mex_column_accept_focus (MxFocusable *focusable,
                         MxFocusHint  hint)
{
  GList *link_;

  MexColumn *self = MEX_COLUMN (focusable);
  MexColumnPrivate *priv = self->priv;

  focusable = NULL;

  switch (hint)
    {
    case MX_FOCUS_HINT_FROM_LEFT:
    case MX_FOCUS_HINT_FROM_RIGHT:
    case MX_FOCUS_HINT_PRIOR:
      if (priv->current_focus &&
          (focusable = mx_focusable_accept_focus (
             MX_FOCUSABLE (priv->current_focus), hint)))
        break;
      /* If there's no prior focus, or the prior focus rejects focus,
       * try to just focus the first actor.
       */

    case MX_FOCUS_HINT_FIRST:
    case MX_FOCUS_HINT_FROM_ABOVE:
      if (priv->n_items)
        focusable = mx_focusable_accept_focus (
                       MX_FOCUSABLE (priv->children->data), hint);
      break;

    case MX_FOCUS_HINT_LAST:
    case MX_FOCUS_HINT_FROM_BELOW:
      link_ = g_list_last (priv->children);
      if (link_)
        focusable = mx_focusable_accept_focus (
                       MX_FOCUSABLE (link_->data), hint);
      break;
    }

  return focusable;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_column_move_focus;
  iface->accept_focus = mex_column_accept_focus;
}

/* MxStylableIface */

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
/*      GParamSpec *pspec;*/

      is_initialized = TRUE;

/*      pspec = g_param_spec_int ("x-mx-overlap",
                                "Overlap",
                                "Overlap between buttons.",
                                0, G_MAXINT, 0,
                                G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_EXPANDER_BOX, pspec);*/
    }
}

/* Actor implementation */

static void
mex_column_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  MexColumn *self = MEX_COLUMN (object);

  switch (prop_id)
    {
    case PROP_HADJUST:
      mex_column_set_adjustments (MX_SCROLLABLE (self), NULL,
                                  g_value_get_object (value));
      break;

    case PROP_VADJUST:
      mex_column_set_adjustments (MX_SCROLLABLE (self),
                                  g_value_get_object (value),
                                  self->priv->adjustment);
      break;

    case PROP_COLLAPSE_ON_FOCUS:
      mex_column_set_collapse_on_focus (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
mex_column_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  MxAdjustment *adjustment;
  MexColumn *self = MEX_COLUMN (object);

  switch (prop_id)
    {
    case PROP_HADJUST:
      mex_column_get_adjustments (MX_SCROLLABLE (self), &adjustment, NULL);
      g_value_set_object (value, adjustment);
      break;

    case PROP_VADJUST:
      mex_column_get_adjustments (MX_SCROLLABLE (self), NULL, &adjustment);
      g_value_set_object (value, adjustment);
      break;

    case PROP_COLLAPSE_ON_FOCUS:
      g_value_set_boolean (value, mex_column_get_collapse_on_focus (self));
      break;

    case PROP_OPENED:
      g_value_set_boolean (value, mex_column_get_opened (self));

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
mex_column_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_column_parent_class)->finalize (object);
}

static void
mex_column_dispose (GObject *object)
{
  MexColumn *self = MEX_COLUMN (object);
  MexColumnPrivate *priv = self->priv;

  if (priv->adjustment)
    {
      g_signal_handlers_disconnect_by_func (priv->adjustment,
                                            clutter_actor_queue_redraw,
                                            object);
      g_object_unref (priv->adjustment);
      priv->adjustment = NULL;
    }

  if (priv->expand_timeline)
    {
      g_object_unref (priv->expand_timeline);
      priv->expand_timeline = NULL;
    }

  while (priv->children)
    clutter_actor_destroy (CLUTTER_ACTOR (priv->children->data));

  G_OBJECT_CLASS (mex_column_parent_class)->dispose (object);
}

static void
mex_column_get_preferred_width (ClutterActor *actor,
                                gfloat        for_height,
                                gfloat       *min_width_p,
                                gfloat       *nat_width_p)
{
  GList *c;
  MxPadding padding;
  gfloat min_width = 0, nat_width = 0;

  MexColumn *self = MEX_COLUMN (actor);
  MexColumnPrivate *priv = self->priv;

  for_height = -1;

  if (priv->n_items > 0)
    {
      min_width = nat_width = 0;
      for_height /= (gfloat)priv->n_items;

      for (c = priv->children; c; c = c->next)
        {
          gfloat child_min_width, child_nat_width;
          ClutterActor *child = c->data;

          clutter_actor_get_preferred_width (child, for_height,
                                             &child_min_width,
                                             &child_nat_width);

          if (child_min_width > min_width)
            min_width = child_min_width;
          if (child_nat_width > nat_width)
            nat_width = child_nat_width;
        }
    }

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_width_p)
    *min_width_p = min_width + padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p = nat_width + padding.left + padding.right;
}

static void
mex_column_get_preferred_height (ClutterActor *actor,
                                 gfloat        for_width,
                                 gfloat       *min_height_p,
                                 gfloat       *nat_height_p)
{
  GList *c;
  gfloat min_height = 0, nat_height = 0;
  MxPadding padding;

  MexColumn *self = MEX_COLUMN (actor);
  MexColumnPrivate *priv = self->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (for_width >= 0)
    for_width = MAX (0, for_width - padding.left - padding.right);

  if (priv->n_items > 0)
    {
      gfloat child_min_height, child_nat_height;

      for (c = priv->children; c; c = c->next)
        {
          ClutterActor *child = c->data;

          clutter_actor_get_preferred_height (child, for_width,
                                              &child_min_height,
                                              &child_nat_height);

          min_height += child_min_height;
          nat_height += child_nat_height;

          if (priv->adjustment)
            break;
        }
    }

  if (min_height_p)
    *min_height_p = min_height + padding.top + padding.bottom;
  if (nat_height_p)
    *nat_height_p += nat_height + padding.top + padding.bottom;
}

static void
mex_column_allocate (ClutterActor          *actor,
                     const ClutterActorBox *box,
                     ClutterAllocationFlags flags)
{
  ClutterActorBox child_box;
  MxPadding padding;
  GList *c;

  MexColumn *column = MEX_COLUMN (actor);
  MexColumnPrivate *priv = column->priv;

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  child_box.x1 = padding.left;
  child_box.x2 = box->x2 - box->x1 - padding.right;
  child_box.y1 = padding.top;
  child_box.y2 = box->y2 - box->y1 - padding.bottom;

  if (priv->n_items)
    {
      /* Calculate child height multiplier */
      gfloat width, pref_height, avail_height, ratio, remainder;

      if (!priv->adjustment)
        {
          /* Find out the height available for each actor as a ratio of
           * their preferred height.
           */
          clutter_actor_get_preferred_height (actor, box->x2 - box->x1,
                                              NULL, &pref_height);
          pref_height -= padding.top + padding.bottom;

          avail_height = child_box.y2 - child_box.y1;

          ratio = avail_height / pref_height;
        }
      else
        ratio = 1;

      /* Allocate children */
      remainder = 0;
      width = child_box.x2 - child_box.x1;

      for (c = priv->children; c; c = c->next)
        {
          gfloat min_height, nat_height, height;
          ClutterActor *child = c->data;

          clutter_actor_get_preferred_height (child, width,
                                              &min_height, &nat_height);

          /* Calculate the allocatable height and keep an accumulator so
           * when we round to a pixel, we don't end up with a lot of
           * empty space at the end of the actor.
           */
          height = MAX (min_height, nat_height / ratio);
          remainder += (height - (gint)height);
          height = (gint)height;

          while (remainder >= 1.f)
            {
              height += 1.f;
              remainder -= 1.f;
            }

          /* Allocate the child */
          child_box.y2 = child_box.y1 + height;
          clutter_actor_allocate (child, &child_box, flags);

          /* Set the top position of the next child box */
          child_box.y1 = child_box.y2;
        }
    }

  /* Make sure the adjustment reflects the column's allocation */
  if (priv->adjustment)
    {
      gdouble page_size = box->y2 - box->y1 - padding.top - padding.bottom;
      mx_adjustment_set_values (priv->adjustment,
                                mx_adjustment_get_value (priv->adjustment),
                                0.0,
                                child_box.y2 - padding.top,
                                1.0,
                                page_size,
                                page_size);
    }
}

static void
mex_column_apply_transform (ClutterActor *actor,
                            CoglMatrix   *matrix)
{
  MexColumnPrivate *priv = MEX_COLUMN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->
    apply_transform (actor, matrix);

  if (priv->adjustment)
    cogl_matrix_translate (matrix, 0,
                           -priv->adjustment_value, 0);
}

static void
mex_column_paint (ClutterActor *actor)
{
  GList *c;
  MxPadding padding;
  ClutterActorBox box;

  MexColumn *self = MEX_COLUMN (actor);
  MexColumnPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->paint (actor);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  clutter_actor_get_allocation_box (actor, &box);

  cogl_clip_push_rectangle (padding.left,
                            padding.top + priv->adjustment_value,
                            box.x2 - box.x1 - padding.right,
                            box.y2 - box.y1 - padding.bottom +
                            priv->adjustment_value);

  for (c = priv->children; c; c = c->next)
    {
      /* skip the current focus and paint it last*/
      if (priv->current_focus != c->data)
        clutter_actor_paint (c->data);
    }

  /* paint the current focused actor last to ensure any shadow is drawn
   * on top of other items */
  if (priv->current_focus)
    clutter_actor_paint (priv->current_focus);

  cogl_clip_pop ();
}

static void
mex_column_pick (ClutterActor *actor, const ClutterColor *color)
{
  GList *c;
  gdouble value;
  MxPadding padding;
  ClutterActorBox box;

  MexColumn *self = MEX_COLUMN (actor);
  MexColumnPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->pick (actor, color);

  /* Don't pick children when we don't have focus */
  if (!priv->has_focus)
    return;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  clutter_actor_get_allocation_box (actor, &box);
  if (priv->adjustment)
    value = priv->adjustment_value;
  else
    value = 0;

  cogl_clip_push_rectangle (padding.left,
                            padding.top + value,
                            box.x2 - box.x1 - padding.right,
                            box.y2 - box.y1 - padding.bottom + value);

  for (c = priv->children; c; c = c->next)
    clutter_actor_paint (c->data);

  cogl_clip_pop ();
}

static void
mex_column_shrink_child (ClutterActor *child)
{
  ClutterActorClass *actor_class;
  gfloat new_height, old_height;

  /* get the preferred directly from the class, to avoid getting any fixed
   * value */
  actor_class = CLUTTER_ACTOR_GET_CLASS (child);
  actor_class->get_preferred_height (child, -1, &new_height, NULL);

  old_height = clutter_actor_get_height (child);

  if (old_height == new_height)
    return;

  /* prevent the completed signal being called if the child was expanding */
  clutter_actor_detach_animation (child);

  clutter_actor_animate (child, CLUTTER_EASE_OUT_CUBIC, 200,
                         "height", new_height, NULL);
}

static void
mex_column_notify_focused_cb (MxFocusManager *manager,
                              GParamSpec     *pspec,
                              MexColumn      *self)
{
  GList *c;
  guint offset, increment;
  ClutterActor *focused, *focused_cell;
  gboolean cell_has_focus, open, set_tile_important;

  MexColumnPrivate *priv = self->priv;

  focused = (ClutterActor *)mx_focus_manager_get_focused (manager);

  /* Check if we have focus, and what child is focused */
  focused_cell = NULL;
  set_tile_important = priv->has_focus;
  cell_has_focus = FALSE;

  if (focused)
    {
      ClutterActor *parent = clutter_actor_get_parent (focused);
      while (parent)
        {
          if (parent == (ClutterActor *)self)
            {
              focused_cell = focused;
              cell_has_focus = TRUE;

              /* Check whether focus has really moved */
              if ((priv->current_focus == focused_cell) &&
                  !priv->has_focus_changed)
                return;
              priv->current_focus = focused_cell;
              break;
            }

          focused = parent;
          parent = clutter_actor_get_parent (focused);
        }
    }

  /* Scroll the adjustment to the top */
  if (!cell_has_focus && priv->adjustment)
    mx_adjustment_interpolate (priv->adjustment, 0, 250,
                               CLUTTER_EASE_OUT_CUBIC);

  /* Open/close boxes as appropriate */
  offset = 0;
  increment = 150;

  /* If we're changing the tile importance, initialise the state manager */
  if (set_tile_important && priv->n_items > 0)
    {
      if (priv->expand_timeline)
        g_object_unref (priv->expand_timeline);
      priv->expand_timeline =
        clutter_timeline_new (priv->n_items * increment);
      if (priv->has_focus_changed)
        clutter_timeline_set_delay (priv->expand_timeline, 350);
    }

  /* Loop through children and set the content box important/unimportant
   * as necessary */
  open = priv->has_focus && (focused_cell == NULL);
  for (c = priv->children; c; c = c->next)
    {
      gchar signal_name[32+16];
      ClutterActor *child = c->data;

      if ((!priv->collapse && priv->has_focus) || (child == focused_cell))
        open = TRUE;

      if (!MEX_IS_CONTENT_BOX (child))
        continue;

      /* Note, 'marker-reached::' is 16 characters long */
      g_snprintf (signal_name, G_N_ELEMENTS (signal_name),
                  "marker-reached::%p", child);

      if (!open)
        {
          /* close */
          if (priv->expand_timeline)
            {
              if (clutter_timeline_has_marker (priv->expand_timeline,
                                               signal_name + 16))
                clutter_timeline_remove_marker (priv->expand_timeline,
                                                signal_name + 16);
              g_signal_handlers_disconnect_by_func (priv->expand_timeline,
                                                    mex_column_expand_child,
                                                    child);
            }
          mex_column_shrink_child (child);
        }
      else if (set_tile_important)
        {
          /* stagger opening */
          clutter_timeline_add_marker_at_time (priv->expand_timeline,
                                               signal_name + 16, offset);
          g_signal_connect_swapped (priv->expand_timeline, signal_name,
                                    G_CALLBACK (mex_column_expand_child),
                                    child);

          offset += increment;
        }
      else
        {
          mex_column_expand_child (child);
        }
      mex_content_box_set_important (MEX_CONTENT_BOX (child), priv->has_focus);
    }

  if (priv->expand_timeline && set_tile_important && (offset >= increment))
    clutter_timeline_start (priv->expand_timeline);

  priv->has_focus_changed = FALSE;
}

static void
mex_column_map (ClutterActor *actor)
{
  MxFocusManager *manager;

  MexColumn *self = MEX_COLUMN (actor);

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->map (actor);

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  g_signal_connect (manager, "notify::focused",
                    G_CALLBACK (mex_column_notify_focused_cb), actor);
  mex_column_notify_focused_cb (manager, NULL, self);
}

static void
mex_column_unmap (ClutterActor *actor)
{
  MxFocusManager *manager =
    mx_focus_manager_get_for_stage ((ClutterStage *)
      clutter_actor_get_stage (actor));
  g_signal_handlers_disconnect_by_func (manager,
                                        mex_column_notify_focused_cb,
                                        actor);

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->unmap (actor);
}

static gboolean
mex_column_get_paint_volume (ClutterActor       *self,
                             ClutterPaintVolume *volume)
{
  MexColumnPrivate *priv = MEX_COLUMN (self)->priv;
  ClutterVertex v;

  if (!clutter_paint_volume_set_from_allocation (volume, self))
    return FALSE;

  if (priv->adjustment)
    {
      clutter_paint_volume_get_origin (volume, &v);
      v.y += priv->adjustment_value,
      clutter_paint_volume_set_origin (volume, &v);
    }

  return TRUE;
}

static void
mex_column_class_init (MexColumnClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *o_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *a_class = CLUTTER_ACTOR_CLASS (klass);

  o_class->dispose = mex_column_dispose;
  o_class->finalize = mex_column_finalize;
  o_class->set_property = mex_column_set_property;
  o_class->get_property = mex_column_get_property;

  a_class->get_preferred_width  = mex_column_get_preferred_width;
  a_class->get_preferred_height = mex_column_get_preferred_height;
  a_class->allocate             = mex_column_allocate;
  a_class->apply_transform      = mex_column_apply_transform;
  a_class->paint                = mex_column_paint;
  a_class->pick                 = mex_column_pick;
  a_class->map                  = mex_column_map;
  a_class->unmap                = mex_column_unmap;
  a_class->get_paint_volume     = mex_column_get_paint_volume;

  g_type_class_add_private (klass, sizeof (MexColumnPrivate));

  pspec = g_param_spec_boolean ("collapse-on-focus",
                                "Collapse On Focus",
                                "Collapse items before the focused item.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_COLLAPSE_ON_FOCUS, pspec);

  pspec = g_param_spec_boolean ("opened",
                                "Opened",
                                "Whether the column has at least one open item.",
                                TRUE,
                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_OPENED, pspec);

  /* MxScrollable properties */
  g_object_class_override_property (o_class,
                                    PROP_HADJUST,
                                    "horizontal-adjustment");
  g_object_class_override_property (o_class,
                                    PROP_VADJUST,
                                    "vertical-adjustment");
}

static void
mex_column_init (MexColumn *self)
{
  MexColumnPrivate *priv = self->priv = GET_PRIVATE (self);

  /* Set the column as reactive and enable collapsing */
  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);
  priv->collapse = TRUE;
}


ClutterActor *
mex_column_new (void)
{
  return g_object_new (MEX_TYPE_COLUMN, NULL);
}

gboolean
mex_column_is_empty (MexColumn *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN (column), TRUE);
  return (column->priv->children == NULL);
}

void
mex_column_set_collapse_on_focus (MexColumn *column,
                                  gboolean   collapse)
{
  MexColumnPrivate *priv;

  g_return_if_fail (MEX_IS_COLUMN (column));

  priv = column->priv;
  if (priv->collapse != collapse)
    {
      ClutterActor *stage;

      priv->collapse = collapse;
      g_object_notify (G_OBJECT (column), "collapse-on-focus");

      if ((stage = clutter_actor_get_stage (CLUTTER_ACTOR (column))))
        {
          MxFocusManager *manager =
            mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));
          mex_column_notify_focused_cb (manager, NULL, column);
        }
    }
}

gboolean
mex_column_get_collapse_on_focus (MexColumn *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN (column), FALSE);
  return column->priv->collapse;
}

void
mex_column_set_focus (MexColumn *column, gboolean focus)
{
  MexColumnPrivate *priv;

  g_return_if_fail (MEX_IS_COLUMN (column));

  priv = column->priv;

  if (priv->has_focus != focus)
    {
      ClutterActor *stage;

      priv->has_focus = focus;
      priv->has_focus_changed = TRUE;

      if ((stage = clutter_actor_get_stage (CLUTTER_ACTOR (column))))
        {
          MxFocusManager *manager =
            mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));
          mex_column_notify_focused_cb (manager, NULL, column);
        }
    }
}

gboolean
mex_column_get_opened (MexColumn *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN (column), FALSE);

  return column->priv->open_boxes != 0;
}
