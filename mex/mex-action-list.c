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

#include "mex-action-list.h"

#include "mex-action-manager.h"
#include "mex-content-view.h"
#include "mex-queue-button.h"
#include "mex-utils.h"

static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mex_content_view_iface_init (MexContentViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexActionList, mex_action_list, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_content_view_iface_init))

#define ACTION_LIST_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_ACTION_LIST, MexActionListPrivate))

struct _MexActionListPrivate
{
  ClutterActor *layout;
  MexContent   *content;
  MexModel     *model;
};

static void
mex_action_list_get_property (GObject    *object,
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
mex_action_list_set_property (GObject      *object,
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
mex_action_list_dispose (GObject *object)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (object)->priv;

  if (priv->content)
    {
      g_object_unref (priv->content);
      priv->content = NULL;
    }

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->layout)
    {
      clutter_actor_destroy (priv->layout);
      priv->layout = NULL;
    }

  G_OBJECT_CLASS (mex_action_list_parent_class)->dispose (object);
}

static void
mex_action_list_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_action_list_parent_class)->finalize (object);
}

static void
mex_action_list_get_preferred_width (ClutterActor *actor,
                                     gfloat        for_height,
                                     gfloat       *min_width_p,
                                     gfloat       *nat_width_p)
{
  MxPadding padding;
  MexActionListPrivate *priv = MEX_ACTION_LIST (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (for_height > 0)
    for_height = MAX (0, for_height - padding.top - padding.bottom);

  clutter_actor_get_preferred_width (priv->layout, for_height,
                                     min_width_p, nat_width_p);

  if (min_width_p)
    *min_width_p += padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p += padding.left + padding.right;
}

static void
mex_action_list_get_preferred_height (ClutterActor *actor,
                                      gfloat        for_width,
                                      gfloat       *min_height_p,
                                      gfloat       *nat_height_p)
{
  MxPadding padding;
  MexActionListPrivate *priv = MEX_ACTION_LIST (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (for_width > 0)
    for_width = MAX (0, for_width - padding.left - padding.right);

  clutter_actor_get_preferred_height (priv->layout, for_width,
                                      min_height_p, nat_height_p);

  if (min_height_p)
    *min_height_p += padding.left + padding.right;
  if (nat_height_p)
    *nat_height_p += padding.left + padding.right;
}

static void
mex_action_list_allocate (ClutterActor           *actor,
                          const ClutterActorBox  *box,
                          ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;

  MexActionListPrivate *priv = MEX_ACTION_LIST (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_action_list_parent_class)->
    allocate (actor, box, flags);

  mx_widget_get_available_area (MX_WIDGET (actor), box, &child_box);
  clutter_actor_allocate (priv->layout, &child_box, flags);
}

static void
mex_action_list_paint (ClutterActor *actor)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_action_list_parent_class)->paint (actor);

  clutter_actor_paint (priv->layout);
}

static void
mex_action_list_pick (ClutterActor       *actor,
                      const ClutterColor *color)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_action_list_parent_class)->pick (actor, color);

  clutter_actor_paint (priv->layout);
}

static void
mex_action_list_class_init (MexActionListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexActionListPrivate));

  object_class->get_property = mex_action_list_get_property;
  object_class->set_property = mex_action_list_set_property;
  object_class->dispose = mex_action_list_dispose;
  object_class->finalize = mex_action_list_finalize;

  actor_class->get_preferred_width = mex_action_list_get_preferred_width;
  actor_class->get_preferred_height = mex_action_list_get_preferred_height;
  actor_class->allocate = mex_action_list_allocate;
  actor_class->paint = mex_action_list_paint;
  actor_class->pick = mex_action_list_pick;
}

static MxFocusable *
mex_action_list_move_focus (MxFocusable      *focusable,
                            MxFocusDirection  direction,
                            MxFocusable      *from)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (focusable)->priv;

  switch (direction)
  {
  case MX_FOCUS_DIRECTION_OUT:
  case MX_FOCUS_DIRECTION_LEFT:
  case MX_FOCUS_DIRECTION_RIGHT:
    return NULL;

  default:
    return mx_focusable_accept_focus (MX_FOCUSABLE (priv->layout),
                                      MX_FOCUS_HINT_PRIOR);
  }

  return NULL;
}

