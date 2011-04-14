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


#include "mex-shell.h"

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexShell, mex_shell, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define SHELL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SHELL, MexShellPrivate))

enum
{
  PROP_0,

  PROP_PRESENTED
};

enum
{
  SIGNAL_TRANSITION_STARTED,
  SIGNAL_TRANSITION_COMPLETED,

  LAST_SIGNAL
};

typedef struct
{
  ClutterActor      *child;
  ClutterAnimator   *animator;
  gboolean           start_animator;
  MexShellDirection  last_direction;
  gfloat             target_width;
  gfloat             target_height;
} MexShellChildData;

struct _MexShellPrivate
{
  GList        *children;
  ClutterActor *presented;
  gint          animating;
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
mex_shell_timeline_started_cb (ClutterTimeline *timeline,
                               MexShell        *self)
{
  MexShellPrivate *priv = self->priv;

  priv->animating ++;

  if (priv->animating == 1)
    g_signal_emit (self, signals[SIGNAL_TRANSITION_STARTED], 0);
}

static void
mex_shell_timeline_completed_cb (ClutterTimeline *timeline,
                               MexShell        *self)
{
  MexShellPrivate *priv = self->priv;

  priv->animating --;

  if (priv->animating == 0)
    g_signal_emit (self, signals[SIGNAL_TRANSITION_COMPLETED], 0);
}

static MexShellChildData *
mex_shell_child_data_new (MexShell *self, ClutterActor *child)
{
  ClutterTimeline *timeline;

  MexShellChildData *data = g_slice_new0 (MexShellChildData);
  data->child = child;
  data->animator = clutter_animator_new ();
  data->last_direction = MEX_SHELL_DIRECTION_RIGHT;

  clutter_animator_set_duration (data->animator, 250);

  timeline = clutter_animator_get_timeline (data->animator);
  g_signal_connect (timeline, "started",
                    G_CALLBACK (mex_shell_timeline_started_cb), self);
  g_signal_connect (timeline, "completed",
                    G_CALLBACK (mex_shell_timeline_completed_cb), self);

  return data;
}

static void
mex_shell_child_data_free (MexShell *self, MexShellChildData *data)
{
  ClutterTimeline *timeline = clutter_animator_get_timeline (data->animator);

  if (clutter_timeline_is_playing (timeline))
    mex_shell_timeline_completed_cb (timeline, self);

  clutter_animator_remove_key (data->animator, NULL, NULL, -1);
  g_object_unref (data->animator);
  g_slice_free (MexShellChildData, data);
}

static gint
mex_shell_find_child (gconstpointer a,
                      gconstpointer b)
{
  const MexShellChildData *data = a;
  const ClutterActor *child = b;

  if (data->child == child)
    return 0;
  else
    return -1;
}

/* ClutterContainerIface */

static void
mex_shell_add (ClutterContainer *container,
               ClutterActor     *actor)
{
  ClutterActorBox box;
  MexShellChildData *data;

  MexShell *self = MEX_SHELL (container);
  MexShellPrivate *priv = self->priv;

  data = mex_shell_child_data_new (self, actor);
  priv->children = g_list_append (priv->children, data);
  clutter_actor_set_parent (actor, CLUTTER_ACTOR (container));
  clutter_actor_set_opacity (actor, 0x00);

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (container), &box);
  data->target_width = box.x2 - box.x1;
  data->target_height = box.y2 - box.y1;
  g_object_set (G_OBJECT (actor),
                "natural-width", data->target_width,
                "natural-height", data->target_height,
                NULL);

  g_signal_emit_by_name (container, "actor-added", actor);
}

static void
mex_shell_remove (ClutterContainer *container,
                  ClutterActor     *actor)
{
  GList *l;
  MexShellChildData *data;

  MexShell *self = MEX_SHELL (container);
  MexShellPrivate *priv = self->priv;

  l = g_list_find_custom (priv->children, actor, mex_shell_find_child);
  if (!l)
    {
      g_warning (G_STRLOC ": Attempted to remove an unknown child.");
      return;
    }

  if (actor == priv->presented)
    priv->presented = NULL;

  data = l->data;
  mex_shell_child_data_free (self, data);
  priv->children = g_list_delete_link (priv->children, l);

  g_object_ref (actor);
  clutter_actor_unparent (actor);

  g_signal_emit_by_name (container, "actor-removed", actor);

  g_object_unref (actor);
}

