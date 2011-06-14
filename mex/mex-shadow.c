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


#include "mex-shadow.h"
#include "mex-utils.h"
#include "mex-enum-types.h"
#include <mx/mx.h>
#include <math.h>

G_DEFINE_TYPE (MexShadow, mex_shadow, CLUTTER_TYPE_EFFECT)

#define SHADOW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SHADOW, MexShadowPrivate))

struct _MexShadowPrivate
{
  guint                     needs_regen  : 1;
  MexPaintTextureFrameFlags paint_flags;

  CoglHandle    material;
  ClutterColor  color;
  gint          radius_x;
  gint          radius_y;
  gint          offset_x;
  gint          offset_y;
};

enum
{
  PROP_0,

  PROP_COLOR,
  PROP_RADIUS_X,
  PROP_RADIUS_Y,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_PAINT_FLAGS
};

static GHashTable *shadow_cache = NULL;

static const ClutterColor mex_shadow_default_color = { 0x00, 0x00, 0x00, 0x80 };
#define DEFAULT_RADIUS 12

static void mex_shadow_regenerate (MexShadow *shadow);

static void
mex_shadow_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  MexShadow *shadow = MEX_SHADOW (object);

  switch (property_id)
    {
    case PROP_COLOR:
      clutter_value_set_color (value, mex_shadow_get_color (shadow));
      break;

    case PROP_RADIUS_X:
      g_value_set_int (value, mex_shadow_get_radius_x (shadow));
      break;

    case PROP_RADIUS_Y:
      g_value_set_int (value, mex_shadow_get_radius_y (shadow));
      break;

    case PROP_OFFSET_X:
      g_value_set_int (value, mex_shadow_get_offset_x (shadow));
      break;

    case PROP_OFFSET_Y:
      g_value_set_int (value, mex_shadow_get_offset_y (shadow));
      break;

    case PROP_PAINT_FLAGS:
      g_value_set_flags (value, mex_shadow_get_paint_flags (shadow));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static gboolean
mex_shadow_paint_cb (ClutterEffect *shadow)
{
  ClutterActor *actor = clutter_actor_meta_get_actor (CLUTTER_ACTOR_META (shadow));
  ClutterActorBox box;
  gfloat radius_x, radius_y, tex_width, tex_height;

  MexShadowPrivate *priv = MEX_SHADOW (shadow)->priv;
  gfloat alpha_mult = clutter_actor_get_paint_opacity (actor) / 255.f;

  mex_shadow_regenerate (MEX_SHADOW (shadow));

  /* Get coordinates for texture frame */
  radius_x = MAX (1.f, priv->radius_x);
  radius_y = MAX (1.f, priv->radius_y);
  tex_width = radius_x * 2;
  tex_height = radius_y * 2;
  clutter_actor_get_allocation_box (actor, &box);

  /* Set source material */
  cogl_material_set_color4ub (priv->material,
                              priv->color.red,
                              priv->color.green,
                              priv->color.blue,
                              (gfloat)priv->color.alpha *
                                alpha_mult);
  cogl_set_source (priv->material);

  /* Paint texture-frame */
  mex_paint_texture_frame (-radius_x + priv->offset_x,
                           -radius_y + priv->offset_y,
                           box.x2 - box.x1 + (priv->radius_x + radius_x),
                           box.y2 - box.y1 + (priv->radius_y + radius_y),
                           tex_width,
                           tex_height,
                           radius_y,
                           radius_x,
                           radius_y,
                           radius_x,
                           priv->paint_flags);

  return TRUE;
}

static gboolean
mex_shadow_get_paint_volume (ClutterEffect      *effect,
                             ClutterPaintVolume *volume)
{
  MexShadowPrivate *priv = MEX_SHADOW (effect)->priv;
  ClutterActor *actor;
  ClutterActorBox box;
  ClutterVertex origin;
  gfloat width, height;

  actor = clutter_actor_meta_get_actor (CLUTTER_ACTOR_META (effect));
  clutter_actor_get_allocation_box (actor, &box);


  clutter_paint_volume_get_origin (volume, &origin);
  width = clutter_paint_volume_get_width (volume);
  height = clutter_paint_volume_get_height (volume);

  width += priv->radius_x * 2;
  height += priv->radius_y * 2;

  origin.x += - priv->radius_x + priv->offset_x;
  origin.y += - priv->radius_y + priv->offset_y;

  clutter_paint_volume_set_origin (volume, &origin);
  clutter_paint_volume_set_width (volume, width);
  clutter_paint_volume_set_height (volume, height);

  return TRUE;
}

static void
mex_shadow_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  MexShadow *shadow = MEX_SHADOW (object);

  switch (property_id)
    {
    case PROP_COLOR:
      mex_shadow_set_color (shadow, clutter_value_get_color (value));
      break;

    case PROP_RADIUS_X:
      mex_shadow_set_radius_x (shadow, g_value_get_int (value));
      break;

    case PROP_RADIUS_Y:
      mex_shadow_set_radius_y (shadow, g_value_get_int (value));
      break;

    case PROP_OFFSET_X:
      mex_shadow_set_offset_x (shadow, g_value_get_int (value));
      break;

    case PROP_OFFSET_Y:
      mex_shadow_set_offset_y (shadow, g_value_get_int (value));
      break;

    case PROP_PAINT_FLAGS:
      mex_shadow_set_paint_flags (shadow, g_value_get_flags (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_shadow_dispose (GObject *object)
{
  MexShadowPrivate *priv = MEX_SHADOW (object)->priv;

  if (priv->material)
    {
      cogl_handle_unref (priv->material);
      priv->material = NULL;
    }

  G_OBJECT_CLASS (mex_shadow_parent_class)->dispose (object);
}

static void
mex_shadow_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_shadow_parent_class)->finalize (object);
}

static void
mex_shadow_class_init (MexShadowClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterEffectClass *effect_class = CLUTTER_EFFECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexShadowPrivate));

  object_class->get_property = mex_shadow_get_property;
  object_class->set_property = mex_shadow_set_property;
  object_class->dispose = mex_shadow_dispose;
  object_class->finalize = mex_shadow_finalize;

  effect_class->pre_paint = mex_shadow_paint_cb;
  effect_class->get_paint_volume = mex_shadow_get_paint_volume;

  pspec = g_param_spec_int ("radius-x",
                            "X Radius",
                            "Shadow horizontal radius.",
                            0, G_MAXINT, DEFAULT_RADIUS,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_RADIUS_X, pspec);

  pspec = g_param_spec_int ("radius-y",
                            "Y Radius",
                            "Shadow vertical radius.",
                            0, G_MAXINT, DEFAULT_RADIUS,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_RADIUS_Y, pspec);

  pspec = g_param_spec_int ("offset-x",
                            "X Offset",
                            "Shadow horizontal offset.",
                            -G_MAXINT, G_MAXINT, 0,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_OFFSET_X, pspec);

  pspec = g_param_spec_int ("offset-y",
                            "Y Offset",
                            "Shadow vertical offset.",
                            -G_MAXINT, G_MAXINT, 0,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_OFFSET_Y, pspec);

  pspec = clutter_param_spec_color ("color",
                                    "Color",
                                    "Shadow color.",
                                    &mex_shadow_default_color,
                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_COLOR, pspec);

  pspec = g_param_spec_flags ("paint-flags",
                              "Paint flags",
                              "Flags to determine which parts of the shadow "
                              "to paint.",
                              MEX_TYPE_PAINT_TEXTURE_FRAME_FLAGS,
                              MEX_PAINT_TEXTURE_FRAME_NOC,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PAINT_FLAGS, pspec);
}

