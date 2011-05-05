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


#include "mex-grid.h"
#include "mex-content-box.h"
#include "mex-content-view.h"
#include <math.h>

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void mx_scrollable_iface_init (MxScrollableIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexGrid, mex_grid, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_SCROLLABLE,
                                                mx_scrollable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define GRID_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GRID, MexGridPrivate))

typedef struct
{
  gfloat left;
  gfloat right;
  gfloat bottom;
} MexGridSpan;

typedef struct
{
  gdouble initial_height;
  gdouble target_height;
  gfloat  y;
  gfloat  y2;
} MexGridRowData;

struct _MexGridPrivate
{
  guint            tile_width_changed : 1;
  guint            tile_height_changed : 1;
  guint            tile_size_notified : 1;
  guint            has_focus : 1;
  guint            next_foreach_is_style_changed : 1;
  guint            focus_waiting;

  GArray          *children;
  ClutterActor    *current_focus;
  gint             focused_row;
  MexActorSortFunc sort_func;
  gpointer         sort_data;

  gint             stride;
  gint             real_stride;
  gfloat           tile_width;
  gfloat           tile_height;
  GArray          *row_sizes;
  GArray          *spans;
  GArray          *boxes;

  ClutterAlpha    *alpha;
  ClutterTimeline *timeline;
  guint            anim_length;

  MxAdjustment    *vadjust;

  gint             first_visible;
  gint             last_visible;

  CoglHandle       highlight;
  CoglHandle       highlight_material;
  MxBorderImage   *highlight_image;
};

enum
{
  PROP_0,

  PROP_STRIDE,
  PROP_HADJUST,
  PROP_VADJUST,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
};

static void mex_grid_start_animation (MexGrid *self);
static void mex_grid_ensure_rows (MexGrid *self, const ClutterActorBox *box);

/* ClutterContainerIface */

static gint
mex_grid_sort_func (ClutterActor **a,
                    ClutterActor **b,
                    MexGrid       *self)
{
  MexGridPrivate *priv = self->priv;

  return priv->sort_func (*a, *b, priv->sort_data);
}

static void
mex_grid_insert_sorted (MexGrid      *self,
                        ClutterActor *new_child)
{
  gint low, high, position;
  MexGridPrivate *priv = self->priv;
  gint result = 0;

  /* Binary search - we assume the array is already sorted */
  low = 0;
  high = priv->children->len - 1;
  position = (high + low + 1) / 2;
  while (high != low)
    {
      ClutterActor *child =
        g_array_index (priv->children, ClutterActor *, position);
      result = priv->sort_func (new_child, child, priv->sort_data);

      if (result > 0)
        {
          if (low == position)
            break;
          low = position;
        }
      else if (result < 0)
        {
          if (high == position)
            break;
          high = position;
        }
      else
        break;

      position = (high + low + 1) / 2;
    }

  /* We may need to insert-after, depending on the last comparison */
  if (result >= 0)
    position ++;

  g_array_insert_val (priv->children, position, new_child);
}

static void
mex_grid_add (ClutterContainer *container,
              ClutterActor     *actor)
{
  MexGrid *self = MEX_GRID (container);
  MexGridPrivate *priv = self->priv;

  if (priv->sort_func && priv->children->len)
    mex_grid_insert_sorted (self, actor);
  else
    g_array_append_val (priv->children, actor);

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (self));

  mex_grid_start_animation (self);
  g_signal_emit_by_name (self, "actor-added", actor);

  if (priv->focus_waiting)
    {
      mex_push_focus (MX_FOCUSABLE (actor));
      priv->focus_waiting = FALSE;
    }
}

static void
mex_grid_remove (ClutterContainer *container,
                 ClutterActor     *actor)
{
  gint i;

  MexGrid *self = MEX_GRID (container);
  MexGridPrivate *priv = self->priv;

  if (priv->current_focus == actor)
    {
      priv->current_focus = NULL;
      priv->focused_row = 0;
    }

  for (i = 0; i < priv->children->len; i++)
    {
      ClutterActor *child = g_array_index (priv->children, ClutterActor *, i);

      if (child != actor)
        continue;

      g_object_ref (actor);

      g_array_remove_index (priv->children, i);
      clutter_actor_unparent (actor);

      g_signal_emit_by_name (self, "actor-removed", actor);

      g_object_unref (actor);
      mex_grid_start_animation (self);

      return;
    }

  g_warning (G_STRLOC ": Trying to remove an unknown child");
}

static void
mex_grid_foreach (ClutterContainer *container,
                  ClutterCallback   callback,
                  gpointer          user_data)
{
  gint i;
  MexGrid *self = MEX_GRID (container);
  MexGridPrivate *priv = self->priv;

  /* Skip this foreach if it came from a style-changed signal. This will
   * break style rules of our children that rely on changing properties of
   * our parents, but it's just too slow.
   */
  if (!priv->next_foreach_is_style_changed)
    {
      /* FIXME: this will probably break if the child list is modified
       * in the callback... We ought to account for this somehow.
       */
      for (i = 0; i < priv->children->len; i++)
        {
          ClutterActor *child =
            g_array_index (priv->children, ClutterActor *, i);
          callback (child, user_data);
        }
    }
  else
    priv->next_foreach_is_style_changed = FALSE;
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = mex_grid_add;
  iface->remove = mex_grid_remove;
  iface->foreach = mex_grid_foreach;
}

/* MxScrollableIface */

static void
mex_grid_set_adjustments (MxScrollable *scrollable,
                          MxAdjustment *hadjust,
                          MxAdjustment *vadjust)
{
  MexGrid *self = MEX_GRID (scrollable);
  MexGridPrivate *priv = self->priv;

  if (vadjust != priv->vadjust)
    {
      if (priv->vadjust)
        {
          g_signal_handlers_disconnect_by_func (priv->vadjust,
                                                clutter_actor_queue_relayout,
                                                self);
          g_object_unref (priv->vadjust);
        }

      if (vadjust)
        {
          g_object_ref (vadjust);
          g_signal_connect_swapped (vadjust,
                                    "notify::value",
                                    G_CALLBACK (clutter_actor_queue_relayout),
                                    self);
        }

      priv->vadjust = vadjust;

      g_object_notify (G_OBJECT (self), "vertical-adjustment");

      /* Relayout to set the correct values on the adjustment */
      clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
    }
}

