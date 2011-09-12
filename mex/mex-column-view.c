/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#include "mex-column.h"
#include "mex-column-view.h"
#include "mex-scroll-view.h"

enum
{
  PROP_0,
  PROP_LABEL,
  PROP_ICON_NAME,
  PROP_PLACEHOLDER_ACTOR
};

enum
{
  ACTIVATED,
  LAST_SIGNAL
};

struct _MexColumnViewPrivate
{
  guint         has_focus : 1;

  ClutterActor *header;
  ClutterActor *button;
  ClutterActor *icon;
  ClutterActor *label;
  ClutterActor *placeholder_actor;
  ClutterActor *scroll;
  ClutterActor *column;

  ClutterActor *current_focus;
};

static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexColumnView, mex_column_view, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define COLUMN_VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_COLUMN_VIEW, MexColumnViewPrivate))


static guint32 signals[LAST_SIGNAL] = { 0, };

/* MxFocusableIface */

static MxFocusable *
mex_column_view_move_focus (MxFocusable      *focusable,
                            MxFocusDirection  direction,
                            MxFocusable      *from)
{
  MxFocusHint hint;
  MexColumnView *self = MEX_COLUMN_VIEW (focusable);
  MexColumnViewPrivate *priv = self->priv;

  focusable = NULL;

  switch (direction)
    {
    case MX_FOCUS_DIRECTION_NEXT:
    case MX_FOCUS_DIRECTION_DOWN:
      if (((ClutterActor *) from == priv->header) &&
          !mex_column_is_empty (MEX_COLUMN (priv->column)))
        {
          hint = (direction == MX_FOCUS_DIRECTION_NEXT) ?
            MX_FOCUS_HINT_FIRST : MX_FOCUS_HINT_FROM_ABOVE;
          focusable = mx_focusable_accept_focus (MX_FOCUSABLE (priv->scroll),
                                                 hint);
          if (focusable)
            priv->current_focus = priv->scroll;
        }
      break;

    case MX_FOCUS_DIRECTION_PREVIOUS:
    case MX_FOCUS_DIRECTION_UP:
      if ((ClutterActor *) from == priv->scroll)
        {
          hint = (direction == MX_FOCUS_DIRECTION_NEXT) ?
            MX_FOCUS_HINT_FIRST : MX_FOCUS_HINT_FROM_ABOVE;
          focusable = mx_focusable_accept_focus (MX_FOCUSABLE (priv->header),
                                                 hint);
          if (focusable)
            priv->current_focus = priv->header;
        }
      break;

    default:
      break;
    }

  return focusable;
}

static MxFocusable *
mex_column_view_accept_focus (MxFocusable *focusable,
                              MxFocusHint  hint)
{
  MexColumnView *self = MEX_COLUMN_VIEW (focusable);
  MexColumnViewPrivate *priv = self->priv;

  focusable = NULL;

  switch (hint)
    {
    case MX_FOCUS_HINT_FROM_LEFT:
    case MX_FOCUS_HINT_FROM_RIGHT:
    case MX_FOCUS_HINT_PRIOR:
      if (priv->current_focus &&
          (focusable = mx_focusable_accept_focus (
             MX_FOCUSABLE (priv->current_focus), hint)))
        break;
      /* If there's no prior focus, or the prior focus rejects focus,
       * try to just focus the first actor.
       */

    case MX_FOCUS_HINT_FIRST:
    case MX_FOCUS_HINT_FROM_ABOVE:
      if ((focusable =
           mx_focusable_accept_focus (MX_FOCUSABLE (priv->header), hint)))
        priv->current_focus = priv->header;
      else if (!mex_column_is_empty (MEX_COLUMN (priv->column)) &&
               (focusable =
                mx_focusable_accept_focus (MX_FOCUSABLE (priv->column),
                                           hint)))
        priv->current_focus = priv->column;
      break;

    case MX_FOCUS_HINT_LAST:
    case MX_FOCUS_HINT_FROM_BELOW:
      if (!mex_column_is_empty (MEX_COLUMN (priv->column))
          && (focusable =
              mx_focusable_accept_focus (MX_FOCUSABLE (priv->column), hint)))
        priv->current_focus = priv->column;
      else if ((focusable =
                mx_focusable_accept_focus (MX_FOCUSABLE (priv->header), hint)))
        priv->current_focus = priv->header;
      break;
    }

  return focusable;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_column_view_move_focus;
  iface->accept_focus = mex_column_view_accept_focus;
}

