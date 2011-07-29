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

#include "mex-grid-view.h"
#include "mex-scene.h"
#include <glib/gi18n.h>

/* defines */
#define MENU_MIN_WIDTH 284.0
#define MENU_SECONDARY_WIDTH 180.0
#define GRID_TOP_PADDING 6
#define ANIMATION_DURATION 400

static void mex_grid_view_focus_iface_init (MxFocusableIface *iface);
static void mex_grid_view_scene_iface_init (MexSceneInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexGridView, mex_grid_view, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mex_grid_view_focus_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_SCENE,
                                                mex_grid_view_scene_iface_init))

#define GRID_VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GRID_VIEW, MexGridViewPrivate))

struct _MexGridViewPrivate
{
  ClutterActor *grid;
  ClutterActor *grid_header;
  ClutterActor *grid_scrollview;
  ClutterActor *grid_layout;
  ClutterActor *grid_title;

  ClutterActor *menu;
  ClutterActor *menu_header;
  ClutterActor *menu_layout;
  ClutterActor *menu_title;
  ClutterActor *menu_icon;
  ClutterActor *order_by_layout;

  MexModel *model;
  MexModel *alt_model;
  MexProxy *proxy;

  guint state;

  ClutterTimeline *timeline;
  ClutterAlpha *alpha;

  ClutterCallback callback;
  gpointer userdata;

  ClutterActorBox target_box;
};

enum
{
  PROP_MODEL = 1
};

enum
{
  STATE_OPENING,
  STATE_CLOSING_STAGE1,
  STATE_CLOSING_STAGE2,
  STATE_OPEN,
  STATE_CLOSED
};

/* functions */
static void mex_grid_view_set_model (MexGridView *view, MexModel *model);
static void mex_grid_view_timeline_cb (ClutterTimeline *timeline,
                                       gint             msecs,
                                       MexGridView     *view);
static void mex_grid_view_timeline_complete_cb (ClutterTimeline *timeline,
                                                MexGridView     *view);

/* focusable implementation */
static MxFocusable *
mex_grid_view_accept_focus (MxFocusable *focusable,
                            MxFocusHint  hint)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (focusable)->priv;

  return mx_focusable_accept_focus (MX_FOCUSABLE (priv->grid_layout), hint);
}

static MxFocusable *
mex_grid_view_move_focus (MxFocusable      *focusable,
                          MxFocusDirection  direction,
                          MxFocusable      *from)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (focusable)->priv;

  if (direction == MX_FOCUS_DIRECTION_LEFT
      && from == MX_FOCUSABLE (priv->grid_layout))
    return mx_focusable_accept_focus (MX_FOCUSABLE (priv->menu_layout),
                                      MX_FOCUS_HINT_PRIOR);
  else if (direction == MX_FOCUS_DIRECTION_RIGHT
           && from == MX_FOCUSABLE (priv->menu_layout))
    return mx_focusable_accept_focus (MX_FOCUSABLE (priv->grid_layout),
                                      MX_FOCUS_HINT_PRIOR);
  else
    return NULL;
}

static void
mex_grid_view_focus_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = mex_grid_view_accept_focus;
  iface->move_focus = mex_grid_view_move_focus;
}


static void
mex_grid_view_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (object)->priv;

  switch (property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_grid_view_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_MODEL:
      mex_grid_view_set_model (MEX_GRID_VIEW (object),
                               g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_grid_view_dispose (GObject *object)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (object)->priv;

  if (priv->proxy)
    {
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  if (priv->alpha)
    {
      g_object_unref (priv->alpha);
      priv->alpha = NULL;
    }

  if (priv->timeline)
    {
      g_signal_handlers_disconnect_by_func (priv->timeline,
                                            mex_grid_view_timeline_cb,
                                            object);
      g_signal_handlers_disconnect_by_func (priv->timeline,
                                            mex_grid_view_timeline_complete_cb,
                                            object);
      g_object_unref (priv->timeline);
      priv->timeline = NULL;
    }

  if (priv->grid_layout)
    {
      clutter_actor_destroy (priv->grid_layout);
      priv->grid = NULL;
      priv->grid_header = NULL;
      priv->grid_layout = NULL;
    }

  if (priv->menu_layout)
    {
      clutter_actor_destroy (priv->menu_layout);
      priv->menu = NULL;
      priv->menu_header = NULL;
      priv->menu_layout = NULL;
    }

  G_OBJECT_CLASS (mex_grid_view_parent_class)->dispose (object);
}

static void
mex_grid_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_grid_view_parent_class)->finalize (object);
}

