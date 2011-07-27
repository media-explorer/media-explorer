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


#include "mex-expander-box.h"
#include "mex-enum-types.h"

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexExpanderBox, mex_expander_box, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define EXPANDER_BOX_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_EXPANDER_BOX, MexExpanderBoxPrivate))

enum
{
  PROP_0,

  PROP_IMPORTANT,
  PROP_OPEN,
  PROP_OPEN_ON_FOCUS,
  PROP_CLOSE_ON_UNFOCUS,
  PROP_IMPORTANT_ON_FOCUS,
  PROP_PRIMARY_CHILD,
  PROP_SECONDARY_CHILD,
  PROP_GROW_DIRECTION,
  PROP_EXPAND
};

struct _MexExpanderBoxPrivate
{
  guint open               : 1;
  guint open_on_focus      : 1;
  guint close_on_unfocus   : 1;
  guint has_focus          : 1;
  guint expand             : 1;
  guint important          : 1;
  guint important_on_focus : 1;
  guint notify_open        : 1;

  MexExpanderBoxDirection direction;

  ClutterAlpha *open_alpha;
  ClutterAlpha *expand_alpha;
  ClutterTimeline *expand_timeline;
  ClutterTimeline *open_timeline;

  ClutterActor *primary;
  ClutterActor *secondary;

  gfloat max_widths[2];
  gfloat max_heights[2];
};

static void mex_expander_box_timeline_completed_cb (ClutterTimeline *timeline,
                                                    ClutterActor    *box);

/* ClutterContainerIface */

static void
mex_expander_box_add (ClutterContainer *container,
                      ClutterActor     *actor)
{
  MexExpanderBox *self = MEX_EXPANDER_BOX (container);
  MexExpanderBoxPrivate *priv = self->priv;

  if (!priv->primary)
    mex_expander_box_set_primary_child (self, actor);
  else if (!priv->secondary)
    mex_expander_box_set_secondary_child (self, actor);
  else
    g_warning (G_STRLOC ": Trying to add actor to full drawer");
}

static void
mex_expander_box_remove (ClutterContainer *container,
                         ClutterActor     *actor)
{
  MexExpanderBox *self = MEX_EXPANDER_BOX (container);
  MexExpanderBoxPrivate *priv = self->priv;

  if (clutter_actor_get_parent (actor) == (ClutterActor *)self)
    {
      if (priv->primary == actor)
        priv->primary = NULL;
      else if (priv->secondary == actor)
        priv->secondary = NULL;
      else
        g_warning (G_STRLOC ": Drawer contains orphaned child");

      g_object_ref (actor);
      clutter_actor_unparent (actor);

      g_signal_emit_by_name (self, "actor-removed", actor);
      g_object_unref (actor);
    }
  else
    g_warning (G_STRLOC ": Drawer is not the parent of the given child");
}

static void
mex_expander_box_foreach (ClutterContainer *container,
                          ClutterCallback   callback,
                          gpointer          user_data)
{
  MexExpanderBox *self = MEX_EXPANDER_BOX (container);
  MexExpanderBoxPrivate *priv = self->priv;

  if (priv->primary)
    callback (priv->primary, user_data);
  if (priv->secondary)
    callback (priv->secondary, user_data);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = mex_expander_box_add;
  iface->remove = mex_expander_box_remove;
  iface->foreach = mex_expander_box_foreach;
}

/* MxFocusableIface */

static void
mex_expander_box_open_primary (MexExpanderBox *box,
                               gboolean        open)
{
  MexExpanderBoxPrivate *priv = box->priv;
  ClutterTimelineDirection direction = open ?
    CLUTTER_TIMELINE_FORWARD : CLUTTER_TIMELINE_BACKWARD;

  if (!priv->primary || !CLUTTER_ACTOR_IS_MAPPED (box))
    {
      /* Reset the expand timeline to the right position */
      clutter_timeline_set_direction (priv->expand_timeline, direction);
      mex_expander_box_timeline_completed_cb (priv->expand_timeline,
                                             CLUTTER_ACTOR (box));

      return;
    }

  if ((!open) && ((priv->has_focus && priv->important_on_focus) ||
                  priv->important))
    return;

  if (open)
    {
      gfloat min_height, pref_height;

      clutter_actor_get_preferred_height (priv->primary,
                                          -1,
                                          &min_height,
                                          &pref_height);

      /* Check if the centre child is large enough to open.
       * If its preferred height isn't greater than its minimum
       * height, running this animation would do nothing. Dropping
       * out here means that if the centre child becomes larger,
       * such as when updating a thumbnail, the size increase will
       * animate.
       */
      if (pref_height <= min_height)
        return;
    }

  if (clutter_timeline_is_playing (priv->expand_timeline))
    clutter_timeline_set_direction (priv->expand_timeline,
                                    direction);
  else
    {
      gdouble progress =
        clutter_timeline_get_progress (priv->expand_timeline);

      if ((open && (progress < 1.0)) ||
          (!open && (progress > 0.0)))
        {
          clutter_timeline_set_direction (priv->expand_timeline, direction);
          clutter_timeline_rewind (priv->expand_timeline);
          clutter_timeline_start (priv->expand_timeline);
        }
    }
}

