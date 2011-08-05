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


#include "mex-content-box.h"
#include "mex-action-button.h"
#include "mex-action-list.h"
#include "mex-action-manager.h"
#include "mex-content-view.h"
#include "mex-main.h"
#include "mex-tile.h"
#include "mex-program.h"
#include "mex-private.h"
#include "mex-utils.h"
#include "mex-queue-button.h"
#include "mex-aspect-frame.h"
#include "mex-info-panel.h"
#include "mex-content-tile.h"

static void mex_content_view_iface_init (MexContentViewIface *iface);
static void mex_content_box_focusable_iface_init (MxFocusableIface *iface);
G_DEFINE_TYPE_WITH_CODE (MexContentBox, mex_content_box, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_content_view_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mex_content_box_focusable_iface_init))

#define CONTENT_BOX_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_CONTENT_BOX, MexContentBoxPrivate))

#define DEFAULT_THUMB_WIDTH 426
#define DEFAULT_THUMB_HEIGHT 240
#define DEFAULT_THUMB_RATIO ((float)DEFAULT_THUMB_HEIGHT/(float)DEFAULT_THUMB_WIDTH)

enum
{
  PROP_0,

  PROP_OPEN,
  PROP_IMPORTANT,
  PROP_THUMB_WIDTH,
  PROP_ACTION_LIST_WIDTH,

  PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _MexContentBoxPrivate
{
  MexContent   *content;
  MexModel     *context;

  ClutterActor *tile;
  ClutterActor *action_list;
  ClutterActor *info_panel;

  guint is_open : 1;
  guint extras_visible : 1;
  guint clip_to_allocation : 1;

  ClutterTimeline *timeline;
  ClutterAlpha *alpha;

  gfloat thumb_width;
};

static void mex_content_box_toggle_open (MexContentBox *box);

/* MexContentViewIface */

static void
mex_content_box_set_content (MexContentView *view,
                             MexContent     *content)
{
  MexContentBox *box = MEX_CONTENT_BOX (view);
  MexContentBoxPrivate *priv = box->priv;

  if (priv->content)
    g_object_unref (priv->content);

  priv->content = g_object_ref (content);
  mex_content_view_set_content (MEX_CONTENT_VIEW (priv->tile), content);
  mex_content_view_set_content (MEX_CONTENT_VIEW (priv->info_panel), content);

  /* setting the content on action_list is delayed until the box is opened to
   * ensure any additional actions registered after set_content is called are
   * available */
}

static MexContent *
mex_content_box_get_content (MexContentView *view)
{
  return MEX_CONTENT_BOX (view)->priv->content;
}

static void
mex_content_box_set_context (MexContentView *view,
                             MexModel       *context)
{
  MexContentBox *box = MEX_CONTENT_BOX (view);
  MexContentBoxPrivate *priv = box->priv;

  if (priv->context == context)
    return;

  if (priv->context)
    g_object_unref (priv->context);

  priv->context = g_object_ref (context);
  mex_content_view_set_context (MEX_CONTENT_VIEW (priv->action_list), context);
  mex_content_view_set_context (MEX_CONTENT_VIEW (priv->tile), context);
  mex_content_view_set_context (MEX_CONTENT_VIEW (priv->info_panel), context);
}

static MexModel*
mex_content_box_get_context (MexContentView *view)
{
  MexContentBox *box = MEX_CONTENT_BOX (view);
  MexContentBoxPrivate *priv = box->priv;

  return priv->context;
}

static void
mex_content_view_iface_init (MexContentViewIface *iface)
{
  iface->set_content = mex_content_box_set_content;
  iface->get_content = mex_content_box_get_content;

  iface->set_context = mex_content_box_set_context;
  iface->get_context = mex_content_box_get_context;
}

/* MxFocusableIface */
static MxFocusable*
mex_content_box_accept_focus (MxFocusable *focusable,
                              MxFocusHint  hint)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (focusable)->priv;
  MxFocusable *focus = MX_FOCUSABLE (priv->tile);

  clutter_actor_grab_key_focus (CLUTTER_ACTOR (focusable));