static void
mex_shell_foreach (ClutterContainer *container,
                   ClutterCallback   callback,
                   gpointer          user_data)
{
  GList *d;
  MexShellPrivate *priv = MEX_SHELL (container)->priv;

  /* Iterate in this way to protect against node removal */
  d = priv->children;
  while (d)
    {
      MexShellChildData *data = d->data;
      d = d->next;
      callback (data->child, user_data);
    }
}

static void
mex_shell_raise (ClutterContainer *container,
                 ClutterActor     *actor,
                 ClutterActor     *sibling)
{
  gint i;
  GList *d, *actor_link = NULL;
  MexShellChildData *actor_data;

  gint position = -1;
  MexShellPrivate *priv = MEX_SHELL (container)->priv;

  for (d = priv->children, i = 0; d; d = d->next, i++)
    {
      MexShellChildData *data = d->data;

      if (data->child == actor)
        actor_link = d;
      if (data->child == sibling)
        position = i;
    }

  if (!actor_link)
    {
      g_warning (G_STRLOC ": Attempted to raise an unknown child");
      return;
    }

  if (!actor_link->next)
    return;

  actor_data = actor_link->data;
  priv->children = g_list_delete_link (priv->children, actor_link);
  priv->children = g_list_insert (priv->children, actor_data, position);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (container));
}

static void
mex_shell_lower (ClutterContainer *container,
                 ClutterActor     *actor,
                 ClutterActor     *sibling)
{
  gint i;
  GList *d, *position, *actor_link = NULL;
  MexShellChildData *actor_data;

  MexShellPrivate *priv = MEX_SHELL (container)->priv;

  if (priv->children && (priv->children->data == actor))
    return;

  position = priv->children;
  for (d = priv->children, i = 0; d; d = d->next, i++)
    {
      MexShellChildData *data = d->data;

      if (data->child == actor)
        actor_link = d;
      if (data->child == sibling)
        position = d;
    }

  if (!actor_link)
    {
      g_warning (G_STRLOC ": Attempted to lower an unknown child");
      return;
    }

  actor_data = actor_link->data;
  priv->children = g_list_delete_link (priv->children, actor_link);
  priv->children = g_list_insert_before (priv->children, position, actor_data);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (container));
}

static gint
mex_shell_sort_depth_order_cb (gconstpointer a,
                               gconstpointer b)
{
  gfloat depth_a, depth_b;

  const MexShellChildData *data_a = a;
  const MexShellChildData *data_b = b;

  depth_a = clutter_actor_get_depth (data_a->child);
  depth_b = clutter_actor_get_depth (data_b->child);

  if (depth_a < depth_b)
    return -1;
  else if (depth_a > depth_b)
    return 1;
  else
    return 0;
}

static void
mex_shell_sort_depth_order (ClutterContainer *container)
{
  MexShellPrivate *priv = MEX_SHELL (container)->priv;
  priv->children = g_list_sort (priv->children, mex_shell_sort_depth_order_cb);
  clutter_actor_queue_redraw (CLUTTER_ACTOR (container));
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = mex_shell_add;
  iface->remove = mex_shell_remove;
  iface->foreach = mex_shell_foreach;

  iface->raise = mex_shell_raise;
  iface->lower = mex_shell_lower;
  iface->sort_depth_order = mex_shell_sort_depth_order;
}

/* MxFocusableIface */

static MxFocusable *
mex_shell_move_focus (MxFocusable      *focusable,
                      MxFocusDirection  direction,
                      MxFocusable      *from)
{
  return NULL;
}

