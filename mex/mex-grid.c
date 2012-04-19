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
#include "mex-scrollable-container.h"
#include "mex-content-tile.h"
#include <math.h>

#define DEFAULT_TILE_RATIO (9.0 / 16.0)
#define SPACING 6.0

static void mx_scrollable_iface_init (MxScrollableIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);
static void mex_scrollable_container_iface_init (MexScrollableContainerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexGrid, mex_grid, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_SCROLLABLE,
                                                mx_scrollable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_SCROLLABLE_CONTAINER,
                                                mex_scrollable_container_iface_init))

#define GRID_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GRID, MexGridPrivate))

struct _MexGridPrivate
{
  guint            has_focus : 1;
  guint            next_foreach_is_style_changed : 1;
  guint            tile_width_changed : 1;
  guint            tile_height_changed : 1;
  guint            focus_waiting;

  GArray          *children;
  ClutterActor    *current_focus;
  gint             focused_row;
  MexActorSortFunc sort_func;
  gpointer         sort_data;

  gint             stride;

  ClutterAlpha    *alpha;
  ClutterTimeline *timeline;
  guint            anim_length;
  gdouble          initial_row;
  gdouble          animated_row;

  MxAdjustment    *vadjust;

  gint             first_visible;
  gint             last_visible;
  gfloat           tile_width;
  gfloat           tile_height;
  gfloat           tile_ratio;

  CoglHandle       highlight;
  CoglHandle       highlight_material;
  MxBorderImage   *highlight_image;

  MexModel        *model;
};

enum
{
  PROP_0,

  PROP_STRIDE,
  PROP_HADJUST,
  PROP_VADJUST,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_TILE_RATIO
};

static void mex_grid_start_animation (MexGrid *self);


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
      dx = -priv->stride;
      hint = MX_FOCUS_HINT_FIRST;
      break;

    case MX_FOCUS_DIRECTION_DOWN:
      dx = priv->stride;
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
              ((index / priv->stride) ==
               ((priv->children->len - 1) / priv->stride) - 1))
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
                  ((i + 1) % priv->stride == 0))
                break;
              if ((direction == MX_FOCUS_DIRECTION_RIGHT) &&
                  (i % priv->stride == 0))
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
              (priv->children->len > priv->stride) &&
              ((index % priv->stride) != (priv->stride - 1)) &&
              ((index / priv->stride) ==
               ((priv->children->len - 1) / priv->stride)))
            {
              child = g_array_index (priv->children, ClutterActor *,
                                     priv->children->len - priv->stride);
              if (MX_IS_FOCUSABLE (child) &&
                  (focusable = mx_focusable_accept_focus (MX_FOCUSABLE (child),
                                                          hint)))
                {
                  i = priv->children->len - priv->stride;
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

      /* Update the focused child/row pointers */
      priv->current_focus = child;
      priv->focused_row = i / priv->stride;
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
              priv->focused_row = i / priv->stride;
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

/* MexScrollableContainerInterface */

static void
mex_grid_get_tile_size (MexGrid               *grid,
                        const ClutterActorBox *box,
                        gfloat                *basic_width,
                        gfloat                *basic_height)
{
  MxPadding padding;
  ClutterActor *first_child;

  MexGridPrivate *priv = grid->priv;

  mx_widget_get_padding (MX_WIDGET (grid), &padding);

  *basic_width = floorf ((box->x2 - box->x1 - padding.right - padding.left) /
                         (gfloat) priv->stride);

  first_child = g_array_index (priv->children, ClutterActor *, 0);

  clutter_actor_get_preferred_height (first_child, *basic_width, NULL,
                                      basic_height);

  *basic_height -= SPACING;
  *basic_width -= SPACING;
}

static void
mex_grid_get_allocation (MexScrollableContainer *self,
                         ClutterActor           *child,
                         ClutterActorBox        *box)
{
  gboolean found;
  MxPadding padding;
  gint i, row, column;
  ClutterActorBox grid_box;
  gfloat avail_width, basic_width, basic_height;

  MexGrid *grid = MEX_GRID (self);
  MexGridPrivate *priv = grid->priv;

  /* Figure out what row the child is on */
  found = FALSE;
  for (i = 0; i < priv->children->len; i++)
    if (g_array_index (priv->children, ClutterActor *, i) == child)
      {
        found = TRUE;
        break;
      }

  if (!found)
    {
      g_warning (G_STRLOC ": Can't give allocation for child not in grid");
      return;
    }

  row = i / priv->stride;
  column = i % priv->stride;

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (grid), &grid_box);
  mx_widget_get_padding (MX_WIDGET (grid), &padding);
  mex_grid_get_tile_size (grid, &grid_box, &basic_width, &basic_height);
  avail_width = grid_box.x2 - grid_box.x1 - padding.left - padding.right;

  box->x1 = column * (basic_width + SPACING);

  if (row > 0)
    {
      box->y1 = row * (basic_height + SPACING);
    }
  else
    box->y1 = 0;

  clutter_actor_get_preferred_size (child, NULL, NULL, &box->x2, &box->y2);
  box->x2 += box->x1;
  box->y2 += box->y1;

  if (box->x2 > avail_width)
    {
      box->x1 -= (box->x2 - avail_width);
      box->x2 = avail_width;
    }
}

static void
mex_scrollable_container_iface_init (MexScrollableContainerInterface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
      is_initialized = TRUE;

      iface->get_allocation = mex_grid_get_allocation;
    }
}

