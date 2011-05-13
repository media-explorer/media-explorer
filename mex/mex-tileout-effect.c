/*
 * mex-tileout-effect.c: A simple tile based effect
 *
 * Copyright © 2011 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Written by: Damien Lespiau <damien.lespiau@intel.com>
 */

#include <string.h>

#include "mex-tileout-effect.h"

G_DEFINE_TYPE (MexTileoutEffect, mex_tileout_effect, CLUTTER_TYPE_OFFSCREEN_EFFECT)

#define TILEOUT_EFFECT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TILEOUT_EFFECT, MexTileoutEffectPrivate))

enum
{
  PROP_0,

  PROP_N_ROWS,
  PROP_N_COLUMNS,
  PROP_PROGRESS
};

typedef struct
{
  gdouble start, duration; /* normalized between 0.0 and 1.0 */
  gint index, length;
} Chunk;

typedef struct
{
  gfloat x1, y1, x2, y2;
  gfloat tx1, ty1, tx2, ty2;
} RectCoords;

typedef struct
{
  gint current_length;      /* length (number of tiles of the diagonal row */
  RectCoords first_rect;    /* first rectangle of the diagonal row */
} ChunkBuildContext;

struct _MexTileoutEffectPrivate
{
  guint need_setup : 1;

  gfloat width, height;
  gfloat tile_width, tile_height;
  gfloat tile_width_tex, tile_height_tex;

  gint n_rows, n_columns;
  GArray *chunks;
  GArray *rects;
  GArray *animated_rects;

  gdouble progress;
};

static void
mex_tileout_effect_set_n_columns_internal (MexTileoutEffect *effect,
                                           gint              n_columns)
{
  MexTileoutEffectPrivate *priv = effect->priv;

  if (priv->n_columns == n_columns)
    return;

  priv->n_columns = n_columns;
  priv->need_setup = TRUE;
}

static void
mex_tileout_effect_set_n_rows_internal (MexTileoutEffect *effect,
                                        gint              n_rows)
{
  MexTileoutEffectPrivate *priv = effect->priv;

  if (priv->n_columns == n_rows)
    return;

  priv->n_rows = n_rows;
  priv->need_setup = TRUE;
}

/*
 * ClutterOffscreenEffect implementation
 */
static CoglHandle
mex_tileout_effect_create_texture (ClutterOffscreenEffect *effect,
                                   gfloat                  width,
                                   gfloat                  height)
{
  MexTileoutEffectPrivate *priv = MEX_TILEOUT_EFFECT (effect)->priv;

  priv->width = width;
  priv->height = height;
  priv->need_setup = TRUE;

  return CLUTTER_OFFSCREEN_EFFECT_CLASS (mex_tileout_effect_parent_class)->
    create_texture (effect, width, height);
}

static void
chunks_length_context_init (MexTileoutEffect  *effect,
                            ChunkBuildContext *ctx)
{
  MexTileoutEffectPrivate *priv = effect->priv;

  ctx->current_length = 0;

  ctx->first_rect.x1 = 0.0f;
  ctx->first_rect.y1 = priv->height;
  ctx->first_rect.x2 = priv->tile_width;
  ctx->first_rect.y2 = priv->height + priv->tile_height;

  ctx->first_rect.tx1 = 0.0f;
  ctx->first_rect.ty1 = 1.0f;
  ctx->first_rect.tx2 = priv->tile_width_tex;
  ctx->first_rect.ty2 = 1.0f + priv->tile_height_tex;
}

static gint
compute_chunk_length (MexTileoutEffect   *effect,
                      ChunkBuildContext *ctx,
                      gint                chunk_nr)
{
  MexTileoutEffectPrivate *priv = effect->priv;

  if (chunk_nr > priv->n_rows + priv->n_columns - 1)
    return -1;

  if (chunk_nr < priv->n_columns)
    ctx->current_length++;
  else
    ctx->current_length--;

  return ctx->current_length;
}

