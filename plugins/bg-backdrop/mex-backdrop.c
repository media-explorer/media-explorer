#include <math.h>

#include <mex/mex-background.h>

#include "mex-backdrop.h"

static void mex_backdrop_background_iface_init (MexBackgroundIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexBackdrop, mex_backdrop, CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_BACKGROUND,
                                                mex_backdrop_background_iface_init));

enum
{
  PROP_0,
  PROP_SOURCE,
  PROP_LAST
};

#define MEX_BACKDROP_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MEX_TYPE_BACKDROP, MexBackdropPrivate))

typedef struct BackdropItem {
  int           active;
  double        scale;         /* spatial scaling factor */
  double        target_scale;  /* target/final scaling factor */
  double        progress;      /* animation progress 0.0 = center, 1.0 = final, negative = upcoming */
  double        direction;     /* 0 .. 2 x PI */
  double        opacity;       /* opacity (computed from progress) */
  double        x;             /* spatial position (computed from progress)*/
  double        y;
  CoglMaterial *material;
  double        size;
} BackdropItem;

#define N_ITEMS     20
#define REL_SIZE    0.35
#define FADE_IN     0.05
#define TRAVEL_TIME 16000.0 /* 15s */
#define MIN_T_SCALE 0.05
#define MAX_T_SCALE 4.0

struct _MexBackdropPrivate
{
  BackdropItem items[N_ITEMS];
  ClutterTimeline *timeline;
  const gchar *name;
  CoglMaterial *material;
};

static void time_step (ClutterTimeline *timeline,
                       gint             msecs,
                       ClutterActor    *self)
{
  MexBackdrop *backdrop = MEX_BACKDROP (self);
  MexBackdropPrivate *priv = backdrop->priv;
  gfloat w,h;
  gfloat radius;
  int i;
  int cx, cy;
  int item_size;
  gint delta = clutter_timeline_get_delta (timeline);
  static gint counter;


  counter += delta;

  if (counter >= 40)
    {
      delta = counter;
      counter = 0;
    }
  else
    return;


  clutter_actor_get_size (self, &w, &h);
  cx = w/2;
  cy = h/2;
  radius = w;
  if (radius < h)
    radius = h;
  radius /= 1.77;
  item_size = radius * REL_SIZE;

  for (i = 0; i < N_ITEMS; i++)
    {
      BackdropItem *item = &priv->items[i];
      if (item->active == 0)
        {
          if (!item->material)
            {
              CoglColor color;

              /* create a cheap copy of the template material */
              item->material = cogl_material_copy (priv->material);
              switch (i % 3)
                {
                  case 0:
                    cogl_color_init_from_4f (&color, 0.2, 0.2, 0.2, 0.2);
                    break;
                  case 1:
                    cogl_color_init_from_4f (&color, 0.4, 0.4, 0.4, 0.4);
                    break;
                  case 2:
                    cogl_color_init_from_4f (&color, 0.6, 0.4, 0.6, 0.7);
                    break;
                  case 3:
                    cogl_color_init_from_4f (&color, 1.0, 0.0, 0.0, 0.7);
                    break;
                  case 4:
                    cogl_color_init_from_4f (&color, 1.0, 0.5, 0.2, 0.8);
                    break;
                }

              cogl_material_set_layer_combine_constant (item->material, 1,
                                                        &color);
              cogl_material_set_layer_combine (item->material, 1,
                                               "RGBA = MODULATE (PREVIOUS, CONSTANT)",
                                               NULL);
            }
          {
            item->active = 1;
            item->progress = g_random_double_range (-1.5, 0.0); /* negative values are initially transparent */
            item->direction = g_random_double_range (0, G_PI * 2);
            item->target_scale = g_random_double_range (MIN_T_SCALE, MAX_T_SCALE);
            item->opacity = 1.0;
          }
        }
      else
        {
          item->progress += delta / TRAVEL_TIME;
          if (item->progress < 0.0)
            item->opacity = 0.0;
          else if (item->progress < FADE_IN)
            item->opacity = item->progress/FADE_IN;
          else
            item->opacity = 1.0 - (item->progress-FADE_IN)/(1.0-FADE_IN);
          if (item->opacity < 0.0)
            item->opacity = 0.0;

          if (item->progress < 0.0)
            item->scale = 0.0;
          else if (item->progress < FADE_IN)
            item->scale = 1.0 - pow(1.0-(item->progress / FADE_IN), 2);
          else
            item->scale = 1.0 * (1.0 - (item->progress-FADE_IN)/(1.0-FADE_IN)) +
                          item->target_scale * ((item->progress-FADE_IN)/(1.0-FADE_IN));

          item->x = cx + cos (item->direction) * item->progress * radius;
          item->y = cy + sin (item->direction) * item->progress * radius;
          if (item->progress >= 1.0)
            {
              item->progress = g_random_double_range (-0.5, 0);
              item->direction = g_random_double_range (0, G_PI * 2);
              item->target_scale = g_random_double_range (MIN_T_SCALE, MAX_T_SCALE);
            }

          /* update the item size */
          item->size = item_size * item->scale;
        }
    }

  clutter_actor_queue_redraw (self);
}