static void
mex_grid_view_allocate (ClutterActor           *actor,
                        const ClutterActorBox  *box,
                        ClutterAllocationFlags  flags)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (actor)->priv;
  gfloat menu_min_width, menu_width;
  ClutterActorBox child_box;

  CLUTTER_ACTOR_CLASS (mex_grid_view_parent_class)->allocate (actor, box,
                                                              flags);

  /* menu */
  clutter_actor_get_preferred_width (priv->menu_layout, -1, &menu_min_width,
                                     &menu_width);
  mx_widget_get_available_area (MX_WIDGET (actor), box, &child_box);
  child_box.x2 = child_box.x1 + MAX (MENU_MIN_WIDTH, menu_width);

  if (priv->state == STATE_CLOSING_STAGE2)
    clutter_actor_box_interpolate (&child_box, &priv->target_box,
                                   clutter_alpha_get_alpha (priv->alpha),
                                   &child_box);

  clutter_actor_allocate (priv->menu_layout, &child_box, flags);


  /* grid */
  mx_widget_get_available_area (MX_WIDGET (actor), box, &child_box);

  child_box.y1 += GRID_TOP_PADDING;
  child_box.x1 = child_box.x1 + MENU_MIN_WIDTH;
  if (priv->state == STATE_OPENING)
    child_box.x2 = child_box.x2 * clutter_alpha_get_alpha (priv->alpha);
  else if (priv->state == STATE_CLOSING_STAGE1)
    child_box.x2 = child_box.x2 * (1 - clutter_alpha_get_alpha (priv->alpha));
  else if (priv->state == STATE_CLOSED || priv->state == STATE_CLOSING_STAGE2)
    child_box.x2 = child_box.x1;
  clutter_actor_allocate (priv->grid_layout, &child_box, flags);
}

static void
mex_grid_view_paint (ClutterActor *actor)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (actor)->priv;
  ClutterActorBox gbox, mbox;

  CLUTTER_ACTOR_CLASS (mex_grid_view_parent_class)->paint (actor);

  clutter_actor_get_allocation_box (priv->menu_layout, &mbox);
  clutter_actor_get_allocation_box (priv->grid_layout, &gbox);

  cogl_clip_push_rectangle (mbox.x2, mbox.y1, gbox.x2, gbox.y2);

  clutter_actor_paint (priv->grid_layout);

  cogl_clip_pop ();

  clutter_actor_paint (priv->menu_layout);
}

static void
mex_grid_view_pick (ClutterActor       *actor,
                    const ClutterColor *color)
{
  mex_grid_view_paint (actor);
}

static void
mex_grid_view_open (MexScene              *actor,
                    const ClutterActorBox *origin,
                    ClutterCallback        callback,
                    gpointer               data)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (actor)->priv;

  clutter_timeline_start (priv->timeline);
  priv->state = STATE_OPENING;

  priv->callback = callback;
  priv->userdata = data;
}

static void
mex_grid_view_close (MexScene              *actor,
                     const ClutterActorBox *target,
                     ClutterCallback        callback,
                     gpointer               data)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (actor)->priv;

  clutter_timeline_start (priv->timeline);
  priv->state = STATE_CLOSING_STAGE1;
  priv->target_box = *target;

  priv->callback = callback;
  priv->userdata = data;

}