static MxFocusable *
mex_expander_box_move_focus (MxFocusable      *focusable,
                             MxFocusDirection  direction,
                             MxFocusable      *from)
{
  MexExpanderBox *self = MEX_EXPANDER_BOX (focusable);
  MexExpanderBoxPrivate *priv = self->priv;
  ClutterActor *current = CLUTTER_ACTOR (from);
  gboolean switch_focus = FALSE;

  if (!priv->primary || !priv->secondary ||
      !CLUTTER_ACTOR_IS_VISIBLE (priv->primary) ||
      !CLUTTER_ACTOR_IS_VISIBLE (priv->secondary) ||
      !MX_IS_FOCUSABLE (priv->primary) ||
      !MX_IS_FOCUSABLE (priv->secondary))
    return NULL;

  if ((current == priv->primary) && !priv->open)
    return NULL;

  switch (direction)
    {
    case MX_FOCUS_DIRECTION_UP:
      if ((current == priv->primary &&
           priv->direction == MEX_EXPANDER_BOX_UP) ||
          (current == priv->secondary &&
           priv->direction == MEX_EXPANDER_BOX_DOWN))
        switch_focus = TRUE;
      break;

    case MX_FOCUS_DIRECTION_DOWN:
      if ((current == priv->primary &&
           priv->direction == MEX_EXPANDER_BOX_DOWN) ||
          (current == priv->secondary &&
           priv->direction == MEX_EXPANDER_BOX_UP))
        switch_focus = TRUE;
      break;

    case MX_FOCUS_DIRECTION_LEFT:
      if ((current == priv->primary &&
           priv->direction == MEX_EXPANDER_BOX_LEFT) ||
          (current == priv->secondary &&
           priv->direction == MEX_EXPANDER_BOX_RIGHT))
        switch_focus = TRUE;
      break;

    case MX_FOCUS_DIRECTION_RIGHT:
      if ((current == priv->primary &&
           priv->direction == MEX_EXPANDER_BOX_RIGHT) ||
          (current == priv->secondary &&
           priv->direction == MEX_EXPANDER_BOX_LEFT))
        switch_focus = TRUE;
      break;

    case MX_FOCUS_DIRECTION_PREVIOUS:
      if (current == priv->secondary)
        switch_focus = TRUE;
      break;

    case MX_FOCUS_DIRECTION_NEXT:
      if (current == priv->primary)
        switch_focus = TRUE;
      break;

    case MX_FOCUS_DIRECTION_OUT:
    default:
      break;
    }

  if (switch_focus)
    {
      if (current == priv->primary)
        return mx_focusable_accept_focus (MX_FOCUSABLE (priv->secondary),
                                          MX_FOCUS_HINT_FIRST);
      else
        return mx_focusable_accept_focus (MX_FOCUSABLE (priv->primary),
                                          MX_FOCUS_HINT_FIRST);
    }

  return NULL;
}

