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


#include "mex-tile.h"

static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexTile, mex_tile, MX_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define TILE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TILE, MexTilePrivate))

#define DURATION 500

enum
{
  PROP_0,

  PROP_PRIMARY_ICON,
  PROP_SECONDARY_ICON,
  PROP_LABEL,
  PROP_HEADER_VISIBLE,
  PROP_IMPORTANT
};

enum
{
  PRIMARY_ICON_CLICKED,
  SECONDARY_ICON_CLICKED,

  LAST_SIGNAL
};

struct _MexTilePrivate
{
  guint            has_focus : 1;
  guint            header_visible : 1;
  guint            important : 1;

  ClutterActor    *table;
  ClutterActor    *icon1;
  ClutterActor    *icon2;
  ClutterActor    *label;

  ClutterTimeline *timeline;
  ClutterAlpha    *important_alpha;
};

static guint signals[LAST_SIGNAL] = { 0, };


/* MxStylableIface */

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
      GParamSpec *pspec;

      is_initialized = TRUE;

      pspec = g_param_spec_uint ("x-mex-label-spacing",
                                 "Label spacing",
                                 "Spacing between the label and "
                                 "the edge of the tile.",
                                 0, G_MAXUINT, 32,
                                 G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_TILE, pspec);
    }
}

/* Actor implementation */