static void
mex_grid_view_get_current_target (MexScene        *scene,
                                  ClutterActorBox *box)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (scene)->priv;

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (priv->menu_layout), box);
}

static void
mex_grid_view_scene_iface_init (MexSceneInterface *iface)
{
  iface->open = mex_grid_view_open;
  iface->close = mex_grid_view_close;
  iface->get_current_target = mex_grid_view_get_current_target;
}


static void
mex_grid_view_timeline_cb (ClutterTimeline *timeline,
                           gint             msecs,
                           MexGridView     *view)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (view)->priv;

  clutter_actor_queue_relayout (CLUTTER_ACTOR (view));

  if (priv->state == STATE_CLOSING_STAGE2)
    {
      gfloat progress = clutter_alpha_get_alpha (priv->alpha);
      gfloat target_width = clutter_actor_box_get_width (&priv->target_box);

      clutter_actor_set_width (priv->menu,
                               MENU_MIN_WIDTH * (1 - progress)
                               + target_width * progress);
    }
  else
    {
      clutter_actor_set_width (priv->menu, MENU_MIN_WIDTH);
    }
}

static void
mex_grid_view_timeline_complete_cb (ClutterTimeline *timeline,
                                    MexGridView     *view)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (view)->priv;

  if (priv->state == STATE_CLOSING_STAGE1)
    {
      GList *l, *actions;

      /* move to stage 2 of the close animation */
      clutter_timeline_rewind (timeline);
      clutter_timeline_start (timeline);
      priv->state = STATE_CLOSING_STAGE2;

      /* remove the actions from the menu */
      actions = mex_menu_get_actions (MEX_MENU (priv->menu_layout), 0);
      for (l = actions; l; l = g_list_next (l))
        mex_menu_remove_action (MEX_MENU (priv->menu_layout), mx_action_get_name (l->data));
      g_list_free (actions);

      return;
    }
  else if (priv->state == STATE_CLOSING_STAGE2)
    {
      priv->state = STATE_CLOSED;
      CLUTTER_ACTOR_CLASS (mex_grid_view_parent_class)->hide (CLUTTER_ACTOR (view));
    }
  else if (priv->state == STATE_OPENING)
    {
      priv->state = STATE_OPEN;
      mex_proxy_set_model (priv->proxy, priv->model);
    }

  if (priv->callback)
    {
      g_object_ref (G_OBJECT (view));

      priv->callback (CLUTTER_ACTOR (view), priv->userdata);
      priv->callback = NULL;
      priv->userdata = NULL;

      g_object_unref (G_OBJECT (view));
    }
}

static void
mex_grid_view_class_init (MexGridViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexGridViewPrivate));

  object_class->get_property = mex_grid_view_get_property;
  object_class->set_property = mex_grid_view_set_property;
  object_class->dispose = mex_grid_view_dispose;
  object_class->finalize = mex_grid_view_finalize;

  actor_class->allocate = mex_grid_view_allocate;
  actor_class->paint = mex_grid_view_paint;
  actor_class->pick = mex_grid_view_pick;

  pspec = g_param_spec_object ("model", "model", "model", G_TYPE_OBJECT,
                               G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MODEL, pspec);
}