static void
mex_shadow_convolve_transpose_normalise (gfloat *kernel,
                                         gint    radius,
                                         guchar *buffer,
                                         guchar *output,
                                         gint    width,
                                         gint    height)
{
  guchar normal;
  gint x, y, col;

  normal = 0;
  for (y = 0; y < height; y++)
    {
      gint out_index = y;
      gint in_index = y * width;

      for (x = 0; x < width; x++, out_index += height)
        {
          gfloat a = 0.f;

          for (col = -radius; col <= radius; col++)
            {
              gfloat f = kernel[col + radius];

              if (f != 0.0)
                {
                  gint ix = CLAMP (x + col, 0, width - 1);
                  guchar pixel = buffer[in_index + ix];
                  a += f * (gfloat)pixel;
                }
            }

          output[out_index] = (guchar)CLAMP (a + 0.5, 0, 0xff);
          if (output[out_index] > normal)
            normal = output[out_index];
        }
    }

  for (x = 0; x < width * height; x++)
    output[x] = (guchar)((gfloat)output[x] / (gfloat)normal * 255.f);
}

static gfloat *
mex_shadow_gaussian_kernel_gen (gint radius, gsize *size)
{
  gint i, row, rows;
  gfloat *kernel, sigma, sigma2_2, sigma_pi_2, sqrt_sigma_pi_2, radius2, sum;

  /* Make Gaussian blur kernel */
  sigma = radius / 3.f;
  sigma2_2 = sigma * sigma * 2;
  sigma_pi_2 = sigma * G_PI * 2;
  sqrt_sigma_pi_2 = sqrtf (sigma_pi_2);
  radius2 = radius * radius;

  rows = radius * 2 + 1;
  *size = sizeof (gfloat) * rows;
  kernel = g_slice_alloc (*size);

  sum = 0;
  for (row = -radius, i = 0; row <= radius; row++, i++)
    {
      gfloat distance = row * row;
      if (distance > radius2)
        kernel[i] = 0;
      else
        kernel[i] = expf (-distance / sigma2_2) / sqrt_sigma_pi_2;
      sum += kernel[i];
    }
  for (i = 0; i < rows; i++)
    kernel[i] /= sum;

  return kernel;
}

