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


#include "mex-resizing-hbox.h"
#include "mex-resizing-hbox-child.h"
#include "mex-scrollable-container.h"
#include "mex-utils.h"
#include "mex-scene.h"
#include "mex-column.h"
#include "mex-scroll-view.h"
#include <math.h>

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void mx_scrollable_iface_init (MxScrollableIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);
static void mex_scrollable_container_iface_init (MexScrollableContainerInterface *iface);
static void mex_scene_iface_init (MexSceneInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexResizingHBox, mex_resizing_hbox, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_SCROLLABLE,
                                                mx_scrollable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_SCROLLABLE_CONTAINER,
                                                mex_scrollable_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_SCENE,
                                                mex_scene_iface_init))

#define RESIZING_HBOX_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_RESIZING_HBOX, MexResizingHBoxPrivate))

#define INACTIVE_OPACITY 64
#define ANIMATION_DURATION 500

enum
{
  PROP_0,

  PROP_RESIZING_ENABLED,
  PROP_HADJUST,
  PROP_VADJUST,
  PROP_HDEPTH_SCALE,
  PROP_VDEPTH_SCALE,
  PROP_DEPTH_FADE,
  PROP_DEPTH_INDEX,
  PROP_MAX_DEPTH
};

struct _MexResizingHBoxPrivate
{
  guint            has_focus        : 1;
  guint            resizing_enabled : 1;
  guint            fade             : 1;
  guint            in_dispose       : 1;

  ClutterActor    *current_focus;
  GList           *children;

  ClutterAlpha    *alpha;
  ClutterTimeline *timeline;
  guint            anim_length;
  guint            stagger_length;
  gfloat           hdepth;
  gfloat           vdepth;
  gint             depth_index;
  gint             max_depth;
  gfloat           prev_offset;
  gfloat           prev_width;

  MxAdjustment    *hadjust;

  CoglHandle       highlight;
  CoglHandle       shadow;
  CoglHandle       border;
  CoglHandle       highlight_material;
  CoglHandle       shadow_material;
  CoglHandle       border_material;
  MxBorderImage   *highlight_image;
  MxBorderImage   *shadow_image;
  MxBorderImage   *border_image;

  ClutterTimeline *state_timeline;
  ClutterAlpha    *state_alpha;
  ClutterActorBox  old_box;
  ClutterCallback  state_callback;
  gpointer state_userdata;
  guint state;
};

enum
{
  STATE_OPEN,
  STATE_CLOSED,
  STATE_CLOSING,
  STATE_OPENING
};

static GQuark mex_resizing_hbox_meta_quark = 0;

static void mex_resizing_hbox_start_animation (MexResizingHBox *self);
static void mex_resizing_hbox_get_allocation (MexScrollableContainer *self,
                                              ClutterActor           *child,
                                              ClutterActorBox        *box);

/* MexSceneInterface */
static void
mex_resizing_hbox_open (MexScene        *actor,
                        ClutterCallback  callback,
                        gpointer         data)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (actor)->priv;

  if (priv->state == STATE_CLOSING || priv->state == STATE_OPENING)
    return;

  if (priv->state == STATE_OPEN)
    {
      if (callback)
        callback (CLUTTER_ACTOR (actor), data);
      return;
    }

  if (!priv->current_focus)
    {
      priv->state = STATE_OPEN;

      if (callback)
        callback (CLUTTER_ACTOR (actor), data);

      return;
    }


  priv->state = STATE_OPENING;

  clutter_timeline_start (priv->state_timeline);

  priv->state_callback = callback;
  priv->state_userdata = data;
}

static void
mex_resizing_hbox_state_timeline_complete_cb (ClutterTimeline *timeline,
                                             MexResizingHBox *hbox)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (hbox)->priv;

  if (priv->state == STATE_OPENING)
    {
      GList *l;

      /* make sure all children are visible again */
      for (l = priv->children; l; l = g_list_next (l))
        {
          if (l->data != priv->current_focus)
            {
              clutter_actor_show (l->data);
              clutter_actor_set_opacity (l->data, 0);
              clutter_actor_animate (l->data, CLUTTER_EASE_OUT_QUAD, ANIMATION_DURATION / 2.0,
                                     "opacity", (guchar) INACTIVE_OPACITY, NULL);
            }
        }
      priv->state = STATE_OPEN;
    }
  else
    priv->state = STATE_CLOSED;

  if (priv->state_callback)
    priv->state_callback (CLUTTER_ACTOR (hbox), priv->state_userdata);

  priv->state_callback = NULL;
  priv->state_userdata = NULL;
}

static void
mex_resizing_hbox_state_timeline_cb (ClutterTimeline *timeline,
                                    gint             msecs,
                                    MexResizingHBox *hbox)
{
  clutter_actor_queue_relayout (CLUTTER_ACTOR (hbox));
}

static void
mex_resizing_hbox_close (MexScene        *actor,
                         ClutterCallback  callback,
                         gpointer         data)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (actor)->priv;
  GList *l;

  if (!priv->current_focus)
    {
      priv->state = STATE_CLOSED;

      if (callback)
        callback (CLUTTER_ACTOR (actor), data);

      return;
    }

  if (priv->state == STATE_CLOSING || priv->state == STATE_OPENING)
    return;

  clutter_actor_get_allocation_box (priv->current_focus, &priv->old_box);

  /* set the current state after retrieving the allocation box, as the
   * current focused actor may not have a valid allocation and therefore
   * resizing hbox must not start the animation before giving the focused
   * actor a valid allocation from which to start from */
  priv->state = STATE_CLOSING;

  clutter_timeline_start (priv->state_timeline);

  priv->state_callback = callback;
  priv->state_userdata = data;


  /* hide all children except the current focused child */
  for (l = priv->children; l; l = g_list_next (l))
    {
      if (l->data != priv->current_focus)
        clutter_actor_hide (l->data);
    }
}

static void
mex_scene_iface_init (MexSceneInterface *iface)
{
  iface->open = mex_resizing_hbox_open;
  iface->close = mex_resizing_hbox_close;
}


/* ClutterContainerIface */

static void
mex_resizing_hbox_notify_visible_cb (ClutterActor    *child,
                                     GParamSpec      *pspec,
                                     MexResizingHBox *self)
{
  /* Start/alter resizing animation */
  mex_resizing_hbox_start_animation (self);
}

static void
mex_resizing_hbox_add (ClutterContainer *container,
                       ClutterActor     *actor)
{
  MexResizingHBox *self = MEX_RESIZING_HBOX (container);
  MexResizingHBoxPrivate *priv = self->priv;

  priv->children = g_list_append (priv->children, actor);

  /* Note, it's important that we connect to this before setting parent,
   * so that animations are refreshed.
   */
  g_signal_connect (actor, "notify::visible",
                    G_CALLBACK (mex_resizing_hbox_notify_visible_cb), self);
  if (priv->fade)
    clutter_actor_set_opacity (actor, INACTIVE_OPACITY);

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (self));

  g_signal_emit_by_name (self, "actor-added", actor);
}