static void
mex_grid_view_init (MexGridView *self)
{
  MexGridViewPrivate *priv = self->priv = GRID_VIEW_PRIVATE (self);
  ClutterActor *scroll_view;
  MexShadow *shadow;
  ClutterColor shadow_color = { 0, 0, 0, 128 };

  priv->state = STATE_CLOSED;

  /* Create the menu */
  priv->menu_layout = mex_menu_new ();
  mex_resizing_hbox_set_max_depth (MEX_RESIZING_HBOX (priv->menu_layout), 1);

  /* Add a title/icon */
  priv->menu_title = mx_label_new ();

  priv->menu_icon = mx_icon_new ();

  priv->menu_header = mx_box_layout_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->menu_header), "Header");
  clutter_actor_set_name (priv->menu_header, "menu-header");

  clutter_container_add (CLUTTER_CONTAINER (priv->menu_header), priv->menu_icon,
                         priv->menu_title, NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->menu_header),
                               priv->menu_icon,
                               "expand", FALSE,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", FALSE,
                               NULL);

  priv->menu = (ClutterActor*) mex_menu_get_layout (MEX_MENU (priv->menu_layout));

  shadow = mex_shadow_new ();
  mex_shadow_set_radius_y (shadow, 0);
  mex_shadow_set_color (shadow, &shadow_color);
  clutter_actor_add_effect_with_name (CLUTTER_ACTOR (priv->menu_layout), "shadow",
                                      CLUTTER_EFFECT (shadow));

  clutter_actor_set_width (priv->menu, MENU_MIN_WIDTH);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->menu), priv->menu_header, 0);


  /* Add the grid */
  priv->grid_layout = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->grid_layout),
                                 MX_ORIENTATION_VERTICAL);

  /* header */
  priv->grid_title = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->grid_title), "Header");
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->grid_layout),
                               priv->grid_title);

  /* scroll view */
  scroll_view = mex_scroll_view_new ();
  mx_kinetic_scroll_view_set_scroll_policy (MX_KINETIC_SCROLL_VIEW (scroll_view),
                                            MX_SCROLL_POLICY_VERTICAL);
  mx_stylable_set_style_class (MX_STYLABLE (scroll_view), "Grid");
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (priv->grid_layout),
                                           scroll_view, 1,
                                           "expand", TRUE, NULL);

  /* grid */
  priv->grid = mex_grid_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view), priv->grid);


  /* Name actors so we can style */
  clutter_actor_set_name (CLUTTER_ACTOR (self), "grid-page");
  clutter_actor_set_name (priv->grid_layout, "content");

  clutter_actor_push_internal (CLUTTER_ACTOR (self));

  clutter_actor_set_parent (priv->menu_layout, CLUTTER_ACTOR (self));
  clutter_actor_set_parent (priv->grid_layout, CLUTTER_ACTOR (self));

  clutter_actor_pop_internal (CLUTTER_ACTOR (self));


  /* timeline for animations */
  priv->timeline = clutter_timeline_new (ANIMATION_DURATION);
  priv->alpha = clutter_alpha_new_full (priv->timeline, CLUTTER_EASE_IN_OUT_CUBIC);
  g_signal_connect (priv->timeline, "new-frame",
                    G_CALLBACK (mex_grid_view_timeline_cb), self);
  g_signal_connect (priv->timeline, "completed",
                    G_CALLBACK (mex_grid_view_timeline_complete_cb), self);
}


static void
mex_explorer_grid_box_notify_open_cb (MexExpanderBox *box,
                                      GParamSpec    *pspec,
                                      MexScrollView *view)
{
  mex_scroll_view_set_indicators_hidden (view, mex_expander_box_get_open (box));
}

static void
mex_explorer_grid_object_created_cb (MexProxy      *proxy,
                                     MexContent    *content,
                                     GObject       *object,
                                     gpointer       grid)
{
  MexContentBox *content_box = MEX_CONTENT_BOX (object);
  ClutterActor *tile = mex_content_box_get_tile (content_box);
  ClutterActor *menu = mex_content_box_get_menu (content_box);

  mex_tile_set_important (MEX_TILE (tile), TRUE);
  mex_expander_box_set_important (MEX_EXPANDER_BOX (object), TRUE);

  /* Make sure the tiles stay the correct size */
  g_object_bind_property (grid, "tile-width",
                          tile, "width",
                          G_BINDING_SYNC_CREATE);

  /* Make sure expanded grid tiles fill a nice-looking space */
  g_object_bind_property (grid, "tile-width",
                          menu, "width",
                          G_BINDING_SYNC_CREATE);

  g_signal_connect (object, "notify::open",
                    G_CALLBACK (mex_explorer_grid_box_notify_open_cb),
                    clutter_actor_get_parent (grid));
}

