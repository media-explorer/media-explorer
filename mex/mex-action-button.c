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


#include "mex-action-button.h"
#include "mex-shadow.h"

static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexActionButton, mex_action_button, MX_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE, mx_focusable_iface_init))

#define ACTION_BUTTON_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_ACTION_BUTTON, MexActionButtonPrivate))

struct _MexActionButtonPrivate
{
  MexShadow *shadow;

  guint has_focus : 1;
};

static MxFocusable *
_move_focus (MxFocusable      *focusable,
             MxFocusDirection  direction,
             MxFocusable      *old_focus)
{
  MexActionButtonPrivate *priv = MEX_ACTION_BUTTON (focusable)->priv;
  MxFocusableIface *iface;

  g_object_unref (priv->shadow);
  priv->shadow = NULL;

  priv->has_focus = FALSE;


  iface = g_type_interface_peek_parent (MX_FOCUSABLE_GET_INTERFACE (focusable));
  return iface->move_focus (focusable, direction, old_focus);
}

static MxFocusable *
_accept_focus (MxFocusable *focusable,
               MxFocusHint  hint)
{
  MexActionButtonPrivate *priv = MEX_ACTION_BUTTON (focusable)->priv;
  MxFocusableIface *iface;
  ClutterColor shadow_color = {0, 0, 0, 64};

  priv->has_focus = TRUE;

  priv->shadow = mex_shadow_new (CLUTTER_ACTOR (focusable));
  mex_shadow_set_radius_x (priv->shadow, 15);
  mex_shadow_set_radius_y (priv->shadow, 15);
  mex_shadow_set_color (priv->shadow, &shadow_color);

  iface = g_type_interface_peek_parent (MX_FOCUSABLE_GET_INTERFACE (focusable));
  return iface->accept_focus (focusable, hint);
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = _move_focus;
  iface->accept_focus = _accept_focus;
}


static void
mex_action_button_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_action_button_set_property (GObject      *object,
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
mex_action_button_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_action_button_parent_class)->dispose (object);
}

static void
mex_action_button_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_action_button_parent_class)->finalize (object);
}

/* FIXME: This scales the background, which is an effect we want, but often it
 * ends up just causing the background to paint underneath other actors and look
 * weird. Disabling this for now, until we figure out a better way of doing
 * this.
 */
#if 0
static void
mex_action_button_paint_background (MxWidget *widget,
                                    ClutterActor *background,
                                    const ClutterColor *color)
{
  MexActionButtonPrivate *priv = MEX_ACTION_BUTTON (widget)->priv;
  gfloat width, height;
  gfloat factor_x, factor_y;

  cogl_push_matrix ();

  if (priv->has_focus)
    {
      clutter_actor_get_size (background, &width, &height);

      factor_x = (width + 4) / width;
      factor_y = (height + 4) / height;

      cogl_translate (width/2, height/2, 0);
      cogl_scale (factor_x, factor_y, 1);
      cogl_translate (-width/2, -height/2, 0);
    }

  MX_WIDGET_CLASS(mex_action_button_parent_class)->paint_background (widget,
                                                                     background,
                                                                     color);
  cogl_pop_matrix ();
}
#endif

static void
mex_action_button_class_init (MexActionButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexActionButtonPrivate));

  object_class->get_property = mex_action_button_get_property;
  object_class->set_property = mex_action_button_set_property;
  object_class->dispose = mex_action_button_dispose;
  object_class->finalize = mex_action_button_finalize;

#if 0
  {
    MxWidgetClass *widget_class = MX_WIDGET_CLASS (klass);
    widget_class->paint_background = mex_action_button_paint_background;
  }
#endif
}

static void
mex_action_button_init (MexActionButton *self)
{
  self->priv = ACTION_BUTTON_PRIVATE (self);

  mx_button_set_icon_position (MX_BUTTON (self), MX_POSITION_RIGHT);

  mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);
}

ClutterActor *
mex_action_button_new (MxAction *action)
{
  return g_object_new (MEX_TYPE_ACTION_BUTTON, "action", action, NULL);
}