static void
mex_resizing_hbox_remove (ClutterContainer *container,
                          ClutterActor     *actor)
{
  GList *c;

  MexResizingHBox *self = MEX_RESIZING_HBOX (container);
  MexResizingHBoxPrivate *priv = self->priv;

  for (c = priv->children; c; c = c->next)
    {
      MexResizingHBoxChild *meta;

      if (c->data != actor)
        continue;

      meta = MEX_RESIZING_HBOX_CHILD (
        clutter_container_get_child_meta (container, actor));

      g_signal_handlers_disconnect_by_func (actor,
                                            mex_resizing_hbox_notify_visible_cb,
                                            self);

      if (!meta->dead && !priv->in_dispose &&
          CLUTTER_ACTOR_IS_REALIZED (actor) && CLUTTER_ACTOR_IS_VISIBLE (actor))
        {
          gdouble progress;
          CoglHandle texture;
          ClutterActor *clone;
          ClutterActorBox box;
          gfloat current_width, current_height;

          guint msecs = priv->stagger_length;

          meta->dead = TRUE;

          /* Make a texture copy of the child to animate away */
          clone = mx_offscreen_new ();
          mx_offscreen_set_child (MX_OFFSCREEN (clone), actor);
          mx_offscreen_set_auto_update (MX_OFFSCREEN (clone), FALSE);
          mx_offscreen_update (MX_OFFSCREEN (clone));

          /* Save the texture and replace the offscreen with a texture */
          texture = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (clone));
          cogl_handle_ref (texture);

          g_object_ref_sink (clone);
          g_object_unref (clone);

          clone = clutter_texture_new ();
          clutter_texture_set_cogl_texture (CLUTTER_TEXTURE (clone), texture);
          cogl_handle_unref (texture);

          /* Set the child properties to animate away */
          progress = clutter_alpha_get_alpha (priv->alpha);
          current_width = (meta->target_width * progress) +
                          (meta->initial_width * (1.0 - progress));
          meta->target_width = meta->initial_width = current_width;
          current_height = (meta->target_height * progress) +
                           (meta->initial_height * (1.0 - progress));

          if (meta->stagger)
            {
              msecs = clutter_timeline_get_elapsed_time (meta->timeline);
              clutter_timeline_stop (meta->timeline);
              clutter_timeline_rewind (meta->timeline);
            }

          clutter_timeline_set_direction (meta->timeline,
                                          CLUTTER_TIMELINE_BACKWARD);
          clutter_timeline_advance (meta->timeline, msecs);
          meta->stagger = TRUE;

          /* Set the size */
          clutter_actor_get_allocation_box (actor, &box);
          clutter_actor_set_size (clone,
                                  (box.x2 - box.x1) / current_width,
                                  (box.y2 - box.y1) / current_height);

          /* Parent the clone */
          clutter_actor_set_parent (clone, CLUTTER_ACTOR (self));

          /* Set child meta on the clone and replace the original actor */
          meta->actor = clone;
          g_object_set_qdata (G_OBJECT (clone), mex_resizing_hbox_meta_quark,
                              meta);
          c->data = clone;
        }
      else
        {
          priv->children = g_list_delete_link (priv->children, c);
          g_object_unref (meta);
          clutter_actor_unparent (actor);

          return;
        }

      /* Remove the old actor */
      g_object_ref (actor);

      g_object_set_qdata (G_OBJECT (actor), mex_resizing_hbox_meta_quark, NULL);
      clutter_actor_unparent (actor);

      if (priv->current_focus == actor)
        {
          priv->current_focus = NULL;
          priv->has_focus = FALSE;
        }

      g_signal_emit_by_name (self, "actor-removed", actor);

      g_object_unref (actor);

      mex_resizing_hbox_start_animation (self);

      return;
    }

  g_warning (G_STRLOC ": Trying to remove an unknown child");
}

static void
mex_resizing_hbox_foreach (ClutterContainer *container,
                           ClutterCallback   callback,
                           gpointer          user_data)
{
  MexResizingHBox *self = MEX_RESIZING_HBOX (container);
  MexResizingHBoxPrivate *priv = self->priv;

  g_list_foreach (priv->children, (GFunc) callback, user_data);
}

static void
mex_resizing_hbox_reorder (ClutterContainer *container,
                           ClutterActor     *actor,
                           ClutterActor     *sibling,
                           gboolean          after)
{
  ClutterActor *child;
  GList *l, *sibling_link;
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (container)->priv;

  l = g_list_find (priv->children, actor);
  sibling_link = g_list_find (priv->children, sibling);

  if (!l || !sibling_link)
    {
      g_warning (G_STRLOC ": Children not found in internal child list");
      return;
    }

  if (after)
    sibling_link = g_list_next (sibling_link);

  if (l == sibling_link)
    return;

  child = l->data;
  priv->children = g_list_delete_link (priv->children, l);
  priv->children = g_list_insert_before (priv->children, sibling_link, child);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));
}

static void
mex_resizing_hbox_raise (ClutterContainer *container,
                         ClutterActor     *actor,
                         ClutterActor     *sibling)
{
  mex_resizing_hbox_reorder (container, actor, sibling, TRUE);
}

static void
mex_resizing_hbox_lower (ClutterContainer *container,
                         ClutterActor     *actor,
                         ClutterActor     *sibling)
{
  mex_resizing_hbox_reorder (container, actor, sibling, FALSE);
}

static void
mex_resizing_hbox_child_completed_cb (ClutterTimeline      *timeline,
                                      MexResizingHBoxChild *meta)
{
  if (meta->dead)
    clutter_actor_destroy (meta->actor);

  meta->stagger = FALSE;
}

static void
mex_resizing_hbox_relayout_meta (MexResizingHBoxChild *meta)
{
  clutter_actor_queue_relayout (meta->actor);
}

static void
mex_resizing_hbox_create_child_meta (ClutterContainer *container,
                                     ClutterActor     *actor)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (container)->priv;
  MexResizingHBoxChild *meta =
    g_object_new (MEX_TYPE_RESIZING_HBOX_CHILD, "actor", actor, NULL);

  meta->actor = actor;
  meta->stagger = TRUE;
  meta->initial_height = meta->target_height = 1.0;
  meta->timeline = clutter_timeline_new (priv->stagger_length);

  g_signal_connect_swapped (meta->timeline, "new-frame",
                            G_CALLBACK (mex_resizing_hbox_relayout_meta), meta);
  g_signal_connect_after (meta->timeline, "completed",
                          G_CALLBACK (mex_resizing_hbox_child_completed_cb),
                          meta);

  g_object_set_qdata (G_OBJECT (actor), mex_resizing_hbox_meta_quark, meta);
}

static ClutterChildMeta *
mex_resizing_hbox_get_child_meta (ClutterContainer *container,
                                  ClutterActor     *actor)
{
  return g_object_get_qdata (G_OBJECT (actor), mex_resizing_hbox_meta_quark);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = mex_resizing_hbox_add;
  iface->remove = mex_resizing_hbox_remove;
  iface->foreach = mex_resizing_hbox_foreach;
  iface->raise = mex_resizing_hbox_raise;
  iface->lower = mex_resizing_hbox_lower;

  iface->create_child_meta = mex_resizing_hbox_create_child_meta;
  iface->destroy_child_meta = NULL;
  iface->get_child_meta = mex_resizing_hbox_get_child_meta;
  iface->child_meta_type = MEX_TYPE_RESIZING_HBOX_CHILD;
}

/* MxScrollableIface */

