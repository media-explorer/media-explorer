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
#include "mex-content-box.h"
#include "mex-expander-box.h"
#include "mex-scroll-view.h"
#include "mex-tile.h"
#include "mex-shadow.h"

enum
{
  PROP_0,
  PROP_LABEL,
  PROP_ICON_NAME,
  PROP_PLACEHOLDER_ACTOR,
  PROP_HADJUST,
  PROP_VADJUST,
  PROP_COLLAPSE_ON_FOCUS
};

enum
{
  ACTIVATED,
  PLAY_REQUESTED,
  LAST_SIGNAL,
};

struct _MexColumnPrivate
{
  guint         has_focus : 1;
  guint         collapse : 1;

  ClutterActor *header;
  ClutterActor *button;
  ClutterActor *icon;
  ClutterActor *label;
  ClutterActor *current_focus;
  ClutterActor *placeholder_actor;

  ClutterTimeline *expand_timeline;

  GList            *children;
  guint             n_items;
  MexActorSortFunc  sort_func;
  gpointer          sort_data;

  MxAdjustment *adjustment;
};

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void mx_scrollable_iface_init (MxScrollableIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MEX_TYPE_COLUMN, MexColumnPrivate))
G_DEFINE_TYPE_WITH_CODE (MexColumn, mex_column, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_SCROLLABLE,
                                                mx_scrollable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

static guint32 signals[LAST_SIGNAL] = { 0, };

static GQuark
_item_shadow_quark ()
{
  static GQuark quark = 0;

  if (G_UNLIKELY (!quark))
    quark = g_quark_from_static_string ("mex-column-item-shadow");

  return quark;
}

static void
expander_box_open_notify (MexExpanderBox *box,
                          GParamSpec     *pspec,
                          MexColumn      *column)
{
  MexColumnPrivate *priv = MEX_COLUMN (column)->priv;
  GList *l;
  MexShadow *shadow;
  ClutterColor shadow_color = { 0, 0, 0, 128 };

  if (mex_expander_box_get_open (box))
    {
      for (l = priv->children; l; l = l->next)
        {
          if (l->data != box)
            clutter_actor_animate (l->data, CLUTTER_EASE_IN_OUT_QUAD, 200,
                                   "opacity", 56, NULL);
        }
      clutter_actor_animate (priv->header, CLUTTER_EASE_IN_OUT_QUAD, 200,
                             "opacity", 56, NULL);

      shadow = mex_shadow_new (CLUTTER_ACTOR (box));
      mex_shadow_set_paint_flags (shadow,
                                  MEX_TEXTURE_FRAME_TOP | MEX_TEXTURE_FRAME_BOTTOM);
      mex_shadow_set_radius_y (shadow, 25);
      mex_shadow_set_color (shadow, &shadow_color);

      g_object_set_qdata (G_OBJECT (box), _item_shadow_quark (), shadow);
    }
  else
    {
      /* restore all children to full opacity */
      for (l = priv->children; l; l = l->next)
        {
          clutter_actor_animate (l->data, CLUTTER_EASE_IN_OUT_QUAD, 200,
                                 "opacity", 255, NULL);
        }
      clutter_actor_animate (priv->header, CLUTTER_EASE_IN_OUT_QUAD, 200,
                             "opacity", 255, NULL);

      shadow = g_object_get_qdata (G_OBJECT (box), _item_shadow_quark ());
      if (shadow)
        {
          g_object_unref (shadow);
          g_object_set_qdata (G_OBJECT (box), _item_shadow_quark (), NULL);
        }
    }
}

/* ClutterContainerIface */

static void
mex_column_add (ClutterContainer *container,
                ClutterActor     *actor)
{
  MexColumn *self = MEX_COLUMN (container);
  MexColumnPrivate *priv = self->priv;

  if (priv->sort_func)
    priv->children =
      g_list_insert_sorted_with_data (priv->children, actor,
                                      (GCompareDataFunc)priv->sort_func,
                                      priv->sort_data);
  else
    priv->children = g_list_append (priv->children, actor);

  priv->n_items ++;

  /* Expand/collapse any drawer that gets added as appropriate */
  if (MEX_IS_EXPANDER_BOX (actor))
    {
      g_signal_connect (actor, "notify::open",
                        G_CALLBACK (expander_box_open_notify), container);

      mex_expander_box_set_important (MEX_EXPANDER_BOX (actor),
                                      priv->has_focus);

      if (MEX_IS_CONTENT_BOX (actor))
        {
          ClutterActor *tile =
            mex_content_box_get_tile (MEX_CONTENT_BOX (actor));
          mex_tile_set_important (MEX_TILE (tile), priv->has_focus);
        }
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

  /* Remove the old actor */
  g_object_ref (actor);

  clutter_actor_unparent (actor);
  priv->children = g_list_delete_link (priv->children, link_);
  priv->n_items --;

  /* children may not be at full opacity if the item removed was "open" */
  if (MEX_IS_EXPANDER_BOX (actor)
      && mex_expander_box_get_open (MEX_EXPANDER_BOX (actor)))
    {
      GList *l;

      for (l = priv->children; l; l = l->next)
        clutter_actor_animate (l->data, CLUTTER_EASE_IN_OUT_QUAD, 200,
                               "opacity", 255, NULL);

      clutter_actor_animate (priv->header, CLUTTER_EASE_IN_OUT_QUAD, 200,
                             "opacity", 255, NULL);
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
mex_column_set_adjustments (MxScrollable *scrollable,
                            MxAdjustment *hadjust,
                            MxAdjustment *vadjust)
{
  MexColumnPrivate *priv = MEX_COLUMN (scrollable)->priv;

  if (priv->adjustment == hadjust)
    return;

  if (priv->adjustment)
    {
      g_signal_handlers_disconnect_by_func (priv->adjustment,
                                            clutter_actor_queue_redraw,
                                            scrollable);
      g_object_unref (priv->adjustment);
    }

  priv->adjustment = hadjust;

  if (priv->adjustment)
    {
      g_object_ref (priv->adjustment);
      g_signal_connect_swapped (priv->adjustment, "notify::value",
                                G_CALLBACK (clutter_actor_queue_redraw),
                                scrollable);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (scrollable));
}

static void
mex_column_get_adjustments (MxScrollable  *scrollable,
                            MxAdjustment **hadjust,
                            MxAdjustment **vadjust)
{
  MexColumnPrivate *priv = MEX_COLUMN (scrollable)->priv;

  if (vadjust)
    *vadjust = NULL;

  if (!hadjust)
    return;

  if (!priv->adjustment)
    mx_scrollable_set_adjustments (scrollable, mx_adjustment_new (), NULL);

  *hadjust = priv->adjustment;
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

  if ((ClutterActor *)from == priv->header)
    {
      if (((direction == MX_FOCUS_DIRECTION_NEXT) ||
           (direction == MX_FOCUS_DIRECTION_DOWN)) &&
          priv->n_items)
        {
          hint = (direction == MX_FOCUS_DIRECTION_NEXT) ?
            MX_FOCUS_HINT_FIRST : MX_FOCUS_HINT_FROM_ABOVE;
          if ((focusable = mx_focusable_accept_focus (
                 MX_FOCUSABLE (priv->children->data), hint)))
            {
              priv->current_focus = priv->children->data;
              return focusable;
            }
        }
      else
        return NULL;
    }

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

      if (!link_)
        {
          if ((focusable = mx_focusable_accept_focus (
                 MX_FOCUSABLE (priv->header), hint)))
            priv->current_focus = priv->header;
        }
      else if ((focusable = mx_focusable_accept_focus (
                  MX_FOCUSABLE (link_->data), hint)))
        priv->current_focus = link_->data;
      break;

    case MX_FOCUS_DIRECTION_NEXT:
    case MX_FOCUS_DIRECTION_DOWN:
      hint = (direction == MX_FOCUS_DIRECTION_NEXT) ?
        MX_FOCUS_HINT_FIRST : MX_FOCUS_HINT_FROM_ABOVE;
      link_ = g_list_next (link_);

      if (link_ && (focusable = mx_focusable_accept_focus (
                     MX_FOCUSABLE (link_->data), hint)))
        priv->current_focus = link_->data;
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
      if ((focusable = mx_focusable_accept_focus (MX_FOCUSABLE (priv->header),
                                                  hint)))
        priv->current_focus = priv->header;
      else if (priv->n_items &&
               (focusable = mx_focusable_accept_focus (
                  MX_FOCUSABLE (priv->children->data), hint)))
        priv->current_focus = priv->children->data;
      break;

    case MX_FOCUS_HINT_LAST:
    case MX_FOCUS_HINT_FROM_BELOW:
      link_ = g_list_last (priv->children);
      if (link_ && (focusable = mx_focusable_accept_focus (
                     MX_FOCUSABLE (link_->data), hint)))
        {
          priv->current_focus = link_->data;
          break;
        }
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
    case PROP_LABEL:
      mex_column_set_label (self, g_value_get_string (value));
      break;

    case PROP_PLACEHOLDER_ACTOR:
      mex_column_set_placeholder_actor (self, g_value_get_object (value));
      break;

    case PROP_ICON_NAME:
      mex_column_set_icon_name (self, g_value_get_string (value));
      break;

    case PROP_HADJUST:
      mex_column_set_adjustments (MX_SCROLLABLE (self),
                                  g_value_get_object (value),
                                  self->priv->adjustment);
      break;

    case PROP_VADJUST:
      mex_column_set_adjustments (MX_SCROLLABLE (self), NULL,
                                  g_value_get_object (value));
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
    case PROP_LABEL:
      g_value_set_string (value, mex_column_get_label (self));
      break;

    case PROP_PLACEHOLDER_ACTOR:
      g_value_set_object (value, mex_column_get_placeholder_actor (self));
      break;

    case PROP_ICON_NAME:
      g_value_set_string (value, mex_column_get_icon_name (self));
      break;

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

  if (priv->header)
    {
      /* The header includes the label and icon */
      clutter_actor_destroy (priv->header);
      priv->header = NULL;
    }

  if (priv->expand_timeline)
    {
      g_object_unref (priv->expand_timeline);
      priv->expand_timeline = NULL;
    }

  if (priv->placeholder_actor)
    {
      clutter_actor_unparent (priv->placeholder_actor);
      priv->placeholder_actor = NULL;
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
  gfloat min_width, nat_width;
  gfloat min_header, nat_header;
  gfloat min_placeholder, nat_placeholder;

  MexColumn *self = MEX_COLUMN (actor);
  MexColumnPrivate *priv = self->priv;

  clutter_actor_get_preferred_width (priv->header,
                                     -1,
                                     &min_header,
                                     &nat_header);

  if (priv->adjustment)
    for_height = -1;
  else if (for_height >= 0)
    {
      gfloat height;
      clutter_actor_get_preferred_height (priv->header, -1, NULL, &height);
      for_height = MAX (0, for_height - height);
    }

  if (priv->n_items < 1)
    {
      if (priv->placeholder_actor)
        {
          clutter_actor_get_preferred_width (priv->placeholder_actor,
                                             for_height,
                                             &min_placeholder,
                                             &nat_placeholder);

          min_width = MAX (min_header, min_placeholder);
          nat_width = MAX (min_header, nat_placeholder);
        }
      else
        {
          min_width = min_header;
          nat_width = nat_header;
        }
    }
  else
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

      if (min_header > min_width)
        min_width = min_header;
      if (min_header > nat_width)
        nat_width = min_header;
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
  gfloat min_height, nat_height;
  MxPadding padding;

  MexColumn *self = MEX_COLUMN (actor);
  MexColumnPrivate *priv = self->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (for_width >= 0)
    for_width = MAX (0, for_width - padding.left - padding.right);

  clutter_actor_get_preferred_height (priv->header,
                                      for_width,
                                      NULL,
                                      &min_height);
  nat_height = min_height;

  if (priv->n_items < 1)
    {
      gfloat min_placeholder, nat_placeholder;

      clutter_actor_get_preferred_height (priv->placeholder_actor,
                                          for_width,
                                          &min_placeholder,
                                          &nat_placeholder);

      min_height += min_placeholder;
      nat_height += nat_placeholder;
    }
  else
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
  gfloat header_pref_height, pref_h, pref_w;
  ClutterActorBox child_box;
  MxPadding padding;
  GList *c;

  MexColumn *column = MEX_COLUMN (actor);
  MexColumnPrivate *priv = column->priv;

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  /* Allocate header */
  child_box.x1 = padding.left;
  child_box.x2 = box->x2 - box->x1 - padding.right;
  child_box.y1 = padding.top;

  clutter_actor_get_preferred_height (priv->header,
                                      child_box.x2 - child_box.x1,
                                      NULL,
                                      &header_pref_height);

  child_box.y2 = child_box.y1 + header_pref_height;

  clutter_actor_allocate (priv->header, &child_box, flags);

  /* Allocate placeholder actor */
  child_box.y1 = child_box.y2;

  /* keep the aspect ratio of the palceholder actor */
  clutter_actor_get_preferred_size (priv->placeholder_actor, NULL, NULL,
                                    &pref_w, &pref_h);
  pref_h = pref_h * ((child_box.x2 - child_box.x1) / pref_w);
  child_box.y2 = child_box.y1 + pref_h;
  clutter_actor_allocate (priv->placeholder_actor, &child_box, flags);


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
          pref_height -= padding.top + padding.bottom + header_pref_height;

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
      g_object_set (G_OBJECT (priv->adjustment),
                    "lower", 0.0,
                    "upper", child_box.y2 - padding.top,
                    "page-size", page_size,
                    "step-increment", 1.0,
                    "page-increment", page_size,
                    NULL);
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
                           -mx_adjustment_get_value (priv->adjustment), 0);
}

static void
mex_column_paint (ClutterActor *actor)
{
  GList *c;

  MexColumn *self = MEX_COLUMN (actor);
  MexColumnPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->paint (actor);

  if (priv->n_items < 1 && priv->placeholder_actor)
    clutter_actor_paint (priv->placeholder_actor);
  else
    {
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
    }

  clutter_actor_paint (priv->header);
}

static void
mex_column_pick (ClutterActor *actor, const ClutterColor *color)
{
  GList *c;

  MexColumn *self = MEX_COLUMN (actor);
  MexColumnPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_column_parent_class)->pick (actor, color);

  /* Don't pick children when we don't have focus */
  if (!priv->has_focus)
    return;

  for (c = priv->children; c; c = c->next)
    clutter_actor_paint (c->data);
  clutter_actor_paint (priv->header);
}

static void
mex_column_expand_drawer_cb (MexExpanderBox *box)
{
  mex_expander_box_set_important (box, TRUE);
}

static void
mex_column_notify_focused_cb (MxFocusManager *manager,
                              GParamSpec     *pspec,
                              MexColumn      *self)
{
  GList *c;
  guint offset, increment;
  ClutterActor *focused, *focused_cell;
  gboolean cell_has_focus, has_focus, open, set_tile_important;

  MexColumnPrivate *priv = self->priv;

  focused = (ClutterActor *)mx_focus_manager_get_focused (manager);

  /* Check if we have focus, and what child is focused */
  focused_cell = NULL;
  set_tile_important = FALSE;
  cell_has_focus = has_focus = FALSE;

  if (focused)
    {
      gboolean contains_column = FALSE;
      ClutterActor *parent = clutter_actor_get_parent (focused);
      while (parent)
        {
          if (parent == (ClutterActor *)self)
            {
              has_focus = TRUE;

              if (!priv->has_focus)
                {
                  set_tile_important = TRUE;
                  priv->has_focus = TRUE;
                }

              if (focused != priv->header)
                {
                  cell_has_focus = TRUE;
                  focused_cell = focused;
                }

              break;
            }
          else if (MEX_IS_COLUMN (parent))
            {
              contains_column = TRUE;
            }

          focused = parent;
          parent = clutter_actor_get_parent (focused);
        }

      if (!contains_column)
        has_focus = TRUE;
    }

  if (!has_focus && priv->has_focus)
    {
      priv->has_focus = FALSE;
      set_tile_important = TRUE;
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
      clutter_timeline_set_delay (priv->expand_timeline, 350);
    }

  /* Loop through children and set the expander box important/unimportant
   * as necessary, and if necessary, do the same for the tile inside the
   * expander-box.
   */
  open = has_focus && !cell_has_focus;
  for (c = priv->children; c; c = c->next)
    {
      gchar signal_name[32+16];
      ClutterActor *child = c->data;

      if (!priv->collapse || (child == focused_cell))
        open = TRUE;

      if (!MEX_IS_EXPANDER_BOX (child))
        continue;

      /* Note, 'marker-reached::' is 16 characters long */
      g_snprintf (signal_name, G_N_ELEMENTS (signal_name),
                  "marker-reached::%p", child);

      if (MEX_IS_CONTENT_BOX (child))
        {
          ClutterActor *tile =
            mex_content_box_get_tile (MEX_CONTENT_BOX (child));
          mex_tile_set_important (MEX_TILE (tile), priv->has_focus);
        }

      if (!open)
        {
          if (priv->expand_timeline)
            {
              if (clutter_timeline_has_marker (priv->expand_timeline,
                                               signal_name + 16))
                clutter_timeline_remove_marker (priv->expand_timeline,
                                                signal_name + 16);
              g_signal_handlers_disconnect_by_func (priv->expand_timeline,
                                                    mex_column_expand_drawer_cb,
                                                    child);
            }
          mex_expander_box_set_important (MEX_EXPANDER_BOX (child), FALSE);
        }
      else if (set_tile_important)
        {

          mex_expander_box_set_important (MEX_EXPANDER_BOX (child), FALSE);
          clutter_timeline_add_marker_at_time (priv->expand_timeline,
                                               signal_name + 16, offset);
          g_signal_connect_swapped (priv->expand_timeline, signal_name,
                                    G_CALLBACK (mex_column_expand_drawer_cb),
                                    child);

          offset += increment;
        }
      else
        mex_expander_box_set_important (MEX_EXPANDER_BOX (child), TRUE);
    }

  if (priv->expand_timeline && set_tile_important && (offset >= increment))
    clutter_timeline_start (priv->expand_timeline);
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
mex_column_captured_event (ClutterActor *actor,
                           ClutterEvent *event)
{
  MexColumnPrivate *priv = MEX_COLUMN (actor)->priv;

  if ((event->type == CLUTTER_BUTTON_PRESS) && !priv->has_focus)
    {
      mex_push_focus (MX_FOCUSABLE (actor));
      return TRUE;
    }

  return FALSE;
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

  a_class->get_preferred_width = mex_column_get_preferred_width;
  a_class->get_preferred_height = mex_column_get_preferred_height;
  a_class->allocate = mex_column_allocate;
  a_class->apply_transform = mex_column_apply_transform;
  a_class->paint = mex_column_paint;
  a_class->pick = mex_column_pick;
  a_class->map = mex_column_map;
  a_class->unmap = mex_column_unmap;
  a_class->captured_event = mex_column_captured_event;

  g_type_class_add_private (klass, sizeof (MexColumnPrivate));

  pspec = g_param_spec_string ("label",
                               "Label",
                               "Text used as the title for this column.",
                               "",
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (o_class, PROP_LABEL, pspec);

  pspec = g_param_spec_object ("placeholder-actor",
                               "Placeholder Actor",
                               "Actor used when this column is empty.",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_PLACEHOLDER_ACTOR, pspec);

  pspec = g_param_spec_string ("icon-name",
                               "Icon Name",
                               "Icon name used by the icon for this column.",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (o_class, PROP_ICON_NAME, pspec);

  pspec = g_param_spec_boolean ("collapse-on-focus",
                                "Collapse On Focus",
                                "Collapse items before the focused item.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_COLLAPSE_ON_FOCUS, pspec);

  /* MxScrollable properties */
  g_object_class_override_property (o_class,
                                    PROP_HADJUST,
                                    "horizontal-adjustment");
  g_object_class_override_property (o_class,
                                    PROP_VADJUST,
                                    "vertical-adjustment");

  signals[ACTIVATED] = g_signal_new ("activated",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_LAST,
                                     G_STRUCT_OFFSET (MexColumnClass,
                                                      activated),
                                     NULL, NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE, 0);
}

static void
mex_column_header_clicked_cb (MxButton *button, MexColumn *self)
{
  g_signal_emit (self, signals[ACTIVATED], 0);
}

static void
mex_column_init (MexColumn *self)
{
  ClutterActor *box;
  MexColumnPrivate *priv = self->priv = GET_PRIVATE (self);

  /* Begin private children */
  clutter_actor_push_internal (CLUTTER_ACTOR (self));

  /* Create the header */
  priv->header = mx_box_layout_new ();
  mx_box_layout_set_orientation ((MxBoxLayout *) priv->header,
                                 MX_ORIENTATION_HORIZONTAL);
  mx_stylable_set_style_class (MX_STYLABLE (priv->header), "Header");

  clutter_actor_push_internal (CLUTTER_ACTOR (self));
  clutter_actor_set_parent (priv->header, CLUTTER_ACTOR (self));
  clutter_actor_pop_internal (CLUTTER_ACTOR (self));

  priv->button = mx_button_new ();
  priv->icon = mx_icon_new ();
  priv->label = mx_label_new ();
  g_object_set (priv->label, "clip-to-allocation", TRUE, "fade-out", TRUE,
                NULL);

  box = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (box), 8);
  clutter_container_add (CLUTTER_CONTAINER (box),
                         priv->icon,
                         priv->label,
                         NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->icon,
                               "expand", FALSE,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", FALSE,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->label,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "x-align", MX_ALIGN_START,
                               "y-fill", FALSE,
                               NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button), box);

  mx_bin_set_fill (MX_BIN (priv->button), TRUE, FALSE);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->header), priv->button);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->header), priv->button,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "x-align", MX_ALIGN_START,
                               "y-fill", TRUE,
                               NULL);

  g_signal_connect (priv->button, "clicked",
                    G_CALLBACK (mex_column_header_clicked_cb), self);

  /* End of private children */
  clutter_actor_pop_internal (CLUTTER_ACTOR (self));

  /* Set the column as reactive and enable collapsing */
  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);
  priv->collapse = TRUE;
}


