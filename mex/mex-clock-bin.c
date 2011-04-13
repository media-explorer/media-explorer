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


#include "mex-clock-bin.h"

G_DEFINE_TYPE (MexClockBin, mex_clock_bin, MX_TYPE_BIN)

#define CLOCK_BIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_CLOCK_BIN, MexClockBinPrivate))

enum
{
  PROP_0,

  PROP_ICON
};

struct _MexClockBinPrivate
{
  ClutterActor    *clock_hbox;
  ClutterActor    *time_label;
  ClutterActor    *icon;
  guint            update_source;
};


static void
mex_clock_bin_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MexClockBin *self = MEX_CLOCK_BIN (object);

  switch (property_id)
    {
    case PROP_ICON:
      g_value_set_object (value, mex_clock_bin_get_icon (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_clock_bin_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_clock_bin_dispose (GObject *object)
{
  MexClockBinPrivate *priv = MEX_CLOCK_BIN (object)->priv;

  if (priv->update_source)
    {
      g_source_remove (priv->update_source);
      priv->update_source = 0;
    }

  G_OBJECT_CLASS (mex_clock_bin_parent_class)->dispose (object);
}

static void
mex_clock_bin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_clock_bin_parent_class)->finalize (object);
}

static void
mex_clock_bin_notify_focused_cb (MxFocusManager *manager,
                                 GParamSpec     *pspec,
                                 MexClockBin    *self)
{
  ClutterActor *parent = NULL;
  MexClockBinPrivate *priv = self->priv;
  ClutterActor *focused = (ClutterActor *)
    mx_focus_manager_get_focused (manager);

  while (focused)
    {
      parent = clutter_actor_get_parent (focused);
      if (focused == (ClutterActor *)self)
        {
          clutter_actor_animate (CLUTTER_ACTOR (priv->clock_hbox),
                                 CLUTTER_EASE_OUT_QUAD,
                                 150, "opacity", 0x00, NULL);
          return;
        }
      focused = parent;
    }

  clutter_actor_animate (CLUTTER_ACTOR (priv->clock_hbox),
                         CLUTTER_EASE_OUT_QUAD, 150,
                         "opacity", 0xff, NULL);
}

static void
mex_clock_bin_map (ClutterActor *actor)
{
  ClutterActor *stage;
  MxFocusManager *manager;

  MexClockBinPrivate *priv = MEX_CLOCK_BIN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_clock_bin_parent_class)->map (actor);

  clutter_actor_map (priv->clock_hbox);

  stage = clutter_actor_get_stage (actor);
  manager = mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));
  g_signal_connect (manager, "notify::focused",
                    G_CALLBACK (mex_clock_bin_notify_focused_cb), actor);
}

static void
mex_clock_bin_unmap (ClutterActor *actor)
{
  ClutterActor *stage;
  MxFocusManager *manager;

  MexClockBinPrivate *priv = MEX_CLOCK_BIN (actor)->priv;

  stage = clutter_actor_get_stage (actor);
  manager = mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));
  g_signal_handlers_disconnect_by_func (manager,
                                        mex_clock_bin_notify_focused_cb,
                                        actor);

  clutter_actor_unmap (priv->clock_hbox);

  CLUTTER_ACTOR_CLASS (mex_clock_bin_parent_class)->unmap (actor);
}

static void
mex_clock_bin_get_preferred_width (ClutterActor *actor,
                                   gfloat        for_height,
                                   gfloat       *min_width_p,
                                   gfloat       *nat_width_p)
{
  MxPadding padding;
  gfloat min_width, nat_width;

  MexClockBinPrivate *priv = MEX_CLOCK_BIN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_clock_bin_parent_class)->
    get_preferred_width (actor, for_height, min_width_p, nat_width_p);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (for_height >= 0)
    for_height = MAX (0, for_height - padding.top - padding.bottom);

  clutter_actor_get_preferred_width (priv->clock_hbox,
                                     for_height,
                                     &min_width,
                                     &nat_width);

  min_width += padding.left + padding.right;
  nat_width += padding.left + padding.right;

  if (min_width_p && (*min_width_p < min_width))
    *min_width_p = min_width;
  if (nat_width_p && (*nat_width_p < nat_width))
    *nat_width_p = nat_width;
}

