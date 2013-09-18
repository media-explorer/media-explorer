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
#include "mex-utils.h"

static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexTile, mex_tile, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define TILE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TILE, MexTilePrivate))

#define DURATION 500

static CoglMaterial *template_material;

enum
{
  PROP_0,

  PROP_PRIMARY_ICON,
  PROP_SECONDARY_ICON,
  PROP_LABEL,
  PROP_SECONDARY_LABEL,
  PROP_HEADER_VISIBLE,
  PROP_IMPORTANT
};

struct _MexTilePrivate
{
  guint            has_focus : 1;
  guint            header_visible : 1;
  guint            important : 1;

  ClutterActor    *child;

  ClutterActor    *icon1;
  ClutterActor    *icon2;
  ClutterActor    *label;
  ClutterActor    *secondary_label;

  ClutterActor    *box_layout;

  ClutterTimeline *timeline;
  ClutterAlpha    *important_alpha;

  CoglMaterial *material;
  MxPadding    *header_padding;
  gfloat        header_height;

  ClutterColor *header_background_color;
};

/* MxStylableIface */

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
      GParamSpec *pspec;

      is_initialized = TRUE;

      pspec = g_param_spec_boxed ("x-mex-header-background",
                                  "Header Background",
                                  "Background image for the title header",
                                  MX_TYPE_BORDER_IMAGE,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_TILE, pspec);

      pspec = g_param_spec_boxed ("x-mex-header-padding",
                                  "Header padding",
                                  "Padding inside the header",
                                  MX_TYPE_PADDING,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_TILE, pspec);

      pspec = g_param_spec_boxed ("x-mex-header-background-color",
                                  "Header background color",
                                  "Background color for the title header",
                                  CLUTTER_TYPE_COLOR,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_TILE, pspec);
    }
}

static void
mex_tile_actor_added (ClutterActor *container,
                      ClutterActor *actor)
{
  MexTilePrivate *priv = MEX_TILE (container)->priv;

  if (MX_IS_TOOLTIP (actor))
    return;

  if (priv->child)
    clutter_actor_remove_child (container, priv->child);

  priv->child = actor;
}