static void
mex_set_detail_and_pop_cb (MxAction    *action,
                           MexGridView *view)
{
  MexGridViewPrivate *priv = view->priv;
  MexModelSortFuncInfo *sort_info;

  mex_menu_action_set_detail (MEX_MENU (priv->menu_layout), "order",
                              mx_action_get_display_name (action));

  sort_info = g_object_get_data (G_OBJECT (action), "sort-info");

  mex_model_set_sort_func (priv->model, sort_info->sort_func,
                           sort_info->userdata);

  mex_menu_pop (MEX_MENU (priv->menu_layout));
}

static void
mex_order_menu_cb (MxAction    *action,
                   MexGridView *view)
{
  MexGridViewPrivate *priv = view->priv;
  const MexModelInfo *info;
  GList *l;
  MexModel *model;
  ClutterActor *header;

  if (priv->order_by_layout)
    return;

  if (MEX_IS_PROXY_MODEL (priv->model))
    model = mex_proxy_model_get_model (MEX_PROXY_MODEL (priv->model));
  else
    model = priv->model;

  info = mex_model_manager_get_model_info (mex_model_manager_get_default (),
                                           model);

  if (!info)
    return;

  mex_menu_push (MEX_MENU (priv->menu_layout));

  /* add header */
  header = mx_label_new_with_text ("Order by");
  mx_stylable_set_style_class (MX_STYLABLE (header), "Header");
  mx_label_set_y_align (MX_LABEL (header), MX_ALIGN_MIDDLE);

  priv->order_by_layout =
    (ClutterActor*) mex_menu_get_layout (MEX_MENU (priv->menu_layout));
  clutter_actor_set_width (CLUTTER_ACTOR (priv->order_by_layout), MENU_SECONDARY_WIDTH);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->order_by_layout), header, 0);

  g_object_add_weak_pointer (G_OBJECT (priv->order_by_layout),
                             (gpointer *) &priv->order_by_layout);

  /* add items */
  for (l = info->sort_infos; l; l = g_list_next (l))
    {
      MexModelSortFuncInfo *sort_info = l->data;
      MxAction *sort_action;

      sort_action = mx_action_new_full (sort_info->name,
                                        sort_info->display_name,
                                        G_CALLBACK (mex_set_detail_and_pop_cb),
                                        view);
      g_object_set_data (G_OBJECT (sort_action), "sort-info", sort_info);
      mex_menu_add_action (MEX_MENU (priv->menu_layout), sort_action,
                           MEX_MENU_NONE);
    }
}

static void
mex_alt_model_cb (MxAction    *action,
                  MexGridView *view)
{
  MexGridViewPrivate *priv = view->priv;

  mex_menu_action_set_toggled (MEX_MENU (priv->menu_layout), "alt-model",
                               !mex_menu_action_get_toggled (MEX_MENU (priv->menu_layout),
                                                             "alt-model"));
  if (MEX_IS_PROXY_MODEL (priv->model))
    {
      MexModel *old_model;

      old_model = mex_proxy_get_model (priv->proxy);

      mex_proxy_set_model (priv->proxy, priv->alt_model);

      priv->alt_model = old_model;
    }

}

static void
mex_clear_queue_cb (MxAction *action,
                    MexModel *model)
{
  MexModel *model_from_proxy;
  model_from_proxy = mex_queue_model_dup_singleton ();

  mex_model_clear (model_from_proxy);
  g_object_unref (model_from_proxy);
}