/* MxStylableIface */

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
/*      GParamSpec *pspec;*/

      is_initialized = TRUE;

/*      pspec = g_param_spec_int ("x-mx-overlap",
                                "Overlap",
                                "Overlap between buttons.",
                                0, G_MAXINT, 0,
                                G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_EXPANDER_BOX, pspec);*/
    }
}

/* Object implementation */

static void
mex_column_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MexColumnView *self = MEX_COLUMN_VIEW (object);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, mex_column_view_get_label (self));
      break;

    case PROP_PLACEHOLDER_ACTOR:
      g_value_set_object (value, mex_column_view_get_placeholder_actor (self));
      break;

    case PROP_ICON_NAME:
      g_value_set_string (value, mex_column_view_get_icon_name (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_column_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MexColumnView *self = MEX_COLUMN_VIEW (object);

  switch (property_id)
    {
    case PROP_LABEL:
      mex_column_view_set_label (self, g_value_get_string (value));
      break;

    case PROP_PLACEHOLDER_ACTOR:
      mex_column_view_set_placeholder_actor (self, g_value_get_object (value));
      break;

    case PROP_ICON_NAME:
      mex_column_view_set_icon_name (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_column_view_dispose (GObject *object)
{
  MexColumnViewPrivate *priv = MEX_COLUMN_VIEW (object)->priv;

  if (priv->header)
    {
      /* The header includes the label and icon */
      clutter_actor_destroy (priv->header);
      priv->header = NULL;
    }

  if (priv->placeholder_actor)
    {
      clutter_actor_unparent (priv->placeholder_actor);
      priv->placeholder_actor = NULL;
    }

  G_OBJECT_CLASS (mex_column_view_parent_class)->dispose (object);
}

static void
mex_column_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_column_view_parent_class)->finalize (object);
}

static void
mex_column_view_get_preferred_width (ClutterActor *actor,
                                     gfloat        for_height,
                                     gfloat       *min_width_p,
                                     gfloat       *nat_width_p)
{
  MxPadding padding;
  gfloat min_width, nat_width;
  gfloat min_header, nat_header;
  gfloat min_placeholder, nat_placeholder;
  gfloat height;

  MexColumnView *self = MEX_COLUMN_VIEW (actor);
  MexColumnViewPrivate *priv = self->priv;

  clutter_actor_get_preferred_width (priv->header,
                                     -1,
                                     &min_header,
                                     &nat_header);

  clutter_actor_get_preferred_height (priv->header, -1, NULL, &height);
  for_height = MAX (0, for_height - height);

  if (mex_column_is_empty (MEX_COLUMN (priv->column)))
    {
      if (priv->placeholder_actor)
        {
          clutter_actor_get_preferred_width (priv->placeholder_actor,
                                             for_height,
                                             &min_placeholder,
                                             &nat_placeholder);

          min_width = MAX (min_header, min_placeholder);
          nat_width = MAX (min_header, nat_placeholder);
        }
      else
        {
          min_width = min_header;
          nat_width = nat_header;
        }
    }
  else
    {
      clutter_actor_get_preferred_width (priv->scroll,
                                         for_height,
                                         &min_placeholder,
                                         &nat_placeholder);

      min_width = MAX (min_header, min_placeholder);
      nat_width = MAX (min_header, nat_placeholder);
    }

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_width_p)
    *min_width_p = min_width + padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p = nat_width + padding.left + padding.right;
}

static void
mex_column_view_get_preferred_height (ClutterActor *actor,
                                      gfloat        for_width,
                                      gfloat       *min_height_p,
                                      gfloat       *nat_height_p)
{
  gfloat min_height, nat_height;
  gfloat min_placeholder, nat_placeholder;
  MxPadding padding;

  MexColumnView *self = MEX_COLUMN_VIEW (actor);
  MexColumnViewPrivate *priv = self->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (for_width >= 0)
    for_width = MAX (0, for_width - padding.left - padding.right);

  clutter_actor_get_preferred_height (priv->header,
                                      for_width,
                                      NULL,
                                      &min_height);
  nat_height = min_height;

  if (mex_column_is_empty (MEX_COLUMN (priv->column)))
    {
      if (priv->placeholder_actor)
        {
          clutter_actor_get_preferred_height (priv->placeholder_actor,
                                              for_width,
                                              &min_placeholder,
                                              &nat_placeholder);

          min_height += min_placeholder;
          nat_height += nat_placeholder;
        }
    }
  else
    {
      clutter_actor_get_preferred_height (priv->scroll,
                                          for_width,
                                          &min_placeholder,
                                          &nat_placeholder);

      min_height += min_placeholder;
      nat_height += nat_placeholder;
    }

  if (min_height_p)
    *min_height_p = min_height + padding.top + padding.bottom;
  if (nat_height_p)
    *nat_height_p += nat_height + padding.top + padding.bottom;
}

static void
mex_column_view_allocate (ClutterActor          *actor,
                          const ClutterActorBox *box,
                          ClutterAllocationFlags flags)
{
  gfloat header_pref_height, pref_h, pref_w;
  ClutterActorBox child_box;
  MxPadding padding;

  MexColumnView *column = MEX_COLUMN_VIEW (actor);
  MexColumnViewPrivate *priv = column->priv;

  CLUTTER_ACTOR_CLASS (mex_column_view_parent_class)->allocate (actor,
                                                                box,
                                                                flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  /* Allocate header */
  child_box.x1 = padding.left;
  child_box.x2 = box->x2 - box->x1 - padding.right;
  child_box.y1 = padding.top;

  clutter_actor_get_preferred_height (priv->header,
                                      child_box.x2 - child_box.x1,
                                      NULL,
                                      &header_pref_height);

  child_box.y2 = child_box.y1 + header_pref_height;

  clutter_actor_allocate (priv->header, &child_box, flags);

  /* Allocate placeholder/column actor */
  child_box.y1 = padding.top + header_pref_height;

  if (mex_column_is_empty (MEX_COLUMN (priv->column)) &&
      priv->placeholder_actor)
    {
      /* keep the aspect ratio of the placeholder actor */
      clutter_actor_get_preferred_size (priv->placeholder_actor, NULL, NULL,
                                        &pref_w, &pref_h);
      pref_h = pref_h * ((child_box.x2 - child_box.x1) / pref_w);
      child_box.y2 = child_box.y1 + pref_h;

      clutter_actor_allocate (priv->placeholder_actor, &child_box, flags);
    }
  else
    {
      child_box.y2 = box->y2 - box->y1 - padding.bottom;
      clutter_actor_allocate (priv->scroll, &child_box, flags);
    }
}

static void
mex_column_view_paint (ClutterActor *actor)
{
  MexColumnView *self = MEX_COLUMN_VIEW (actor);
  MexColumnViewPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_column_view_parent_class)->paint (actor);

  if (mex_column_is_empty (MEX_COLUMN (priv->column)))
    {
      if (priv->placeholder_actor)
        clutter_actor_paint (priv->placeholder_actor);
    }
  else
    clutter_actor_paint (priv->scroll);

  clutter_actor_paint (priv->header);
}

static void
mex_column_view_pick (ClutterActor *actor, const ClutterColor *color)
{
  MexColumnView *self = MEX_COLUMN_VIEW (actor);
  MexColumnViewPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_column_view_parent_class)->pick (actor, color);

  /* Don't pick children when we don't have focus */
  if (!priv->has_focus)
    return;

  if (mex_column_is_empty (MEX_COLUMN (priv->column)))
    {
      if (priv->placeholder_actor)
        clutter_actor_paint (priv->placeholder_actor);
    }
  else
    clutter_actor_paint (priv->scroll);

  clutter_actor_paint (priv->header);
}