static void
mex_resizing_hbox_get_adjustments (MxScrollable  *scrollable,
                                   MxAdjustment **hadjust,
                                   MxAdjustment **vadjust)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (scrollable)->priv;

  if (vadjust)
    *vadjust = NULL;

  if (hadjust)
    {
      if (!priv->hadjust)
        {
          priv->hadjust = mx_adjustment_new ();
          g_signal_connect_swapped (priv->hadjust, "notify::value",
                                    G_CALLBACK (clutter_actor_queue_redraw),
                                    scrollable);
          clutter_actor_queue_relayout (CLUTTER_ACTOR (scrollable));
        }

      *hadjust = priv->hadjust;
    }
}

static void
mx_scrollable_iface_init (MxScrollableIface *iface)
{
  iface->get_adjustments = mex_resizing_hbox_get_adjustments;
}

/* MxFocusableIface */

static MxFocusable *
mex_resizing_hbox_move_focus (MxFocusable      *focusable,
                              MxFocusDirection  direction,
                              MxFocusable      *from)
{
  GList *l;
  MxFocusHint hint;

  MexResizingHBox *self = MEX_RESIZING_HBOX (focusable);
  MexResizingHBoxPrivate *priv = self->priv;
  ClutterActor *new_focus = NULL;

  if (!from)
    return NULL;

  l = g_list_find (priv->children, from);
  if (!l)
    return NULL;

  hint = MX_FOCUS_HINT_PRIOR;
  switch (direction)
    {
    case MX_FOCUS_DIRECTION_PREVIOUS:
      hint = MX_FOCUS_HINT_LAST;
    case MX_FOCUS_DIRECTION_LEFT:
      do {
        l = l->prev;
        if (l)
          new_focus = l->data;
        else
          break;
      } while (!MX_IS_FOCUSABLE (new_focus) ||
               !CLUTTER_ACTOR_IS_VISIBLE (new_focus));
      break;

    case MX_FOCUS_DIRECTION_NEXT:
      hint = MX_FOCUS_HINT_FIRST;
    case MX_FOCUS_DIRECTION_RIGHT:
      do {
        l = l->next;
        if (l)
          new_focus = l->data;
        else
          break;
      } while (!MX_IS_FOCUSABLE (new_focus) ||
               !CLUTTER_ACTOR_IS_VISIBLE (new_focus));
      break;

    default:
      l = NULL;
      break;
    }

  if (l)
    return mx_focusable_accept_focus (MX_FOCUSABLE (new_focus), hint);
  else
    return NULL;
}

static MxFocusable *
mex_resizing_hbox_accept_focus (MxFocusable *focusable, MxFocusHint hint)
{
  GList *c;
  gboolean reverse;

  MexResizingHBox *self = MEX_RESIZING_HBOX (focusable);
  MexResizingHBoxPrivate *priv = self->priv;

  if (hint == MX_FOCUS_HINT_FROM_LEFT)
    hint = MX_FOCUS_HINT_FIRST;
  else if (hint == MX_FOCUS_HINT_FROM_RIGHT)
    hint = MX_FOCUS_HINT_LAST;
  else if ((hint == MX_FOCUS_HINT_FROM_ABOVE) ||
           (hint == MX_FOCUS_HINT_FROM_BELOW))
    hint = MX_FOCUS_HINT_PRIOR;

  if ((hint == MX_FOCUS_HINT_PRIOR) && priv->current_focus)
    focusable = mx_focusable_accept_focus (MX_FOCUSABLE (priv->current_focus),
                                           hint);
  else
    {
      focusable = NULL;
      hint = MX_FOCUS_HINT_FIRST;
    }

  if (!focusable)
    {
      reverse = (hint == MX_FOCUS_HINT_FIRST) ? FALSE : TRUE;
      for (c = reverse ? g_list_last (priv->children) : priv->children;
           c; c = reverse ? c->prev : c->next)
        {
          ClutterActor *child = c->data;

          if (!MX_IS_FOCUSABLE (child) ||
              !CLUTTER_ACTOR_IS_VISIBLE (child))
            continue;

          focusable = mx_focusable_accept_focus (MX_FOCUSABLE (child), hint);
          break;
        }
    }

  return focusable;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_resizing_hbox_move_focus;
  iface->accept_focus = mex_resizing_hbox_accept_focus;
}

/* MxStylableIface */

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
      GParamSpec *pspec;

      is_initialized = TRUE;

      pspec = g_param_spec_boxed ("x-mex-highlight",
                                  "Highlight",
                                  "Image to use for the highlight.",
                                  MX_TYPE_BORDER_IMAGE,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_RESIZING_HBOX, pspec);

      pspec = g_param_spec_boxed ("x-mex-shadow",
                                  "Shadow",
                                  "Image to use for the shadow.",
                                  MX_TYPE_BORDER_IMAGE,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_RESIZING_HBOX, pspec);

      pspec = g_param_spec_boxed ("x-mex-border",
                                  "Border",
                                  "Image to use for the border.",
                                  MX_TYPE_BORDER_IMAGE,
                                  G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MEX_TYPE_RESIZING_HBOX, pspec);
    }
}

/* MexScrollableContainerInterface */

static void
mex_scrollable_container_iface_init (MexScrollableContainerInterface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
      is_initialized = TRUE;

      iface->get_allocation = mex_resizing_hbox_get_allocation;
    }
}

/* Actor implementation */

