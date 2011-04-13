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


#include "mex-scroll-indicator.h"
#include "mex-utils.h"

static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexScrollIndicator, mex_scroll_indicator,
                         MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define SCROLL_INDICATOR_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SCROLL_INDICATOR, MexScrollIndicatorPrivate))

enum
{
  PROP_0,

  PROP_ADJUSTMENT
};

struct _MexScrollIndicatorPrivate
{
  CoglHandle     handle;
  MxBorderImage *handle_border_image;
  CoglHandle     step;
  MxBorderImage *step_border_image;
  MxAdjustment  *adjustment;
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

      pspec = g_param_spec_boxed ("x-mex-scroll-handle-image",
                                  "Scroll-handle image",
                                  "Scroll-handle image filename",
                                  MX_TYPE_BORDER_IMAGE,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface,
                                          MEX_TYPE_SCROLL_INDICATOR,
                                          pspec);

      pspec = g_param_spec_boxed ("x-mex-scroll-step-image",
                                  "Scroll-step image",
                                  "Scroll-step image filename",
                                  MX_TYPE_BORDER_IMAGE,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface,
                                          MEX_TYPE_SCROLL_INDICATOR,
                                          pspec);
    }
}

/* Actor implementation */

static void
mex_scroll_indicator_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  MexScrollIndicator *self = MEX_SCROLL_INDICATOR (object);

  switch (property_id)
    {
    case PROP_ADJUSTMENT:
      g_value_set_object (value, mex_scroll_indicator_get_adjustment (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_scroll_indicator_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  MexScrollIndicator *self = MEX_SCROLL_INDICATOR (object);

  switch (property_id)
    {
    case PROP_ADJUSTMENT:
      mex_scroll_indicator_set_adjustment (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_scroll_indicator_dispose (GObject *object)
{
  MexScrollIndicatorPrivate *priv = MEX_SCROLL_INDICATOR (object)->priv;

  if (priv->handle)
    {
      cogl_handle_unref (priv->handle);
      priv->handle = NULL;
    }

  if (priv->step)
    {
      cogl_handle_unref (priv->step);
      priv->step = NULL;
    }

  G_OBJECT_CLASS (mex_scroll_indicator_parent_class)->dispose (object);
}

static void
mex_scroll_indicator_finalize (GObject *object)
{
  MexScrollIndicatorPrivate *priv = MEX_SCROLL_INDICATOR (object)->priv;

  if (priv->step_border_image)
    g_boxed_free (MX_TYPE_BORDER_IMAGE, priv->step_border_image);

  if (priv->handle_border_image)
    g_boxed_free (MX_TYPE_BORDER_IMAGE, priv->handle_border_image);

  G_OBJECT_CLASS (mex_scroll_indicator_parent_class)->finalize (object);
}

static void
mex_scroll_indicator_get_preferred_width (ClutterActor *actor,
                                          gfloat        for_height,
                                          gfloat       *min_width_p,
                                          gfloat       *nat_width_p)
{
  gfloat width;
  MxPadding padding;

  MexScrollIndicatorPrivate *priv = MEX_SCROLL_INDICATOR (actor)->priv;

  if (priv->handle)
    {
      CoglHandle texture = cogl_material_layer_get_texture (
        g_list_nth_data ((GList *)cogl_material_get_layers (priv->handle), 0));
      width = cogl_texture_get_width (texture);
    }
  else
    width = 0;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_width_p)
    *min_width_p = width;
  if (nat_width_p)
    *nat_width_p = width;
}

static void
mex_scroll_indicator_get_preferred_height (ClutterActor *actor,
                                           gfloat        for_width,
                                           gfloat       *min_height_p,
                                           gfloat       *nat_height_p)
{
  gfloat height;
  MxPadding padding;

  MexScrollIndicatorPrivate *priv = MEX_SCROLL_INDICATOR (actor)->priv;

  if (priv->handle)
    {
      CoglHandle texture = cogl_material_layer_get_texture (
        g_list_nth_data ((GList *)cogl_material_get_layers (priv->handle), 0));
      height = cogl_texture_get_height (texture);
    }
  else
    height = 0;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_height_p)
    *min_height_p = height;
  if (nat_height_p)
    *nat_height_p = height;
}

static inline gfloat
mex_scroll_indicator_scale_value (gfloat value,
                                  gfloat min,
                                  gfloat max,
                                  gfloat new_min,
                                  gfloat new_max)
{
  return (((value - min) / (max - min)) * (new_max - new_min)) + new_min;
}

static void
mex_scroll_indicator_paint (ClutterActor *actor)
{
  guint8 alpha;
  MxPadding padding;
  ClutterActorBox box;
  gdouble position, lower, upper, page_size, value;
  gfloat handle_width, handle_height, handle_tex_height, x = 0.0, y = 0.0;

  MexScrollIndicatorPrivate *priv = MEX_SCROLL_INDICATOR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_scroll_indicator_parent_class)->paint (actor);

  if (!priv->handle && !priv->step)
    return;

  clutter_actor_get_allocation_box (actor, &box);
  alpha = clutter_actor_get_paint_opacity (actor);

  if (priv->adjustment)
    {
      mx_adjustment_get_values (priv->adjustment,
                                &value,
                                &lower,
                                &upper,
                                NULL,
                                NULL,
                                &page_size);

      position = CLAMP ((value - lower) / (upper - page_size - lower),
                        0.0, 1.0);
    }
  else
    position = value = lower = upper = page_size = 0;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (priv->handle)
    {
      CoglHandle texture = cogl_material_layer_get_texture (
        g_list_nth_data ((GList *)cogl_material_get_layers (priv->handle), 0));

      handle_width = cogl_texture_get_width (texture);
      if (page_size > 0)
        handle_height = (page_size / (upper - lower)) * (box.y2 - box.y1);
      else
        handle_height = 0;

      handle_tex_height = cogl_texture_get_height (texture);
      handle_height = MAX ((gint)handle_height, handle_tex_height);

      x = box.x2 - box.x1 - handle_width;
      y = (box.y2 - box.y1 - handle_height) * position;
    }
  else
    {
      handle_width = box.x2 - box.x1 - padding.left - padding.right;
      handle_height = 0;
    }

  if (priv->step)
    {
      gint i;

      CoglHandle texture = cogl_material_layer_get_texture (
        g_list_nth_data ((GList *)cogl_material_get_layers (priv->step), 0));
      gfloat tex_width = cogl_texture_get_width (texture);
      gfloat tex_height = cogl_texture_get_height (texture);
      gint steps = ((box.y2 - box.y1 - padding.top - padding.bottom) /
                    tex_height) / 2;
      gfloat right = box.x2 - box.x1 - padding.right;

      cogl_set_source (priv->step);

      for (i = 0; i < steps; i++)
        {
          guint8 opacity;
          gfloat final_width, distance;
          gfloat offset_y = (i * 2 * tex_height) + padding.top;

          if (offset_y < y)
            distance = y - offset_y;
          else if (offset_y > y + handle_height)
            distance = offset_y - (y + handle_height);
          else
            distance = 0;

          opacity = (guint8)((1.f - distance / (box.y2 - box.y1)) *
                             (gfloat)(alpha));

          /* These numbers are a bit arbitrary, might be nice to come up
           * with a nice formula and algorithm to generate them...
           */
          distance /= tex_height * 2;
          if (distance < 2)
            final_width = handle_width / 1.75f;
          else if (distance < 3)
            final_width = handle_width /
              mex_scroll_indicator_scale_value (distance, 2, 3, 1.75f, 3);
          else if (distance < 7)
            final_width = handle_width / 3.f;
          else if (distance < 10)
            final_width = handle_width /
              mex_scroll_indicator_scale_value (distance, 7, 10, 3, 5);
          else
            final_width = handle_width / 5.f;

          cogl_material_set_color4ub (priv->step,
                                      opacity,
                                      opacity,
                                      opacity,
                                      opacity);
          mex_paint_texture_frame (right - final_width,
                                   offset_y,
                                   final_width,
                                   tex_height,
                                   tex_width,
                                   tex_height,
                                   priv->step_border_image->top,
                                   priv->step_border_image->right,
                                   priv->step_border_image->bottom,
                                   priv->step_border_image->left,
                                   MEX_PAINT_TEXTURE_FRAME_ALL);
        }
    }

  if (priv->handle)
    {
      cogl_material_set_color4ub (priv->handle, alpha, alpha, alpha, alpha);
      cogl_set_source (priv->handle);
      mex_paint_texture_frame (x, y, handle_width, handle_height,
                               handle_width, handle_tex_height,
                               priv->handle_border_image->top,
                               priv->handle_border_image->right,
                               priv->handle_border_image->bottom,
                               priv->handle_border_image->left,
                               MEX_PAINT_TEXTURE_FRAME_ALL);
    }
}

static void
mex_scroll_indicator_class_init (MexScrollIndicatorClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexScrollIndicatorPrivate));

  object_class->get_property = mex_scroll_indicator_get_property;
  object_class->set_property = mex_scroll_indicator_set_property;
  object_class->dispose = mex_scroll_indicator_dispose;
  object_class->finalize = mex_scroll_indicator_finalize;

  actor_class->get_preferred_width = mex_scroll_indicator_get_preferred_width;
  actor_class->get_preferred_height = mex_scroll_indicator_get_preferred_height;
  actor_class->paint = mex_scroll_indicator_paint;

  pspec = g_param_spec_object ("adjustment",
                               "Adjustment",
                               "The MxAdjustment this indicator visualises.",
                               MX_TYPE_ADJUSTMENT,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ADJUSTMENT, pspec);
}