static void
mex_tile_actor_removed (ClutterActor *container,
                        ClutterActor *actor)
{
  MexTilePrivate *priv = MEX_TILE (container)->priv;

  if (priv->child == actor)
    priv->child = NULL;
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

    case PROP_SECONDARY_LABEL:
      g_value_set_string (value, mex_tile_get_secondary_label (self));
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

    case PROP_SECONDARY_LABEL:
      mex_tile_set_secondary_label (self, g_value_get_string (value));
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

  /* Use icon setting functions to remove icons */
  mex_tile_set_primary_icon (self, NULL);
  mex_tile_set_secondary_icon (self, NULL);

  if (priv->box_layout)
    {
      clutter_actor_destroy (priv->box_layout);
      priv->box_layout = NULL;

      /* box_layout contains label and secondary_label */
      priv->label = NULL;
      priv->secondary_label = NULL;
    }

  if (priv->header_padding)
    {
      g_boxed_free (MX_TYPE_PADDING, priv->header_padding);
      priv->header_padding = NULL;
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

  if (priv->material)
    {
      cogl_object_unref (priv->material);
      priv->material = NULL;
    }

  G_OBJECT_CLASS (mex_tile_parent_class)->dispose (object);
}

static void
mex_tile_finalize (GObject *object)
{
  MexTile *self = MEX_TILE (object);

  if (self->priv->header_background_color)
    {
      g_boxed_free (CLUTTER_TYPE_COLOR, self->priv->header_background_color);
      self->priv->header_background_color = NULL;
    }

  G_OBJECT_CLASS (mex_tile_parent_class)->finalize (object);
}

static void
mex_tile_get_preferred_height (ClutterActor *actor,
                               gfloat        for_width,
                               gfloat       *min_height_p,
                               gfloat       *nat_height_p)
{
  MxPadding padding;
  gfloat box_height, label_h, icon1_h, icon2_h;

  MexTilePrivate *priv = MEX_TILE (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->
    get_preferred_height (actor, for_width, NULL, nat_height_p);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  for_width -= padding.left + padding.right;

  /* Header */
  clutter_actor_get_preferred_height (priv->box_layout, for_width, NULL, &label_h);

  if (priv->icon1)
    clutter_actor_get_preferred_height (priv->icon1, for_width, NULL, &icon1_h);
  else
    icon1_h = 0;

  if (priv->icon2)
    clutter_actor_get_preferred_height (priv->icon2, for_width, NULL, &icon2_h);
  else
    icon2_h = 0;

  box_height = MAX (label_h, MAX (icon1_h, icon2_h)) +
    ((priv->header_padding) ? priv->header_padding->top + priv->header_padding->bottom : 0);


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
mex_tile_get_preferred_width (ClutterActor *actor,
                              gfloat        for_height,
                              gfloat       *min_height_p,
                              gfloat       *nat_height_p)
{
  MexTilePrivate *priv = MEX_TILE (actor)->priv;
  MxPadding padding;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  for_height -= padding.top + padding.bottom;
  clutter_actor_get_preferred_width (priv->child, for_height, min_height_p,
                                     nat_height_p);
  if (min_height_p)
    *min_height_p += padding.top + padding.bottom;

  if (nat_height_p)
    *nat_height_p += padding.top + padding.bottom;
}

static void
mex_tile_allocate (ClutterActor           *actor,
                   const ClutterActorBox  *box,
                   ClutterAllocationFlags  flags)
{
  MxPadding padding;
  ClutterActorBox child_box;
  gfloat available_width, available_height;
  ClutterEffect *fade;

  MexTilePrivate *priv = MEX_TILE (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  available_width = box->x2 - box->x1 - padding.left - padding.right;
  available_height = box->y2 - box->y1 - padding.top - padding.bottom;

  if (priv->child)
    {
      gfloat child_width, full_width, full_height;

      clutter_actor_get_preferred_size (priv->child, NULL, NULL,
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

      mx_allocate_align_fill (priv->child, &child_box,
                              MX_ALIGN_MIDDLE, MX_ALIGN_MIDDLE, FALSE, FALSE);
      clutter_actor_allocate (priv->child, &child_box, flags);
    }

  /* Allocate Header */
  if (priv->header_visible)
    {
      gfloat icon1_w, icon1_h, icon2_w, icon2_h, label_h, label_w, header_h;
      gfloat middle_w;

      if (priv->header_padding)
        {
          padding.top += priv->header_padding->top;
          padding.right += priv->header_padding->right;
          padding.bottom += priv->header_padding->bottom;
          padding.left += priv->header_padding->left;
        }

      clutter_actor_get_preferred_size (priv->box_layout, NULL, NULL, &label_w,
                                        &label_h);

      if (priv->icon1)
        clutter_actor_get_preferred_size (priv->icon1, NULL, NULL, &icon1_w,
                                          &icon1_h);
      else
        icon1_h = icon1_w = 0;

      if (priv->icon2)
        clutter_actor_get_preferred_size (priv->icon2, NULL, NULL, &icon2_w,
                                          &icon2_h);
      else
        icon2_h = icon2_w = 0;

      header_h = MAX (icon1_h, MAX (icon2_h, label_h));

      /* primary icon */
      if (priv->icon1)
        {
          child_box.y1 = padding.top + (header_h / 2.0) - (icon1_h / 2.0);
          child_box.x1 = padding.left;
          child_box.y2 = child_box.y1 + icon1_h;
          child_box.x2 = child_box.x1 + icon1_w;

          clutter_actor_allocate (priv->icon1, &child_box, flags);
          child_box.x1 += icon1_w + 8;
        }
      else
        child_box.x1 = padding.left;

      /* label */
      child_box.x2 = child_box.x1 + label_w;
      child_box.y1 = (int) (padding.top + (header_h / 2.0) - (label_h / 2.0));
      child_box.y2 = child_box.y1 + label_h;

      fade = clutter_actor_get_effect (priv->box_layout, "fade");

      middle_w = available_width - icon1_w - icon2_w;
      if (priv->header_padding)
        middle_w -= priv->header_padding->left + priv->header_padding->right;
      clutter_actor_meta_set_enabled (CLUTTER_ACTOR_META (fade),
                                      !(middle_w > label_w));
      mx_fade_effect_set_bounds (MX_FADE_EFFECT (fade), 0, 0, middle_w, 0);

      clutter_actor_allocate (priv->box_layout, &child_box, flags);

      /* secondary icon */
      if (priv->icon2)
        {
          child_box.x2 = (box->x2 - box->x1) - padding.right;
          child_box.x1 = child_box.x2 - icon2_w;
          child_box.y1 = padding.top + (header_h / 2.0) - (icon2_h / 2.0);
          child_box.y2 = child_box.y1 + icon2_h;

          clutter_actor_allocate (priv->icon2, &child_box, flags);
        }

      priv->header_height = header_h;
      if (priv->header_padding)
        priv->header_height += priv->header_padding->top
          + priv->header_padding->bottom;
    }
}

static void
mex_tile_paint (ClutterActor *actor)
{
  MexTilePrivate *priv = MEX_TILE (actor)->priv;
  MxPadding padding;
  ClutterActorBox box;

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->paint (actor);

  clutter_actor_paint (priv->child);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (priv->header_visible)
    {
      clutter_actor_get_allocation_box (actor, &box);


      if (priv->header_background_color)
        {
          cogl_set_source_color4ub (priv->header_background_color->red,
                                    priv->header_background_color->green,
                                    priv->header_background_color->blue,
                                    priv->header_background_color->alpha);

          cogl_rectangle (padding.left, padding.top,
                          box.x2 - box.x1 - padding.right,
                          priv->header_height);
        }


      if (cogl_material_get_n_layers (priv->material) > 0)
        {
          guint8 opacity;

          opacity = clutter_actor_get_paint_opacity (actor);

          cogl_material_set_color4ub (priv->material, opacity, opacity, opacity,
                                      opacity);
          cogl_set_source (priv->material);

          cogl_rectangle (padding.left, padding.top,
                          box.x2 - box.x1 - padding.right,
                          priv->header_height);
        }

      clutter_actor_paint (priv->box_layout);

      if (priv->icon1)
        clutter_actor_paint (priv->icon1);

      if (priv->icon2)
        clutter_actor_paint (priv->icon2);
    }
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

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->map (actor);

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

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    g_signal_handlers_disconnect_by_func (manager,
                                          mex_tile_notify_focused_cb,
                                          actor);

  CLUTTER_ACTOR_CLASS (mex_tile_parent_class)->unmap (actor);
}

static void
mex_tile_class_init (MexTileClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexTilePrivate));

  object_class->get_property = mex_tile_get_property;
  object_class->set_property = mex_tile_set_property;
  object_class->dispose = mex_tile_dispose;
  object_class->finalize = mex_tile_finalize;

  actor_class->get_preferred_height = mex_tile_get_preferred_height;
  actor_class->get_preferred_width = mex_tile_get_preferred_width;
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

  pspec = g_param_spec_string ("secondary-label",
                               "Secondary Label",
                               "Text to use for the secondary label",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SECONDARY_LABEL, pspec);

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
}

static void
mex_tile_style_changed_cb (MexTile *self, MxStyleChangedFlags flags)
{
  MexTilePrivate *priv = self->priv;
  MxBorderImage *image;

  if (priv->header_padding)
    {
      g_boxed_free (MX_TYPE_PADDING, priv->header_padding);
      priv->header_padding = NULL;
    }

  if (priv->header_background_color)
    {
      g_boxed_free (CLUTTER_TYPE_COLOR, priv->header_background_color);
      priv->header_background_color = NULL;
    }

  mx_stylable_get (MX_STYLABLE (self),
                   "x-mex-header-background", &image,
                   "x-mex-header-background-color", &priv->header_background_color,
                   "x-mex-header-padding", &priv->header_padding,
                   NULL);

  mx_stylable_apply_clutter_text_attributes (MX_STYLABLE (self),
                                             CLUTTER_TEXT (priv->label));

  mx_stylable_apply_clutter_text_attributes (MX_STYLABLE (self),
                                             CLUTTER_TEXT (priv->secondary_label));

  if (image && image->uri)
    {
      CoglObject *background;

      background =
        mx_texture_cache_get_cogl_texture (mx_texture_cache_get_default (),
                                           image->uri);
      cogl_material_set_layer (priv->material, 0, background);
    }
  else
    {
      if (cogl_material_get_n_layers (priv->material))
        cogl_material_remove_layer (priv->material, 0);
    }

  if (image)
    g_boxed_free (MX_TYPE_BORDER_IMAGE, image);

  if (priv->icon1)
    mx_stylable_style_changed (MX_STYLABLE (priv->icon1), flags);

  if (priv->icon2)
    mx_stylable_style_changed (MX_STYLABLE (priv->icon2), flags);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

static void
mex_tile_important_new_frame_cb (ClutterTimeline *timeline,
                                 gint             frame_num,
                                 MexTile         *tile)
{
  MexTilePrivate *priv = tile->priv;

  if (priv->child)
    {
      gdouble opacity = clutter_alpha_get_alpha (priv->important_alpha);

      if (opacity < 0.5)
        opacity = 1.0 - (opacity * 2.0);
      else
        opacity = (opacity - 0.5) * 2.0;

      clutter_actor_set_opacity (priv->child, opacity * 255);
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
  const ClutterColor opaque = { 0x00, 0x00, 0x00, 0x00 };
  ClutterEffect *fade;

  /* create a template material for the header background from which cheap
   * copies can be made for each instance */
  if (G_UNLIKELY (!template_material))
    template_material = cogl_material_new ();

  priv->material = cogl_material_copy (template_material);


  /* layout for primary and secondary labels */
  priv->box_layout = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->box_layout), 12);

  /* add fade effect to the box layout */
  fade = (ClutterEffect*) mx_fade_effect_new ();
  mx_fade_effect_set_border (MX_FADE_EFFECT (fade), 0, 50, 0, 0);
  mx_fade_effect_set_color (MX_FADE_EFFECT (fade), &opaque);
  clutter_actor_add_effect_with_name (priv->box_layout, "fade", fade);
  clutter_actor_meta_set_enabled (CLUTTER_ACTOR_META (fade), TRUE);

  clutter_actor_push_internal (CLUTTER_ACTOR (self));
  clutter_actor_set_parent (priv->box_layout, CLUTTER_ACTOR (self));
  clutter_actor_pop_internal (CLUTTER_ACTOR (self));


  priv->label = clutter_text_new ();
  priv->secondary_label = clutter_text_new ();
  clutter_actor_set_opacity (priv->secondary_label, 128);

  clutter_container_add (CLUTTER_CONTAINER (priv->box_layout), priv->label,
                         priv->secondary_label, NULL);

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

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mex_tile_style_changed_cb), NULL);

  g_signal_connect (self, "actor-added",
                    G_CALLBACK (mex_tile_actor_added), NULL);
  g_signal_connect (self, "actor-removed",
                    G_CALLBACK (mex_tile_actor_removed), NULL);
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
  clutter_text_set_text (CLUTTER_TEXT (priv->label), (label) ? label : "");

  g_object_notify (G_OBJECT (tile), "label");
}

const gchar *
mex_tile_get_label (MexTile *tile)
{
  g_return_val_if_fail (MEX_IS_TILE (tile), NULL);
  return clutter_text_get_text (CLUTTER_TEXT (tile->priv->label));
}

void
mex_tile_set_secondary_label (MexTile *tile, const gchar *label)
{
  g_return_if_fail (MEX_IS_TILE (tile));

  clutter_text_set_text (CLUTTER_TEXT (tile->priv->secondary_label),
                         (label) ? label : "");

  g_object_notify (G_OBJECT (tile), "secondary-label");
}

const gchar *
mex_tile_get_secondary_label (MexTile *tile)
{
  g_return_val_if_fail (MEX_IS_TILE (tile), NULL);
  return clutter_text_get_text (CLUTTER_TEXT (tile->priv->secondary_label));
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
        clutter_actor_destroy (priv->icon1);

      if (icon)
        {
          clutter_actor_push_internal (CLUTTER_ACTOR (tile));
          clutter_actor_set_parent (icon, CLUTTER_ACTOR (tile));
          clutter_actor_pop_internal (CLUTTER_ACTOR (tile));
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
        clutter_actor_destroy (priv->icon2);

      if (icon)
        {
          clutter_actor_push_internal (CLUTTER_ACTOR (tile));
          clutter_actor_set_parent (icon, CLUTTER_ACTOR (tile));
          clutter_actor_pop_internal (CLUTTER_ACTOR (tile));
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