static void
mex_grid_get_adjustments (MxScrollable  *scrollable,
                          MxAdjustment **hadjust,
                          MxAdjustment **vadjust)
{
  MexGrid *self = MEX_GRID (scrollable);
  MexGridPrivate *priv = self->priv;

  if (hadjust)
    *hadjust = NULL;

  if (vadjust)
    {
      if (priv->vadjust)
        *vadjust = priv->vadjust;
      else
        {
          *vadjust = mx_adjustment_new ();
          mex_grid_set_adjustments (scrollable, NULL, *vadjust);
          g_object_unref (*vadjust);
        }
    }
}

static void
mx_scrollable_iface_init (MxScrollableIface *iface)
{
  iface->set_adjustments = mex_grid_set_adjustments;
  iface->get_adjustments = mex_grid_get_adjustments;
}

/* MxFocusableIface */

static MxFocusable *
mex_grid_move_focus (MxFocusable      *focusable,
                     MxFocusDirection  direction,
                     MxFocusable      *from)
{
  gint i, index, dx;
  MxFocusHint hint;
  ClutterActor *child;

  MexGrid *self = MEX_GRID (focusable);
  MexGridPrivate *priv = self->priv;

  switch (direction)
    {
    case MX_FOCUS_DIRECTION_PREVIOUS:
    case MX_FOCUS_DIRECTION_LEFT:
      dx = -1;
      hint = MX_FOCUS_HINT_LAST;
      break;

    case MX_FOCUS_DIRECTION_UP:
      dx = -priv->real_stride;
      hint = MX_FOCUS_HINT_FIRST;
      break;

    case MX_FOCUS_DIRECTION_DOWN:
      dx = priv->real_stride;
      hint = MX_FOCUS_HINT_FIRST;
      break;

    default:
    case MX_FOCUS_DIRECTION_NEXT:
    case MX_FOCUS_DIRECTION_RIGHT:
      dx = 1;
      hint = MX_FOCUS_HINT_FIRST;
      break;
    }

  child = NULL;
  focusable = NULL;
  for (index = 0; index < priv->children->len; index++)
    {
      child = g_array_index (priv->children, ClutterActor *, index);

      if (child != (ClutterActor *)from)
        continue;

      switch (direction)
        {
        case MX_FOCUS_DIRECTION_UP:
        case MX_FOCUS_DIRECTION_DOWN:
          for (i = index + dx; (i >= 0) && (i < priv->children->len); i += dx)
            {
              child = g_array_index (priv->children, ClutterActor *, i);
              if (MX_IS_FOCUSABLE (child) &&
                  (focusable = mx_focusable_accept_focus (MX_FOCUSABLE (child),
                                                          hint)))
                break;
            }

          /* If we're on the row before last, we possibly want to focus
           * the last item
           */
          if (!focusable &&
              (direction == MX_FOCUS_DIRECTION_DOWN) &&
              ((index / priv->real_stride) ==
               ((priv->children->len - 1) / priv->real_stride) - 1))
            {
              child = g_array_index (priv->children, ClutterActor *,
                                     priv->children->len - 1);
              if (MX_IS_FOCUSABLE (child) &&
                  (focusable = mx_focusable_accept_focus (MX_FOCUSABLE (child),
                                                          hint)))
                break;
            }

          break;

        case MX_FOCUS_DIRECTION_NEXT:
        case MX_FOCUS_DIRECTION_RIGHT:
        case MX_FOCUS_DIRECTION_PREVIOUS:
        case MX_FOCUS_DIRECTION_LEFT:
          for (i = index + dx; (i >= 0) && (i < priv->children->len); i += dx)
            {
              if ((direction == MX_FOCUS_DIRECTION_LEFT) &&
                  ((i + 1) % priv->real_stride == 0))
                break;
              if ((direction == MX_FOCUS_DIRECTION_RIGHT) &&
                  (i % priv->real_stride == 0))
                break;
              child = g_array_index (priv->children, ClutterActor *, i);
              if (MX_IS_FOCUSABLE (child) &&
                  (focusable = mx_focusable_accept_focus (MX_FOCUSABLE (child),
                                                          hint)))
                break;
            }

          /* If we're on the last row, we possibly want to focus the
           * right hand side item on the previous row.
           */
          if (!focusable &&
              (direction == MX_FOCUS_DIRECTION_RIGHT) &&
              ((index % priv->real_stride) != (priv->real_stride - 1)) &&
              ((index / priv->real_stride) ==
               ((priv->children->len - 1) / priv->real_stride)))
            {
              child = g_array_index (priv->children, ClutterActor *,
                                     priv->children->len - priv->real_stride);
              if (MX_IS_FOCUSABLE (child) &&
                  (focusable = mx_focusable_accept_focus (MX_FOCUSABLE (child),
                                                          hint)))
                {
                  i = priv->children->len - priv->real_stride;
                  break;
                }
            }
          break;

        default:
          break;
        }

      break;
    }

  if (focusable)
    {
      /* Update the focused child/row. We do this here to avoid
       * having to iterate over all our children later on when
       * we're notified of the focus change.
       */
      priv->current_focus = child;
      priv->focused_row = i / priv->real_stride;
    }

  return focusable;
}

