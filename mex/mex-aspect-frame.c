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


#include "mex-aspect-frame.h"

G_DEFINE_TYPE (MexAspectFrame, mex_aspect_frame, MX_TYPE_BIN)

#define ASPECT_FRAME_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_ASPECT_FRAME, MexAspectFramePrivate))

enum
{
  PROP_0,

  PROP_EXPAND,
  PROP_RATIO
};

struct _MexAspectFramePrivate
{
  guint expand : 1;

  gfloat ratio;
};


static void
mex_aspect_frame_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MexAspectFrame *frame = MEX_ASPECT_FRAME (object);

  switch (property_id)
    {
    case PROP_EXPAND:
      g_value_set_boolean (value, mex_aspect_frame_get_expand (frame));
      break;

    case PROP_RATIO:
      g_value_set_float (value, mex_aspect_frame_get_ratio (frame));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_aspect_frame_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_EXPAND:
      mex_aspect_frame_set_expand (MEX_ASPECT_FRAME (object),
                                   g_value_get_boolean (value));
      break;

    case PROP_RATIO:
      mex_aspect_frame_set_ratio (MEX_ASPECT_FRAME (object),
                                  g_value_get_float (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_aspect_frame_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_aspect_frame_parent_class)->dispose (object);
}

static void
mex_aspect_frame_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_aspect_frame_parent_class)->finalize (object);
}

static void
mex_aspect_frame_get_preferred_width (ClutterActor *actor,
                                      gfloat        for_height,
                                      gfloat       *min_width_p,
                                      gfloat       *nat_width_p)
{
  gboolean override;
  MxPadding padding;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (for_height >= 0)
    for_height = MAX (0, for_height - padding.top - padding.bottom);

  if (for_height >= 0)
    override = FALSE;
  else
    g_object_get (G_OBJECT (actor), "natural-height-set", &override, NULL);

  if (override)
    g_object_get (G_OBJECT (actor), "natural-height", &for_height, NULL);

  CLUTTER_ACTOR_CLASS (mex_aspect_frame_parent_class)->
    get_preferred_width (actor, for_height, min_width_p, nat_width_p);

  if (min_width_p)
    *min_width_p += padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p += padding.left + padding.right;
}

static void
mex_aspect_frame_get_preferred_height (ClutterActor *actor,
                                       gfloat        for_width,
                                       gfloat       *min_height_p,
                                       gfloat       *nat_height_p)
{
  gboolean override;
  MxPadding padding;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (for_width >= 0)
    for_width = MAX (0, for_width - padding.left - padding.right);

  if (for_width >= 0)
    override = FALSE;
  else
    g_object_get (G_OBJECT (actor), "natural-width-set", &override, NULL);

  if (override)
    g_object_get (G_OBJECT (actor), "natural-width", &for_width, NULL);

  CLUTTER_ACTOR_CLASS (mex_aspect_frame_parent_class)->
    get_preferred_height (actor, for_width, min_height_p, nat_height_p);
}

static void
mex_aspect_frame_allocate (ClutterActor           *actor,
                           const ClutterActorBox  *box,
                           ClutterAllocationFlags  flags)
{
  MxPadding padding;
  ClutterActor *child;
  ClutterActorBox child_box;
  gfloat aspect, child_aspect, width, height, box_width, box_height;

  MexAspectFramePrivate *priv = MEX_ASPECT_FRAME (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_aspect_frame_parent_class)->
    allocate (actor, box, flags);

  child = mx_bin_get_child (MX_BIN (actor));
  if (!child)
    return;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  box_width = box->x2 - box->x1 - padding.left - padding.right;
  box_height = box->y2 - box->y1 - padding.top - padding.bottom;
  clutter_actor_get_preferred_size (child, NULL, NULL, &width, &height);

  aspect = box_width / box_height;
  if (priv->ratio >= 0.f)
    child_aspect = priv->ratio;
  else
    child_aspect = width / height;

  if ((aspect < child_aspect) ^ priv->expand)
    {
      width = box_width;
      height = box_width / child_aspect;
    }
  else
    {
      height = box_height;
      width = box_height * child_aspect;
    }

  child_box.x1 = (box_width - width) / 2 + padding.left;
  child_box.y1 = (box_height - height) / 2 + padding.top;
  child_box.x2 = child_box.x1 + width;
  child_box.y2 = child_box.y1 + height;

  clutter_actor_allocate (child, &child_box, flags);
}