static MxFocusable *
mex_expander_box_accept_focus (MxFocusable *focusable,
                               MxFocusHint  hint)
{
  MexExpanderBox *self = MEX_EXPANDER_BOX (focusable);
  MexExpanderBoxPrivate *priv = self->priv;

  focusable = NULL;

  if (priv->primary && MX_IS_FOCUSABLE (priv->primary) &&
      (!MX_IS_WIDGET (priv->primary) ||
       !mx_widget_get_disabled (MX_WIDGET (priv->primary))))
    focusable = mx_focusable_accept_focus (MX_FOCUSABLE (priv->primary),
                                           hint);
  if (!focusable && priv->secondary && MX_IS_FOCUSABLE (priv->secondary) &&
      (!MX_IS_WIDGET (priv->secondary) ||
       !mx_widget_get_disabled (MX_WIDGET (priv->secondary))))
    focusable = mx_focusable_accept_focus (MX_FOCUSABLE (priv->secondary),
                                           hint);

  return focusable;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_expander_box_move_focus;
  iface->accept_focus = mex_expander_box_accept_focus;
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
mex_expander_box_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MexExpanderBox *self = MEX_EXPANDER_BOX (object);

  switch (property_id)
    {
    case PROP_IMPORTANT:
      g_value_set_boolean (value, mex_expander_box_get_important (self));
      break;

    case PROP_IMPORTANT_ON_FOCUS:
      g_value_set_boolean (value,
                           mex_expander_box_get_important_on_focus (self));
      break;

    case PROP_OPEN:
      g_value_set_boolean (value, mex_expander_box_get_open (self));
      break;

    case PROP_OPEN_ON_FOCUS:
      g_value_set_boolean (value, mex_expander_box_get_open_on_focus (self));
      break;

    case PROP_CLOSE_ON_UNFOCUS:
      g_value_set_boolean (value, mex_expander_box_get_close_on_unfocus (self));
      break;

    case PROP_PRIMARY_CHILD:
      g_value_set_object (value, mex_expander_box_get_primary_child (self));
      break;

    case PROP_SECONDARY_CHILD:
      g_value_set_object (value, mex_expander_box_get_secondary_child (self));
      break;

    case PROP_GROW_DIRECTION:
      g_value_set_enum (value, mex_expander_box_get_grow_direction (self));
      break;

    case PROP_EXPAND:
      g_value_set_boolean (value, mex_expander_box_get_expand (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_expander_box_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MexExpanderBox *self = MEX_EXPANDER_BOX (object);

  switch (property_id)
    {
    case PROP_IMPORTANT:
      mex_expander_box_set_important (self, g_value_get_boolean (value));
      break;

    case PROP_IMPORTANT_ON_FOCUS:
      mex_expander_box_set_important_on_focus (self,
                                              g_value_get_boolean (value));
      break;

    case PROP_OPEN:
      mex_expander_box_set_open (self, g_value_get_boolean (value));
      break;

    case PROP_OPEN_ON_FOCUS:
      mex_expander_box_set_open_on_focus (self, g_value_get_boolean (value));
      break;

    case PROP_CLOSE_ON_UNFOCUS:
      mex_expander_box_set_close_on_unfocus (self, g_value_get_boolean (value));
      break;

    case PROP_PRIMARY_CHILD:
      mex_expander_box_set_primary_child (self,
                                 (ClutterActor *)g_value_get_object (value));
      break;

    case PROP_SECONDARY_CHILD:
      mex_expander_box_set_secondary_child (self,
                                 (ClutterActor *)g_value_get_object (value));
      break;

    case PROP_GROW_DIRECTION:
      mex_expander_box_set_grow_direction (self, g_value_get_enum (value));
      break;

    case PROP_EXPAND:
      mex_expander_box_set_expand (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_expander_box_dispose (GObject *object)
{
  MexExpanderBoxPrivate *priv = MEX_EXPANDER_BOX (object)->priv;

  if (priv->open_alpha)
    {
      g_object_unref (priv->open_alpha);
      priv->open_alpha = NULL;
    }

  if (priv->expand_alpha)
    {
      g_object_unref (priv->expand_alpha);
      priv->expand_alpha = NULL;
    }

  if (priv->open_timeline)
    {
      clutter_timeline_stop (priv->open_timeline);
      g_object_unref (priv->open_timeline);
      priv->open_timeline = NULL;
    }

  if (priv->expand_timeline)
    {
      clutter_timeline_stop (priv->expand_timeline);
      g_object_unref (priv->expand_timeline);
      priv->expand_timeline = NULL;
    }

  G_OBJECT_CLASS (mex_expander_box_parent_class)->dispose (object);
}

static void
mex_expander_box_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_expander_box_parent_class)->finalize (object);
}

static void
mex_expander_box_destroy (ClutterActor *actor)
{
  MexExpanderBoxPrivate *priv = MEX_EXPANDER_BOX (actor)->priv;

  if (priv->primary)
    clutter_actor_destroy (priv->primary);
  if (priv->secondary)
    clutter_actor_destroy (priv->secondary);
}

static void
mex_expander_box_get_preferred_width (ClutterActor *actor,
                                      gfloat        for_height,
                                      gfloat       *min_width_p,
                                      gfloat       *nat_width_p)
{
  MxPadding padding;
  gdouble open_progress;
  gfloat total_min_width, total_nat_width, min_width[2], nat_width[2];

  MexExpanderBoxPrivate *priv = MEX_EXPANDER_BOX (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  min_width[0] = nat_width[0] = min_width[1] = nat_width[1] = 0;

  open_progress = clutter_alpha_get_alpha (priv->open_alpha);

  if (!(priv->expand && (priv->direction == MEX_EXPANDER_BOX_LEFT ||
                         priv->direction == MEX_EXPANDER_BOX_RIGHT)))
    for_height = -1;
  else if (for_height > 0)
    for_height = MAX (0, for_height - (padding.top + padding.bottom));

  if (priv->primary)
    {
      clutter_actor_get_preferred_width (priv->primary,
                                         for_height,
                                         &min_width[0],
                                         &nat_width[0]);

      if (priv->max_widths[0] >= 0 &&
          nat_width[0] > priv->max_widths[0])
        nat_width[0] = priv->max_widths[0];

      if (nat_width[0] < min_width[0])
        nat_width[0] = min_width[0];
    }

  if (priv->secondary && (priv->direction == MEX_EXPANDER_BOX_UP ||
                          priv->direction == MEX_EXPANDER_BOX_DOWN ||
                          open_progress > 0.0))
    {
      if ((!priv->expand || (for_height < 0)) && priv->primary)
        clutter_actor_get_preferred_height (priv->primary,
                                            nat_width[0],
                                            NULL,
                                            &for_height);

      clutter_actor_get_preferred_width (priv->secondary,
                                         for_height,
                                         &min_width[1],
                                         &nat_width[1]);

      if (priv->max_widths[1] >= 0 &&
          nat_width[1] > priv->max_widths[1])
        nat_width[1] = priv->max_widths[1];

      if (nat_width[1] < min_width[1])
        nat_width[1] = min_width[1];
    }

  total_min_width = MAX (min_width[0], min_width[1]);

  switch (priv->direction)
    {
    case MEX_EXPANDER_BOX_UP:
    case MEX_EXPANDER_BOX_DOWN:
      total_nat_width = MAX (nat_width[0], min_width[1]);
      break;

    case MEX_EXPANDER_BOX_LEFT:
    case MEX_EXPANDER_BOX_RIGHT:
      total_nat_width = nat_width[0] + (open_progress * nat_width[1]);
      break;
    }

  if (min_width_p)
    *min_width_p = (gint)total_min_width + padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p = (gint)total_nat_width + padding.left + padding.right;
}

static void
mex_expander_box_get_preferred_height (ClutterActor *actor,
                                       gfloat        for_width,
                                       gfloat       *min_height_p,
                                       gfloat       *nat_height_p)
{
  MxPadding padding;
  gdouble expand_progress, open_progress;
  gfloat total_min_height, total_nat_height, min_height[2], nat_height[2];

  MexExpanderBoxPrivate *priv = MEX_EXPANDER_BOX (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  min_height[0] = nat_height[0] = min_height[1] = nat_height[1] = 0;

  expand_progress = clutter_alpha_get_alpha (priv->expand_alpha);
  open_progress = clutter_alpha_get_alpha (priv->open_alpha);

  if (!(priv->expand && (priv->direction == MEX_EXPANDER_BOX_UP ||
                         priv->direction == MEX_EXPANDER_BOX_DOWN)))
    for_width = -1;
  else if (for_width > 0)
    for_width = MAX (0, for_width - (padding.left + padding.right));

  if (priv->primary)
    {
      clutter_actor_get_preferred_height (priv->primary,
                                          for_width,
                                          &min_height[0],
                                          &nat_height[0]);

      if (priv->max_heights[0] >= 0 &&
          nat_height[0] > priv->max_heights[0])
        nat_height[0] = priv->max_heights[0];

      if (nat_height[0] < min_height[0])
        nat_height[0] = min_height[0];
    }

  if (priv->secondary && (open_progress > 0.0 ||
                          ((priv->direction == MEX_EXPANDER_BOX_LEFT ||
                            priv->direction == MEX_EXPANDER_BOX_RIGHT) &&
                           expand_progress > 0.0)))
    {
      if ((!priv->expand || (for_width < 0)) && priv->primary)
        clutter_actor_get_preferred_width (priv->primary,
                                           -1,
                                           NULL,
                                           &for_width);

      clutter_actor_get_preferred_height (priv->secondary,
                                          for_width,
                                          &min_height[1],
                                          &nat_height[1]);

      if (priv->max_heights[1] >= 0 &&
          nat_height[1] > priv->max_heights[1])
        nat_height[1] = priv->max_heights[1];

      if (nat_height[1] < min_height[1])
        nat_height[1] = min_height[1];
    }

  total_nat_height = (min_height[0] * (1.0 - expand_progress)) +
                     (nat_height[0] * expand_progress);

  total_min_height = MAX (min_height[0], min_height[1] * open_progress);

  switch (priv->direction)
    {
    case MEX_EXPANDER_BOX_UP:
    case MEX_EXPANDER_BOX_DOWN:
      total_nat_height += (open_progress * nat_height[1]);
      break;

    case MEX_EXPANDER_BOX_LEFT:
    case MEX_EXPANDER_BOX_RIGHT:
      total_nat_height = MAX (total_nat_height, nat_height[1] * open_progress);
      break;
    }

  if (min_height_p)
    *min_height_p = (gint)total_min_height + padding.top + padding.bottom;
  if (nat_height_p)
    *nat_height_p = (gint)total_nat_height + padding.top + padding.bottom;
}

static void
mex_expander_box_allocate (ClutterActor           *actor,
                           const ClutterActorBox  *box,
                           ClutterAllocationFlags  flags)
{
  MxPadding padding;
  ClutterActorBox child_box;
  gdouble expand_progress, open_progress;
  gfloat available_width, available_height,
         primary_width, primary_height, primary_min_height,
         secondary_width, secondary_height;

  MexExpanderBox *self = MEX_EXPANDER_BOX (actor);
  MexExpanderBoxPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_expander_box_parent_class)->
    allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  available_width = box->x2 - box->x1 - padding.left - padding.right;
  available_height = box->y2 - box->y1 - padding.top - padding.bottom;

  if (priv->primary)
    clutter_actor_get_preferred_size (priv->primary,
                                      NULL, &primary_min_height,
                                      &primary_width, &primary_height);
  else
    primary_width = primary_height = 0;

  open_progress = clutter_alpha_get_alpha (priv->open_alpha);
  expand_progress = clutter_alpha_get_alpha (priv->expand_alpha);

  /* Work out the target size of the primary actor. If we're set to
   * expand, this will be modified when working out the size of the
   * secondary actor.
   */
  if (priv->expand)
    {
      primary_width = available_width;
      primary_height = (primary_min_height * (1.0 - expand_progress)) +
                       (available_height * expand_progress);
    }
  else
    {
      if (available_width < primary_width)
        primary_width = available_width;
      primary_height = (primary_min_height * (1.0 - expand_progress)) +
                       (primary_height * expand_progress);
    }

  if (primary_width > available_width)
    primary_width = available_width;
  if (primary_height > available_height)
    primary_height = available_height;

  if (priv->secondary && open_progress > 0)
    {
      switch (priv->direction)
        {
        case MEX_EXPANDER_BOX_UP:
        case MEX_EXPANDER_BOX_DOWN:
          clutter_actor_get_preferred_width (priv->secondary, -1,
                                             &secondary_width, NULL);
          if (secondary_width < primary_width)
            secondary_width = primary_width;

          clutter_actor_get_preferred_height (priv->secondary,
                                              secondary_width,
                                              NULL,
                                              &secondary_height);

          if (secondary_height > available_height)
            secondary_height = available_height;

          if (priv->expand)
            {
              primary_height = (available_height * (1.0 - open_progress)) +
                ((available_height - secondary_height) * open_progress);
            }
          else
            {
              if (secondary_height > available_height -
                                     (primary_height * open_progress))
                secondary_height = available_height -
                                   (primary_height * open_progress);
            }
          break;

        case MEX_EXPANDER_BOX_LEFT:
        case MEX_EXPANDER_BOX_RIGHT:
          clutter_actor_get_preferred_height (priv->secondary, -1,
                                              &secondary_height, NULL);
          if (secondary_height < primary_height)
            secondary_height = primary_height;

          clutter_actor_get_preferred_width (priv->secondary,
                                             secondary_height,
                                             NULL,
                                             &secondary_width);

          if (secondary_width > available_width)
            secondary_width = available_width;

          if (priv->expand)
            {
              primary_width = (available_width * (1.0 - open_progress)) +
                ((available_width - secondary_width) * open_progress);
            }
          else
            {
              if (secondary_width > available_width -
                                    (primary_width * open_progress))
                secondary_width = available_width -
                                  (primary_width * open_progress);
            }
          break;
        }

      switch (priv->direction)
        {
        case MEX_EXPANDER_BOX_LEFT:
        case MEX_EXPANDER_BOX_UP:
          child_box.x1 = padding.left;
          child_box.y1 = padding.top;
          break;

        case MEX_EXPANDER_BOX_DOWN:
          child_box.x1 = padding.left;
          child_box.y1 = (gint) primary_height ;
          clutter_actor_set_clip (priv->secondary, 0, 0, secondary_width,
                                  secondary_height * open_progress);
          break;

        case MEX_EXPANDER_BOX_RIGHT:
          child_box.x1 = (gint) primary_width ;
          child_box.y1 = padding.top;
          clutter_actor_set_clip (priv->secondary, 0, 0,
                                  secondary_width * open_progress,
                                  secondary_height);
          break;
        }


      child_box.x2 = child_box.x1 + secondary_width;
      child_box.y2 = child_box.y1 + secondary_height;
      clutter_actor_allocate (priv->secondary, &child_box, flags);
    }
  else
    secondary_width = secondary_height = 0;

  if (priv->primary)
    {
      switch (priv->direction)
        {
        case MEX_EXPANDER_BOX_RIGHT:
        case MEX_EXPANDER_BOX_DOWN:
          child_box.x1 = padding.left;
          child_box.y1 = padding.top;
          break;

        case MEX_EXPANDER_BOX_UP:
          child_box.x1 = padding.left;
          child_box.y1 = padding.top + (gint)(secondary_height * open_progress);
          break;

        case MEX_EXPANDER_BOX_LEFT:
          child_box.x1 = padding.left + (gint)(secondary_width * open_progress);
          child_box.y1 = padding.top;
          break;
        }

      child_box.x2 = child_box.x1 + primary_width;
      child_box.y2 = child_box.y1 + primary_height;
      clutter_actor_allocate (priv->primary, &child_box, flags);
    }
}

static void
mex_expander_box_paint (ClutterActor *actor)
{
  MexExpanderBox *self = MEX_EXPANDER_BOX (actor);
  MexExpanderBoxPrivate *priv = self->priv;

  /* Chain up for background */
  CLUTTER_ACTOR_CLASS (mex_expander_box_parent_class)->paint (actor);

  if (priv->secondary)
    {
      if (clutter_alpha_get_alpha (priv->open_alpha) > 0.0)
        clutter_actor_paint (priv->secondary);
    }

  if (priv->primary)
    clutter_actor_paint (priv->primary);

  if (priv->important || (priv->has_focus && priv->important_on_focus))
    mex_expander_box_open_primary (self, TRUE);
}

static void
mex_expander_box_pick (ClutterActor       *actor,
                       const ClutterColor *color)
{
  MexExpanderBoxPrivate *priv = MEX_EXPANDER_BOX (actor)->priv;

  /* Chain up for background rect */
  CLUTTER_ACTOR_CLASS (mex_expander_box_parent_class)->pick (actor, color);

  if (priv->secondary)
    {
      if (clutter_alpha_get_alpha (priv->open_alpha) > 0.0)
        clutter_actor_paint (priv->secondary);
    }

  if (priv->primary)
    clutter_actor_paint (priv->primary);
}

static void
mex_expander_box_notify_focused_cb (MxFocusManager *manager,
                                    GParamSpec     *pspec,
                                    MexExpanderBox *self)
{
  ClutterActor *focused;

  MexExpanderBoxPrivate *priv = self->priv;

  focused = (ClutterActor *)mx_focus_manager_get_focused (manager);

  /* If we gain focus, open the drawer (only open the centre if it's
   * the centre that's gained focus, otherwise open the draw completely).
   */
  if (focused)
    {
      ClutterActor *parent = clutter_actor_get_parent (focused);
      while (parent)
        {
          if (parent == (ClutterActor *)self)
            {
              if (!priv->has_focus)
                {
                  priv->has_focus = TRUE;
                  mx_stylable_style_pseudo_class_add (MX_STYLABLE (self),
                                                      "active");

                  if (priv->open_on_focus)
                    mex_expander_box_set_open (self, TRUE);
                  else if (priv->important_on_focus)
                    mex_expander_box_open_primary (self, TRUE);
                }

              return;
            }

          focused = parent;
          parent = clutter_actor_get_parent (focused);
        }
    }

  if (priv->has_focus)
    {
      priv->has_focus = FALSE;
      if (priv->open_on_focus || priv->close_on_unfocus)
        mex_expander_box_set_open (self, FALSE);
      if (priv->important_on_focus && !priv->important)
        mex_expander_box_open_primary (self, FALSE);

      mx_stylable_style_pseudo_class_remove (MX_STYLABLE (self), "active");
    }
}

static void
mex_expander_box_map (ClutterActor *actor)
{
  MxFocusManager *manager;

  CLUTTER_ACTOR_CLASS (mex_expander_box_parent_class)->map (actor);

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    {
      g_signal_connect (manager, "notify::focused",
                        G_CALLBACK (mex_expander_box_notify_focused_cb), actor);
      mex_expander_box_notify_focused_cb (manager, NULL,
                                          (MexExpanderBox *)actor);
    }
}

static void
mex_expander_box_unmap (ClutterActor *actor)
{
  MxFocusManager *manager;

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    g_signal_handlers_disconnect_by_func (manager,
                                          mex_expander_box_notify_focused_cb,
                                          actor);

  CLUTTER_ACTOR_CLASS (mex_expander_box_parent_class)->unmap (actor);
}

static void
mex_expander_box_class_init (MexExpanderBoxClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexExpanderBoxPrivate));

  object_class->get_property = mex_expander_box_get_property;
  object_class->set_property = mex_expander_box_set_property;
  object_class->dispose = mex_expander_box_dispose;
  object_class->finalize = mex_expander_box_finalize;

  actor_class->get_preferred_width = mex_expander_box_get_preferred_width;
  actor_class->get_preferred_height = mex_expander_box_get_preferred_height;
  actor_class->allocate = mex_expander_box_allocate;
  actor_class->paint = mex_expander_box_paint;
  actor_class->pick = mex_expander_box_pick;
  actor_class->map = mex_expander_box_map;
  actor_class->unmap = mex_expander_box_unmap;
  actor_class->destroy = mex_expander_box_destroy;

  pspec = g_param_spec_boolean ("important",
                                "Important",
                                "Whether the centre child should be "
                                "displayed, regardless of focus.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_IMPORTANT, pspec);

  pspec = g_param_spec_boolean ("important-on-focus",
                                "Important on focus",
                                "Mark the box as important when it receives "
                                "focus.",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_IMPORTANT_ON_FOCUS,
                                   pspec);

  pspec = g_param_spec_boolean ("open",
                                "Open",
                                "Whether the expander are open.",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_OPEN, pspec);

  pspec = g_param_spec_boolean ("open-on-focus",
                                "Open on focus",
                                "Open the expander when it receives focus.",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_OPEN_ON_FOCUS, pspec);

  pspec = g_param_spec_boolean ("close-on-unfocus",
                                "Close on unfocus",
                                "Close the expander when it loses focus.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CLOSE_ON_UNFOCUS, pspec);

  pspec = g_param_spec_object ("primary-child",
                               "Primary child",
                               "Primary child of the widget.",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PRIMARY_CHILD, pspec);

  pspec = g_param_spec_object ("secondary-child",
                               "Secondary child",
                               "Child that expands out of the widget.",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SECONDARY_CHILD, pspec);

  pspec = g_param_spec_enum ("grow-direction",
                             "Grow direction",
                             "Expand direction of the secondary child.",
                             MEX_TYPE_EXPANDER_BOX_DIRECTION,
                             MEX_EXPANDER_BOX_DOWN,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_GROW_DIRECTION, pspec);

  pspec = g_param_spec_boolean ("expand",
                                "Expand",
                                "Expand into allocated space.",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_EXPAND, pspec);
}

static void
mex_expander_box_timeline_completed_cb (ClutterTimeline *timeline,
                                        ClutterActor    *box)
{
  MexExpanderBoxPrivate *priv = MEX_EXPANDER_BOX (box)->priv;
  ClutterTimelineDirection direction =
    clutter_timeline_get_direction (timeline);

  /* Reversing the direction and rewinding will make sure that on
   * completion, progress stays at 0.0 or 1.0, depending on the
   * direction of the timeline.
   */
  clutter_timeline_set_direction (timeline,
                                  (direction == CLUTTER_TIMELINE_FORWARD) ?
                                  CLUTTER_TIMELINE_BACKWARD :
                                  CLUTTER_TIMELINE_FORWARD);
  clutter_timeline_rewind (timeline);

  clutter_actor_queue_relayout (box);

  if (priv->notify_open)
    {
      priv->notify_open = FALSE;
      priv->open = (direction == CLUTTER_TIMELINE_FORWARD);
      g_object_notify (G_OBJECT (box), "open");
    }

  if (!priv->open && priv->secondary)
    clutter_actor_hide (priv->secondary);
}

static void
mex_expander_box_init (MexExpanderBox *self)
{
  MexExpanderBoxPrivate *priv = self->priv = EXPANDER_BOX_PRIVATE (self);

  priv->close_on_unfocus = TRUE;

  priv->open_alpha = clutter_alpha_new ();
  clutter_alpha_set_mode (priv->open_alpha, CLUTTER_EASE_OUT_CUBIC);

  priv->expand_alpha = clutter_alpha_new ();
  clutter_alpha_set_mode (priv->expand_alpha, CLUTTER_EASE_OUT_CUBIC);

  priv->direction = MEX_EXPANDER_BOX_DOWN;
  priv->expand_timeline = clutter_timeline_new (150);
  priv->open_timeline = clutter_timeline_new (200);

  clutter_alpha_set_timeline (priv->open_alpha, priv->open_timeline);
  clutter_alpha_set_timeline (priv->expand_alpha, priv->expand_timeline);

  g_signal_connect_swapped (priv->expand_timeline, "new-frame",
                            G_CALLBACK (clutter_actor_queue_relayout), self);
  g_signal_connect (priv->expand_timeline, "completed",
                    G_CALLBACK (mex_expander_box_timeline_completed_cb), self);

  g_signal_connect_swapped (priv->open_timeline, "new-frame",
                            G_CALLBACK (clutter_actor_queue_relayout), self);
  g_signal_connect (priv->open_timeline, "completed",
                    G_CALLBACK (mex_expander_box_timeline_completed_cb), self);
}

ClutterActor *
mex_expander_box_new (void)
{
  return g_object_new (MEX_TYPE_EXPANDER_BOX, NULL);
}

static void
mex_expander_box_set_child (MexExpanderBox *box,
                            ClutterActor   *actor,
                            gboolean        primary)
{
  ClutterActor **actor_p;
  MexExpanderBoxPrivate *priv = box->priv;

  actor_p = primary ? &priv->primary : &priv->secondary;

  if (*actor_p == actor)
    return;

  if (*actor_p)
    {
      ClutterActor *old_child = g_object_ref (*actor_p);

      *actor_p = NULL;
      clutter_actor_unparent (old_child);

      g_signal_emit_by_name (box, "actor-removed", old_child);

      g_object_unref (old_child);
    }

  if (actor)
    {
      *actor_p = actor;
      clutter_actor_set_parent (actor, CLUTTER_ACTOR (box));

      /* Reset the max width/height when a new actor is added */
      priv->max_widths[primary ? 0 : 1] = -1;
      priv->max_heights[primary ? 0 : 1] = -1;

      g_signal_emit_by_name (box, "actor-added", actor);
    }

  if (!primary && !priv->open)
    clutter_actor_hide (actor);
  else
    clutter_actor_queue_relayout (CLUTTER_ACTOR (box));

  g_object_notify (G_OBJECT (box),
                   primary ? "primary-child" : "secondary-child");
}

void
mex_expander_box_set_primary_child (MexExpanderBox *box,
                                    ClutterActor   *actor)
{
  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor) || !actor);

  mex_expander_box_set_child (box, actor, TRUE);
}

void
mex_expander_box_set_secondary_child (MexExpanderBox *box,
                                     ClutterActor  *actor)
{
  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor) || !actor);

  mex_expander_box_set_child (box, actor, FALSE);
}

ClutterActor *
mex_expander_box_get_primary_child (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), NULL);
  return box->priv->primary;
}