static void
mex_shadow_regenerate (MexShadow *shadow)
{
  gpointer hash;
  gsize image_size;
  gsize xsize, ysize;
  CoglHandle texture;
  gint radius_x, radius_y;
  guchar *buffer1, *buffer2;
  gfloat *xkernel, *ykernel;

  MexShadowPrivate *priv = shadow->priv;

  if (!priv->needs_regen)
    return;

  priv->needs_regen = FALSE;

  radius_x = MAX (1, priv->radius_x);
  radius_y = MAX (1, priv->radius_y);

  /* Let's assume that no one's going to ask for a shadow with x or y radius
   * greater than 65535 pixels (16 bits)
   */
  hash = GINT_TO_POINTER ((radius_x << 16) | radius_y);

  if (shadow_cache)
    texture = g_hash_table_lookup (shadow_cache, hash);
  else
    {
      shadow_cache = g_hash_table_new (NULL, NULL);
      texture = NULL;
    }

  if (!texture)
    {
      xkernel = mex_shadow_gaussian_kernel_gen (radius_x, &xsize);
      ykernel = mex_shadow_gaussian_kernel_gen (radius_y, &ysize);

      /* Create surface to blur */
      image_size = sizeof (guchar) * (radius_x * 2) * (radius_y * 2);
      buffer1 = g_slice_alloc0 (image_size);
      buffer2 = g_slice_alloc0 (image_size);
      buffer1[radius_y * (radius_x * 2) + radius_x] = 0xff;

      /* Blur */
      mex_shadow_convolve_transpose_normalise (xkernel,
                                               radius_x,
                                               buffer1,
                                               buffer2,
                                               radius_x * 2,
                                               radius_y * 2);
      mex_shadow_convolve_transpose_normalise (ykernel,
                                               radius_y,
                                               buffer2,
                                               buffer1,
                                               radius_y * 2,
                                               radius_x * 2);

      /* We don't need the kernels anymore */
      g_slice_free1 (xsize, xkernel);
      g_slice_free1 (ysize, ykernel);

      /* We don't need the 2nd buffer anymore */
      g_slice_free1 (image_size, buffer2);

      /* Create texture */
      texture = cogl_texture_new_from_data (radius_x * 2,
                                            radius_y * 2,
                                            COGL_TEXTURE_NONE,
                                            COGL_PIXEL_FORMAT_A_8,
                                            COGL_PIXEL_FORMAT_RGBA_8888_PRE,
                                            radius_x * 2,
                                            (guint8 *)buffer1);
      g_slice_free1 (image_size, buffer1);

      /* Insert this shadow texture into the cache */
      g_hash_table_insert (shadow_cache, hash, texture);
    }

  /* Set texture on material */
  cogl_material_set_layer (priv->material, 0, texture);
}

static void
mex_shadow_init (MexShadow *self)
{
  MexShadowPrivate *priv = self->priv = SHADOW_PRIVATE (self);

  priv->radius_x = DEFAULT_RADIUS;
  priv->radius_y = DEFAULT_RADIUS;
  priv->paint_flags = MEX_PAINT_TEXTURE_FRAME_NOC;
  priv->offset_x = 0;
  priv->offset_y = 0;
  priv->color = mex_shadow_default_color;
  priv->material = cogl_material_new ();
  cogl_material_set_layer_combine (priv->material, 0,
                                   "RGBA = MODULATE (PREVIOUS, TEXTURE[A])",
                                   NULL);
}