static void
mex_aspect_frame_paint (ClutterActor *actor)
{
  ClutterActor *child = mx_bin_get_child (MX_BIN (actor));
  MexAspectFramePrivate *priv = MEX_ASPECT_FRAME (actor)->priv;

  if (!child)
    return;

  if (priv->expand)
    {
      MxPadding padding;
      gfloat width, height;

      clutter_actor_get_size (actor, &width, &height);
      mx_widget_get_padding (MX_WIDGET (actor), &padding);

      /* Special-case textures and just munge their coordinates.
       * This avoids clipping, which can break Clutter's batching.
       */
      if (CLUTTER_IS_TEXTURE (child))
        {
          guint8 opacity;
          gfloat x, y, tx, ty;
          CoglHandle material;

          clutter_actor_get_position (child, &x, &y);

          material =
            clutter_texture_get_cogl_material (CLUTTER_TEXTURE (child));
          opacity = clutter_actor_get_paint_opacity (child);
          cogl_material_set_color4ub (material,
                                      opacity, opacity, opacity, opacity);
          cogl_set_source (material);

          x -= padding.left;
          y -= padding.top;
          width -= padding.left + padding.right;
          height -= padding.top + padding.bottom;

          tx = (width / (width - (x * 2.f))) / 2.f;
          ty = (height / (height - (y * 2.f))) / 2.f;

          cogl_rectangle_with_texture_coords (padding.left, padding.top,
                                              padding.left + width,
                                              padding.top + height,
                                              0.5f - tx, 0.5f - ty,
                                              0.5f + tx, 0.5f + ty);
        }
      else
        {
          cogl_clip_push_rectangle (padding.left, padding.top,
                                    padding.left + width, padding.top + height);
          clutter_actor_paint (child);
          cogl_clip_pop ();
        }
    }
  else
    clutter_actor_paint (child);
}

static void
mex_aspect_frame_pick (ClutterActor       *actor,
                       const ClutterColor *color)
{
  ClutterActorBox box;

  ClutterActor *child = mx_bin_get_child (MX_BIN (actor));
  MexAspectFramePrivate *priv = MEX_ASPECT_FRAME (actor)->priv;

  clutter_actor_get_allocation_box (actor, &box);

  cogl_set_source_color4ub (color->red, color->green,
                            color->blue, color->alpha);
  cogl_rectangle (box.x1, box.y1, box.x2, box.y2);

  if (!child)
    return;

  if (priv->expand)
    {
      MxPadding padding;
      mx_widget_get_padding (MX_WIDGET (actor), &padding);

      cogl_clip_push_rectangle (padding.left, padding.top,
                                padding.left + (box.x2 - box.x1),
                                padding.top + (box.y2 - box.y1));
      clutter_actor_paint (child);
      cogl_clip_pop ();
    }
  else
    clutter_actor_paint (child);
}

static void
mex_aspect_frame_class_init (MexAspectFrameClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexAspectFramePrivate));

  object_class->get_property = mex_aspect_frame_get_property;
  object_class->set_property = mex_aspect_frame_set_property;
  object_class->dispose = mex_aspect_frame_dispose;
  object_class->finalize = mex_aspect_frame_finalize;

  actor_class->get_preferred_width = mex_aspect_frame_get_preferred_width;
  actor_class->get_preferred_height = mex_aspect_frame_get_preferred_height;
  actor_class->allocate = mex_aspect_frame_allocate;
  actor_class->paint = mex_aspect_frame_paint;
  actor_class->pick = mex_aspect_frame_pick;

  pspec = g_param_spec_boolean ("expand",
                                "Expand",
                                "Fill the allocated area with the child and "
                                "clip off the excess.",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_EXPAND, pspec);

  pspec = g_param_spec_float ("ratio",
                              "Ratio",
                              "Override the child's aspect ratio "
                              "(width/height).",
                              -1.f, G_MAXFLOAT, -1.f,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_RATIO, pspec);
}

static void
mex_aspect_frame_init (MexAspectFrame *self)
{
  MexAspectFramePrivate *priv = self->priv = ASPECT_FRAME_PRIVATE (self);
  priv->ratio = -1.f;
}

ClutterActor *
mex_aspect_frame_new (void)
{
  return g_object_new (MEX_TYPE_ASPECT_FRAME, NULL);
}

void
mex_aspect_frame_set_expand (MexAspectFrame *frame, gboolean expand)
{
  MexAspectFramePrivate *priv;

  g_return_if_fail (MEX_IS_ASPECT_FRAME (frame));

  priv = frame->priv;
  if (priv->expand != expand)
    {
      priv->expand = expand;
      clutter_actor_queue_relayout (CLUTTER_ACTOR (frame));
      g_object_notify (G_OBJECT (frame), "expand");
    }
}

gboolean
mex_aspect_frame_get_expand (MexAspectFrame *frame)
{
  g_return_val_if_fail (MEX_IS_ASPECT_FRAME (frame), FALSE);
  return frame->priv->expand;
}

void
mex_aspect_frame_set_ratio (MexAspectFrame *frame, gfloat ratio)
{
  MexAspectFramePrivate *priv;

  g_return_if_fail (MEX_IS_ASPECT_FRAME (frame));

  priv = frame->priv;
  if (priv->ratio != ratio)
    {
      priv->ratio = ratio;
      clutter_actor_queue_relayout (CLUTTER_ACTOR (frame));
      g_object_notify (G_OBJECT (frame), "ratio");
    }
}

gfloat
mex_aspect_frame_get_ratio (MexAspectFrame *frame)
{
  g_return_val_if_fail (MEX_IS_ASPECT_FRAME (frame), -1.f);
  return frame->priv->ratio;
}