ClutterActor *
mex_expander_box_get_secondary_child (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), NULL);
  return box->priv->secondary;
}

void
mex_expander_box_set_grow_direction (MexExpanderBox          *box,
                                     MexExpanderBoxDirection  direction)
{
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;
  if (priv->direction != direction)
    {
      priv->direction = direction;

      clutter_actor_queue_relayout (CLUTTER_ACTOR (box));

      g_object_notify (G_OBJECT (box), "grow-direction");
    }
}

MexExpanderBoxDirection
mex_expander_box_get_grow_direction (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), MEX_EXPANDER_BOX_DOWN);
  return box->priv->direction;
}

void
mex_expander_box_set_important (MexExpanderBox *box,
                                gboolean        important)
{
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;
  if (priv->important != important)
    {
      priv->important = important;

      g_object_notify (G_OBJECT (box), "important");

      /* Animate the centre child open/closed, if necessary */
      mex_expander_box_open_primary (box, important);
    }
}

gboolean
mex_expander_box_get_important (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), FALSE);
  return box->priv->important;
}

void
mex_expander_box_set_open (MexExpanderBox *box,
                           gboolean        open)
{
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;
  if (priv->open != open)
    {
      /* Delay notification of the open property to when the drawer is
       * fully closed, when closing the drawer.
       */
      if (open)
        {
          priv->open = open;

          if (priv->secondary)
            clutter_actor_show (priv->secondary);

          if (!priv->notify_open)
            g_object_notify (G_OBJECT (box), "open");
          else
            priv->notify_open = FALSE;

          mex_expander_box_open_primary (box, TRUE);
        }
      else
        {
          priv->notify_open = TRUE;

          if (!priv->important &&
              !(priv->has_focus && priv->important_on_focus))
            mex_expander_box_open_primary (box, FALSE);
        }

      /* Start animations for opening/closing */
      clutter_timeline_set_direction (priv->open_timeline,
                                      open ? CLUTTER_TIMELINE_FORWARD :
                                             CLUTTER_TIMELINE_BACKWARD);
      if (!clutter_timeline_is_playing (priv->open_timeline))
        {
          if (CLUTTER_ACTOR_IS_MAPPED (box))
            {
              clutter_timeline_rewind (priv->open_timeline);
              clutter_timeline_start (priv->open_timeline);
            }
          else
            mex_expander_box_timeline_completed_cb (priv->open_timeline,
                                                   CLUTTER_ACTOR (box));
        }
    }
}