MexShadow *
mex_shadow_new (void)
{
  return g_object_new (MEX_TYPE_SHADOW, NULL);
}

void
mex_shadow_set_color (MexShadow *shadow, const ClutterColor *color)
{
  MexShadowPrivate *priv;

  g_return_if_fail (MEX_IS_SHADOW (shadow));

  priv = shadow->priv;

  if ((color->red != priv->color.red) ||
      (color->green != priv->color.green) ||
      (color->blue != priv->color.blue) ||
      (color->alpha != priv->color.alpha))
    {
      priv->color = *color;
      g_object_notify (G_OBJECT (shadow), "color");

      priv->needs_regen = TRUE;
    }
}

const ClutterColor *
mex_shadow_get_color (MexShadow *shadow)
{
  g_return_val_if_fail (MEX_IS_SHADOW (shadow), NULL);
  return &shadow->priv->color;
}

void
mex_shadow_set_radius_x (MexShadow *shadow, gint radius)
{
  MexShadowPrivate *priv;

  g_return_if_fail (MEX_IS_SHADOW (shadow));
  g_return_if_fail (radius >= 0);

  priv = shadow->priv;

  if (priv->radius_x != radius)
    {
      priv->radius_x = radius;
      mex_shadow_regenerate (shadow);

      g_object_notify (G_OBJECT (shadow), "radius-x");

      priv->needs_regen = TRUE;
    }
}

gint
mex_shadow_get_radius_x (MexShadow *shadow)
{
  g_return_val_if_fail (MEX_IS_SHADOW (shadow), 0);
  return shadow->priv->radius_x;
}

void
mex_shadow_set_radius_y (MexShadow *shadow, gint radius)
{
  MexShadowPrivate *priv;

  g_return_if_fail (MEX_IS_SHADOW (shadow));
  g_return_if_fail (radius >= 0);

  priv = shadow->priv;

  if (priv->radius_y != radius)
    {
      priv->radius_y = radius;
      mex_shadow_regenerate (shadow);

      g_object_notify (G_OBJECT (shadow), "radius-y");

      priv->needs_regen = TRUE;
    }
}

gint
mex_shadow_get_radius_y (MexShadow *shadow)
{
  g_return_val_if_fail (MEX_IS_SHADOW (shadow), 0);
  return shadow->priv->radius_y;
}

void
mex_shadow_set_offset_x (MexShadow *shadow, gint offset)
{
  MexShadowPrivate *priv;

  g_return_if_fail (MEX_IS_SHADOW (shadow));

  priv = shadow->priv;

  if (priv->offset_x != offset)
    {
      priv->offset_x = offset;

      g_object_notify (G_OBJECT (shadow), "offset-x");

      priv->needs_regen = TRUE;
    }
}

gint
mex_shadow_get_offset_x (MexShadow *shadow)
{
  g_return_val_if_fail (MEX_IS_SHADOW (shadow), 0);
  return shadow->priv->offset_x;
}

void
mex_shadow_set_offset_y (MexShadow *shadow, gint offset)
{
  MexShadowPrivate *priv;

  g_return_if_fail (MEX_IS_SHADOW (shadow));

  priv = shadow->priv;

  if (priv->offset_y != offset)
    {
      priv->offset_y = offset;

      g_object_notify (G_OBJECT (shadow), "offset-y");

      priv->needs_regen = TRUE;
    }
}

gint
mex_shadow_get_offset_y (MexShadow *shadow)
{
  g_return_val_if_fail (MEX_IS_SHADOW (shadow), 0);
  return shadow->priv->offset_y;
}

void
mex_shadow_set_paint_flags (MexShadow                 *shadow,
                            MexPaintTextureFrameFlags  flags)
{
  MexShadowPrivate *priv;

  g_return_if_fail (MEX_IS_SHADOW (shadow));

  priv = shadow->priv;

  if (priv->paint_flags != flags)
    {
      priv->paint_flags = flags;

      g_object_notify (G_OBJECT (shadow), "paint-flags");

      priv->needs_regen = TRUE;
    }
}

MexPaintTextureFrameFlags
mex_shadow_get_paint_flags (MexShadow *shadow)
{
  g_return_val_if_fail (MEX_IS_SHADOW (shadow), FALSE);
  return shadow->priv->paint_flags;
}