static void
mex_resizing_hbox_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MexResizingHBox *self = MEX_RESIZING_HBOX (object);

  switch (property_id)
    {
    case PROP_RESIZING_ENABLED:
      g_value_set_boolean (value,
                           mex_resizing_hbox_get_resizing_enabled (self));
      break;

    case PROP_HADJUST:
      g_value_set_object (value, self->priv->hadjust);
      break;

    case PROP_VADJUST:
      g_value_set_object (value, NULL);
      break;

    case PROP_HDEPTH_SCALE:
      g_value_set_float (value,
                         mex_resizing_hbox_get_horizontal_depth_scale (self));
      break;

    case PROP_VDEPTH_SCALE:
      g_value_set_float (value,
                         mex_resizing_hbox_get_vertical_depth_scale (self));
      break;

    case PROP_DEPTH_FADE:
      g_value_set_boolean (value,
                           mex_resizing_hbox_get_depth_fade (self));
      break;

    case PROP_DEPTH_INDEX:
      g_value_set_int (value, mex_resizing_hbox_get_depth_index (self));
      break;

    case PROP_MAX_DEPTH:
      g_value_set_int (value, mex_resizing_hbox_get_max_depth (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_resizing_hbox_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MexResizingHBox *self = MEX_RESIZING_HBOX (object);

  switch (property_id)
    {
    case PROP_RESIZING_ENABLED:
      mex_resizing_hbox_set_resizing_enabled (self,
                                              g_value_get_boolean (value));
      break;

    case PROP_HADJUST:
    case PROP_VADJUST:
      g_warning (G_STRLOC ": MexResizingHBox doesn't support "
                 "setting adjustments");
      break;

    case PROP_HDEPTH_SCALE:
      mex_resizing_hbox_set_horizontal_depth_scale (self,
                                                    g_value_get_float (value));
      break;

    case PROP_VDEPTH_SCALE:
      mex_resizing_hbox_set_vertical_depth_scale (self,
                                                  g_value_get_float (value));
      break;

    case PROP_DEPTH_FADE:
      mex_resizing_hbox_set_depth_fade (self, g_value_get_boolean (value));
      break;

    case PROP_DEPTH_INDEX:
      mex_resizing_hbox_set_depth_index (self, g_value_get_int (value));
      break;

    case PROP_MAX_DEPTH:
      mex_resizing_hbox_set_max_depth (self, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_resizing_hbox_dispose (GObject *object)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (object)->priv;

  priv->in_dispose = TRUE;

  if (priv->hadjust)
    {
      g_signal_handlers_disconnect_by_func (priv->hadjust,
                                            clutter_actor_queue_redraw,
                                            object);
      g_object_unref (priv->hadjust);
      priv->hadjust = NULL;
    }

  if (priv->highlight)
    {
      cogl_handle_unref (priv->highlight);
      cogl_handle_unref (priv->highlight_material);
      priv->highlight = NULL;
      priv->highlight_material = NULL;
    }

  if (priv->shadow)
    {
      cogl_handle_unref (priv->shadow);
      cogl_handle_unref (priv->shadow_material);
      priv->shadow = NULL;
      priv->shadow_material = NULL;
    }

  if (priv->border)
    {
      cogl_handle_unref (priv->border);
      cogl_handle_unref (priv->border_material);
      priv->border = NULL;
      priv->border_material = NULL;
    }

  if (priv->alpha)
    {
      g_object_unref (priv->alpha);
      priv->alpha = NULL;
    }

  if (priv->timeline)
    {
      clutter_timeline_stop (priv->timeline);
      g_object_unref (priv->timeline);
      priv->timeline = NULL;
    }

  if (priv->state_timeline)
    {
      g_object_unref (priv->state_timeline);
      priv->state_timeline = NULL;
    }

  if (priv->state_alpha)
    {
      g_object_unref (priv->state_alpha);
      priv->state_alpha = NULL;
    }

  G_OBJECT_CLASS (mex_resizing_hbox_parent_class)->dispose (object);
}

static void
mex_resizing_hbox_finalize (GObject *object)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (object)->priv;

  if (priv->highlight_image)
    {
      g_boxed_free (MX_TYPE_BORDER_IMAGE, priv->highlight_image);
      priv->highlight_image = NULL;
    }

  if (priv->shadow_image)
    {
      g_boxed_free (MX_TYPE_BORDER_IMAGE, priv->shadow_image);
      priv->shadow_image = NULL;
    }

  if (priv->border_image)
    {
      g_boxed_free (MX_TYPE_BORDER_IMAGE, priv->border_image);
      priv->border_image = NULL;
    }

  G_OBJECT_CLASS (mex_resizing_hbox_parent_class)->finalize (object);
}

static void
mex_resizing_hbox_start_animation (MexResizingHBox *self)
{
  GList *c;
  gdouble progress;
  guint anim_delay;
  gint i, focus;
  MexResizingHBoxPrivate *priv = self->priv;

  if (!priv->children)
    {
      clutter_timeline_stop (priv->timeline);
      return;
    }

  if (priv->current_focus)
    {
      if (priv->depth_index < 0)
        {
          for (i = 0, c = priv->children; c; c = c->next)
            {
              MexResizingHBoxChild *meta;
              ClutterActor *child = c->data;

              if (child == priv->current_focus)
                break;

              meta = MEX_RESIZING_HBOX_CHILD (
                clutter_container_get_child_meta (CLUTTER_CONTAINER (self),
                                                  child));

              if (!CLUTTER_ACTOR_IS_VISIBLE (child) || meta->dead)
                continue;

              i++;
            }
          focus = i;
        }
      else
        focus = priv->depth_index;
    }
  else
    focus = priv->depth_index;

  if (priv->state == STATE_OPENING || priv->state == STATE_OPEN)
    progress = 1.0;
  else
    progress = clutter_alpha_get_alpha (priv->alpha);

  anim_delay = 0;

  for (i = 0, c = priv->children; c; c = c->next)
    {
      ClutterActor *child = c->data;
      MexResizingHBoxChild *meta = MEX_RESIZING_HBOX_CHILD (
        clutter_container_get_child_meta (CLUTTER_CONTAINER (self), child));

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        {
          meta->initial_width = 0.0;
          meta->target_width = 0.0;
          clutter_timeline_stop (meta->timeline);
          clutter_timeline_rewind (meta->timeline);
          continue;
        }

      if (!meta->dead)
        {
          gdouble current_width = (meta->target_width * progress) +
                                  (meta->initial_width * (1.0 - progress));
          gdouble current_height = (meta->target_height * progress) +
                                   (meta->initial_height * (1.0 - progress));

          meta->initial_width = current_width;
          meta->initial_height = current_height;

          if (!priv->current_focus && priv->resizing_enabled &&
              priv->depth_index < 0)
            {
              meta->target_width = 1.0 * pow (priv->hdepth,
                                              MIN (priv->max_depth, 2));
              meta->target_height = 1.0 * pow (priv->vdepth,
                                               MIN (priv->max_depth, 2));
            }
          else if (((priv->depth_index < 0) &&
                   (child == priv->current_focus)) ||
                    !priv->resizing_enabled)
            {
              meta->target_width = 1.0;
              meta->target_height = 1.0;
            }
          else
            {
              meta->target_width = 1.0 *
                pow (priv->hdepth, MIN (priv->max_depth, ABS (focus - i)));
              meta->target_height = 1.0 *
                pow (priv->vdepth, MIN (priv->max_depth, ABS (focus - i)));
            }
        }

      if (meta->stagger)
        {
          if (clutter_timeline_is_playing (meta->timeline))
            anim_delay += clutter_timeline_get_duration (meta->timeline) -
                          clutter_timeline_get_elapsed_time (meta->timeline);
          else
            {
              clutter_timeline_set_delay (meta->timeline, anim_delay);
              clutter_timeline_start (meta->timeline);
              anim_delay += priv->stagger_length;
            }
        }

      if (!meta->dead)
        i++;
    }

  clutter_timeline_set_direction (priv->timeline, CLUTTER_TIMELINE_FORWARD);
  clutter_timeline_rewind (priv->timeline);
  clutter_timeline_start (priv->timeline);
}

static void
mex_resizing_hbox_destroy (ClutterActor *actor)
{
  GList *children = clutter_container_get_children (CLUTTER_CONTAINER (actor));

  g_list_foreach (children, (GFunc)clutter_actor_destroy, NULL);
  g_list_free (children);

  if (CLUTTER_ACTOR_CLASS (mex_resizing_hbox_parent_class)->destroy)
    CLUTTER_ACTOR_CLASS (mex_resizing_hbox_parent_class)->destroy (actor);
}

static void
mex_resizing_hbox_get_preferred_width (ClutterActor *actor,
                                       gfloat        for_height,
                                       gfloat       *min_width_p,
                                       gfloat       *nat_width_p)
{
  GList *c;
  MxPadding padding;
  gfloat min_width, nat_width;

  MexResizingHBox *self = MEX_RESIZING_HBOX (actor);
  MexResizingHBoxPrivate *priv = self->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  for_height -= padding.top + padding.bottom;

  min_width = nat_width = 0;

  for (c = priv->children; c; c = c->next)
    {
      gfloat child_min_width, child_nat_width, multiplier;

      ClutterActor *child = c->data;
      MexResizingHBoxChild *meta = MEX_RESIZING_HBOX_CHILD (
        clutter_container_get_child_meta (CLUTTER_CONTAINER (actor), child));

      clutter_actor_get_preferred_width (child,
                                         for_height,
                                         &child_min_width,
                                         &child_nat_width);

      if (meta->stagger)
        {
          clutter_alpha_set_timeline (priv->alpha, meta->timeline);
          multiplier = clutter_alpha_get_alpha (priv->alpha);
        }
      else
        multiplier = 1.f;

      min_width += (int)(child_min_width * multiplier);
      nat_width += (int)(child_nat_width * multiplier);
    }

  clutter_alpha_set_timeline (priv->alpha, priv->timeline);

  if (min_width_p)
    *min_width_p = min_width + padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p = nat_width + padding.left + padding.right;
}

static void
mex_resizing_hbox_get_preferred_height (ClutterActor *actor,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *nat_height_p)
{
  GList *c;
  MxPadding padding;
  gfloat min_height, nat_height;

  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (actor)->priv;

  min_height = nat_height = 0;

  for (c = priv->children; c; c = c->next)
    {
      gfloat child_min_height, child_nat_height;

      ClutterActor *child = c->data;

      clutter_actor_get_preferred_height (child,
                                          -1,
                                          &child_min_height,
                                          &child_nat_height);

      if (child_min_height > min_height)
        min_height = child_min_height;
      if (child_nat_height > nat_height)
        nat_height = child_nat_height;
    }

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_height_p)
    *min_height_p = min_height + padding.top + padding.bottom;
  if (nat_height_p)
    *nat_height_p = nat_height + padding.top + padding.bottom;
}

static void
mex_resizing_hbox_allocate_children (MexResizingHBox        *self,
                                     const ClutterActorBox  *box,
                                     ClutterAllocationFlags  flags,
                                     gdouble                 progress,
                                     ClutterActor           *find,
                                     ClutterActorBox        *find_box)
{
  GList *c;
  MxPadding padding;
  ClutterActorBox actor_box;
  ClutterActorBox child_box;
  gint space, n_children;
  gfloat extra_space, min_width, nat_width, height, width;

  MexResizingHBoxPrivate *priv = self->priv;

  actor_box = *box;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  width = box->x2 - box->x1 - padding.left - padding.right;
  height = box->y2 - box->y1 - padding.top - padding.bottom;
  child_box.x1 = padding.left;
  child_box.y2 = padding.top + height;

  if ((priv->state == STATE_CLOSING || priv->state == STATE_CLOSED
      || priv->state == STATE_OPENING) && priv->current_focus)
    {
      ClutterActorBox final_box;
      gfloat state_progress;

      /* only the current focused child is visible in these states, so no need
       * to allocate the other children */
      state_progress = clutter_alpha_get_alpha (priv->state_alpha);

      /* reverse the animation when opening */
      if (priv->state == STATE_OPENING)
        state_progress = 1 - state_progress;

      final_box.x1 = padding.left;
      final_box.y1 = padding.top;
      final_box.x2 = final_box.x1 + 284;
      final_box.y2 = final_box.y1 + clutter_actor_get_height (priv->current_focus);

      if (priv->state == STATE_CLOSED)
        {
          clutter_actor_allocate (priv->current_focus, &final_box, flags);
          return;
        }

      clutter_actor_box_interpolate (&priv->old_box, &final_box,
                                     state_progress,
                                     &child_box);

      /* fade in/out the children of the column */
      if (MEX_IS_SCROLL_VIEW (priv->current_focus)
          && MEX_IS_COLUMN (mx_bin_get_child (MX_BIN (priv->current_focus))))
        {
          ClutterActor *container;
          GList *l, *children;

          container = mx_bin_get_child (MX_BIN (priv->current_focus));
          children = clutter_container_get_children (CLUTTER_CONTAINER (container));

          for (l = children; l; l = g_list_next (l))
            {
              clutter_actor_set_opacity (l->data, 255 * (1 - state_progress));
            }

          g_list_free (children);
        }

      clutter_actor_allocate (priv->current_focus, &child_box, flags);
      return;
    }

  /* We don't want any set value here, so we call the function directly
   * instead of calling clutter_actor_get_preferred_width(). This also
   * means that we don't get the advantage of Clutter's caching, but
   * all the sizes of our children will still be cached and our own
   * get_preferred_width isn't too expensive.
   */
  mex_resizing_hbox_get_preferred_width (CLUTTER_ACTOR (self),
                                         box->y2 - box->y1,
                                         &min_width,
                                         &nat_width);

  n_children = 0;
  for (c = priv->children; c; c = c->next)
    {
      MexResizingHBoxChild *meta = MEX_RESIZING_HBOX_CHILD (
        clutter_container_get_child_meta (CLUTTER_CONTAINER (self), c->data));

      n_children ++;
      if (meta->push)
        {
          gfloat hmult;
          gfloat child_width = 0;
          clutter_actor_get_preferred_width (c->data,
                                             box->y2 - box->y1 - padding.top -
                                             padding.bottom,
                                             NULL,
                                             &child_width);

          hmult = (meta->target_width * progress) +
                  (meta->initial_width * (1.f - progress));
          child_width *= hmult;

          if (meta->stagger)
            {
              clutter_alpha_set_timeline (priv->alpha, meta->timeline);
              child_width *= clutter_alpha_get_alpha (priv->alpha);
            }

          actor_box.x2 += child_width;
        }
    }

  if (priv->hadjust)
    extra_space = 0;
  else
    extra_space = ((actor_box.x2 - actor_box.x1) - nat_width);


  /* calculate the starting offset to ensure the focused column is centered */
  if (priv->current_focus)
    {
      gfloat cumulative_width = 0;

      for (c = priv->children; c; c = g_list_next (c))
        {
          MexResizingHBoxChild *meta;
          gfloat child_min_width, child_nat_width, child_height;

          ClutterActor *child = c->data;

          if (!CLUTTER_ACTOR_IS_VISIBLE (child))
            continue;

          clutter_actor_get_preferred_width (child,
                                             -1,
                                             &child_min_width,
                                             &child_nat_width);

          meta = MEX_RESIZING_HBOX_CHILD (clutter_container_get_child_meta (CLUTTER_CONTAINER (self), child));

          if (child == priv->current_focus)
            {

              if (priv->max_depth > 1)
                {
                  cumulative_width += (child_nat_width * meta->target_width) / 2.0;

                  child_box.x1 = padding.left + (width / 2.0)
                    - (cumulative_width * progress)
                    - (priv->prev_width * (1 - progress));
                }
              else
                {
                  gfloat offset;
                  gfloat current_focus_width = child_nat_width * meta->target_width;
                  child_box.x1 = 0;

                  offset = (cumulative_width * progress)
                    + (priv->prev_width * (1 - progress));

                  /* attempt to use the previous offset */
                  child_box.x1 = priv->prev_offset;

                  /* adjust for moving right */
                  if (child_box.x1 + offset + current_focus_width > width)
                    child_box.x1 = width - (offset + current_focus_width);

                  /* adjust for moving left */
                  if (child_box.x1 + offset < 0)
                    child_box.x1 = child_box.x1 + -1 * (child_box.x1 + offset);

                  /* store the current offset */
                  priv->prev_offset = child_box.x1;

                  child_box.x1 += padding.left;

                }

              if (progress == 1.0)
                priv->prev_width = cumulative_width;

              break;
            }
          cumulative_width += child_nat_width * meta->target_width;
        }


    }

  for (c = priv->children; c; c = c->next)
    {
      MexResizingHBoxChild *meta;
      gfloat child_min_width, child_nat_width, hmult, vmult, child_height;

      ClutterActor *child = c->data;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        continue;

      meta = MEX_RESIZING_HBOX_CHILD (
        clutter_container_get_child_meta (CLUTTER_CONTAINER (self), child));

      clutter_actor_get_preferred_width (child,
                                         -1,
                                         &child_min_width,
                                         &child_nat_width);
      hmult = (meta->target_width * progress) +
              (meta->initial_width * (1.f - progress));

      child_box.x2 = child_box.x1 + (child_nat_width * hmult);

      vmult = (meta->target_height * progress) +
              (meta->initial_height * (1.f - progress));
      child_height = (gint)(height * vmult);
      child_box.y1 = (padding.top + (height - child_height));

      if (child == find)
        {
          *find_box = child_box;
          return;
        }

      /* Change the offset for the staggered expanding of children. */
      if (meta->stagger)
        {
          gfloat offset, new_width;
          clutter_alpha_set_timeline (priv->alpha, meta->timeline);
          hmult *= clutter_alpha_get_alpha (priv->alpha);

          new_width = (child_box.x2 - child_box.x1) * hmult;

          offset = (child_box.x2 - child_box.x1) - new_width;
          child_box.x1 -= (gint)offset;
          child_box.x2 -= (gint)offset;

          meta->staggered_width = new_width;
        }

      if (!find)
        clutter_actor_allocate (child, &child_box, flags);

      child_box.x1 = (int)child_box.x2;
    }

  clutter_alpha_set_timeline (priv->alpha, priv->timeline);

  /* Update adjustment */
  if (priv->hadjust)
    {
      mx_adjustment_set_page_size (priv->hadjust,
                                   actor_box.x2 - actor_box.x1 -
                                   padding.left - padding.right);
      mx_adjustment_set_upper (priv->hadjust,
                               nat_width - padding.left - padding.right);
    }
}

static void
mex_resizing_hbox_allocate (ClutterActor           *actor,
                            const ClutterActorBox  *box,
                            ClutterAllocationFlags  flags)
{
  gdouble progress;

  MexResizingHBox *self = MEX_RESIZING_HBOX (actor);
  MexResizingHBoxPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_resizing_hbox_parent_class)->
    allocate (actor, box, flags);

  /* Allocate children and update adjustments */
  if (clutter_timeline_is_playing (priv->timeline))
    progress = clutter_alpha_get_alpha (priv->alpha);
  else
    progress = 1.0;

  mex_resizing_hbox_allocate_children (self, box, flags, progress, NULL, NULL);
}

static void
mex_resizing_hbox_get_allocation (MexScrollableContainer *self,
                                  ClutterActor           *child,
                                  ClutterActorBox        *box)
{
  ClutterActorBox self_box;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (self));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));
  g_return_if_fail (box != NULL);

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (self), &self_box);
  mex_resizing_hbox_allocate_children ((MexResizingHBox *) self,
                                       &self_box, 0, 1.0, child, box);
}