static void
append_rects (MexTileoutEffect  *effect,
              ChunkBuildContext *ctx,
              Chunk             *chunk)
{
  MexTileoutEffectPrivate *priv = effect->priv;
  RectCoords *previous_rect, *rect;
  gint i;

  /* first tile */
  previous_rect = &g_array_index (priv->rects, RectCoords, chunk->index);
  if (ctx->first_rect.ty1 - priv->tile_width_tex < 0)
    {
      /* we're starting from the top row, ie at the right of the last tile */
      previous_rect->x1 = ctx->first_rect.x2;
      previous_rect->y1 = ctx->first_rect.y1;
      previous_rect->x2 = ctx->first_rect.x2 + priv->tile_width;
      previous_rect->y2 = ctx->first_rect.y2;

      previous_rect->tx1 = ctx->first_rect.tx2;
      previous_rect->ty1 = ctx->first_rect.ty1;
      previous_rect->tx2 = ctx->first_rect.tx2 + priv->tile_width_tex;
      previous_rect->ty2 = ctx->first_rect.ty2;
    }
  else
    {
      /* we're starting above the last cell */
      previous_rect->x1 = ctx->first_rect.x1;
      previous_rect->y1 = ctx->first_rect.y1 - priv->tile_height;
      previous_rect->x2 = ctx->first_rect.x2;
      previous_rect->y2 = ctx->first_rect.y1;

      previous_rect->tx1 = ctx->first_rect.tx1;
      previous_rect->ty1 = ctx->first_rect.ty1 - priv->tile_height_tex;
      previous_rect->tx2 = ctx->first_rect.tx2;
      previous_rect->ty2 = ctx->first_rect.ty1;
    }

  /* rest of the diagonal */
  for (i = 1; i < chunk->length; i++)
    {
      rect = &g_array_index (priv->rects, RectCoords, chunk->index + i);

      rect->x1 = previous_rect->x2;
      rect->y1 = previous_rect->y2;
      rect->x2 = previous_rect->x2 + priv->tile_width;
      rect->y2 = previous_rect->y2 + priv->tile_height;

      rect->tx1 = previous_rect->tx2;
      rect->ty1 = previous_rect->ty2;
      rect->tx2 = previous_rect->tx2 + priv->tile_width_tex;
      rect->ty2 = previous_rect->ty2 + priv->tile_height_tex;

      previous_rect = rect;
    }

  /* update the context with the first tile of the diagonal row */
  rect = &g_array_index (priv->rects, RectCoords, chunk->index);
  memcpy (&ctx->first_rect, rect, sizeof (RectCoords));
}

static void
setup_effect (MexTileoutEffect *effect)
{
  MexTileoutEffectPrivate *priv = effect->priv;
  ChunkBuildContext ctx;
  gint n_chunks, idx, i;

  priv->tile_width = priv->width / priv->n_columns;
  priv->tile_height = priv->height / priv->n_rows;
  priv->tile_width_tex = priv->tile_width / priv->width;
  priv->tile_height_tex = priv->tile_height / priv->height;

  n_chunks = priv->n_rows + priv->n_columns - 1;
  idx = 0;

  if (priv->chunks)
    g_array_free (priv->chunks, TRUE);
  priv->chunks = g_array_sized_new (FALSE, FALSE, sizeof (Chunk), n_chunks);
  g_array_set_size (priv->chunks, n_chunks);

  if (priv->rects)
    g_array_free (priv->rects, TRUE);
  priv->rects = g_array_sized_new (FALSE, FALSE, sizeof (RectCoords),
                                   priv->n_columns * priv->n_rows);
  g_array_set_size (priv->rects, priv->n_columns * priv->n_rows);

  chunks_length_context_init (effect, &ctx);

  for (i = 0; i < priv->chunks->len; i++)
    {
      Chunk *chunk = &g_array_index (priv->chunks, Chunk, i);
      gfloat interval;

      interval = 1.0f / (n_chunks + 4);
      chunk->start = i * interval;
      chunk->duration = 4 * interval;
      chunk->index = idx;

      /* This is a bit more tricky */
      chunk->length = compute_chunk_length (effect, &ctx, i);
      append_rects (effect, &ctx, chunk);

      idx += chunk->length;
    }

  g_assert (idx == (priv->n_columns * priv->n_rows));

  /* copy rects to animated_rects */
  if (priv->animated_rects)
    g_array_free (priv->animated_rects, TRUE);
  priv->animated_rects = g_array_sized_new (FALSE, FALSE, sizeof (RectCoords),
                                            priv->rects->len);
  g_array_set_size (priv->animated_rects, priv->rects->len);

  memcpy (priv->animated_rects->data, priv->rects->data,
          priv->rects->len * sizeof (RectCoords));
}