static gboolean
mex_column_button_release_event (ClutterActor       *actor,
                                 ClutterButtonEvent *event)
{
  gboolean returnval;
  MexColumnViewPrivate *priv = MEX_COLUMN_VIEW (actor)->priv;

  returnval = CLUTTER_ACTOR_CLASS (mex_column_view_parent_class)->
    button_release_event (actor, event);

  if (!returnval && !priv->has_focus)
    {
      mex_push_focus (MX_FOCUSABLE (actor));
      return TRUE;
    }

  return returnval;
}

static void
mex_column_view_class_init (MexColumnViewClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *o_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *a_class = CLUTTER_ACTOR_CLASS (klass);

  o_class->get_property = mex_column_view_get_property;
  o_class->set_property = mex_column_view_set_property;
  o_class->dispose      = mex_column_view_dispose;
  o_class->finalize     = mex_column_view_finalize;

  a_class->get_preferred_width  = mex_column_view_get_preferred_width;
  a_class->get_preferred_height = mex_column_view_get_preferred_height;
  a_class->allocate             = mex_column_view_allocate;
  a_class->paint                = mex_column_view_paint;
  a_class->pick                 = mex_column_view_pick;
  a_class->button_release_event = mex_column_button_release_event;

  g_type_class_add_private (klass, sizeof (MexColumnViewPrivate));

  pspec = g_param_spec_string ("label",
                               "Label",
                               "Text used as the title for this column.",
                               "",
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (o_class, PROP_LABEL, pspec);

  pspec = g_param_spec_object ("placeholder-actor",
                               "Placeholder Actor",
                               "Actor used when this column is empty.",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_PLACEHOLDER_ACTOR, pspec);

  pspec = g_param_spec_string ("icon-name",
                               "Icon Name",
                               "Icon name used by the icon for this column.",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (o_class, PROP_ICON_NAME, pspec);

  signals[ACTIVATED] = g_signal_new ("activated",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_LAST,
                                     G_STRUCT_OFFSET (MexColumnClass,
                                                      activated),
                                     NULL, NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE, 0);
}

static void
mex_column_header_clicked_cb (MxButton *button, MexColumn *self)
{
  g_signal_emit (self, signals[ACTIVATED], 0);
}

static void
mex_column_view_opened_cb (MexColumn     *column,
                           GParamSpec    *pspec,
                           MexColumnView *column_view)
{
  MexColumnViewPrivate *priv = column_view->priv;

  if (mex_column_get_opened (column))
    clutter_actor_animate (priv->header,
                           CLUTTER_EASE_IN_OUT_QUAD, 200,
                           "opacity", 56, NULL);
  else
    clutter_actor_animate (priv->header,
                           CLUTTER_EASE_IN_OUT_QUAD, 200,
                           "opacity", 255, NULL);
}

static void
mex_column_view_init (MexColumnView *self)
{
  ClutterActor *box;
  MexColumnViewPrivate *priv = self->priv = COLUMN_VIEW_PRIVATE (self);

  /* Begin private children */
  clutter_actor_push_internal (CLUTTER_ACTOR (self));

  /* Create the header */
  priv->header = mx_box_layout_new ();
  mx_box_layout_set_orientation ((MxBoxLayout *) priv->header,
                                 MX_ORIENTATION_HORIZONTAL);

  clutter_actor_push_internal (CLUTTER_ACTOR (self));
  clutter_actor_set_parent (priv->header, CLUTTER_ACTOR (self));
  clutter_actor_pop_internal (CLUTTER_ACTOR (self));

  priv->button = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->button), "Header");
  priv->icon = mx_icon_new ();
  priv->label = mx_label_new ();
  g_object_set (priv->label, "clip-to-allocation", TRUE, "fade-out", TRUE,
                NULL);

  box = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (box), 8);
  clutter_container_add (CLUTTER_CONTAINER (box),
                         priv->icon,
                         priv->label,
                         NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->icon,
                               "expand", FALSE,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", FALSE,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->label,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "x-align", MX_ALIGN_START,
                               "y-fill", FALSE,
                               NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button), box);

  mx_bin_set_fill (MX_BIN (priv->button), TRUE, FALSE);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->header), priv->button);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->header), priv->button,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "x-align", MX_ALIGN_START,
                               "y-fill", TRUE,
                               NULL);

  g_signal_connect (priv->button, "clicked",
                    G_CALLBACK (mex_column_header_clicked_cb), self);


  priv->scroll = mex_scroll_view_new ();
  mx_kinetic_scroll_view_set_scroll_policy (MX_KINETIC_SCROLL_VIEW (priv->scroll),
                                            MX_SCROLL_POLICY_VERTICAL);
  mex_scroll_view_set_indicators_hidden (MEX_SCROLL_VIEW (priv->scroll),
                                         TRUE);
  clutter_actor_set_parent (priv->scroll, CLUTTER_ACTOR (self));

  priv->column = mex_column_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->scroll),
                               priv->column);

  g_signal_connect (priv->column, "notify::opened",
                    G_CALLBACK (mex_column_view_opened_cb), self);

  /* End of private children */
  clutter_actor_pop_internal (CLUTTER_ACTOR (self));

  /* Set the column as reactive and enable collapsing */
  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);
}