static void
mex_scroll_indicator_style_changed_cb (MxStylable          *stylable,
                                       MxStyleChangedFlags  flags,
                                       MexScrollIndicator  *self)
{
  CoglHandle new_handle, new_step;
  MxBorderImage *handle, *step;

  MexScrollIndicatorPrivate *priv = self->priv;
  MxTextureCache *cache = mx_texture_cache_get_default ();

  handle = step = NULL;
  mx_stylable_get (stylable,
                   "x-mex-scroll-handle-image", &handle,
                   "x-mex-scroll-step-image", &step,
                   NULL);

  new_handle = new_step = NULL;

  if (handle && handle->uri)
    new_handle = mx_texture_cache_get_cogl_texture (cache, handle->uri);
  else if (handle)
    g_boxed_free (MX_TYPE_BORDER_IMAGE, handle);

  if (step && step->uri)
    new_step = mx_texture_cache_get_cogl_texture (cache, step->uri);
  else if (step)
    g_boxed_free (MX_TYPE_BORDER_IMAGE, step);

  if (priv->handle)
    {
      cogl_handle_unref (priv->handle);
      priv->handle = NULL;
    }

  if (priv->step)
    {
      cogl_handle_unref (priv->step);
      priv->step = NULL;
    }

  if (priv->step_border_image)
    g_boxed_free (MX_TYPE_BORDER_IMAGE, priv->step_border_image);

  if (priv->handle_border_image)
    g_boxed_free (MX_TYPE_BORDER_IMAGE, priv->handle_border_image);

  if (new_handle)
    {
      priv->handle_border_image = handle;
      priv->handle = cogl_material_new ();
      cogl_material_set_layer (priv->handle, 0, new_handle);
      cogl_material_set_color4ub (priv->handle, 0xff, 0xff, 0xff, 0xff);
      cogl_handle_unref (new_handle);
    }

  if (new_step)
    {
      priv->step_border_image = step;
      priv->step = cogl_material_new ();
      cogl_material_set_layer (priv->step, 0, new_step);
      cogl_handle_unref (new_step);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

static void
mex_scroll_indicator_init (MexScrollIndicator *self)
{
  self->priv = SCROLL_INDICATOR_PRIVATE (self);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mex_scroll_indicator_style_changed_cb), self);
}

ClutterActor *
mex_scroll_indicator_new (void)
{
  return g_object_new (MEX_TYPE_SCROLL_INDICATOR, NULL);
}

void
mex_scroll_indicator_set_adjustment (MexScrollIndicator *scroll,
                                     MxAdjustment       *adjustment)
{
  MexScrollIndicatorPrivate *priv;

  g_return_if_fail (MEX_IS_SCROLL_INDICATOR (scroll));
  g_return_if_fail (!adjustment || MX_IS_ADJUSTMENT (adjustment));

  priv = scroll->priv;

  if (priv->adjustment != adjustment)
    {
      if (adjustment)
        g_object_ref (adjustment);

      if (priv->adjustment)
        g_object_unref (priv->adjustment);

      priv->adjustment = adjustment;

      g_object_notify (G_OBJECT (scroll), "adjustment");

      clutter_actor_queue_redraw (CLUTTER_ACTOR (scroll));
    }
}

MxAdjustment *
mex_scroll_indicator_get_adjustment (MexScrollIndicator *scroll)
{
  g_return_val_if_fail (MEX_IS_SCROLL_INDICATOR (scroll), NULL);
  return scroll->priv->adjustment;
}