static void
mex_grid_view_set_model (MexGridView *view,
                         MexModel    *model)
{
  MexGridViewPrivate *priv = MEX_GRID_VIEW (view)->priv;
  ClutterActor *stage;
  MxAction *order;
  const MexModelInfo *info;

  g_return_if_fail (model != NULL);

  if (model == priv->model)
    return;

  priv->model = model;

  if (MEX_IS_PROXY_MODEL (model))
    info = mex_model_manager_get_model_info (mex_model_manager_get_default (),
                                             mex_proxy_model_get_model (MEX_PROXY_MODEL (model)));
  else
    info = mex_model_manager_get_model_info (mex_model_manager_get_default (),
                                             model);

  if (info)
    {
      /* Add the clear queue option */
      if (g_strcmp0 (info->category, "queue") == 0)
        {
          MxAction *clear_queue;

          clear_queue =
            mx_action_new_full ("clear-queue",
                                _("Clear queue"),
                                G_CALLBACK (mex_clear_queue_cb),
                                model);

          mex_menu_add_action (MEX_MENU (priv->menu_layout),
                               clear_queue,
                               MEX_MENU_NONE);
        }

      /* Add the order-by menu */
      if (info->sort_infos)
        {
          MexModelSortFuncInfo *sort_info;

          order = mx_action_new_full ("order",
                                      _("Order by"),
                                      G_CALLBACK (mex_order_menu_cb),
                                      view);

          mex_menu_add_action (MEX_MENU (priv->menu_layout),
                               order,
                               MEX_MENU_RIGHT);

          sort_info = g_list_nth_data (info->sort_infos,
                                       info->default_sort_index);

          if (sort_info)
            mex_menu_action_set_detail (MEX_MENU (priv->menu_layout), "order",
                                        sort_info->display_name);
          else
            mex_menu_action_set_detail (MEX_MENU (priv->menu_layout), "order",
                                        _("Unsorted"));
        }

      /* Add alt model option */
      if (info->alt_model)
        {
          /* check if the alt_model is being set */
          if (info->alt_model_active)
            priv->alt_model = info->model;
          else
            priv->alt_model = info->alt_model;

          order = mx_action_new_full ("alt-model",
                                      info->alt_model_string,
                                      G_CALLBACK (mex_alt_model_cb),
                                      view);

          mex_menu_add_action (MEX_MENU (priv->menu_layout), order,
                               MEX_MENU_TOGGLE);

          mex_menu_action_set_toggled (MEX_MENU (priv->menu_layout),
                                       "alt-model",
                                       info->alt_model_active);
        }
      else
        {
          priv->alt_model = NULL;
        }
    }

  stage = clutter_actor_get_stage (CLUTTER_ACTOR (view));

  if (priv->proxy)
    g_object_unref (priv->proxy);

  priv->proxy = mex_content_proxy_new (NULL, CLUTTER_CONTAINER (priv->grid),
                                       MEX_TYPE_CONTENT_BOX);
  mex_content_proxy_set_stage (MEX_CONTENT_PROXY (priv->proxy),
                               CLUTTER_STAGE (stage));
  g_signal_connect (priv->proxy, "object-created",
                    G_CALLBACK (mex_explorer_grid_object_created_cb),
                    priv->grid);

  if (priv->state == STATE_OPEN && CLUTTER_ACTOR_IS_MAPPED (view))
    mex_proxy_set_model (priv->proxy, priv->model);

  /* set grid title */
  g_object_bind_property (model, "title",
                          priv->grid_title, "text",
                          G_BINDING_SYNC_CREATE);

  /* set column title and icon */
  if (info)
    {
      const MexModelCategoryInfo *cat_info;
      cat_info = mex_model_manager_get_category_info (mex_model_manager_get_default (),
                                                      info->category);
      mx_label_set_text (MX_LABEL (priv->menu_title), cat_info->display_name);

      mx_icon_set_icon_name (MX_ICON (priv->menu_icon), cat_info->icon_name);
    }
}

ClutterActor *
mex_grid_view_new (MexModel *model)
{
  return g_object_new (MEX_TYPE_GRID_VIEW, "model", model, NULL);
}