static MxFocusable *
mex_shell_accept_focus (MxFocusable *focusable, MxFocusHint hint)
{
  MexShellPrivate *priv = MEX_SHELL (focusable)->priv;

  if (priv->presented)
    return mx_focusable_accept_focus (MX_FOCUSABLE (priv->presented), hint);
  else
    return NULL;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_shell_move_focus;
  iface->accept_focus = mex_shell_accept_focus;
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
      mx_stylable_iface_install_property (iface, MEX_TYPE_SHELL, pspec);*/
    }
}

/* Actor implementation */

static void
mex_shell_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_PRESENTED:
      g_value_set_object (value, mex_shell_get_presented (MEX_SHELL (object)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_shell_set_property (GObject      *object,
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
mex_shell_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_shell_parent_class)->dispose (object);
}

static void
mex_shell_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_shell_parent_class)->finalize (object);
}

static void
mex_shell_destroy (ClutterActor *actor)
{
  GList *children = clutter_container_get_children (CLUTTER_CONTAINER (actor));

  g_list_foreach (children, (GFunc)clutter_actor_destroy, NULL);
  g_list_free (children);

  if (CLUTTER_ACTOR_CLASS (mex_shell_parent_class)->destroy)
    CLUTTER_ACTOR_CLASS (mex_shell_parent_class)->destroy (actor);
}