static MxFocusable *
mex_action_list_accept_focus (MxFocusable *focusable,
                              MxFocusHint  hint)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (focusable)->priv;
  return mx_focusable_accept_focus (MX_FOCUSABLE (priv->layout), hint);
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_action_list_move_focus;
  iface->accept_focus = mex_action_list_accept_focus;
}

static void
mex_action_list_content_updated_cb (MexContent    *content,
                                    GParamSpec    *pspec,
                                    MexActionList *action_list)
{
  mex_action_list_refresh (action_list);
}

static void
mex_action_list_set_content (MexContentView *view,
                             MexContent     *content)
{
  MexActionList *action_list = MEX_ACTION_LIST (view);
  MexActionListPrivate *priv = action_list->priv;

  if (priv->content == content)
    return;

  if (priv->content)
    {
      g_signal_handlers_disconnect_by_func (priv->content,
                                            mex_action_list_content_updated_cb,
                                            action_list);
      g_object_unref (priv->content);
      priv->content = NULL;
    }

  if (content)
    {
      priv->content = g_object_ref (content);
      g_signal_connect (priv->content, "notify",
                        G_CALLBACK (mex_action_list_content_updated_cb),
                        action_list);
    }

  mex_action_list_refresh (action_list);
}

static MexContent *
mex_action_list_get_content (MexContentView *view)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (view)->priv;

  return priv->content;
}

static void
mex_action_list_set_context (MexContentView *view,
                             MexModel       *model)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (view)->priv;

  if (priv->model == model)
    return;

  if (priv->model)
    g_object_unref (priv->model);

  priv->model = model;

  if (model)
    g_object_ref (model);
}

static MexModel *
mex_action_list_get_context (MexContentView *view)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (view)->priv;
  return priv->model;
}

static void
mex_content_view_iface_init (MexContentViewIface *iface)
{
  iface->set_content = mex_action_list_set_content;
  iface->get_content = mex_action_list_get_content;

  iface->set_context = mex_action_list_set_context;
  iface->get_context = mex_action_list_get_context;
}

static void
mex_action_list_style_changed_cb (MxStylable          *stylable,
                                  MxStyleChangedFlags  flags)
{
  MexActionListPrivate *priv = MEX_ACTION_LIST (stylable)->priv;
  mx_stylable_style_changed (MX_STYLABLE (priv->layout), flags);
}

static void
mex_action_list_init (MexActionList *self)
{
  MexActionListPrivate *priv = self->priv = ACTION_LIST_PRIVATE (self);

  priv->layout = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->layout),
                                 MX_ORIENTATION_VERTICAL);
  clutter_actor_set_parent (priv->layout, CLUTTER_ACTOR (self));

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mex_action_list_style_changed_cb), NULL);
}

ClutterActor *
mex_action_list_new (void)
{
  return g_object_new (MEX_TYPE_ACTION_LIST, NULL);
}

void
mex_action_list_refresh (MexActionList *action_list)
{
  GList *a, *actions;
  MexActionManager *manager;
  MexActionListPrivate *priv;

  g_return_if_fail (MEX_IS_ACTION_LIST (action_list));

  priv = action_list->priv;

  manager = mex_action_manager_get_default ();

  /* Clear old menu contents */
  clutter_container_foreach (CLUTTER_CONTAINER (priv->layout),
                             (ClutterCallback)clutter_actor_destroy,
                             NULL);

  if (!priv->content)
    return;

  actions = mex_action_manager_get_actions_for_content (manager,
                                                        priv->content);

  if (!actions)
    return;

  /* Fill in new menu contents */
  for (a = actions; a; a = a->next)
    {
      MxAction *action = a->data;
      ClutterActor *button;

      /* We treat the queue action specially .. we don't turn the
       * action into an action button we instead use the special queue
       * action button which allows our special effects
       */
      if (g_str_equal (mx_action_get_name (action), "enqueue"))
        {
          button = mex_queue_button_new ();
          mex_content_view_set_content (MEX_CONTENT_VIEW (button),
                                        priv->content);
        }
      else
        {
          button = mex_action_button_new (action);

          mx_bin_set_fill (MX_BIN (button), TRUE, FALSE);
          mex_action_set_content (action, priv->content);
          mex_action_set_context (action, priv->model);
        }

        clutter_container_add_actor (CLUTTER_CONTAINER (priv->layout),
                                     button);
        g_object_set (G_OBJECT (button), "min-width", 240.0, NULL);
    }

  g_list_free (actions);
}