ClutterActor *
mex_column_new (const char *label,
                const char *icon_name)
{
  return g_object_new (MEX_TYPE_COLUMN,
                       "label", label,
                       "icon-name", icon_name,
                       NULL);
}

const gchar *
mex_column_get_label (MexColumn *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN (column), NULL);
  return mx_label_get_text (MX_LABEL (column->priv->label));
}

void
mex_column_set_label (MexColumn *column, const gchar *label)
{
  g_return_if_fail (MEX_IS_COLUMN (column));
  mx_label_set_text (MX_LABEL (column->priv->label), label ? label : "");
}

ClutterActor*
mex_column_get_placeholder_actor (MexColumn *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN (column), NULL);

  return column->priv->placeholder_actor;
}

void
mex_column_set_placeholder_actor (MexColumn    *column,
                                  ClutterActor *actor)
{
  MexColumnPrivate *priv = column->priv;

  g_return_if_fail (MEX_IS_COLUMN (column));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  /* placeholder label */
  if (priv->placeholder_actor)
    clutter_actor_unparent (priv->placeholder_actor);

  priv->placeholder_actor = actor;
  clutter_actor_push_internal (CLUTTER_ACTOR (column));
  clutter_actor_set_parent (priv->placeholder_actor, CLUTTER_ACTOR (column));
  clutter_actor_pop_internal (CLUTTER_ACTOR (column));

  clutter_actor_queue_relayout (CLUTTER_ACTOR (column));
}