gboolean
mex_expander_box_get_open (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), FALSE);
  return box->priv->open;
}

void
mex_expander_box_set_open_on_focus (MexExpanderBox *box,
                                    gboolean        open)
{
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;
  if (priv->open_on_focus != open)
    {
      priv->open_on_focus = open;

      if (open && priv->has_focus)
        mex_expander_box_set_open (box, TRUE);

      g_object_notify (G_OBJECT (box), "open-on-focus");
    }
}

gboolean
mex_expander_box_get_open_on_focus (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), FALSE);
  return box->priv->open_on_focus;
}

void
mex_expander_box_set_close_on_unfocus (MexExpanderBox *box,
                                       gboolean        close_on_unfocus)
{
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;
  if (priv->close_on_unfocus != close_on_unfocus)
    {
      priv->close_on_unfocus = close_on_unfocus;

      if (close_on_unfocus && !priv->has_focus)
        mex_expander_box_set_open (box, FALSE);

      g_object_notify (G_OBJECT (box), "close-on-unfocus");
    }
}

gboolean
mex_expander_box_get_close_on_unfocus (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), FALSE);
  return box->priv->close_on_unfocus;
}

void
mex_expander_box_set_expand (MexExpanderBox *box,
                             gboolean        expand)
{
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;
  if (priv->expand != expand)
    {
      priv->expand = expand;

      clutter_actor_queue_relayout (CLUTTER_ACTOR (box));

      g_object_notify (G_OBJECT (box), "expand");
    }
}