  return mx_focusable_accept_focus (focus, hint);
}

static MxFocusable*
mex_content_box_move_focus (MxFocusable      *focusable,
                            MxFocusDirection  direction,
                            MxFocusable      *from)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (focusable)->priv;
  MxFocusable *result = NULL;

  if (priv->is_open)
    {
      if (direction == MX_FOCUS_DIRECTION_RIGHT &&
          (ClutterActor *) from != priv->action_list)
        result = mx_focusable_accept_focus (MX_FOCUSABLE (priv->action_list), 0);
      else if (direction == MX_FOCUS_DIRECTION_LEFT &&
               (ClutterActor *) from != priv->tile)
        result = mx_focusable_accept_focus (MX_FOCUSABLE (priv->tile), 0);

      if (result == NULL)
        {
          /* close the content box if it is open */
          if (priv->is_open)
            mex_content_box_toggle_open (MEX_CONTENT_BOX (focusable));
        }
    }

  return result;
}

static void
mex_content_box_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = mex_content_box_accept_focus;
  iface->move_focus = mex_content_box_move_focus;
}

static void
mex_content_box_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (object)->priv;
  gint int_value;

  switch (property_id)
    {
    case PROP_OPEN:
      g_value_set_boolean (value, MEX_CONTENT_BOX (object)->priv->is_open);
      break;

    case PROP_IMPORTANT:
      g_value_set_boolean (value,
                           mex_content_box_get_important (MEX_CONTENT_BOX (object)));
      break;

    case PROP_THUMB_WIDTH:
      g_object_get (priv->tile, "thumb-width", &int_value, NULL);
      g_value_set_int (value, int_value);
      break;

    case PROP_ACTION_LIST_WIDTH:
      g_value_set_int (value, clutter_actor_get_width (priv->action_list));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_content_box_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (object)->priv;
  gint int_value;

  switch (property_id)
    {
    case PROP_IMPORTANT:
      mex_content_box_set_important (MEX_CONTENT_BOX (object),
                                     g_value_get_boolean (value));
      break;

    case PROP_THUMB_WIDTH:
      int_value = g_value_get_int (value);
      if (int_value > 0)
        g_object_set (priv->tile,
                      "thumb-width", int_value,
                      "thumb-height", (int) (int_value * DEFAULT_THUMB_RATIO),
                      NULL);
      break;

    case PROP_ACTION_LIST_WIDTH:
      clutter_actor_set_width (priv->action_list, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_content_box_dispose (GObject *object)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (object)->priv;

  if (priv->content)
    {
      g_object_unref (priv->content);
      priv->content = NULL;
    }

  if (priv->context)
    {
      g_object_unref (priv->context);
      priv->context = NULL;
    }

  if (priv->tile)
    {
      clutter_actor_destroy (priv->tile);
      priv->tile = NULL;
    }

  if (priv->action_list)
    {
      clutter_actor_destroy (priv->action_list);
      priv->action_list = NULL;
    }

  if (priv->info_panel)
    {
      clutter_actor_destroy (priv->info_panel);
      priv->info_panel = NULL;
    }

  if (priv->timeline)
    {
      g_object_unref (priv->timeline);
      priv->timeline = NULL;
    }

  if (priv->alpha)
    {
      g_object_unref (priv->alpha);
      priv->alpha = NULL;
    }

  G_OBJECT_CLASS (mex_content_box_parent_class)->dispose (object);
}

static void
mex_content_box_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_content_box_parent_class)->finalize (object);
}