#if 0
static void
show_rectangles (MexTileoutEffect *effect)
{
  MexTileoutEffectPrivate *priv = effect->priv;
  guint i;

  for (i = 0; i < priv->rects->len; i++)
    {
      RectCoords *rect = &g_array_index (priv->rects, RectCoords, i);

      g_message ("Rect%d: (%.02f,%.02f) (%.02f,%.02f) - "
                 "(%.02f,%.02f) )%.02f,%.02f)",
                 i, rect->x1, rect->y1, rect->x2, rect->y2, 
                 rect->tx1, rect->ty1, rect->tx2, rect->ty2);
    }

}
#endif

/* pre-condition is that progress is between chunk->start and chunk->start +
 * chunk->duration */
static void
animate_chunk (MexTileoutEffect *effect,
               Chunk            *chunk,
               gdouble           progress)
{
  MexTileoutEffectPrivate *priv = effect->priv;
  guint i;
  gdouble percent;

  for (i = 0; i < chunk->length; i++)
    {
      RectCoords *rect;
      RectCoords *animated_rect;
      gfloat new_width, new_height;
      gfloat new_width_tex, new_height_tex;

      rect = &g_array_index (priv->rects, RectCoords, chunk->index + i);
      animated_rect = &g_array_index (priv->animated_rects, RectCoords,
                                      chunk->index + i);

      percent = (priv->progress - chunk->start) / (chunk->duration);

      new_width = priv->tile_width * (1.0f - percent);
      new_height = priv->tile_height * (1.0f - percent);

      animated_rect->x1 = rect->x1 + (priv->tile_width - new_width) / 2;
      animated_rect->y1 = rect->y1 + (priv->tile_height - new_height) / 2;
      animated_rect->x2 = animated_rect->x1 + new_width;
      animated_rect->y2 = animated_rect->y1 + new_height;

      new_width_tex = priv->tile_width_tex * (1.0f - percent);
      new_height_tex = priv->tile_height_tex * (1.0f -  percent);

      animated_rect->tx1 = rect->tx1 + (priv->tile_width_tex - new_width_tex) / 2;
      animated_rect->ty1 = rect->ty1 + (priv->tile_height_tex - new_height_tex) / 2;
      animated_rect->tx2 = animated_rect->tx1 + new_width_tex;
      animated_rect->ty2 = animated_rect->ty1 + new_height_tex;
    }

}

static void
mex_tileout_effect_paint_target (ClutterOffscreenEffect *effect)
{
  MexTileoutEffect *self = MEX_TILEOUT_EFFECT (effect);
  MexTileoutEffectPrivate *priv = self->priv;
  CoglHandle material;
  guint i;

  if (priv->need_setup)
    {
      setup_effect (self);
      priv->need_setup = FALSE;
    }

  /* at the end of the effect, there's nothing to draw */
  if (priv->progress >= 1.0)
    return;

  material = clutter_offscreen_effect_get_target (effect);
  cogl_set_source (material);

  /* when at 0.0, we can issue one cogl call */
  if (priv->progress <= 0.0)
    {
      cogl_rectangle_with_texture_coords (0, 0, priv->width, priv->height,
                                          0.f, 0.f, 1.f, 1.f);
      return;
    }

  for (i = 0; i < priv->chunks->len; i++)
    {
      Chunk *chunk = &g_array_index (priv->chunks, Chunk, i);
      RectCoords *rect;

      /* this chunk has finished animating */
      if (priv->progress > (chunk->start + chunk->duration))
          continue;

      if (priv->progress > chunk->start &&
          priv->progress < (chunk->start + chunk->duration))
        {
          animate_chunk (self, chunk, priv->progress);

        }

      rect = &g_array_index (priv->animated_rects, RectCoords, chunk->index);
      cogl_rectangles_with_texture_coords ((float *)rect, chunk->length);
    }
}

/*
 * GObject implementation
 */