const gchar *
mex_column_get_icon_name (MexColumn *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN (column), NULL);
  return mx_icon_get_icon_name (MX_ICON (column->priv->icon));
}

void
mex_column_set_icon_name (MexColumn *column, const gchar *name)
{
  g_return_if_fail (MEX_IS_COLUMN (column));
  mx_icon_set_icon_name (MX_ICON (column->priv->icon), name);
}

void
mex_column_set_sort_func (MexColumn        *column,
                          MexActorSortFunc  func,
                          gpointer          userdata)
{
  MexColumnPrivate *priv;

  g_return_if_fail (MEX_IS_COLUMN (column));

  priv = column->priv;
  if ((priv->sort_func != func) || (priv->sort_data != userdata))
    {
      priv->sort_func = func;
      priv->sort_data = userdata;

      if (func)
        {
          priv->children = g_list_sort_with_data (priv->children,
                                                  (GCompareDataFunc)func,
                                                  userdata);
          clutter_actor_queue_relayout (CLUTTER_ACTOR (column));
        }
    }
}

MexActorSortFunc
mex_column_get_sort_func (MexColumn *column,
                          gpointer  *userdata)
{
  g_return_val_if_fail (MEX_IS_COLUMN (column), NULL);

  if (userdata)
    *userdata = column->priv->sort_data;

  return column->priv->sort_func;
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