static void
mex_resizing_hbox_draw_overlay (MexResizingHBox *self,
                                ClutterActorBox *box,
                                guint8           alpha,
                                gboolean         highlight_left,
                                gboolean         highlight_right)
{
  MexResizingHBoxPrivate *priv = self->priv;

  if (priv->shadow_material)
    {
      cogl_material_set_color4ub (priv->shadow_material,
                                  alpha, alpha, alpha, alpha);
      cogl_set_source (priv->shadow_material);

      if (!highlight_left)
        cogl_rectangle_with_texture_coords (
          box->x1, box->y1,
          box->x1 + cogl_texture_get_width (priv->shadow),
          box->y2,
          0, 0, 1, 1);

      if (!highlight_right)
        cogl_rectangle_with_texture_coords (
          box->x2, box->y1,
          box->x2 - cogl_texture_get_width (priv->shadow),
          box->y2,
          0, 0, 1, 1);
    }

  if (priv->highlight_material)
    {
      cogl_material_set_color4ub (priv->highlight_material,
                                  alpha, alpha, alpha, alpha);
      cogl_set_source (priv->highlight_material);

      if (highlight_left)
        cogl_rectangle_with_texture_coords (
          box->x1, box->y1,
          box->x1 + cogl_texture_get_width (priv->highlight),
          box->y2,
          0, 0, 1, 1);

      if (highlight_right)
        cogl_rectangle_with_texture_coords (
          box->x2, box->y1,
          box->x2 - cogl_texture_get_width (priv->highlight),
          box->y2,
          0, 0, 1, 1);
    }

  if (priv->border_material)
    {
      cogl_material_set_color4ub (priv->border_material,
                                  alpha, alpha, alpha, alpha);
      cogl_set_source (priv->border_material);

      cogl_rectangle_with_texture_coords (
        box->x1, box->y1,
        box->x1 + cogl_texture_get_width (priv->border),
        box->y2,
        0, 0, 1, 1);
      cogl_rectangle_with_texture_coords (
        box->x2, box->y1,
        box->x2 - cogl_texture_get_width (priv->border),
        box->y2,
        0, 0, 1, 1);
    }
}