static void
mex_backdrop_paint (ClutterActor *self)
{
  MexBackdrop *backdrop = MEX_BACKDROP (self);
  MexBackdropPrivate *priv = backdrop->priv;
  int i;

  for (i = 0; i < N_ITEMS; i++)
    {
      BackdropItem *item = &priv->items[i];

      if (item->active)
        {
          cogl_set_source (item->material);
          cogl_rectangle (item->x, item->y, item->x + item->size,
                          item->y + item->size);
        }
    }
}

static void
mex_backdrop_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  switch (prop_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
  }
}

static void
mex_backdrop_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  switch (prop_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
mex_backdrop_dispose (GObject *object)
{
  MexBackdropPrivate *priv = MEX_BACKDROP (object)->priv;
  int i;

  if (priv->timeline)
    {
      g_object_unref (priv->timeline);
      priv->timeline = NULL;
    }

  for (i = 0; i < N_ITEMS; i++)
    {
      BackdropItem *item = &priv->items[i];

      if (item->material)
        {
          cogl_object_unref (item->material);
          item->material = NULL;
        }
    }

  if (priv->material)
    {
      cogl_object_unref (priv->material);
      priv->material = NULL;
    }

  G_OBJECT_CLASS (mex_backdrop_parent_class)->dispose (object);
}

static void
mex_backdrop_class_init (MexBackdropClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (gobject_class, sizeof (MexBackdropPrivate));

  actor_class->paint    = mex_backdrop_paint;

  gobject_class->dispose      = mex_backdrop_dispose;
  gobject_class->set_property = mex_backdrop_set_property;
  gobject_class->get_property = mex_backdrop_get_property;
}

static void
mex_backdrop_init (MexBackdrop *self)
{
  MexBackdropPrivate *priv;
  CoglHandle *texture;
  GError *error = NULL;

  self->priv = priv = MEX_BACKDROP_GET_PRIVATE (self);

  priv->name = "backdrop";

  priv->timeline = clutter_timeline_new (100000000);
  clutter_timeline_set_loop (priv->timeline, TRUE);

  /* create template material */
  priv->material = cogl_material_new ();

  texture = cogl_texture_new_from_file (MEX_DATA_PLUGIN_DIR "/sprite.png",
                                        COGL_TEXTURE_NONE,
                                        COGL_PIXEL_FORMAT_ANY, NULL);

  if (error)
    {
      g_warning ("Error loading texture: %s", error->message);
      g_clear_error (&error);
    }
  else
    {
      cogl_material_set_layer (priv->material, 0, texture);
    }

  g_signal_connect (priv->timeline, "new-frame", G_CALLBACK (time_step), self);
}

ClutterActor *mex_backdrop_new (void)
{
  return g_object_new (MEX_TYPE_BACKDROP, NULL);
}

static void
mex_backdrop_set_active (MexBackground *self,
                         gboolean       active)
{
  MexBackdropPrivate *priv = MEX_BACKDROP (self)->priv;

  if (active)
    clutter_timeline_start (priv->timeline);
  else
    clutter_timeline_pause (priv->timeline);
}

static const gchar*
mex_backdrop_get_name (MexBackground *self)
{
  MexBackdropPrivate *priv = MEX_BACKDROP (self)->priv;

  return priv->name;
}

static void
mex_backdrop_background_iface_init (MexBackgroundIface *iface)
{
  iface->set_active = mex_backdrop_set_active;
  iface->get_name = mex_backdrop_get_name;
}