static void
mex_content_box_toggle_open (MexContentBox *box)
{
  MexContentBoxPrivate *priv = box->priv;
  gboolean close_notified;

  /* if the close animation was cancelled then no notify for the closed state
   * will have been sent, therefore notify for the opened state does not need 
   * to be emitted */
  close_notified = (!priv->is_open
                    && !clutter_timeline_is_playing (priv->timeline));

  priv->is_open = !priv->is_open;

  priv->extras_visible = TRUE;
  if (priv->is_open)
    {
      /* opening */
      clutter_timeline_set_direction (priv->timeline,
                                      CLUTTER_TIMELINE_FORWARD);
      mx_stylable_set_style_class (MX_STYLABLE (box), "open");

      /* refresh the action list */
      mex_content_view_set_content (MEX_CONTENT_VIEW (priv->action_list),
                                    priv->content);

      if (close_notified)
        g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_OPEN]);
    }
  else
    {
      /* closing */
      clutter_timeline_set_direction (priv->timeline,
                                      CLUTTER_TIMELINE_BACKWARD);
    }

  if (!clutter_timeline_is_playing (priv->timeline))
    clutter_timeline_rewind (priv->timeline);

  clutter_timeline_start (priv->timeline);
}

static gboolean
mex_content_box_key_press_event_cb (ClutterActor    *actor,
                                    ClutterKeyEvent *event,
                                    gpointer         user_data)
{

  MexActionManager *manager = mex_action_manager_get_default ();
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (actor)->priv;

  if (MEX_KEY_OK (event->keyval))
    {
      GList *actions;

      actions = mex_action_manager_get_actions_for_content (manager,
                                                            priv->content);

      /* find the first action and "activate" it */
      if (actions)
        {
          MxAction *action = actions->data;

          mex_action_set_context (action, priv->context);
          mex_action_set_content (action, priv->content);

          g_signal_emit_by_name (action, "activated", 0);

          g_list_free (actions);

          return TRUE;
        }
    }
  else if (MEX_KEY_INFO (event->keyval))
    {
      mex_content_box_toggle_open (MEX_CONTENT_BOX (actor));
    }

  return FALSE;
}

static void
mex_content_box_paint (ClutterActor *actor)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_content_box_parent_class)->paint (actor);

  if (G_UNLIKELY (priv->clip_to_allocation))
    {
      ClutterActorBox box;
      clutter_actor_get_allocation_box (actor, &box);
      cogl_clip_push_rectangle (0, 0, box.x2 - box.x1, box.y2 - box.y1);
    }

  clutter_actor_paint (priv->tile);

  if (G_UNLIKELY (priv->extras_visible))
    {
      ClutterActorBox box;

      clutter_actor_paint (priv->action_list);
      clutter_actor_paint (priv->info_panel);

      /* separator */
      cogl_set_source_color4ub (255, 255, 255, 51);
      clutter_actor_get_allocation_box (priv->info_panel, &box);
      cogl_path_line (box.x1, box.y1, box.x2, box.y1);
      cogl_path_stroke ();
    }

  if (G_UNLIKELY (priv->clip_to_allocation))
    cogl_clip_pop ();
}

static void
mex_content_box_pick (ClutterActor       *actor,
                      const ClutterColor *color)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (actor)->priv;

  clutter_actor_paint (priv->tile);
  if (G_UNLIKELY (priv->extras_visible))
    clutter_actor_paint (priv->action_list);
}

static gboolean
mex_content_box_get_paint_volume (ClutterActor       *actor,
                                  ClutterPaintVolume *volume)
{
  return clutter_paint_volume_set_from_allocation (volume, actor);
}

static void
mex_content_box_get_preferred_width (ClutterActor *actor,
                                     gfloat        for_height,
                                     gfloat       *min_width,
                                     gfloat       *pref_width)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (actor)->priv;
  gfloat list_w;

  clutter_actor_get_preferred_width (priv->tile, for_height, min_width,
                                     pref_width);
  if (!priv->extras_visible)
    return;

  if (pref_width)
    {
      clutter_actor_get_preferred_width (priv->action_list, for_height, NULL,
                                         &list_w);

      if (clutter_timeline_is_playing (priv->timeline))
        *pref_width = *pref_width +
          (list_w * clutter_alpha_get_alpha (priv->alpha));
      else
        *pref_width = *pref_width + list_w;
    }
}