/* Actor implementation */

static void
mex_grid_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  MxAdjustment *adjustment;
  MexGrid *self = MEX_GRID (object);

  switch (property_id)
    {
    case PROP_STRIDE:
      g_value_set_int (value, mex_grid_get_stride (self));
      break;

    case PROP_HADJUST:
      mex_grid_get_adjustments (MX_SCROLLABLE (object), &adjustment, NULL);
      g_value_set_object (value, adjustment);
      break;

    case PROP_VADJUST:
      mex_grid_get_adjustments (MX_SCROLLABLE (object), NULL, &adjustment);
      g_value_set_object (value, adjustment);
      break;

    case PROP_TILE_WIDTH:
      g_value_set_float (value, self->priv->tile_width);
      break;

    case PROP_TILE_HEIGHT:
      g_value_set_float (value, self->priv->tile_height);
      break;

    case PROP_TILE_RATIO:
      g_value_set_float (value, self->priv->tile_ratio);
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

    case PROP_TILE_RATIO:
      self->priv->tile_ratio = g_value_get_float (value);
      g_object_notify (object, "tile-ratio");
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
      clutter_timeline_stop (priv->timeline);
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


  /* remove signal handlers from model controller and remove all children */
  mex_grid_set_model (MEX_GRID (object), NULL);


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
  MexGridPrivate *priv = self->priv;

  if (!priv->timeline)
    return;

  if (!priv->children->len)
    {
      clutter_timeline_stop (priv->timeline);
      return;
    }

  priv->initial_row = priv->animated_row;

  clutter_timeline_set_direction (priv->timeline, CLUTTER_TIMELINE_FORWARD);
  clutter_timeline_rewind (priv->timeline);
  clutter_timeline_start (priv->timeline);
}

static void
mex_grid_get_preferred_width (ClutterActor *actor,
                              gfloat        for_height,
                              gfloat       *min_width_p,
                              gfloat       *nat_width_p)
{
  gfloat min_width, width;
  MxPadding padding;

  MexGridPrivate *priv = MEX_GRID (actor)->priv;

  if (!priv->children->len)
    min_width = width = 0;
  else
    {
      ClutterActor *child = g_array_index (priv->children, ClutterActor *, 0);
      clutter_actor_get_preferred_width (child, -1, NULL, &min_width);
      width = min_width * priv->stride;
    }

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_width_p)
    *min_width_p = min_width + padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p = width + padding.left + padding.right;
}

static void
mex_grid_get_preferred_height (ClutterActor *actor,
                               gfloat        for_width,
                               gfloat       *min_height_p,
                               gfloat       *nat_height_p)
{
  gfloat height;
  MxPadding padding;

  MexGridPrivate *priv = MEX_GRID (actor)->priv;

  if (!priv->children->len)
    height = 0;
  else
    {
      ClutterActor *child = priv->current_focus ?
        priv->current_focus :
        g_array_index (priv->children, ClutterActor *, 0);

      clutter_actor_get_preferred_height (child, -1, NULL, &height);
    }

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  height += padding.top + padding.bottom;

  if (min_height_p)
    *min_height_p = height;
  if (nat_height_p)
    *nat_height_p = height;
}