static void
mex_tile_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  MexTile *self = MEX_TILE (object);

  switch (property_id)
    {
    case PROP_PRIMARY_ICON:
      g_value_set_object (value, mex_tile_get_primary_icon (self));
      break;

    case PROP_SECONDARY_ICON:
      g_value_set_object (value, mex_tile_get_secondary_icon (self));
      break;

    case PROP_LABEL:
      g_value_set_string (value, mex_tile_get_label (self));
      break;

    case PROP_HEADER_VISIBLE:
      g_value_set_boolean (value, mex_tile_get_header_visible (self));
      break;

    case PROP_IMPORTANT:
      g_value_set_boolean (value, mex_tile_get_important (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tile_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  MexTile *self = MEX_TILE (object);

  switch (property_id)
    {
    case PROP_PRIMARY_ICON:
      mex_tile_set_primary_icon (self, g_value_get_object (value));
      break;

    case PROP_SECONDARY_ICON:
      mex_tile_set_secondary_icon (self, g_value_get_object (value));
      break;

    case PROP_LABEL:
      mex_tile_set_label (self, g_value_get_string (value));
      break;

    case PROP_HEADER_VISIBLE:
      mex_tile_set_header_visible (self, g_value_get_boolean (value));
      break;

    case PROP_IMPORTANT:
      mex_tile_set_important (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tile_dispose (GObject *object)
{
  MexTile *self = MEX_TILE (object);
  MexTilePrivate *priv = self->priv;

  /* Use icon setting functions to disconnect from signal handlers */
  mex_tile_set_primary_icon (self, NULL);
  mex_tile_set_secondary_icon (self, NULL);

  if (priv->table)
    {
      /* Label is in table, so no need to destroy it */
      clutter_actor_unparent (priv->table);
      priv->table = NULL;
    }

  if (priv->important_alpha)
    {
      g_object_unref (priv->important_alpha);
      priv->important_alpha = NULL;
    }

  if (priv->timeline)
    {
      clutter_timeline_stop (priv->timeline);
      g_object_unref (priv->timeline);
      priv->timeline = NULL;
    }

  G_OBJECT_CLASS (mex_tile_parent_class)->dispose (object);
}

static void
mex_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_tile_parent_class)->finalize (object);
}

static void
mex_tile_get_preferred_height (ClutterActor *actor,
                               gfloat        for_width,
                               gfloat       *min_height_p,
                               gfloat       *nat_height_p)
{
  MxPadding padding;
  gfloat box_height;

  MexTilePrivate *priv = MEX_TILE (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->
    get_preferred_height (actor, for_width, NULL, nat_height_p);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  for_width -= padding.left + padding.right;

  clutter_actor_get_preferred_height (priv->table,
                                      for_width,
                                      NULL,
                                      &box_height);

  /* Override the minimum height with the height of the box */
  if (min_height_p)
    *min_height_p = box_height;

  if (nat_height_p)
    {
      gdouble progress;

      /* When the timeline is at 0.5, the actor isn't visible - this is
       * the time where we switch sizes between the natural height and the
       * box height.
       */
      if (clutter_alpha_get_alpha (priv->important_alpha) < 0.5)
        progress = 0.0;
      else
        progress = 1.0;

      if (*nat_height_p < box_height)
        *nat_height_p = box_height;
      else if (progress == 0.0)
        *nat_height_p = box_height;
      else if (progress < 1.0)
        *nat_height_p = (box_height * (1.0 - progress)) +
                        (*nat_height_p * progress);
    }
}

static void
mex_tile_allocate (ClutterActor           *actor,
                   const ClutterActorBox  *box,
                   ClutterAllocationFlags  flags)
{
  MxPadding padding;
  ClutterActor *child;
  ClutterActorBox child_box;
  gfloat available_width, available_height, box_height;

  MexTilePrivate *priv = MEX_TILE (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  available_width = box->x2 - box->x1 - padding.left - padding.right;
  available_height = box->y2 - box->y1 - padding.top - padding.bottom;

  clutter_actor_get_preferred_height (priv->table,
                                      available_width,
                                      NULL,
                                      &box_height);

  /* Allocate child */
  child = mx_bin_get_child (MX_BIN (actor));
  if (child)
    {
      gboolean x_fill, y_fill;
      MxAlign x_align, y_align;
      gfloat child_width, full_width, full_height;

      clutter_actor_get_preferred_size (child, NULL, NULL,
                                        &full_width, &full_height);

      child_box.y1 = padding.top;

      if (clutter_alpha_get_alpha (priv->important_alpha) < 0.5)
        {
          child_width = full_width * (available_height / full_height);
          if (child_width > available_width)
            child_width = available_width;

          child_box.y2 = child_box.y1 + available_height;

          /* When we're in unimportant state, make sure the label
           * doesn't overlap the image.
           */
          if (available_height < full_height)
            available_width -= child_width *
              ((0.5 - clutter_alpha_get_alpha (priv->important_alpha)) * 2);
        }
      else
        {
          child_width = available_width;
          clutter_actor_set_clip_to_allocation (
            actor, (full_height > available_height));

          child_box.y2 = child_box.y1 + full_height;
        }

      child_box.x2 = box->x2 - box->x1 - padding.right;
      child_box.x1 = child_box.x2 - child_width;

      mx_bin_get_fill (MX_BIN (actor), &x_fill, &y_fill);
      mx_bin_get_alignment (MX_BIN (actor), &x_align, &y_align);

      mx_allocate_align_fill (child, &child_box,
                              x_align, y_align, x_fill, y_fill);
      clutter_actor_allocate (child, &child_box, flags);
    }

  /* Allocate title-box */
  child_box.x1 = padding.left;
  child_box.y1 = padding.top;

  child_box.y2 = child_box.y1 + box_height;
  child_box.x2 = child_box.x1 + available_width;

  clutter_actor_allocate (priv->table, &child_box, flags);
}

static void
mex_tile_paint (ClutterActor *actor)
{
  MexTilePrivate *priv = MEX_TILE (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->paint (actor);

  clutter_actor_paint (priv->table);
}

static void
mex_tile_notify_focused_cb (MxFocusManager *manager,
                            GParamSpec     *pspec,
                            MexTile        *self)
{
  ClutterActor *focus;

  MexTilePrivate *priv = self->priv;

  focus = (ClutterActor *)mx_focus_manager_get_focused (manager);

  if (focus)
    {
      ClutterActor *parent = clutter_actor_get_parent (focus);
      while (parent)
        {
          if (focus == (ClutterActor *)self)
            {
              if (!priv->has_focus)
                {
                  priv->has_focus = TRUE;
                  mx_stylable_style_pseudo_class_add (MX_STYLABLE (self),
                                                      "focus");
                }

              return;
            }

          focus = parent;
          parent = clutter_actor_get_parent (focus);
        }
    }

  priv->has_focus = FALSE;
  mx_stylable_style_pseudo_class_remove (MX_STYLABLE (self), "focus");
}

static void
mex_tile_map (ClutterActor *actor)
{
  MxFocusManager *manager;
  MexTilePrivate *priv = MEX_TILE (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->map (actor);

  clutter_actor_map (priv->table);

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    {
      g_signal_connect (manager, "notify::focused",
                        G_CALLBACK (mex_tile_notify_focused_cb), actor);
      mex_tile_notify_focused_cb (manager, NULL, (MexTile *)actor);
    }
}

static void
mex_tile_unmap (ClutterActor *actor)
{
  MxFocusManager *manager;
  MexTilePrivate *priv = MEX_TILE (actor)->priv;

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    g_signal_handlers_disconnect_by_func (manager,
                                          mex_tile_notify_focused_cb,
                                          actor);

  clutter_actor_unmap (priv->table);

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->unmap (actor);
}

static void
mex_tile_apply_style (MxWidget *widget,
                      MxStyle  *style)
{
  MexTilePrivate *priv = MEX_TILE (widget)->priv;
  mx_stylable_set_style (MX_STYLABLE (priv->table), style);
}

static void
mex_tile_class_init (MexTileClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MxWidgetClass *widget_class = MX_WIDGET_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexTilePrivate));

  object_class->get_property = mex_tile_get_property;
  object_class->set_property = mex_tile_set_property;
  object_class->dispose = mex_tile_dispose;
  object_class->finalize = mex_tile_finalize;

  widget_class->apply_style = mex_tile_apply_style;

  actor_class->get_preferred_height = mex_tile_get_preferred_height;
  actor_class->allocate = mex_tile_allocate;
  actor_class->paint = mex_tile_paint;
  actor_class->map = mex_tile_map;
  actor_class->unmap = mex_tile_unmap;

  pspec = g_param_spec_string ("label",
                               "Label",
                               "Text to use for the label of the tile.",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_LABEL, pspec);

  pspec = g_param_spec_object ("primary-icon",
                               "Primary icon",
                               "ClutterActor to display in the primary "
                               "icon position of the tile.",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PRIMARY_ICON, pspec);

  pspec = g_param_spec_object ("secondary-icon",
                               "Secondary icon",
                               "ClutterActor to display in the secondary "
                               "icon position of the tile.",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SECONDARY_ICON, pspec);

  pspec = g_param_spec_boolean ("header-visible",
                                "Header Visible",
                                "Whether the tile header is visible or not",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_HEADER_VISIBLE, pspec);

  pspec = g_param_spec_boolean ("important",
                                "Important",
                                "Whether the tile is important",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_IMPORTANT, pspec);

  signals[PRIMARY_ICON_CLICKED] = g_signal_new ("primary-icon-clicked",
                                                G_TYPE_FROM_CLASS (klass),
                                                G_SIGNAL_RUN_LAST,
                                                0, NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE, 0);

  signals[SECONDARY_ICON_CLICKED] = g_signal_new ("secondary-icon-clicked",
                                                  G_TYPE_FROM_CLASS (klass),
                                                  G_SIGNAL_RUN_LAST,
                                                  0, NULL, NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE, 0);
}

static void
mex_tile_style_changed_cb (MexTile *self, MxStyleChangedFlags flags)
{
  MexTilePrivate *priv = self->priv;
  mx_stylable_style_changed (MX_STYLABLE (priv->table), flags);
  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

static void
mex_tile_important_new_frame_cb (ClutterTimeline *timeline,
                                 gint             frame_num,
                                 MexTile         *tile)
{
  MexTilePrivate *priv = tile->priv;
  ClutterActor *child = mx_bin_get_child (MX_BIN (tile));

  if (child)
    {
      gdouble opacity = clutter_alpha_get_alpha (priv->important_alpha);

      if (opacity < 0.5)
        opacity = 1.0 - (opacity * 2.0);
      else
        opacity = (opacity - 0.5) * 2.0;

      clutter_actor_set_opacity (child, opacity * 255);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (tile));
}

static void
mex_tile_timeline_completed_cb (ClutterTimeline *timeline,
                                MexTile          *tile)
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

  mex_tile_important_new_frame_cb (timeline, 0, tile);
}

static void
mex_tile_init (MexTile *self)
{
  MexTilePrivate *priv = self->priv = TILE_PRIVATE (self);

  priv->table = mx_table_new ();

  clutter_actor_push_internal (CLUTTER_ACTOR (self));
  clutter_actor_set_parent (priv->table, CLUTTER_ACTOR (self));
  clutter_actor_pop_internal (CLUTTER_ACTOR (self));

  mx_stylable_set_style_class (MX_STYLABLE (priv->table), "MexTile");

  priv->label = mx_label_new ();

  mx_label_set_fade_out (MX_LABEL (priv->label), TRUE);

  priv->header_visible = TRUE;

  priv->timeline = clutter_timeline_new (DURATION);
  priv->important_alpha =
    clutter_alpha_new_full (priv->timeline, CLUTTER_EASE_OUT_QUAD);
  g_signal_connect_object (priv->timeline, "new-frame",
                           G_CALLBACK (mex_tile_important_new_frame_cb), self,
                           0);
  g_signal_connect_object (priv->timeline, "completed",
                           G_CALLBACK (mex_tile_timeline_completed_cb), self,
                           0);

  mx_table_add_actor_with_properties (MX_TABLE (priv->table),
                                      priv->label, 0, 1,
                                      "y-fill", FALSE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mex_tile_style_changed_cb), NULL);
}

ClutterActor *
mex_tile_new (void)
{
  return g_object_new (MEX_TYPE_TILE, NULL);
}

ClutterActor *
mex_tile_new_with_label (const gchar *label)
{
  return g_object_new (MEX_TYPE_TILE, "label", label, NULL);
}

void
mex_tile_set_label (MexTile *tile, const gchar *label)
{
  MexTilePrivate *priv;

  g_return_if_fail (MEX_IS_TILE (tile));

  priv = tile->priv;
  mx_label_set_text (MX_LABEL (priv->label), label);

  g_object_notify (G_OBJECT (tile), "label");
}

const gchar *
mex_tile_get_label (MexTile *tile)
{
  g_return_val_if_fail (MEX_IS_TILE (tile), NULL);
  return mx_label_get_text (MX_LABEL (tile->priv->label));
}

static gboolean
mex_tile_icon_button_release_cb (ClutterActor       *icon,
                                 ClutterButtonEvent *event,
                                 MexTile            *self)
{
  MexTilePrivate *priv = self->priv;

  if (icon == priv->icon1)
    g_signal_emit (self, signals[PRIMARY_ICON_CLICKED], 0);
  else
    g_signal_emit (self, signals[SECONDARY_ICON_CLICKED], 0);

  return TRUE;
}

static gboolean
mex_tile_icon_leave_event_cb (ClutterActor         *icon,
                              ClutterCrossingEvent *event,
                              MexTile              *self)
{
  g_signal_handlers_disconnect_by_func (icon,
                                        mex_tile_icon_leave_event_cb,
                                        self);
  g_signal_handlers_disconnect_by_func (icon,
                                        mex_tile_icon_button_release_cb,
                                        self);

  return FALSE;
}

static gboolean
mex_tile_icon_button_press_cb (ClutterActor       *icon,
                               ClutterButtonEvent *event,
                               MexTile            *self)
{
  g_signal_connect (icon, "leave-event",
                    G_CALLBACK (mex_tile_icon_leave_event_cb), self);
  g_signal_connect (icon, "button-release-event",
                    G_CALLBACK (mex_tile_icon_leave_event_cb), self);

  return TRUE;
}

void
mex_tile_set_primary_icon (MexTile      *tile,
                           ClutterActor *icon)
{
  MexTilePrivate *priv;

  g_return_if_fail (MEX_IS_TILE (tile));
  g_return_if_fail (!icon || CLUTTER_IS_ACTOR (icon));

  priv = tile->priv;
  if (priv->icon1 != icon)
    {
      if (priv->icon1)
        {
          g_signal_handlers_disconnect_by_func (priv->icon1,
                                                mex_tile_icon_button_press_cb,
                                                tile);
          g_signal_handlers_disconnect_by_func (priv->icon1,
                                                mex_tile_icon_leave_event_cb,
                                                tile);
          g_signal_handlers_disconnect_by_func (priv->icon1,
                                                mex_tile_icon_button_release_cb,
                                                tile);
          clutter_container_remove_actor (CLUTTER_CONTAINER (priv->table),
                                          priv->icon1);
        }

      if (icon)
        {
          mx_table_add_actor_with_properties (MX_TABLE (priv->table),
                                              icon, 0, 0,
                                              "x-expand", FALSE,
                                              "y-expand", FALSE,
                                              "x-fill", FALSE,
                                              "y-fill", FALSE,
                                              "x-align", MX_ALIGN_START,
                                              NULL);

          clutter_actor_set_reactive (icon, TRUE);
          g_signal_connect (icon, "button-press-event",
                            G_CALLBACK (mex_tile_icon_button_press_cb), tile);
        }

      priv->icon1 = icon;

      g_object_notify (G_OBJECT (tile), "primary-icon");
    }
}

ClutterActor *
mex_tile_get_primary_icon (MexTile *tile)
{
  g_return_val_if_fail (MEX_IS_TILE (tile), NULL);
  return tile->priv->icon1;
}

void
mex_tile_set_secondary_icon (MexTile      *tile,
                             ClutterActor *icon)
{
  MexTilePrivate *priv;

  g_return_if_fail (MEX_IS_TILE (tile));
  g_return_if_fail (!icon || CLUTTER_IS_ACTOR (icon));

  priv = tile->priv;
  if (priv->icon2 != icon)
    {
      if (priv->icon2)
        {
          g_signal_handlers_disconnect_by_func (priv->icon2,
                                                mex_tile_icon_button_press_cb,
                                                tile);
          g_signal_handlers_disconnect_by_func (priv->icon2,
                                                mex_tile_icon_leave_event_cb,
                                                tile);
          g_signal_handlers_disconnect_by_func (priv->icon2,
                                                mex_tile_icon_button_release_cb,
                                                tile);
          clutter_container_remove_actor (CLUTTER_CONTAINER (priv->table),
                                          priv->icon2);
        }

      if (icon)
        {
          mx_table_add_actor_with_properties (MX_TABLE (priv->table),
                                              icon, 0, 2,
                                              "x-expand", FALSE,
                                              "y-expand", FALSE,
                                              "x-fill", FALSE,
                                              "y-fill", FALSE,
                                              "x-align", MX_ALIGN_END,
                                              NULL);
          g_signal_connect (icon, "button-press-event",
                            G_CALLBACK (mex_tile_icon_button_press_cb), tile);
        }

      priv->icon2 = icon;

      g_object_notify (G_OBJECT (tile), "secondary-icon");
    }
}

ClutterActor *
mex_tile_get_secondary_icon (MexTile *tile)
{
  g_return_val_if_fail (MEX_IS_TILE (tile), NULL);
  return tile->priv->icon2;
}

gboolean
mex_tile_get_header_visible (MexTile *tile)
{
  g_return_val_if_fail (MEX_IS_TILE (tile), FALSE);

  return tile->priv->header_visible;
}

void
mex_tile_set_header_visible (MexTile  *tile,
                             gboolean  header_visible)
{
  g_return_if_fail (MEX_IS_TILE (tile));

  if (header_visible != tile->priv->header_visible)
    {
      if (header_visible)
        clutter_actor_show (tile->priv->table);
      else
        clutter_actor_hide (tile->priv->table);

      tile->priv->header_visible = header_visible;

      g_object_notify (G_OBJECT (tile), "header-visible");
    }

}

gboolean
mex_tile_get_important (MexTile *tile)
{
  g_return_val_if_fail (MEX_IS_TILE (tile), FALSE);

  return tile->priv->important;
}

void
mex_tile_set_important (MexTile  *tile,
                        gboolean  important)
{
  MexTilePrivate *priv;

  g_return_if_fail (MEX_IS_TILE (tile));

  priv = tile->priv;
  if (priv->important != important)
    {
      priv->important = important;

      g_object_notify (G_OBJECT (tile), "important");

      mx_stylable_set_style_class (MX_STYLABLE (tile),
                                   important ? "Important" : NULL);

      if (clutter_timeline_is_playing (priv->timeline))
        clutter_timeline_set_direction (priv->timeline, important ?
                                        CLUTTER_TIMELINE_FORWARD :
                                        CLUTTER_TIMELINE_BACKWARD);
      else
        {
          if (CLUTTER_ACTOR_IS_MAPPED (tile))
            {
              clutter_timeline_rewind (priv->timeline);
              clutter_timeline_start (priv->timeline);
            }
          else
            {
              clutter_timeline_advance (priv->timeline, DURATION);
              mex_tile_important_new_frame_cb (priv->timeline, 0, tile);
              mex_tile_timeline_completed_cb (priv->timeline, tile);
            }
        }
    }
}