static MxFocusable *
mex_grid_accept_focus (MxFocusable *focusable, MxFocusHint hint)
{
  MxFocusable *returnval = NULL;
  MexGrid *self = MEX_GRID (focusable);
  MexGridPrivate *priv = self->priv;

  switch (hint)
    {
    case MX_FOCUS_HINT_PRIOR:
    case MX_FOCUS_HINT_FROM_LEFT:
    case MX_FOCUS_HINT_FROM_ABOVE:
      if (priv->current_focus && !(returnval =
          mx_focusable_accept_focus (MX_FOCUSABLE (priv->current_focus), hint)))
        hint = MX_FOCUS_HINT_FIRST;
      break;
    case MX_FOCUS_HINT_FROM_RIGHT:
    case MX_FOCUS_HINT_FROM_BELOW:
      if (priv->current_focus && !(returnval =
          mx_focusable_accept_focus (MX_FOCUSABLE (priv->current_focus), hint)))
        hint = MX_FOCUS_HINT_LAST;
      break;

    default:
      break;
    }

  if (!returnval)
    {
      gint i;

      gboolean reverse = (hint != MX_FOCUS_HINT_LAST) ? FALSE : TRUE;

      for (i = reverse ? priv->children->len - 1 : 0;
           (i >= 0) && (i < priv->children->len); i += reverse ? -1 : 1)
        {
          ClutterActor *child =
            g_array_index (priv->children, ClutterActor *, i);

          if (!MX_IS_FOCUSABLE (child))
            continue;

          returnval = mx_focusable_accept_focus (MX_FOCUSABLE (child), hint);

          if (returnval)
            {
              priv->current_focus = child;
              priv->focused_row = i / priv->real_stride;
              break;
            }
        }
    }

  /* keep focus so that children added later can be focused */
  if (!returnval)
    {
      returnval = focusable;
      priv->focus_waiting = TRUE;
    }
  else
    priv->focus_waiting = FALSE;

  return returnval;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_grid_move_focus;
  iface->accept_focus = mex_grid_accept_focus;
}

/* MxStylableIface */

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
      GParamSpec *pspec;

      is_initialized = TRUE;

      pspec = g_param_spec_boxed ("x-mex-highlight",
                                  "Highlight",
                                  "Image to use for the highlight.",
                                  MX_TYPE_BORDER_IMAGE,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_GRID, pspec);
    }
}

/* Actor implementation */