static void
mex_tileout_effect_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MexTileoutEffect *effect = MEX_TILEOUT_EFFECT (object);
  MexTileoutEffectPrivate *priv = effect->priv;

  switch (property_id)
    {
    case PROP_N_ROWS:
      g_value_set_int (value, priv->n_rows);
      break;
    case PROP_N_COLUMNS:
      g_value_set_int (value, priv->n_columns);
      break;
    case PROP_PROGRESS:
      g_value_set_double (value, priv->progress);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tileout_effect_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MexTileoutEffect *effect = MEX_TILEOUT_EFFECT (object);
  MexTileoutEffectPrivate *priv = effect->priv;

  switch (property_id)
    {
    case PROP_N_ROWS:
      mex_tileout_effect_set_n_rows_internal (effect,
                                              g_value_get_int (value));
      break;
    case PROP_N_COLUMNS:
      mex_tileout_effect_set_n_columns_internal (effect,
                                                 g_value_get_int (value));
      break;
    case PROP_PROGRESS:
      priv->progress = g_value_get_double (value);
#if CLUTTER_CHECK_VERSION (1, 7, 1)
      /* If we have a recent version of Clutter then we can queue a
         rerun of the effect instead of redrawing the entire actor */
      clutter_effect_queue_rerun (CLUTTER_EFFECT (effect));
#else
      {
        ClutterActor *target;
        target = clutter_actor_meta_get_actor (CLUTTER_ACTOR_META (effect));
        clutter_actor_queue_redraw (target);
      }
#endif
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tileout_effect_finalize (GObject *object)
{
  MexTileoutEffect *effect = MEX_TILEOUT_EFFECT (object);
  MexTileoutEffectPrivate *priv = effect->priv;

  if (priv->chunks)
    g_array_free (priv->chunks, TRUE);

  if (priv->rects)
    g_array_free (priv->rects, TRUE);

  G_OBJECT_CLASS (mex_tileout_effect_parent_class)->finalize (object);
}

static void
mex_tileout_effect_class_init (MexTileoutEffectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterOffscreenEffectClass *offscreen_class;
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexTileoutEffectPrivate));

  object_class->get_property = mex_tileout_effect_get_property;
  object_class->set_property = mex_tileout_effect_set_property;
  object_class->finalize = mex_tileout_effect_finalize;

  offscreen_class = CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);
  offscreen_class->paint_target = mex_tileout_effect_paint_target;
  offscreen_class->create_texture = mex_tileout_effect_create_texture;

  pspec = g_param_spec_int ("n-rows",
                            "N-Rows",
                            "The number of rows in the grid",
                            1, G_MAXINT, 8,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_N_ROWS, pspec);

  pspec = g_param_spec_int ("n-columns",
                            "N-Columns",
                            "The number of columns in the grid",
                            1, G_MAXINT, 8,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_N_COLUMNS, pspec);

  pspec = g_param_spec_double ("progress",
                               "Progress",
                               "The progress of the effect",
                               0.0, 1.0, 0.0,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PROGRESS, pspec);
}

static void
mex_tileout_effect_init (MexTileoutEffect *self)
{
  MexTileoutEffectPrivate *priv;

  self->priv = priv = TILEOUT_EFFECT_PRIVATE (self);

  priv->need_setup = TRUE;
}

ClutterEffect *
mex_tileout_effect_new (void)
{
  return g_object_new (MEX_TYPE_TILEOUT_EFFECT, NULL);
}

void
mex_tileout_effect_set_n_columns (MexTileoutEffect *effect,
                                  gint              n_columns)
{
  g_return_if_fail (MEX_IS_TILEOUT_EFFECT (effect));

  mex_tileout_effect_set_n_columns_internal (effect, n_columns);

  g_object_notify (G_OBJECT (effect), "n-columns");
}

gint
mex_tileout_effect_get_n_columns (MexTileoutEffect *effect)
{
  g_return_val_if_fail (MEX_IS_TILEOUT_EFFECT (effect), 1);

  return effect->priv->n_columns;
}

void
mex_tileout_effect_set_n_rows (MexTileoutEffect *effect,
                               gint              n_rows)
{
  g_return_if_fail (MEX_IS_TILEOUT_EFFECT (effect));

  mex_tileout_effect_set_n_rows_internal (effect, n_rows);

  g_object_notify (G_OBJECT (effect), "n-rows");
}

gint
mex_tileout_effect_get_n_rows (MexTileoutEffect *effect)
{
  g_return_val_if_fail (MEX_IS_TILEOUT_EFFECT (effect), 1);

  return effect->priv->n_rows;
}