static void
mex_resizing_hbox_draw_child (MexResizingHBox *self,
                              ClutterActor    *child,
                              gdouble          progress,
                              gboolean         highlight_left,
                              gboolean         highlight_right,
                              guint8           opacity)
{
  ClutterActorBox child_box;
  MexResizingHBoxChild *meta;

  if (!CLUTTER_ACTOR_IS_VISIBLE (child))
    return;

  meta = MEX_RESIZING_HBOX_CHILD (
    clutter_container_get_child_meta (CLUTTER_CONTAINER (self), child));

  clutter_actor_get_allocation_box (child, &child_box);

  /* Clip the child for the staggered expanding animation. */
  if (meta->stagger)
    {
      child_box.x1 = child_box.x2 - meta->staggered_width;
      cogl_clip_push_rectangle (child_box.x1, child_box.y1,
                                child_box.x2, child_box.y2);
    }

  clutter_actor_paint (child);

  mex_resizing_hbox_draw_overlay (self, &child_box, opacity,
                                  highlight_left, highlight_right);

  if (meta->stagger)
    cogl_clip_pop ();
}

static void
mex_resizing_hbox_apply_transform (ClutterActor *actor,
                                   CoglMatrix   *matrix)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_resizing_hbox_parent_class)->
    apply_transform (actor, matrix);

  if (priv->hadjust)
    cogl_matrix_translate (matrix,
                           -mx_adjustment_get_value (priv->hadjust), 0, 0);
}