static void
mex_grid_allocate (ClutterActor           *actor,
                   const ClutterActorBox  *box,
                   ClutterAllocationFlags  flags)
{
  gdouble value;
  MxPadding padding;
  ClutterActorBox child_box;
  gint i, first_row, last_row;
  gfloat basic_width, basic_height, avail_width, avail_height, bottom;

  MexGrid *self = MEX_GRID (actor);
  MexGridPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->allocate (actor, box, flags);

  /* Bail out if we have no children */
  priv->first_visible = priv->last_visible = -1;
  if (!priv->children->len)
    return;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  avail_width = box->x2 - box->x1 - padding.left - padding.right;
  avail_height = box->y2 - box->y1 - padding.top - padding.bottom;

  /* Allocate all actors in the visible range their preferred size. The
   * visible range is based on the preferred height of the first actor.
   */
  mex_grid_get_tile_size (self, box, &basic_width, &basic_height);

  /* Check if the basic size has changed and set a variable to notify later.
   * We notify later to avoid allocation cycles, so children can bind their
   * size properties directly to the tile-width property.
   */
  if (priv->tile_width != basic_width)
    {
      priv->tile_width = basic_width;
      priv->tile_width_changed = TRUE;
    }

  if (priv->tile_height != basic_height)
    {
      priv->tile_height = basic_height;
      priv->tile_height_changed = TRUE;
    }

  if (priv->vadjust)
    value = (gint)mx_adjustment_get_value (priv->vadjust);
  else
    value = 0;

  /* Calculate our visible range - we buffer it by a few rows, for lingering
   * animations/rounding errors.
   */
  first_row = MAX (0, (value / (gint)(basic_height)) - 3);
  priv->first_visible = first_row * priv->stride;
  last_row = ((value + avail_height) / (gint)(basic_height)) + 3;
  priv->last_visible = MIN (priv->children->len - 1, last_row * priv->stride);

  bottom = 0;

  child_box.y1 = first_row * (basic_height + SPACING);

  /* Allocate all the visible children */
  for (i = first_row; i <= last_row; i++)
    {
      gint j;

      for (j = i * priv->stride;
           j < MIN ((i + 1) * priv->stride, priv->children->len); j++)
        {
          ClutterActor *child =
            g_array_index (priv->children, ClutterActor *, j);

          child_box.x1 = ((basic_width + SPACING) * (j % priv->stride)) + padding.left;

          /* Get the preferred size of the child */
          clutter_actor_get_preferred_size (child, NULL, NULL,
                                            &child_box.x2, &child_box.y2);
          child_box.x2 += child_box.x1;
          child_box.y2 += child_box.y1;

          if (child_box.y2 > bottom)
            bottom = child_box.y2;

          /* Make sure the box stays within the bounds of the parents */
          if (child_box.x2 > avail_width)
            {
              child_box.x1 -= child_box.x2 - avail_width;
              child_box.x2 = avail_width;
            }

          child_box.x1 = (int) child_box.x1;
          child_box.x2 = (int) child_box.x2;
          child_box.y1 = (int) child_box.y1;
          child_box.y2 = (int) child_box.y2;

          clutter_actor_allocate (child, &child_box, flags);
        }

      if (j >= priv->children->len)
        break;

      child_box.y1 += basic_height + SPACING * 2;
    }

  /* Set the adjustment values */
  if (priv->vadjust)
    {
      if (priv->last_visible != priv->children->len - 1)
        bottom = ((priv->children->len - 1) / priv->stride + 1) * (basic_height + SPACING);

      g_object_set (G_OBJECT (priv->vadjust),
                    "lower", 0.0,
                    "upper", (gdouble) bottom,
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
                           0, (int) -mx_adjustment_get_value (priv->vadjust), 0);
}

static void
mex_grid_paint (ClutterActor *actor)
{
  gint i;
  gfloat y;
  MxPadding padding;
  ClutterActorBox box;
  guint8 paint_opacity;
  gboolean draw_focus;

  MexGrid *self = MEX_GRID (actor);
  MexGridPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->paint (actor);

  if (priv->first_visible == -1)
    return;

  clutter_actor_get_allocation_box (actor, &box);
  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (priv->vadjust)
    y = (int) mx_adjustment_get_value (priv->vadjust);
  else
    y = 0;

  cogl_clip_push_rectangle (0, y,
                            box.x2 - box.x1,
                            y + box.y2 - box.y1);

  draw_focus = FALSE;
  paint_opacity = clutter_actor_get_paint_opacity (actor);

  /* Draw children */
  for (i = priv->first_visible; i <= priv->last_visible; i++)
    {
      ClutterActor *child = g_array_index (priv->children, ClutterActor *, i);

      /* Paint child */
      if (priv->has_focus && (child == priv->current_focus))
        draw_focus = TRUE;
      else
        clutter_actor_paint (child);
    }

  /* Draw the focused actor */
  if (draw_focus)
    {
      clutter_actor_paint (priv->current_focus);
    }

  /* Unset the clip around the actor */
  cogl_clip_pop ();


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

  /* Notify the tile-size properties */
  if (priv->tile_width_changed)
    {
      priv->tile_width_changed = FALSE;
      g_object_notify (G_OBJECT (actor), "tile-width");
    }
  if (priv->tile_height_changed)
    {
      priv->tile_height_changed = FALSE;
      g_object_notify (G_OBJECT (actor), "tile-height");
    }
}

static void
mex_grid_pick (ClutterActor *actor, const ClutterColor *color)
{
  gint i;
  gboolean draw_focus;
  MexGridPrivate *priv = MEX_GRID (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_grid_parent_class)->pick (actor, color);

  if (priv->first_visible == -1)
    return;

  draw_focus = FALSE;

  for (i = priv->first_visible; i <= priv->last_visible; i++)
    {
      ClutterActor *child = g_array_index (priv->children, ClutterActor *, i);
      if (priv->has_focus && (child == priv->current_focus))
        draw_focus = TRUE;
      else
        clutter_actor_paint (child);
    }

  if (draw_focus)
    clutter_actor_paint (priv->current_focus);
}

static void
mex_grid_destroy (ClutterActor *actor)
{
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

      priv->focused_row = i / priv->stride;
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

static gboolean
mex_grid_get_paint_volume (ClutterActor       *actor,
                           ClutterPaintVolume *volume)
{
  MexGridPrivate *priv = MEX_GRID (actor)->priv;
  ClutterVertex v;

  if (!clutter_paint_volume_set_from_allocation (volume, actor))
    return FALSE;

  clutter_paint_volume_get_origin (volume, &v);
  v.y += mx_adjustment_get_value (priv->vadjust),
  clutter_paint_volume_set_origin (volume, &v);

  return TRUE;
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
  actor_class->get_paint_volume = mex_grid_get_paint_volume;

  pspec = g_param_spec_int ("stride",
                            "Stride",
                            "Amount of widgets to pack horizontally.",
                            1, G_MAXINT, 3,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_STRIDE, pspec);

  pspec = g_param_spec_float ("tile-width",
                              "Tile width",
                              "Convenience property representing the width "
                              "of a tile in the grid.",
                              0.f, G_MAXFLOAT, 0.f,
                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TILE_WIDTH, pspec);

  pspec = g_param_spec_float ("tile-height",
                              "Tile height",
                              "Convenience property representing the height "
                              "of a row in the grid.",
                              0.f, G_MAXFLOAT, 0.f,
                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TILE_HEIGHT, pspec);

  pspec = g_param_spec_float ("tile-ratio",
                              "Tile ratio",
                              "The aspect ratio of the tile",
                              0.f, G_MAXFLOAT, DEFAULT_TILE_RATIO,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TILE_RATIO, pspec);

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
                                MexGrid         *self)
{
  MexGridPrivate *priv = self->priv;
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

  priv->animated_row = priv->focused_row;
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
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
mex_grid_timeline_new_frame_cb (ClutterTimeline *timeline,
                                gint             msecs,
                                MexGrid         *self)
{
  gdouble progress;
  MexGridPrivate *priv = self->priv;

  progress = clutter_alpha_get_alpha (priv->alpha);
  priv->animated_row = ((1.0 - progress) * priv->initial_row) +
                       (progress * priv->focused_row);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

static void
mex_grid_init (MexGrid *self)
{
  MexGridPrivate *priv = self->priv = GRID_PRIVATE (self);

  priv->children = g_array_new (FALSE, FALSE, sizeof (ClutterActor *));
  priv->first_visible = priv->last_visible = -1;
  priv->stride = 3;

  priv->anim_length = 150;
  priv->timeline = clutter_timeline_new (priv->anim_length);
  priv->alpha = clutter_alpha_new_full (priv->timeline, CLUTTER_EASE_OUT_QUAD);

  g_signal_connect (priv->timeline, "new-frame",
                    G_CALLBACK (mex_grid_timeline_new_frame_cb), self);
  g_signal_connect (priv->timeline, "completed",
                    G_CALLBACK (mex_grid_timeline_completed_cb),
                    self);

  g_signal_connect_after (self, "style-changed",
                          G_CALLBACK (mex_grid_style_changed_cb), self);

  priv->tile_ratio = DEFAULT_TILE_RATIO;
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
      priv->stride = stride;

      g_object_notify (G_OBJECT (grid), "stride");

      /* Starting the animation will also ensure we have
       * the row data initialised and the stride calculated.
       */
      mex_grid_start_animation (grid);
    }
}

/**
 * mex_grid_add_content:
 *
 * Add an item to the grid for the given content and position
 */
static void
mex_grid_add_content (MexGrid    *grid,
                      MexContent *content,
                      gint        position)
{
  MexGridPrivate *priv = grid->priv;
  ClutterActor *box;

  box = mex_content_box_new ();

  mex_content_box_set_important (MEX_CONTENT_BOX (box), TRUE);

  /* Make sure the tiles stay the correct size */
  g_object_bind_property (grid, "tile-width",
                          box, "thumb-width",
                          G_BINDING_SYNC_CREATE);

  /* Make sure expanded grid tiles fill a nice-looking space */
  g_object_bind_property (grid, "tile-width",
                          box, "action-list-width",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (grid, "tile-ratio",
                          box, "thumb-ratio",
                          G_BINDING_SYNC_CREATE);

  mex_content_view_set_content (MEX_CONTENT_VIEW (box), content);
  mex_content_view_set_context (MEX_CONTENT_VIEW (box), priv->model);

  clutter_actor_set_parent (box, CLUTTER_ACTOR (grid));

  g_array_insert_val (priv->children, position, box);
}

/**
 * mex_grid_clear:
 *
 * Remove all the items from the grid
 */
static void
mex_grid_clear (MexGrid *grid)
{
  MexGridPrivate *priv = grid->priv;

  /* remove all children */
  while (priv->children->len > 0)
    {
      clutter_actor_destroy (g_array_index (priv->children, ClutterActor*,
                                            0));
      g_array_remove_index_fast (priv->children, 0);
    }
  priv->current_focus = NULL;
}

/**
 * mex_grid_populate:
 *
 * Populate the grid with the items from the model
 */
static void
mex_grid_populate (MexGrid *grid)
{
  MexContent *content;
  gint i = 0;

  while ((content = mex_model_get_content (grid->priv->model, i)))
    mex_grid_add_content (grid, content, i++);
}

static void
mex_grid_controller_changed (GController          *controller,
                             GControllerAction     action,
                             GControllerReference *ref,
                             MexGrid              *grid)
{
  MexGridPrivate *priv = grid->priv;
  gint i, n_indices;
  MexContent *content;
  ClutterActor *box;

  n_indices = g_controller_reference_get_n_indices (ref);

  switch (action)
    {
    case G_CONTROLLER_ADD:
      for (i = 0; i < n_indices; i++)
        {
          gint content_index = g_controller_reference_get_index_uint (ref, i);
          content = mex_model_get_content (priv->model, content_index);

          mex_grid_add_content (grid, content, content_index);
        }
      break;

    case G_CONTROLLER_REMOVE:
      for (i = 0; i < n_indices; i++)
        {
          gint content_index = g_controller_reference_get_index_uint (ref, i);

          box = g_array_index (priv->children, ClutterActor*, content_index);


          if (box == priv->current_focus)
            priv->current_focus = NULL;

          clutter_actor_destroy (box);
          g_array_remove_index (priv->children, content_index);
        }
      break;

    case G_CONTROLLER_UPDATE:
      /* Should be no need for this, GBinding sorts it out for us :) */
      break;

    case G_CONTROLLER_CLEAR:
      mex_grid_clear (grid);
      break;

    case G_CONTROLLER_REPLACE:
      mex_grid_clear (grid);
      mex_grid_populate (grid);
      break;

    case G_CONTROLLER_INVALID_ACTION:
      g_warning (G_STRLOC ": Controller has issued an error");
      break;

    default:
      g_warning (G_STRLOC ": Unhandled action");
      break;
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
}

void
mex_grid_set_model (MexGrid *grid, MexModel *model)
{
  MexGridPrivate *priv;
  GController *controller;

  g_return_if_fail (MEX_IS_GRID (grid));
  g_return_if_fail (model == NULL || MEX_IS_MODEL (model));

  priv = grid->priv;

  if (priv->model)
    {
      /* remove all children */
      mex_grid_clear (grid);

      /* remove "changed" signal handler */
      controller = mex_model_get_controller (priv->model);
      g_signal_handlers_disconnect_by_func (controller,
                                            mex_grid_controller_changed, grid);

      g_object_unref (priv->model);
    }

  if (model)
    {
      priv->model = g_object_ref (model);

      /* add items currently in the model */
      mex_grid_populate (grid);


      controller = mex_model_get_controller (model);
      g_signal_connect (controller, "changed",
                        G_CALLBACK (mex_grid_controller_changed), grid);
    }
  else
    {
      priv->model = NULL;
    }
}

MexModel *
mex_grid_get_model (MexGrid *grid)
{
  g_return_val_if_fail (MEX_IS_GRID (grid), NULL);

  return grid->priv->model;
}