static void
mex_shell_get_preferred_width (ClutterActor *actor,
                               gfloat        for_height,
                               gfloat       *min_width_p,
                               gfloat       *nat_width_p)
{
  GList *c;
  gfloat min_width;
  MxPadding padding;

  MexShellPrivate *priv = MEX_SHELL (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  for_height -= padding.top + padding.bottom;

  min_width = 0;
  for (c = priv->children; c; c = c->next)
    {
      gfloat child_min_width;

      MexShellChildData *data = c->data;
      ClutterActor *child = data->child;

      clutter_actor_get_preferred_width (child,
                                         for_height,
                                         &child_min_width,
                                         NULL);

      if (child_min_width > min_width)
        min_width = child_min_width;
    }

  min_width += padding.left + padding.right;

  if (min_width_p)
    *min_width_p = min_width;
  if (nat_width_p)
    *nat_width_p = min_width;
}

static void
mex_shell_get_preferred_height (ClutterActor *actor,
                                gfloat        for_width,
                                gfloat       *min_height_p,
                                gfloat       *nat_height_p)
{
  GList *c;
  gfloat min_height;
  MxPadding padding;

  MexShellPrivate *priv = MEX_SHELL (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  for_width -= padding.left + padding.right;

  min_height = 0;
  for (c = priv->children; c; c = c->next)
    {
      gfloat child_min_height;

      MexShellChildData *data = c->data;
      ClutterActor *child = data->child;

      clutter_actor_get_preferred_height (child,
                                          for_width,
                                          &child_min_height,
                                          NULL);

      if (child_min_height > min_height)
        min_height = child_min_height;
    }

  min_height += padding.top + padding.bottom;

  if (min_height_p)
    *min_height_p = min_height;
  if (nat_height_p)
    *nat_height_p = min_height;
}

static void
mex_shell_allocate (ClutterActor           *actor,
                    const ClutterActorBox  *box,
                    ClutterAllocationFlags  flags)
{
  GList *c;
  MxPadding padding;
  gfloat width, height;

  MexShellPrivate *priv = MEX_SHELL (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_shell_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  width = clutter_actor_box_get_width (box) - padding.left - padding.right;
  height = clutter_actor_box_get_height (box) - padding.top - padding.bottom;

  for (c = priv->children; c; c = c->next)
    {
      MexShellChildData *data = c->data;

      /* By animating this, we avoid an allocation cycle
       * and it also looks kinda cool.
       */
      clutter_actor_animate (data->child,
                             CLUTTER_EASE_OUT_QUAD, 250,
                             "natural-width", width,
                             "natural-height", height,
                             NULL);
      data->target_width = width;
      data->target_height = height;

      clutter_actor_allocate_preferred_size (data->child, flags);
    }
}

static void
mex_shell_paint (ClutterActor *actor)
{
  GList *c;
  MexShellPrivate *priv = MEX_SHELL (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_shell_parent_class)->paint (actor);

  for (c = priv->children; c; c = c->next)
    {
      gboolean do_paint;
      MexShellChildData *data = c->data;

      if (data->child == priv->presented)
        do_paint = TRUE;
      else if (data->start_animator)
        do_paint = TRUE;
      else if (clutter_timeline_is_playing (
                 clutter_animator_get_timeline (data->animator)))
        do_paint = TRUE;
      else
        do_paint = FALSE;

      if (do_paint)
        clutter_actor_paint (((MexShellChildData *)c->data)->child);

      if (data->start_animator)
        {
          clutter_animator_start (data->animator);
          data->start_animator = FALSE;
        }
    }
}

static void
mex_shell_pick (ClutterActor *actor, const ClutterColor *color)
{
  MexShellPrivate *priv = MEX_SHELL (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_shell_parent_class)->pick (actor, color);

  if (priv->presented)
    clutter_actor_paint (priv->presented);
}

static void
mex_shell_class_init (MexShellClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexShellPrivate));

  object_class->get_property = mex_shell_get_property;
  object_class->set_property = mex_shell_set_property;
  object_class->dispose = mex_shell_dispose;
  object_class->finalize = mex_shell_finalize;

  actor_class->destroy = mex_shell_destroy;
  actor_class->get_preferred_width = mex_shell_get_preferred_width;
  actor_class->get_preferred_height = mex_shell_get_preferred_height;
  actor_class->allocate = mex_shell_allocate;
  actor_class->paint = mex_shell_paint;
  actor_class->pick = mex_shell_pick;

  pspec = g_param_spec_object ("presented",
                               "Presented",
                               "The last presented actor",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PRESENTED, pspec);

  signals[SIGNAL_TRANSITION_STARTED] =
    g_signal_new ("transition-started",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexShellClass, transition_started),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[SIGNAL_TRANSITION_COMPLETED] =
    g_signal_new ("transition-completed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexShellClass, transition_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mex_shell_init (MexShell *self)
{
  self->priv = SHELL_PRIVATE (self);
}

ClutterActor *
mex_shell_new (void)
{
  return g_object_new (MEX_TYPE_SHELL, NULL);
}

ClutterActor *
mex_shell_get_presented (MexShell *shell)
{
  g_return_val_if_fail (MEX_IS_SHELL (shell), NULL);
  return shell->priv->presented;
}

void
mex_shell_present (MexShell          *shell,
                   ClutterActor      *actor,
                   MexShellDirection  in)
{
  GList *l;
  ClutterActor *stage;
  ClutterActorBox box;
  MexShellPrivate *priv;
  MexShellChildData *data;
  gfloat x, y, width, height;

  g_return_if_fail (MEX_IS_SHELL (shell));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  priv = shell->priv;
  l = g_list_find_custom (priv->children, actor, mex_shell_find_child);
  if (!l)
    {
      g_warning (G_STRLOC ": Attempted to present an unknown child.");
      return;
    }
  data = l->data;

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (shell), &box);
  width = box.x2 - box.x1;
  height = box.y2 - box.y1;
  x = 0;
  y = 0;

  if (clutter_actor_get_opacity (data->child) == 0x00)
    {
      switch (in)
        {
        case MEX_SHELL_DIRECTION_NONE:
          clutter_actor_set_position (data->child, 0, 0);
          break;

        case MEX_SHELL_DIRECTION_TOP:
          clutter_actor_set_position (data->child, x, -height);
          break;

        case MEX_SHELL_DIRECTION_RIGHT:
          clutter_actor_set_position (data->child, width, y);
          break;

        default:
        case MEX_SHELL_DIRECTION_BOTTOM:
          clutter_actor_set_position (data->child, x, height);
          break;

        case MEX_SHELL_DIRECTION_LEFT:
          clutter_actor_set_position (data->child, -width, y);
          break;
        }

      data->last_direction = in;
    }

  clutter_animator_remove_key (data->animator,
                               (GObject *)data->child, NULL, -1);
  clutter_animator_set (data->animator,
                data->child, "x", CLUTTER_LINEAR, 0.0, 0.f,
                data->child, "x", CLUTTER_EASE_OUT_QUAD, 1.0, x,
                data->child, "y", CLUTTER_LINEAR, 0.0, 0.f,
                data->child, "y", CLUTTER_EASE_OUT_QUAD, 1.0, y,
                data->child, "opacity", CLUTTER_LINEAR, 0.0, 0x00,
                data->child, "opacity", CLUTTER_EASE_OUT_QUAD, 0.1, 0xff,
                NULL);
  clutter_animator_property_set_ease_in (data->animator, (GObject *)data->child,
                                         "x", TRUE);
  clutter_animator_property_set_ease_in (data->animator, (GObject *)data->child,
                                         "y", TRUE);
  clutter_animator_property_set_ease_in (data->animator, (GObject *)data->child,
                                         "opacity", TRUE);
  data->start_animator = TRUE;

  if (priv->presented && (priv->presented != actor))
    {
      MexShellDirection dir;

      l = g_list_find_custom (priv->children,
                                 priv->presented,
                                 mex_shell_find_child);
      if (!l)
        {
          g_warning (G_STRLOC ": Former presented actor is unknown");
          return;
        }
      data = l->data;

      x = y = 0;

      switch (in)
        {
        case 0:
          x = y = 0;
          dir = 0;
          break;

        case MEX_SHELL_DIRECTION_TOP:
          y = height;
          dir = MEX_SHELL_DIRECTION_BOTTOM;
          break;

        case MEX_SHELL_DIRECTION_RIGHT:
          x = -width;
          dir = MEX_SHELL_DIRECTION_LEFT;
          break;

        case MEX_SHELL_DIRECTION_BOTTOM:
          y = -height;
          dir = MEX_SHELL_DIRECTION_TOP;
          break;

        default:
        case MEX_SHELL_DIRECTION_LEFT:
          x = width;
          dir = MEX_SHELL_DIRECTION_RIGHT;
          break;
        }

      data->last_direction = dir;
      clutter_animator_remove_key (data->animator,
                                   (GObject *)data->child, NULL, -1);
      clutter_animator_set (data->animator,
                data->child, "x", CLUTTER_LINEAR, 0.0, 0.f,
                data->child, "x", CLUTTER_EASE_OUT_QUAD, 1.0, x,
                data->child, "y", CLUTTER_LINEAR, 0.0, 0.f,
                data->child, "y", CLUTTER_EASE_OUT_QUAD, 1.0, y,
                data->child, "opacity", CLUTTER_LINEAR, 0.9, 0xff,
                data->child, "opacity", CLUTTER_EASE_OUT_QUAD, 1.0, 0x00,
                NULL);
      clutter_animator_property_set_ease_in (data->animator,
                                             (GObject *)data->child,
                                             "x", TRUE);
      clutter_animator_property_set_ease_in (data->animator,
                                             (GObject *)data->child,
                                             "y", TRUE);
      clutter_animator_property_set_ease_in (data->animator,
                                             (GObject *)data->child,
                                             "opacity", TRUE);
      data->start_animator = TRUE;
    }

  priv->presented = actor;
  stage = clutter_actor_get_stage (actor);
  if (stage)
    {
      MxFocusManager *manager =
        mx_focus_manager_get_for_stage ((ClutterStage *)stage);

      if (manager)
        {
          if (MX_IS_FOCUSABLE (actor))
            mx_focus_manager_push_focus_with_hint (manager,
                                                   MX_FOCUSABLE (shell),
                                                   MX_FOCUS_HINT_PRIOR);
          else
            mx_focus_manager_move_focus (manager, MX_FOCUS_DIRECTION_OUT);
        }
    }

  clutter_actor_queue_redraw (CLUTTER_ACTOR (shell));
}