static void
mex_grid_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  MexGrid *grid = (MexGrid *) object;
  MexGridPrivate *priv = grid->priv;
  MxAdjustment *adjustment;

  switch (property_id)
    {
    case PROP_STRIDE:
      g_value_set_int (value, mex_grid_get_stride (MEX_GRID (object)));
      break;

    case PROP_TILE_WIDTH:
      g_value_set_float (value, priv->tile_width);
      break;

    case PROP_TILE_HEIGHT:
      g_value_set_float (value, priv->tile_height);
      break;

    case PROP_HADJUST:
      mex_grid_get_adjustments (MX_SCROLLABLE (object), &adjustment, NULL);
      g_value_set_object (value, adjustment);
      break;

    case PROP_VADJUST:
      mex_grid_get_adjustments (MX_SCROLLABLE (object), NULL, &adjustment);
      g_value_set_object (value, adjustment);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_grid_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  MexGrid *self = MEX_GRID (object);
  MexGridPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_STRIDE:
      mex_grid_set_stride (self, g_value_get_int (value));
      break;

    case PROP_HADJUST:
      mex_grid_set_adjustments (MX_SCROLLABLE (object),
                                g_value_get_object (value),
                                self->priv->vadjust);
      break;

    case PROP_VADJUST:
      mex_grid_set_adjustments (MX_SCROLLABLE (object),
                                NULL,
                                g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_grid_dispose (GObject *object)
{
  MexGridPrivate *priv = MEX_GRID (object)->priv;

  if (priv->vadjust)
    {
      g_object_unref (priv->vadjust);
      priv->vadjust = NULL;
    }

  if (priv->alpha)
    {
      g_object_unref (priv->alpha);
      priv->alpha = NULL;
    }

  if (priv->timeline)
    {
      g_object_unref (priv->timeline);
      priv->timeline = NULL;
    }

  if (priv->highlight)
    {
      cogl_handle_unref (priv->highlight);
      cogl_handle_unref (priv->highlight_material);
      priv->highlight = NULL;
      priv->highlight_material = NULL;
    }

  G_OBJECT_CLASS (mex_grid_parent_class)->dispose (object);
}

static void
mex_grid_finalize (GObject *object)
{
  MexGridPrivate *priv = MEX_GRID (object)->priv;

  if (priv->children)
    {
      g_array_unref (priv->children);
      priv->children = NULL;
    }

  if (priv->row_sizes)
    {
      g_array_unref (priv->row_sizes);
      priv->row_sizes = NULL;
    }

  if (priv->spans)
    {
      g_array_unref (priv->spans);
      priv->spans = NULL;
    }

  if (priv->boxes)
    {
      g_array_unref (priv->boxes);
      priv->boxes = NULL;
    }

  if (priv->highlight_image)
    {
      g_boxed_free (MX_TYPE_BORDER_IMAGE, priv->highlight_image);
      priv->highlight_image = NULL;
    }

  G_OBJECT_CLASS (mex_grid_parent_class)->finalize (object);
}

static void
mex_grid_start_animation (MexGrid *self)
{
  gint i;
  gdouble progress;
  MexGridPrivate *priv = self->priv;

  mex_grid_ensure_rows (self, NULL);

  if (!priv->timeline)
    return;

  if (!priv->children->len)
    {
      clutter_timeline_stop (priv->timeline);
      return;
    }

  progress = clutter_alpha_get_alpha (priv->alpha);

  for (i = 0; i <= (priv->children->len - 1) / priv->real_stride; i++)
    {
      gdouble current_height;
      MexGridRowData *data =
        &g_array_index (priv->row_sizes, MexGridRowData, i);

      current_height = (data->target_height * progress) +
                       (data->initial_height * (1.0 - progress));

      data->initial_height = current_height;
      if (!priv->has_focus)
        data->target_height = 1.0 / pow (1.5, 2);
      else if (i == priv->focused_row)
        data->target_height = 1.0;
      else
        data->target_height = 1.0 /
                              pow (1.5, MIN (2, ABS (priv->focused_row - i)));
    }

  clutter_timeline_set_direction (priv->timeline, CLUTTER_TIMELINE_FORWARD);
  clutter_timeline_rewind (priv->timeline);
  clutter_timeline_start (priv->timeline);
}

static void
mex_grid_ensure_rows (MexGrid               *self,
                      const ClutterActorBox *box)
{
  gint needed_rows;
  MexGridPrivate *priv = self->priv;

  if (!priv->real_stride || box)
    {
      if (priv->stride)
        priv->real_stride = priv->stride;
      else
        {
          gfloat width;
          MxPadding padding;
          ClutterActorBox alloc_box;

          /* Make sure we have the allocation */
          if (!box)
            clutter_actor_get_allocation_box (CLUTTER_ACTOR (self),
                                              &alloc_box);
          else
            alloc_box = *box;

          /* Take into account padding */
          mx_widget_get_padding (MX_WIDGET (self), &padding);
          width = alloc_box.x2 - alloc_box.x1 - padding.left - padding.right;

          /* Calculate how many tiles we can fit into our allocated width */
          if (width > priv->tile_width)
            priv->real_stride = (gint)(width / priv->tile_width);
          else
            priv->real_stride = 1;
        }
    }

  needed_rows = (priv->children->len + priv->real_stride - 1) /
    priv->real_stride;

  if ((priv->row_sizes->len < needed_rows) ||
      (priv->row_sizes->len > needed_rows + 1))
    {
      /* Grow or shrink array */
      g_array_set_size (priv->row_sizes, needed_rows);
    }
}

static void
mex_grid_get_preferred_width (ClutterActor *actor,
                              gfloat        for_height,
                              gfloat       *min_width_p,
                              gfloat       *nat_width_p)
{
  gfloat width;
  MxPadding padding;

  MexGridPrivate *priv = MEX_GRID (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (priv->stride)
    width = (priv->tile_width * priv->stride) + padding.left + padding.right;
  else
    width = priv->tile_width + padding.left + padding.right;

  if (min_width_p)
    *min_width_p = width;
  if (nat_width_p)
    *nat_width_p = width;
}

static void
mex_grid_get_preferred_height (ClutterActor *actor,
                               gfloat        for_width,
                               gfloat       *min_height_p,
                               gfloat       *nat_height_p)
{
  gfloat min_height, height;
  MxPadding padding;

  MexGridPrivate *priv = MEX_GRID (actor)->priv;

  if (min_height_p)
    *min_height_p = 0;
  if (nat_height_p)
    *nat_height_p = 0;

  if (priv->has_focus && priv->current_focus)
    clutter_actor_get_preferred_height (priv->current_focus,
                                        -1,
                                        &min_height,
                                        &height);
  else
    min_height = height = 0.f;

  if (min_height < priv->tile_height)
    min_height = priv->tile_height;
  if (height < min_height)
    height = min_height;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_height_p)
    *min_height_p = min_height + padding.top + padding.bottom;
  if (nat_height_p)
    *nat_height_p = height + padding.top + padding.bottom;
}

static gboolean
mex_grid_notify_tile_size (MexGrid *grid)
{
  MexGridPrivate *priv = grid->priv;

  priv->tile_size_notified = TRUE;

  if (priv->tile_width_changed)
    g_object_notify (G_OBJECT (grid), "tile-width");
  if (priv->tile_height_changed)
    g_object_notify (G_OBJECT (grid), "tile-height");

  priv->tile_width_changed = FALSE;
  priv->tile_height_changed = FALSE;

  return FALSE;
}

static void
mex_grid_allocate (ClutterActor           *actor,
                   const ClutterActorBox  *box,
                   ClutterAllocationFlags  flags)
{
  gint i, row;
  MxPadding padding;
  gdouble value, progress;
  ClutterActorBox child_box;
  gboolean do_second_phase, allocated_focus;
  gfloat basic_width, basic_height, avail_width, avail_height, top, bottom,
         focus_top;

  MexGrid *self = MEX_GRID (actor);
  MexGridPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->allocate (actor, box, flags);

  /* Bail out if we have no children */
  priv->first_visible = priv->last_visible = -1;
  if (!priv->children->len)
    return;

  progress = clutter_alpha_get_alpha (priv->alpha);
  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  avail_width = box->x2 - box->x1 - padding.left - padding.right;
  avail_height = box->y2 - box->y1 - padding.top - padding.bottom;

  /* We start off by assuming that all the actors are the same size - for
   * this size, we use the set tile-size.
   */
  basic_width = round (avail_width / (gfloat) priv->real_stride);
  if (basic_width != priv->tile_width)
    {
      basic_height = basic_width * priv->tile_height / priv->tile_width;

      priv->tile_width_changed = TRUE;
      priv->tile_width = basic_width;

      if (basic_height != priv->tile_height)
        {
          priv->tile_height_changed = TRUE;
          priv->tile_height = basic_height;
        }

      priv->tile_size_notified = FALSE;
    }
  else
    basic_height = priv->tile_height;

  /* Ok... That's a terrible kludge. Why the hell are we doing that ??
   *
   * Simple, changing the tiles' size triggers a relayouting of the
   * tiles and consequently a relayouting of the tiles' parent (ie.
   * the grid itself) which we are trying to layout in this function.
   * So this timeout is just a workaround to prevent that to happen.
   *
   * If you know a better way, please fix that...
   */
  if ((priv->tile_width_changed || priv->tile_height_changed) &&
      !priv->tile_size_notified)
    g_idle_add_full (G_PRIORITY_HIGH,
                     (GSourceFunc) mex_grid_notify_tile_size,
                     self, NULL);

  if (priv->vadjust)
    value = (gint)mx_adjustment_get_value (priv->vadjust);
  else
    value = 0;

  /* Allocate all actors in the visible range the standard size, unless
   * the current row height isn't the target height. When we get to an
   * animating row, we break and allow the second, more complex
   * allocation phase to allocate the rest.
   *
   * The assumption here is that any transition the child has that affects
   * its size will take less time than the row-size transition in this
   * container.
   *
   * NOTE: There's also a bad assumption that anything that isn't in
   *       view is the basic size - unfortunately, there's not really
   *       any way to fix this without having to measure it, and then
   *       we become slow again.
   *       This will produce incorrect results visually if you scroll
   *       vertically very quickly with actors that are resizing, but
   *       unless the animation speed is very slow, should be
   *       unnoticeable.
   */
  focus_top = -1;
  do_second_phase = FALSE;
  bottom = top = padding.top;
  mex_grid_ensure_rows (self, box);
  for (row = 0; row <= (priv->children->len - 1) / priv->real_stride; row++)
    {
      MexGridRowData *data =
        &g_array_index (priv->row_sizes, MexGridRowData, row);
      gdouble current_height = (data->target_height * progress) +
                               (data->initial_height * (1.0 - progress));

      /* Check if we're visible */
      bottom = top + (gint)round (basic_height * current_height);
      if ((bottom - value >= 0) && (top - value < box->y2 - box->y1))
        {
          gint index = row * priv->real_stride;

          /* Store the first visible child index, and the height at which
           * it's visible.
           */
          if (priv->first_visible == -1)
            priv->first_visible = index;
          priv->last_visible = MIN (priv->children->len - 1,
                                    index + (priv->real_stride - 1));

          if ((current_height == data->target_height) &&
              (!priv->has_focus || (row != priv->focused_row)))
            {
              child_box.y1 = top;
              child_box.x1 = padding.left;

              for (i = index;
                   (i < index + priv->real_stride) && (i < priv->children->len);
                   i++)
                {
                  gfloat width, height;
                  ClutterActor *child =
                    g_array_index (priv->children, ClutterActor *, i);

                  clutter_actor_get_preferred_size (child,
                                                    NULL, NULL,
                                                    &width, &height);
                  child_box.x2 = child_box.x1 + MAX (basic_width, width);
                  child_box.y2 = child_box.y1 + MAX (basic_height, height);
                  clutter_actor_allocate (child, &child_box, flags);
                  child_box.x1 = child_box.x2;
                }
            }
          else
            {
              /* This row is animating, or focused - break out into
               * the more complicated allocation.
               */
              do_second_phase = TRUE;
              break;
            }
        }

      /* Store top/bottom of row */
      data->y = top;
      data->y2 = bottom;

      if (priv->has_focus && (row == priv->focused_row))
        focus_top = bottom;

      top = bottom;
    }

  /* Allocate animating rows and any subsequent rows beneath them. */
  allocated_focus = FALSE;
  if (do_second_phase)
    {
      g_array_set_size (priv->spans, 0);
      g_array_set_size (priv->boxes, priv->real_stride);
      for (; row <= (priv->children->len - 1) / priv->real_stride; row++)
        {
          gint s;
          gboolean do_spanning;

          gint index = row * priv->real_stride;
          MexGridRowData *data =
            &g_array_index (priv->row_sizes, MexGridRowData, row);
          gdouble current_height = (data->target_height * progress) +
                                   (data->initial_height * (1.0 - progress));

          /* Work out the allocation boxes */
          child_box.x1 = padding.left;
          do_spanning = FALSE;

          if (priv->first_visible == -1)
            priv->first_visible = index;

          for (i = index;
               (i < index + priv->real_stride) && (i < priv->children->len);
               i++)
            {
              gfloat width, height;

              ClutterActor *child =
                g_array_index (priv->children, ClutterActor *, i);
              ClutterActorBox *box_store =
                &g_array_index (priv->boxes, ClutterActorBox, i - index);

              /* Get the size of the child */
              clutter_actor_get_preferred_size (child,
                                                NULL, NULL,
                                                &width, &height);

              /* Make sure it's at least the basic size */
              if (width < basic_width)
                width = basic_width;
              if (height < basic_height)
                height = basic_height;

              child_box.x2 = child_box.x1 + width;

              /* If we have spans stored, use those */
              if (priv->spans->len)
                {
                  child_box.y1 = 0;
                  for (s = 0; s < priv->spans->len; s++)
                    {
                      MexGridSpan *span =
                        &g_array_index (priv->spans, MexGridSpan, s);

                      if ((child_box.x1 < span->right) &&
                          (child_box.x2 > span->left))
                        {
                          if (span->bottom > child_box.y1)
                            child_box.y1 = span->bottom;
                        }
                    }
                }
              else
                child_box.y1 = top;
              child_box.y2 = child_box.y1 + height;

              /* Store box */
              *box_store = child_box;

              /* Push boxes left if necessary, but only do this for an
               * abnormally sized actor.
               */
              if (((width != basic_width) || (height != basic_height)) &&
                  (box_store->x2 - padding.left >= avail_width))
                {
                  gint back;
                  gfloat push = (box_store->x2 - padding.left) - avail_width;
                  for (back = i - index; back >= 0; back--)
                    {
                      ClutterActorBox *prev_box =
                        &g_array_index (priv->boxes, ClutterActorBox, back);
                      prev_box->x2 -= push;
                      prev_box->x1 -= push;
                    }
                }

              child_box.x1 = box_store->x2;
            }

          /* Figure out if we need to store spans */
          for (i = 0; i < priv->real_stride - 1; i++)
            {
              ClutterActorBox *box1, *box2;

              box1 = &g_array_index (priv->boxes, ClutterActorBox, i);
              box2 = &g_array_index (priv->boxes, ClutterActorBox, i + 1);

              if (box2->y2 != box1->y2)
                {
                  do_spanning = TRUE;
                  break;
                }
            }

          /* Store top of row */
          data->y = top;

          /* Store child spans - basically copies of the boxes */
          if (do_spanning)
            {
              /* Create spans */
              top = G_MAXFLOAT;
              bottom = 0;
              g_array_set_size (priv->spans, priv->real_stride);
              for (i = 0; i < priv->real_stride; i++)
                {
                  ClutterActorBox *box_store =
                    &g_array_index (priv->boxes, ClutterActorBox, i);
                  MexGridSpan *span =
                    &g_array_index (priv->spans, MexGridSpan, i);

                  span->left = box_store->x1;
                  span->right = box_store->x2;
                  span->bottom = box_store->y1 +
                                 (gint)round ((box_store->y2 - box_store->y1) *
                                              current_height);

                  if (span->bottom < top)
                    top = span->bottom;
                  if (span->bottom > bottom)
                    bottom = span->bottom;
                }
            }
          else
            {
              g_array_set_size (priv->spans, 0);
              top = child_box.y1 + (gint)round ((child_box.y2 - child_box.y1) *
                                                current_height);
              bottom = top;
            }

          /* Store bottom of row */
          data->y2 = bottom;

          /* Allocate */
          for (i = index;
               (i < index + priv->real_stride) && (i < priv->children->len);
               i++)
            {
              ClutterActor *child =
                g_array_index (priv->children, ClutterActor *, i);
              ClutterActorBox *box_store =
                &g_array_index (priv->boxes, ClutterActorBox, i - index);

              clutter_actor_allocate (child, box_store, flags);
              if (child == priv->current_focus)
                allocated_focus = TRUE;
            }

          priv->last_visible = MIN (priv->children->len - 1,
                                    index + (priv->real_stride - 1));

          /* If we've moved outside of the visible range, break */
          if (top - value >= box->y2 - box->y1)
            break;
        }
    }

  /* Make sure to always allocate the focused actor, so that it's
   * possible to scroll to it based on its allocation box.
   */
  if (!allocated_focus && priv->has_focus)
    {
      gint index = priv->focused_row * priv->real_stride;

      /* Find out where the top of the focused actor is */
      if (focus_top < 0)
        {
          focus_top = top;
          for (; row < priv->focused_row; row++)
            {
              MexGridRowData *data =
                &g_array_index (priv->row_sizes, MexGridRowData, row);
              gdouble current_height = (data->target_height * progress) +
                                       (data->initial_height * (1.0 - progress));
              focus_top += basic_height * current_height;
            }
        }

      for (i = 0; i < priv->real_stride; i++)
        {
          ClutterActor *child =
            g_array_index (priv->children, ClutterActor *, i + index);

          if (child == priv->current_focus)
            {
              /* Allocate the focused actor the basic size */
              child_box.x1 = basic_width * i;
              child_box.x2 = child_box.x1 + basic_width;
              child_box.y1 = focus_top;
              child_box.y2 = child_box.y1 + basic_height;

              clutter_actor_allocate (child, &child_box, flags);
              allocated_focus = TRUE;

              break;
            }
        }
    }

  if (!allocated_focus && priv->has_focus)
    g_warning (G_STRLOC ": Haven't allocated the focused actor");

  /* Set the adjustment values */
  if (priv->vadjust)
    {
      for (; row <= (priv->children->len - 1) / priv->real_stride; row++)
        {
          MexGridRowData *data =
            &g_array_index (priv->row_sizes, MexGridRowData, row);
          gdouble current_height = (data->target_height * progress) +
                                   (data->initial_height * (1.0 - progress));
          bottom += (gint)round (basic_height * current_height);
        }

      g_object_set (G_OBJECT (priv->vadjust),
                    "lower", 0.0,
                    "upper", (gdouble)bottom,
                    "step-increment", (gdouble)basic_height,
                    "page-increment", floor (avail_height / basic_height) *
                                      basic_height,
                    "page-size", (gdouble)avail_height,
                    NULL);
    }
}

static void
mex_grid_apply_transform (ClutterActor *actor,
                          CoglMatrix   *matrix)
{
  MexGridPrivate *priv = MEX_GRID (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->apply_transform (actor, matrix);

  if (priv->vadjust)
    cogl_matrix_translate (matrix,
                           0, -mx_adjustment_get_value (priv->vadjust), 0);
}

static void
mex_grid_paint (ClutterActor *actor)
{
  gfloat y;
  MxPadding padding;
  gint i, focused_row;
  ClutterActorBox box;
  guint8 paint_opacity;

  MexGrid *grid = MEX_GRID (actor);
  MexGridPrivate *priv = grid->priv;

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->paint (actor);

  if (priv->first_visible == -1)
    return;

  /* Clip our allocated area + padding */
  clutter_actor_get_allocation_box (actor, &box);
  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (priv->vadjust)
    y = mx_adjustment_get_value (priv->vadjust);
  else
    y = 0;

  cogl_clip_push_rectangle (padding.left,
                            y + padding.top,
                            box.x2 - box.x1 - padding.right,
                            y + box.y2 - box.y1 - padding.bottom);

  paint_opacity = clutter_actor_get_paint_opacity (actor);

  /* Clip around all rows above the focused row */
  if (priv->has_focus)
    focused_row = MIN (priv->focused_row * priv->real_stride,
                       priv->last_visible);
  else
    focused_row = priv->last_visible;

  if (priv->has_focus && (focused_row > priv->first_visible))
    {
      MexGridRowData *focused_row_data =
        &g_array_index (priv->row_sizes, MexGridRowData,
                        focused_row / priv->real_stride);

      /* The focused row might not span the entire width of the grid, so
       * craft a path that goes around it.
       */
      if (priv->last_visible < focused_row + priv->real_stride - 1)
        {
          ClutterActorBox child_box;
          ClutterActor *child = g_array_index (priv->children, ClutterActor *,
                                               priv->last_visible);

          clutter_actor_get_allocation_box (child, &child_box);

          cogl_path_move_to (padding.left, focused_row_data->y);
          cogl_path_line_to (padding.left, y + padding.top);
          cogl_path_line_to (box.x2 - box.x1 - padding.right, y + padding.top);
          cogl_path_line_to (box.x2 - box.x1 - padding.right, child_box.y2);
          cogl_path_line_to (child_box.x2, child_box.y2);
          cogl_path_line_to (child_box.x2, focused_row_data->y);
          cogl_path_close ();

          cogl_clip_push_from_path ();
        }
      else
        cogl_clip_push_rectangle (padding.left, y + padding.top,
                                  box.x2 - box.x1 - padding.right,
                                  focused_row_data->y);
    }

  /* Draw children */
  for (i = priv->first_visible; i <= priv->last_visible; i++)
    {
      ClutterActor *child = g_array_index (priv->children, ClutterActor *, i);

      /* Unclip rows above the focused row */
      if (priv->has_focus && (focused_row > priv->first_visible) &&
          (i == focused_row))
        cogl_clip_pop ();

      /* Paint child */
      clutter_actor_paint (child);

      /* Draw lowlight */
      if (!priv->has_focus || (i < focused_row) ||
          (i >= focused_row + priv->real_stride))
        {
          ClutterActorBox child_box;
          MexGridRowData *data =
            &g_array_index (priv->row_sizes, MexGridRowData,
                            i / priv->real_stride);

          gdouble progress = clutter_alpha_get_alpha (priv->alpha);

          gdouble current_height = (data->target_height * progress) +
                                   (data->initial_height * (1.0 - progress));

          guint8 opacity = (guint8)((paint_opacity * 0.75) *
                                    (1.0 - current_height));

          clutter_actor_get_allocation_box (child, &child_box);

          cogl_set_source_color4ub (0, 0, 0, opacity);
          cogl_rectangle (child_box.x1, child_box.y1,
                          child_box.x2, child_box.y2);
        }
    }

  /* Draw the highlight */
  if (priv->highlight_material)
    {
      cogl_material_set_color4ub (priv->highlight_material,
                                  paint_opacity, paint_opacity, paint_opacity,
                                  paint_opacity);
      cogl_set_source (priv->highlight_material);

      if (/*highlight_left*/0)
        cogl_rectangle_with_texture_coords (
          box.x1, box.y1 + y,
          box.x1 + cogl_texture_get_width (priv->highlight),
          box.y2 + y,
          0, 0, 1, 1);

      if (/*highlight_right*/1)
        cogl_rectangle_with_texture_coords (
          box.x2, box.y1 + y,
          box.x2 - cogl_texture_get_width (priv->highlight),
          box.y2 + y,
          0, 0, 1, 1);
    }

  /* Unset the clip around the actor */
  cogl_clip_pop ();
}

static void
mex_grid_pick (ClutterActor *actor, const ClutterColor *color)
{
  gint i;
  MexGridPrivate *priv = MEX_GRID (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->pick (actor, color);

  if (priv->first_visible == -1)
    return;

  for (i = priv->first_visible; i <= priv->last_visible; i++)
    {
      ClutterActor *child = g_array_index (priv->children, ClutterActor *, i);
      clutter_actor_paint (child);
    }
}

static void
mex_grid_destroy (ClutterActor *actor)
{
  GList *children = clutter_container_get_children (CLUTTER_CONTAINER (actor));

  g_list_foreach (children, (GFunc)clutter_actor_destroy, NULL);
  g_list_free (children);

  if (CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->destroy)
    CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->destroy (actor);
}

static void
mex_grid_set_focused_actor (MexGrid      *self,
                            ClutterActor *actor)
{
  MexGridPrivate *priv = self->priv;

  priv->current_focus = actor;

  /* Find what row this actor is on */
  if (actor)
    {
      gint i;

      for (i = 0; i < priv->children->len; i++)
        if (g_array_index (priv->children, ClutterActor *, i) == actor)
          break;

      priv->focused_row = i / priv->real_stride;
    }

  /* Animate to possibly newly focused row (or reset) */
  if (priv->has_focus)
    mex_grid_start_animation (self);
}

static void
mex_grid_notify_focused_cb (MxFocusManager *manager,
                            GParamSpec     *pspec,
                            MexGrid        *self)
{
  ClutterActor *focused;
  MexGridPrivate *priv = self->priv;

  focused = (ClutterActor *)mx_focus_manager_get_focused (manager);

  /* Find out if the current focus is a child of ours and animate
   * the rows to the correct size accordingly.
   */
  if (focused)
    {
      ClutterActor *parent = clutter_actor_get_parent (focused);

      while (parent)
        {
          if (parent == (ClutterActor *)self)
            {
              priv->has_focus = TRUE;

              if (priv->current_focus != focused)
                mex_grid_set_focused_actor (self, focused);
              else
                mex_grid_start_animation (self);

              return;
            }

          focused = parent;
          parent = clutter_actor_get_parent (focused);
        }
    }

  /* Lose focus and animate back to normal size */
  priv->has_focus = FALSE;
  mex_grid_start_animation (self);
}

static void
mex_grid_map (ClutterActor *actor)
{
  MxFocusManager *manager;

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->map (actor);

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    {
      g_signal_connect (manager, "notify::focused",
                        G_CALLBACK (mex_grid_notify_focused_cb), actor);
      mex_grid_notify_focused_cb (manager, NULL, (MexGrid *)actor);
    }
}

static void
mex_grid_unmap (ClutterActor *actor)
{
  MxFocusManager *manager;

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    g_signal_handlers_disconnect_by_func (manager,
                                          mex_grid_notify_focused_cb,
                                          actor);

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->unmap (actor);
}

static void
mex_grid_class_init (MexGridClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexGridPrivate));

  object_class->get_property = mex_grid_get_property;
  object_class->set_property = mex_grid_set_property;
  object_class->dispose = mex_grid_dispose;
  object_class->finalize = mex_grid_finalize;

  actor_class->get_preferred_width = mex_grid_get_preferred_width;
  actor_class->get_preferred_height = mex_grid_get_preferred_height;
  actor_class->allocate = mex_grid_allocate;
  actor_class->apply_transform = mex_grid_apply_transform;
  actor_class->paint = mex_grid_paint;
  actor_class->pick = mex_grid_pick;
  actor_class->destroy = mex_grid_destroy;
  actor_class->map = mex_grid_map;
  actor_class->unmap = mex_grid_unmap;

  pspec = g_param_spec_int ("stride",
                            "Stride",
                            "Amount of widgets to pack horizontally. "
                            "0 = Automatic, based on tile width.",
                            0, G_MAXINT, 3,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_STRIDE, pspec);

  pspec = g_param_spec_float ("tile-width", "Tile width",
                              "Minimum width to set on new tiles",
                              -1, G_MAXFLOAT, 360,
                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TILE_WIDTH, pspec);

  pspec = g_param_spec_float ("tile-height", "Tile height",
                              "Minimum height to set on new tiles",
                              -1, G_MAXFLOAT, 202,
                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TILE_HEIGHT, pspec);

  /* MxScrollable properties */
  g_object_class_override_property (object_class,
                                    PROP_HADJUST,
                                    "horizontal-adjustment");

  g_object_class_override_property (object_class,
                                    PROP_VADJUST,
                                    "vertical-adjustment");
}

/* Following function copied from MexDrawersBox */
static void
mex_grid_timeline_completed_cb (ClutterTimeline *timeline,
                                ClutterActor    *box)
{
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
}

static void
mex_grid_style_changed_cb (MxStylable          *stylable,
                           MxStyleChangedFlags  flags,
                           MexGrid             *self)
{
  MxBorderImage *highlight;

  MexGridPrivate *priv = self->priv;

  mx_stylable_get (stylable,
                   "x-mex-highlight", &highlight,
                   NULL);

  mex_replace_border_image (&priv->highlight,
                            highlight,
                            &priv->highlight_image,
                            &priv->highlight_material);

  /* This is a hack to get around style-changed iterating over
   * all our children. As we may have hundreds, or even thousands
   * of children, we need to intercept this.
   */
  priv->next_foreach_is_style_changed = TRUE;

  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

static void
mex_grid_init (MexGrid *self)
{
  MexGridPrivate *priv = self->priv = GRID_PRIVATE (self);

  priv->children = g_array_new (FALSE, FALSE, sizeof (ClutterActor *));
  priv->first_visible = priv->last_visible = -1;
  priv->row_sizes = g_array_new (FALSE, TRUE, sizeof (MexGridRowData));
  priv->spans = g_array_new (FALSE, TRUE, sizeof (MexGridSpan));
  priv->boxes = g_array_new (FALSE, TRUE, sizeof (ClutterActorBox));
  priv->stride = 3;
  priv->tile_width = 360;
  priv->tile_height = 202;

  priv->anim_length = 150;
  priv->timeline = clutter_timeline_new (priv->anim_length);
  priv->alpha = clutter_alpha_new_full (priv->timeline, CLUTTER_EASE_OUT_QUAD);

  g_signal_connect_swapped (priv->timeline, "new-frame",
                            G_CALLBACK (clutter_actor_queue_relayout),
                            self);
  g_signal_connect (priv->timeline, "completed",
                    G_CALLBACK (mex_grid_timeline_completed_cb),
                    self);

  g_signal_connect_after (self, "style-changed",
                          G_CALLBACK (mex_grid_style_changed_cb), self);
}

ClutterActor *
mex_grid_new (void)
{
  return g_object_new (MEX_TYPE_GRID, NULL);
}

gint
mex_grid_get_stride (MexGrid *grid)
{
  g_return_val_if_fail (MEX_IS_GRID (grid), 0);
  return grid->priv->stride;
}

void
mex_grid_set_stride (MexGrid *grid, gint stride)
{
  MexGridPrivate *priv;

  g_return_if_fail (MEX_IS_GRID (grid));
  g_return_if_fail (stride >= 0);

  priv = grid->priv;
  if (priv->stride != stride)
    {
      priv->real_stride = priv->stride = stride;

      g_object_notify (G_OBJECT (grid), "stride");

      /* Starting the animation will also ensure we have
       * the row data initialised and the stride calculated.
       */
      mex_grid_start_animation (grid);
    }
}

void
mex_grid_get_tile_size (MexGrid *grid,
                        gfloat  *width,
                        gfloat  *height)
{
  g_return_if_fail (MEX_IS_GRID (grid));

  if (width)
    *width = grid->priv->tile_width;
  if (height)
    *height = grid->priv->tile_height;
}

static void
mex_grid_refresh (MexGrid *grid)
{
  MexGridPrivate *priv = grid->priv;

  clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));

  /* If we have a stored focused actor, recalculate the row
   * that it's in. If we're focused, also re-animate the
   * focused row.
   */
  if (priv->current_focus)
    mex_grid_set_focused_actor (grid, priv->has_focus ?
                                priv->current_focus : NULL);
}

void
mex_grid_set_sort_func (MexGrid          *grid,
                        MexActorSortFunc  func,
                        gpointer          userdata)
{
  MexGridPrivate *priv;

  g_return_if_fail (MEX_IS_GRID (grid));

  priv = grid->priv;
  if ((priv->sort_func != func) || (priv->sort_data != userdata))
    {
      priv->sort_func = func;
      priv->sort_data = userdata;

      if (func)
        {
          g_array_sort_with_data (priv->children,
                                  (GCompareDataFunc)mex_grid_sort_func,
                                  grid);

          mex_grid_refresh (grid);
        }
    }
}

MexActorSortFunc
mex_grid_get_sort_func (MexGrid  *grid,
                        gpointer *userdata)
{
  g_return_val_if_fail (MEX_IS_GRID (grid), NULL);

  if (userdata)
    *userdata = grid->priv->sort_data;

  return grid->priv->sort_func;
}

void
mex_grid_resort (MexGrid      *grid,
                 ClutterActor *child)
{
  gint i;
  MexGridPrivate *priv;

  g_return_if_fail (MEX_IS_GRID (grid));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  priv = grid->priv;

  if (!priv->sort_func)
    return;

  if (clutter_actor_get_parent (child) != (ClutterActor *)grid)
    {
      g_warning (G_STRLOC ": Child does not belong to grid");
      return;
    }

  for (i = 0; i < priv->children->len; i++)
    {
      ClutterActor *actor = g_array_index (priv->children, ClutterActor *, i);
      if (actor == child)
        {
          g_array_remove_index (priv->children, i);
          mex_grid_insert_sorted (grid, actor);

          mex_grid_refresh (grid);

          return;
        }
    }

  g_warning (G_STRLOC ": Child not found in grid");
}

void
mex_grid_resort_all (MexGrid      *grid,
                     ClutterActor *child)
{
  MexGridPrivate *priv;
  MexActorSortFunc func;

  g_return_if_fail (MEX_IS_GRID (grid));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  priv = grid->priv;

  if (!priv->sort_func)
    return;

  func = priv->sort_func;
  priv->sort_func = NULL;
  mex_grid_set_sort_func (grid, func, priv->sort_data);
}