static void
mex_resizing_hbox_paint (ClutterActor *actor)
{
  gint i;
  GList *c;
  guint8 opacity;
  gdouble progress;

  MexResizingHBox *self = MEX_RESIZING_HBOX (actor);
  MexResizingHBoxPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mex_resizing_hbox_parent_class)->paint (actor);

  progress = clutter_alpha_get_alpha (priv->alpha);
  opacity = clutter_actor_get_paint_opacity (actor);
  for (i = 0, c = priv->children; c; c = c->next, i++)
    {
      ClutterActor *child = c->data;

      if (((priv->depth_index < 0) &&
           ((!priv->current_focus) || (child == priv->current_focus))) ||
          (i == priv->depth_index))
        {
          GList *sub;

          /* Draw all the children to the right of the focus */
          for (sub = g_list_last (c); sub != c; sub = sub->prev)
            mex_resizing_hbox_draw_child (self, sub->data, progress,
                                          FALSE, TRUE, opacity);

          /* Draw all the children to the left of the focus */
          for (sub = g_list_first (c); sub != c; sub = sub->next)
            mex_resizing_hbox_draw_child (self, sub->data, progress,
                                          TRUE, FALSE, opacity);

          /* Draw the focused child */
          mex_resizing_hbox_draw_child (self, child, progress,
                                        TRUE, TRUE, opacity);

          break;
        }
    }
}

static void
mex_resizing_hbox_pick (ClutterActor       *actor,
                        const ClutterColor *color)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_resizing_hbox_parent_class)->pick (actor, color);

  g_list_foreach (priv->children, (GFunc) clutter_actor_paint, NULL);
}

static void
mex_resizing_hbox_notify_focused_cb (MxFocusManager  *manager,
                                     GParamSpec      *pspec,
                                     MexResizingHBox *self)
{
  ClutterActor *focused;
  MexResizingHBoxPrivate *priv = self->priv;

  focused = (ClutterActor *)mx_focus_manager_get_focused (manager);

  /* Check if our child received focus and store which child it was,
   * or lose focus.
   */
  if (focused)
    {
      ClutterActor *parent = clutter_actor_get_parent (focused);
      while (parent)
        {
          if (parent == (ClutterActor *)self)
            {
              if (priv->current_focus == focused)
                return;

              if (priv->fade && priv->current_focus)
                clutter_actor_animate (priv->current_focus,
                                       CLUTTER_EASE_OUT_QUAD, 250,
                                       "opacity", INACTIVE_OPACITY,
                                       NULL);

              priv->current_focus = focused;
              priv->has_focus = TRUE;

              clutter_actor_get_allocation_box (priv->current_focus,
                                                &priv->old_box);

              if (priv->fade)
                clutter_actor_animate (priv->current_focus,
                                       CLUTTER_EASE_OUT_QUAD, 250,
                                       "opacity", 0xff,
                                       NULL);

              mex_resizing_hbox_start_animation (self);

              return;
            }

          focused = parent;
          parent = clutter_actor_get_parent (focused);
        }
    }

  if (priv->has_focus)
    priv->has_focus = FALSE;

  mex_resizing_hbox_start_animation (self);
}

static void
mex_resizing_hbox_map (ClutterActor *actor)
{
  MxFocusManager *manager;

  CLUTTER_ACTOR_CLASS (mex_resizing_hbox_parent_class)->map (actor);

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    {
      g_signal_connect (manager, "notify::focused",
                        G_CALLBACK (mex_resizing_hbox_notify_focused_cb),
                        actor);
      mex_resizing_hbox_notify_focused_cb (manager, NULL,
                                           (MexResizingHBox *)actor);
    }
}

static void
mex_resizing_hbox_unmap (ClutterActor *actor)
{
  MxFocusManager *manager;

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    g_signal_handlers_disconnect_by_func (manager,
                                          mex_resizing_hbox_notify_focused_cb,
                                          actor);

  CLUTTER_ACTOR_CLASS (mex_resizing_hbox_parent_class)->unmap (actor);
}

static gboolean
mex_resizing_hbox_get_paint_volume (ClutterActor       *actor,
                                    ClutterPaintVolume *volume)
{
  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (actor)->priv;
  GList *l;

  if (priv->children == NULL)
    return clutter_paint_volume_set_from_allocation (volume, actor);

  for (l = priv->children; l != NULL; l = l->next)
    {
      ClutterActor *child = l->data;
      const ClutterPaintVolume *child_volume;

      child_volume = clutter_actor_get_transformed_paint_volume (child, actor);
      if (!child_volume)
        return FALSE;

      clutter_paint_volume_union (volume, child_volume);
    }

  return TRUE;
}

static void
mex_resizing_hbox_class_init (MexResizingHBoxClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexResizingHBoxPrivate));

  object_class->get_property = mex_resizing_hbox_get_property;
  object_class->set_property = mex_resizing_hbox_set_property;
  object_class->dispose = mex_resizing_hbox_dispose;
  object_class->finalize = mex_resizing_hbox_finalize;

  actor_class->destroy = mex_resizing_hbox_destroy;
  actor_class->get_preferred_width = mex_resizing_hbox_get_preferred_width;
  actor_class->get_preferred_height = mex_resizing_hbox_get_preferred_height;
  actor_class->allocate = mex_resizing_hbox_allocate;
  actor_class->apply_transform = mex_resizing_hbox_apply_transform;
  actor_class->paint = mex_resizing_hbox_paint;
  actor_class->pick = mex_resizing_hbox_pick;
  actor_class->map = mex_resizing_hbox_map;
  actor_class->unmap = mex_resizing_hbox_unmap;
  actor_class->get_paint_volume = mex_resizing_hbox_get_paint_volume;

  pspec = g_param_spec_boolean ("resizing-enabled",
                                "Resizing enabled",
                                "Whether to size children with respect to "
                                "which child currently has focus.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_RESIZING_ENABLED, pspec);

  pspec = g_param_spec_float ("horizontal-depth-scale",
                              "Horizontal depth scale",
                              "The multiplier used to determine how much "
                              "children should shrink beyond the child "
                              "designated by the depth-index, horizontally.",
                              0.f, 1.f, 0.667f,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_HDEPTH_SCALE, pspec);

  pspec = g_param_spec_float ("vertical-depth-scale",
                              "Vertical depth scale",
                              "The multiplier used to determine how much "
                              "children should shrink beyond the child "
                              "designated by the depth-index, vertically.",
                              0.f, 1.f, 0.99f,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_VDEPTH_SCALE, pspec);

  pspec = g_param_spec_boolean ("depth-fade",
                                "Depth fade",
                                "Whether to fade children with respect to "
                                "their simulated depth.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DEPTH_FADE, pspec);

  pspec = g_param_spec_int ("depth-index",
                            "Depth index",
                            "Index of the child to use as the foreground "
                            "child. Negative values mean to use the focused "
                            "child.",
                            -1, G_MAXINT, -1,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DEPTH_INDEX, pspec);

  pspec = g_param_spec_int ("max-depth",
                            "Max depth",
                            "The maximum amount of depth steps.",
                            0, G_MAXINT, 5,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MAX_DEPTH, pspec);

  /* MxScrollable properties */
  g_object_class_override_property (object_class,
                                    PROP_HADJUST,
                                    "horizontal-adjustment");

  g_object_class_override_property (object_class,
                                    PROP_VADJUST,
                                    "vertical-adjustment");

  mex_resizing_hbox_meta_quark =
    g_quark_from_static_string ("mex-resizing-hbox-meta");
}

/* Following function copied from MexDrawersBox */
static void
mex_resizing_hbox_timeline_completed_cb (ClutterTimeline *timeline,
                                         ClutterActor    *box)
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

  clutter_actor_queue_relayout (box);
}