ClutterActor *
mex_column_view_new (const gchar *label,
                     const gchar *icon_name)
{
  return g_object_new (MEX_TYPE_COLUMN_VIEW,
                       "label", label,
                       "icon-name", icon_name,
                       NULL);
}

MexColumn *
mex_column_view_get_column (MexColumnView *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN_VIEW (column), NULL);
  return MEX_COLUMN (column->priv->column);
}

const gchar *
mex_column_view_get_label (MexColumnView *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN_VIEW (column), NULL);
  return mx_label_get_text (MX_LABEL (column->priv->label));
}

void
mex_column_view_set_label (MexColumnView *column, const gchar *label)
{
  g_return_if_fail (MEX_IS_COLUMN_VIEW (column));
  mx_label_set_text (MX_LABEL (column->priv->label), label ? label : "");
}

ClutterActor *
mex_column_view_get_placeholder_actor (MexColumnView *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN_VIEW (column), NULL);

  return column->priv->placeholder_actor;
}

void
mex_column_view_set_placeholder_actor (MexColumnView *column,
                                       ClutterActor  *actor)
{
  MexColumnViewPrivate *priv;

  g_return_if_fail (MEX_IS_COLUMN_VIEW (column));
  g_return_if_fail (actor == NULL || CLUTTER_IS_ACTOR (actor));

  priv = column->priv;

  /* placeholder label */
  if (priv->placeholder_actor)
    clutter_actor_unparent (priv->placeholder_actor);

  priv->placeholder_actor = actor;

  if (actor)
    {
      clutter_actor_push_internal (CLUTTER_ACTOR (column));
      clutter_actor_set_parent (priv->placeholder_actor, CLUTTER_ACTOR (column));
      clutter_actor_pop_internal (CLUTTER_ACTOR (column));
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (column));
}


const gchar *
mex_column_view_get_icon_name (MexColumnView *column)
{
  g_return_val_if_fail (MEX_IS_COLUMN_VIEW (column), NULL);
  return mx_icon_get_icon_name (MX_ICON (column->priv->icon));
}

void
mex_column_view_set_icon_name (MexColumnView *column, const gchar *name)
{
  g_return_if_fail (MEX_IS_COLUMN_VIEW (column));
  mx_icon_set_icon_name (MX_ICON (column->priv->icon), name);
}

void
mex_column_view_set_focus (MexColumnView *column, gboolean focus)
{
  MexColumnViewPrivate *priv;

  g_return_if_fail (MEX_IS_COLUMN_VIEW (column));

  priv = column->priv;
  priv->has_focus = focus;

  mex_column_set_focus (MEX_COLUMN (priv->column), focus);
}