static void
mex_content_box_get_preferred_height (ClutterActor *actor,
                                      gfloat        for_width,
                                      gfloat       *min_height,
                                      gfloat       *pref_height)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (actor)->priv;
  gfloat info_h;

  clutter_actor_get_preferred_height (priv->tile, for_width, min_height,
                                      pref_height);
  if (!priv->extras_visible)
    return;

  if (pref_height)
    {
      clutter_actor_get_preferred_height (priv->info_panel, for_width, NULL,
                                          &info_h);
      if (clutter_timeline_is_playing (priv->timeline))
        *pref_height = *pref_height +
          (info_h * clutter_alpha_get_alpha (priv->alpha));
      else
        *pref_height = *pref_height + info_h;
    }
}

static void
mex_content_box_allocate (ClutterActor           *actor,
                          const ClutterActorBox  *box,
                          ClutterAllocationFlags  flags)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (actor)->priv;
  ClutterActorBox child_box;
  gfloat pref_w = 0, pref_h = 0, tile_w, tile_h;

  CLUTTER_ACTOR_CLASS (mex_content_box_parent_class)->allocate (actor, box,
                                                                flags);

  tile_w = box->x2 - box->x1;
  clutter_actor_get_preferred_width (priv->tile, -1, NULL, &tile_w);
  if (tile_w > box->x2 - box->x1)
    tile_w = box->x2 - box->x1;

  clutter_actor_get_preferred_height (priv->tile, tile_w, NULL, &tile_h);
  child_box.x1 = 0;
  child_box.x2 = child_box.x1 + tile_w;
  child_box.y1 = 0;
  child_box.y2 = child_box.y1 + tile_h;

  clutter_actor_allocate (priv->tile, &child_box, flags);

  if (G_UNLIKELY (priv->extras_visible))
    {
      /* action list */
      clutter_actor_get_preferred_width (priv->action_list, -1, NULL, &pref_w);
      clutter_actor_get_preferred_height (priv->info_panel, -1, NULL, &pref_h);

      child_box.x1 = tile_w;
      child_box.x2 = tile_w + pref_w;
      child_box.y1 = 0;
      child_box.y2 = tile_h;
      clutter_actor_allocate (priv->action_list, &child_box, flags);


      child_box.x1 = 0;
      child_box.x2 = tile_w + pref_w;
      child_box.y1 = tile_h;
      child_box.y2 = tile_h + pref_h;
      clutter_actor_allocate (priv->info_panel, &child_box, flags);
    }

  /* enable clip-to-allocation if the children will extend beyond the allocated
   * box */
  if ((tile_w + pref_w) > (box->x2 - box->x1)
      || (tile_h + pref_h) > (box->y2 - box->y1))
    priv->clip_to_allocation = TRUE;
  else
    priv->clip_to_allocation = FALSE;
}

static void
mex_content_box_class_init (MexContentBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexContentBoxPrivate));

  object_class->get_property = mex_content_box_get_property;
  object_class->set_property = mex_content_box_set_property;
  object_class->dispose = mex_content_box_dispose;
  object_class->finalize = mex_content_box_finalize;

  actor_class->paint = mex_content_box_paint;
  actor_class->pick = mex_content_box_pick;
  actor_class->get_paint_volume = mex_content_box_get_paint_volume;
  actor_class->get_preferred_width = mex_content_box_get_preferred_width;
  actor_class->get_preferred_height = mex_content_box_get_preferred_height;
  actor_class->allocate = mex_content_box_allocate;

  properties[PROP_OPEN] = g_param_spec_boolean ("open",
                                                "Open",
                                                "Whether the action buttons and"
                                                " info panel are visible.",
                                                FALSE,
                                                G_PARAM_READABLE
                                                | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_OPEN,
                                   properties[PROP_OPEN]);


  properties[PROP_IMPORTANT] = g_param_spec_boolean ("important",
                                                "Important",
                                                "Sets the \"important\" property"
                                                " of the internal MxTile.",
                                                FALSE,
                                                G_PARAM_READWRITE
                                                | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_IMPORTANT,
                                   properties[PROP_IMPORTANT]);

  properties[PROP_THUMB_WIDTH] =
    g_param_spec_int ("thumb-width",
                      "Thumbnail Width",
                      "Width of the thumbail",
                      -1, G_MAXINT, DEFAULT_THUMB_WIDTH,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_THUMB_WIDTH,
                                   properties[PROP_THUMB_WIDTH]);

  properties[PROP_ACTION_LIST_WIDTH] =
    g_param_spec_int ("action-list-width",
                      "Action List Width",
                      "Width of the action list, or -1 to use the natural"
                      " width.",
                      -1, G_MAXINT, -1,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ACTION_LIST_WIDTH,
                                   properties[PROP_ACTION_LIST_WIDTH]);
}