static void
mex_resizing_hbox_style_changed_cb (MxStylable          *self,
                                    MxStyleChangedFlags  flags)
{
  MxBorderImage *highlight, *shadow, *border;

  MexResizingHBoxPrivate *priv = MEX_RESIZING_HBOX (self)->priv;

  mx_stylable_get (self,
                   "x-mex-highlight", &highlight,
                   "x-mex-shadow", &shadow,
                   "x-mex-border", &border,
                   NULL);

  mex_replace_border_image (&priv->highlight,
                            highlight,
                            &priv->highlight_image,
                            &priv->highlight_material);
  mex_replace_border_image (&priv->shadow,
                            shadow,
                            &priv->shadow_image,
                            &priv->shadow_material);
  mex_replace_border_image (&priv->border,
                            border,
                            &priv->border_image,
                            &priv->border_material);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));

  priv->state_timeline = clutter_timeline_new (ANIMATION_DURATION);
  priv->state_alpha = clutter_alpha_new_full (priv->state_timeline,
                                             CLUTTER_EASE_IN_OUT_CUBIC);
  g_signal_connect (priv->state_timeline, "new-frame",
                    G_CALLBACK (mex_resizing_hbox_state_timeline_cb), self);
  g_signal_connect (priv->state_timeline, "completed",
                    G_CALLBACK (mex_resizing_hbox_state_timeline_complete_cb), self);
}

static void
mex_resizing_hbox_init (MexResizingHBox *self)
{
  MexResizingHBoxPrivate *priv = self->priv = RESIZING_HBOX_PRIVATE (self);

  priv->anim_length = 300;
  priv->stagger_length = 350;
  priv->timeline = clutter_timeline_new (priv->anim_length);
  priv->alpha = clutter_alpha_new_full (priv->timeline, CLUTTER_EASE_OUT_QUAD);
  priv->resizing_enabled = TRUE;
  priv->depth_index = -1;
  priv->hdepth = 0.667f;
  priv->vdepth = 0.99f;
  priv->max_depth = 5;
  priv->fade = TRUE;

  g_signal_connect_object (priv->timeline, "new-frame",
                           G_CALLBACK (clutter_actor_queue_relayout),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->timeline, "completed",
                           G_CALLBACK (mex_resizing_hbox_timeline_completed_cb),
                           self, 0);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mex_resizing_hbox_style_changed_cb), NULL);
}

ClutterActor *
mex_resizing_hbox_new (void)
{
  return g_object_new (MEX_TYPE_RESIZING_HBOX, NULL);
}

void
mex_resizing_hbox_set_resizing_enabled (MexResizingHBox *hbox,
                                        gboolean         enabled)
{
  MexResizingHBoxPrivate *priv;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (hbox));

  priv = hbox->priv;
  if (priv->resizing_enabled != enabled)
    {
      priv->resizing_enabled = enabled;

      mex_resizing_hbox_start_animation (hbox);

      g_object_notify (G_OBJECT (hbox), "resizing-enabled");
    }
}

gboolean
mex_resizing_hbox_get_resizing_enabled (MexResizingHBox *hbox)
{
  g_return_val_if_fail (MEX_IS_RESIZING_HBOX (hbox), FALSE);
  return hbox->priv->resizing_enabled;
}

void
mex_resizing_hbox_set_horizontal_depth_scale (MexResizingHBox *hbox,
                                              gfloat           multiplier)
{
  MexResizingHBoxPrivate *priv;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (hbox));

  priv = hbox->priv;
  if (priv->hdepth != multiplier)
    {
      priv->hdepth = multiplier;

      mex_resizing_hbox_start_animation (hbox);

      g_object_notify (G_OBJECT (hbox), "horizontal-depth-scale");
    }
}

gfloat
mex_resizing_hbox_get_horizontal_depth_scale (MexResizingHBox *hbox)
{
  g_return_val_if_fail (MEX_IS_RESIZING_HBOX (hbox), 0.f);
  return hbox->priv->hdepth;
}

void
mex_resizing_hbox_set_vertical_depth_scale (MexResizingHBox *hbox,
                                            gfloat           multiplier)
{
  MexResizingHBoxPrivate *priv;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (hbox));

  priv = hbox->priv;
  if (priv->vdepth != multiplier)
    {
      priv->vdepth = multiplier;

      mex_resizing_hbox_start_animation (hbox);

      g_object_notify (G_OBJECT (hbox), "vertical-depth-scale");
    }
}

gfloat
mex_resizing_hbox_get_vertical_depth_scale (MexResizingHBox *hbox)
{
  g_return_val_if_fail (MEX_IS_RESIZING_HBOX (hbox), 0.f);
  return hbox->priv->vdepth;
}

void
mex_resizing_hbox_set_depth_index (MexResizingHBox *hbox,
                                   gint             index)
{
  MexResizingHBoxPrivate *priv;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (hbox));

  priv = hbox->priv;
  if (priv->depth_index != index)
    {
      priv->depth_index = index;

      mex_resizing_hbox_start_animation (hbox);

      g_object_notify (G_OBJECT (hbox), "depth-index");
    }
}

gint
mex_resizing_hbox_get_depth_index (MexResizingHBox *hbox)
{
  g_return_val_if_fail (MEX_IS_RESIZING_HBOX (hbox), -1);
  return hbox->priv->depth_index;
}

void
mex_resizing_hbox_set_max_depth (MexResizingHBox *hbox,
                                 gint             depth)
{
  MexResizingHBoxPrivate *priv;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (hbox));

  priv = hbox->priv;
  if (priv->max_depth != depth)
    {
      priv->max_depth = depth;

      mex_resizing_hbox_start_animation (hbox);

      g_object_notify (G_OBJECT (hbox), "max-depth");
    }
}

gint
mex_resizing_hbox_get_max_depth (MexResizingHBox *hbox)
{
  g_return_val_if_fail (MEX_IS_RESIZING_HBOX (hbox), 0);
  return hbox->priv->max_depth;
}

void
mex_resizing_hbox_set_depth_fade (MexResizingHBox *hbox,
                                  gboolean         fade)
{
  MexResizingHBoxPrivate *priv;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (hbox));

  priv = hbox->priv;
  if (priv->fade != fade)
    {
      GList *c;

      priv->fade = fade;

      for (c = priv->children; c; c = c->next)
        {
          ClutterActor *child = c->data;
          guint8 opacity = fade ? INACTIVE_OPACITY : 0xff;

          if (fade && priv->has_focus && (priv->current_focus == child))
            opacity = 0xff;

          clutter_actor_animate (child,
                                 CLUTTER_EASE_OUT_QUAD, 250,
                                 "opacity", opacity,
                                 NULL);
        }

      g_object_notify (G_OBJECT (hbox), "depth-fade");
    }
}

gboolean
mex_resizing_hbox_get_depth_fade (MexResizingHBox *hbox)
{
  g_return_val_if_fail (MEX_IS_RESIZING_HBOX (hbox), FALSE);
  return hbox->priv->fade;
}