gboolean
mex_expander_box_get_expand (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), FALSE);
  return box->priv->expand;
}

/**
 * mex_expander_box_set_max_size:
 * @box: The expander box
 * @primary: %TRUE to set the limit on the primary child, %FALSE for the
 *   secondary.
 * @max_width: Maximum width for the child in the @position to be limited to
 *   or -1 to use the natural width.
 * @max_height: Maximum height for the child in the @position to be limited to
 *   or -1 to use the natural height.
 *
 * Note this function must be called after populating the box. Adding an
 * actor resets its set max_width/max_height to -1
 */
void
mex_expander_box_set_max_size (MexExpanderBox *box,
                               gboolean        primary,
                               gfloat          max_width,
                               gfloat          max_height)
{
  gint position;
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;

  position = primary ? 0 : 1;

  priv->max_widths[position] = max_width;
  priv->max_heights[position] = max_height;
}


/**
 * mex_expander_box_get_max_size:
 * @box: The expander box
 * @primary: %TRUE to get the limit on the primary child, %FALSE for the
 *   secondary.
 * @max_width: (out): Out variable for maximum width that the child in
 *   @position is limited to or -1 if it's not limited.
 * @max_height: (out): Out variable for maximum height that the child in
 *   @position is limited to or -1 if it's not limited.
 */
void
mex_expander_box_get_max_size (MexExpanderBox *box,
                               gboolean        primary,
                               gfloat         *max_width,
                               gfloat         *max_height)
{
  gint position;
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;

  position = primary ? 0 : 1;

  if (max_width)
    *max_width = priv->max_widths[position];

  if (max_height)
    *max_height = priv->max_heights[position];
}

void
mex_expander_box_set_important_on_focus (MexExpanderBox *box,
                                         gboolean        important_on_focus)
{
  MexExpanderBoxPrivate *priv;

  g_return_if_fail (MEX_IS_EXPANDER_BOX (box));

  priv = box->priv;
  if (priv->important_on_focus != important_on_focus)
    {
      priv->important_on_focus = important_on_focus;
      g_object_notify (G_OBJECT (box), "important-on-focus");

      if (priv->has_focus && !priv->open && !priv->important)
        mex_expander_box_open_primary (box, important_on_focus);
    }
}

gboolean
mex_expander_box_get_important_on_focus (MexExpanderBox *box)
{
  g_return_val_if_fail (MEX_IS_EXPANDER_BOX (box), FALSE);
  return box->priv->important_on_focus;
}