static gboolean
mex_content_box_tile_clicked_cb (ClutterActor       *tile,
                                 ClutterButtonEvent *event,
                                 MexContentBox      *self)
{
  mex_push_focus (MX_FOCUSABLE (tile));

  mex_content_box_toggle_open (MEX_CONTENT_BOX (self));

  return TRUE;
}

static void
mex_content_box_timeline_completed (ClutterTimeline *timeline,
                                    MexContentBox   *box)
{
  MexContentBoxPrivate *priv = box->priv;

  priv->extras_visible =
    (clutter_timeline_get_direction (timeline) == CLUTTER_TIMELINE_FORWARD);

  if (!priv->extras_visible)
    {
      /* box is now "closed" */
      mx_stylable_set_style_class (MX_STYLABLE (box), "");
      g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_OPEN]);
    }
}

static void
mex_content_box_init (MexContentBox *self)
{
  MexContentBoxPrivate *priv = self->priv = CONTENT_BOX_PRIVATE (self);
  ClutterActor *icon;

  clutter_actor_push_internal (CLUTTER_ACTOR (self));

  priv->info_panel = mex_info_panel_new (MEX_INFO_PANEL_MODE_SIMPLE);
  clutter_actor_set_parent (priv->info_panel, CLUTTER_ACTOR (self));

  /* monitor key press events */
  g_signal_connect (self, "key-press-event",
                    G_CALLBACK (mex_content_box_key_press_event_cb), NULL);

  /* Create tile */
  icon = mx_icon_new ();
  priv->tile = mex_content_tile_new ();
  clutter_actor_set_parent (priv->tile, CLUTTER_ACTOR (self));
  g_object_set (G_OBJECT (priv->tile),
                "thumb-width", DEFAULT_THUMB_WIDTH,
                "thumb-height", DEFAULT_THUMB_HEIGHT,
                NULL);
  mx_stylable_set_style_class (MX_STYLABLE (icon), "Info");
  mex_tile_set_secondary_icon (MEX_TILE (priv->tile), icon);


  clutter_actor_set_reactive (priv->tile, TRUE);
  g_signal_connect (priv->tile, "button-release-event",
                    G_CALLBACK (mex_content_box_tile_clicked_cb), self);

  /* Create the action list */
  priv->action_list = mex_action_list_new ();
  clutter_actor_set_parent (priv->action_list, CLUTTER_ACTOR (self));

  clutter_actor_pop_internal (CLUTTER_ACTOR (self));


  priv->timeline = clutter_timeline_new (200);
  priv->alpha = clutter_alpha_new_full (priv->timeline, CLUTTER_EASE_OUT_CUBIC);

  g_signal_connect_swapped (priv->timeline, "new-frame",
                            G_CALLBACK (clutter_actor_queue_relayout), self);
  g_signal_connect (priv->timeline, "completed",
                    G_CALLBACK (mex_content_box_timeline_completed), self);
}

ClutterActor *
mex_content_box_new (void)
{
  return g_object_new (MEX_TYPE_CONTENT_BOX, NULL);
}

gboolean
mex_content_box_get_open (MexContentBox *box)
{
  return box->priv->extras_visible;
}

void
mex_content_box_set_important (MexContentBox *box,
                               gboolean       important)
{
  mex_tile_set_important (MEX_TILE (box->priv->tile), important);
}

gboolean
mex_content_box_get_important (MexContentBox *box)
{
  return mex_tile_get_important (MEX_TILE (box->priv->tile));
}