static void
mex_clock_bin_get_preferred_height (ClutterActor *actor,
                                    gfloat        for_width,
                                    gfloat       *min_height_p,
                                    gfloat       *nat_height_p)
{
  MxPadding padding;
  gfloat min_height, nat_height;

  MexClockBinPrivate *priv = MEX_CLOCK_BIN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_clock_bin_parent_class)->
    get_preferred_height (actor, for_width, min_height_p, nat_height_p);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (for_width >= 0)
    for_width = MAX (0, for_width - padding.left - padding.right);

  clutter_actor_get_preferred_height (priv->clock_hbox,
                                      for_width,
                                      &min_height,
                                      &nat_height);

  min_height += padding.top + padding.bottom;
  nat_height += padding.top + padding.bottom;

  if (min_height_p && (*min_height_p < min_height))
    *min_height_p = min_height;
  if (nat_height_p && (*nat_height_p < nat_height))
    *nat_height_p = nat_height;
}

static void
mex_clock_bin_allocate (ClutterActor           *actor,
                        const ClutterActorBox  *box,
                        ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;
  MexClockBinPrivate *priv = MEX_CLOCK_BIN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_clock_bin_parent_class)->
    allocate (actor, box, flags);

  mx_bin_allocate_child (MX_BIN (actor), box, flags);

  mx_widget_get_available_area (MX_WIDGET (actor), box, &child_box);
  clutter_actor_allocate (priv->clock_hbox, &child_box, flags);
}

static void
mex_clock_bin_paint (ClutterActor *actor)
{
  MexClockBinPrivate *priv = MEX_CLOCK_BIN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_clock_bin_parent_class)->paint (actor);

  clutter_actor_paint (priv->clock_hbox);
}

static void
mex_clock_bin_class_init (MexClockBinClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexClockBinPrivate));

  object_class->get_property = mex_clock_bin_get_property;
  object_class->set_property = mex_clock_bin_set_property;
  object_class->dispose = mex_clock_bin_dispose;
  object_class->finalize = mex_clock_bin_finalize;

  actor_class->get_preferred_width = mex_clock_bin_get_preferred_width;
  actor_class->get_preferred_height = mex_clock_bin_get_preferred_height;
  actor_class->allocate = mex_clock_bin_allocate;
  actor_class->paint = mex_clock_bin_paint;
  actor_class->map = mex_clock_bin_map;
  actor_class->unmap = mex_clock_bin_unmap;

  pspec = g_param_spec_object ("icon",
                               "Icon",
                               "The MxIcon displayed to the "
                               "right of the clock.",
                               MX_TYPE_ICON,
                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ICON, pspec);
}

static gboolean
mex_clock_update_cb (MexClockBin *self)
{
  gchar text[100];
  struct tm *current_time;
  time_t now_t = time (NULL);
  MexClockBinPrivate *priv = self->priv;

  current_time = localtime (&now_t);

  strftime (text, 100, "%l:%M %p", current_time);

  mx_label_set_text (MX_LABEL (priv->time_label), text);

  priv->update_source = g_timeout_add_seconds (60 - current_time->tm_sec,
                                               (GSourceFunc)mex_clock_update_cb,
                                               self);

  return FALSE;
}

static void
mex_clock_bin_init (MexClockBin *self)
{
  MexClockBinPrivate *priv = self->priv = CLOCK_BIN_PRIVATE (self);

  priv->clock_hbox = mx_box_layout_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->clock_hbox),
                               "MexClockBin");
  clutter_actor_set_parent (priv->clock_hbox, CLUTTER_ACTOR (self));

  priv->time_label = mx_label_new ();
  priv->icon = mx_icon_new ();

  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (priv->clock_hbox),
                                           priv->time_label, 0,
                                           "x-fill", FALSE,
                                           "y-fill", FALSE,
                                           "x-align", MX_ALIGN_START,
                                           "expand", TRUE,
                                           NULL);
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (priv->clock_hbox),
                                           priv->icon, 1,
                                           "x-fill", FALSE,
                                           "y-fill", FALSE,
                                           NULL);

  mex_clock_update_cb (self);
}

ClutterActor *
mex_clock_bin_new (void)
{
  return g_object_new (MEX_TYPE_CLOCK_BIN, NULL);
}

MxIcon *
mex_clock_bin_get_icon (MexClockBin *bin)
{
  g_return_val_if_fail (MEX_IS_CLOCK_BIN (bin), NULL);
  return MX_ICON (bin->priv->icon);
}
