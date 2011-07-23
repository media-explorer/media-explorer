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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <signal.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <grilo.h>
#include <mex/mex.h>
#ifdef USE_PLAYER_CLUTTER_GST
#include <clutter-gst/clutter-gst.h>
#endif
#include <gio/gdesktopappinfo.h>

#include <glib/gi18n.h>

#include "mex-version.h"

#if HAVE_CLUTTER_X11
#include <clutter/x11/clutter-x11.h>
#endif
#if HAVE_REBINDER
#include "rebinder.h"
#endif

#define ALPHA CLUTTER_EASE_OUT_QUAD
#define DURATION 400

#define MEX_LOG_DOMAIN_DEFAULT  main_log_domain
MEX_LOG_DOMAIN_STATIC(main_log_domain);

typedef struct
{
  MxWindow     *window;
  ClutterStage *stage;
  ClutterActor *stack;
  ClutterActor *layout;
  ClutterActor *tool_area;
  ClutterActor *explorer;
  ClutterActor *spinner;
  ClutterActor *version;
  ClutterActor *error_dialog;
  ClutterActor *error_label;
  ClutterActor *slide_show;
  ClutterActor *info_bar;
  ClutterActor *volume_control;

  ClutterActor *player;
  ClutterMedia *media;

  ClutterActor *current_actor;

  ClutterActor    *current_tool;
  MexToolProvider *current_tool_provider;

  ClutterActor *current_applet;
  ClutterActor *last_applet;

  GHashTable         *model_from_category;
  GHashTable         *model_to_provider;
  MexModel           *toplevel_model;
  MexModel           *toplevel_plugin_model;
  guint               toplevel_plugin_model_depth;
  gboolean            using_alt;
  ClutterBindingPool *bindings;

  guint hide_volume_source;

  MexContent *folder_content;
  gboolean    folder_opened;

  gint busy_count;

  guint cursor_timeout_source;
} MexData;

static void mex_show_actor (MexData *data, ClutterActor *actor);
static void mex_hide_actor (MexData *data, ClutterActor *actor);

static void mex_push_busy (MexData *data);
static void mex_pop_busy (MexData *data);

static gboolean opt_fullscreen = FALSE;
static gboolean opt_version    = FALSE;
static gboolean opt_ignore_res = FALSE;
static gboolean opt_touch      = FALSE;
static gchar **opt_file = NULL;

static void
mex_header_activated_cb (MexExplorer *explorer,
                         MexModel    *model,
                         MexData     *data);

static void
error_dialog_dismiss_action_cb (MxAction *action,
                                MexData  *data)
{
  MxFocusManager *fmanager;
  fmanager = mx_focus_manager_get_for_stage (data->stage);

  clutter_actor_hide (data->error_dialog);
  mx_focus_manager_push_focus_with_hint (fmanager,
                                         MX_FOCUSABLE (data->error_dialog),
                                         MX_FOCUS_HINT_PRIOR);
}

static void
mex_player_media_error_cb (ClutterMedia *media,
                           const GError *error,
                           MexData      *data)
{
  gchar *error_msg;
  MxAction *action;

  if (mex_player_get_idle_mode (MEX_PLAYER (data->player)))
    {
      g_warning ("Error playing content: %s", error->message);
      return;
    }

  if (!data->error_dialog)
    {
      action = mx_action_new_full ("dismiss", _("Dismiss"),
                                   (GCallback)error_dialog_dismiss_action_cb,
                                   data);
      data->error_dialog = mx_dialog_new ();
      mx_stylable_set_style_class (MX_STYLABLE (data->error_dialog),
                                   "MexErrorDialog");
      clutter_actor_set_name (data->error_dialog, "mex-player-error-dialog");
      mx_dialog_set_transient_parent (MX_DIALOG (data->error_dialog),
                                      data->stack);
      mx_dialog_add_action (MX_DIALOG (data->error_dialog), action);
    }


  error_msg = g_strdup_printf (_("Error playing content: %s"),
                                 error->message);

  if (!data->error_label)
    {
      data->error_label = mx_label_new_with_text (error_msg);
      clutter_actor_set_name (data->error_dialog, "mex-player-error-label");
      clutter_container_add_actor (CLUTTER_CONTAINER (data->error_dialog),
                                   data->error_label);
    }
  else
    mx_label_set_text (MX_LABEL (data->error_label), error_msg);

  g_free (error_msg);
  clutter_actor_show (data->error_dialog);
  mex_push_focus (MX_FOCUSABLE (data->error_dialog));
}

static void
mex_player_media_notify_buffer_fill_cb (ClutterMedia *media,
                                        GParamSpec   *pspec,
                                        MexData      *data)
{
  static gdouble prev_buffer_fill = 1.0;
  gdouble buffer_fill;

  buffer_fill = clutter_media_get_buffer_fill (media);

  if (buffer_fill == 1.0 && prev_buffer_fill < 1.0)
    mex_pop_busy (data);
  else if (buffer_fill < 1.0 && prev_buffer_fill == 1.0)
    mex_push_busy (data);

  prev_buffer_fill = buffer_fill;
}

static void
play_transition_complete (ClutterAnimation *animation,
                          ClutterActor     *actor)
{
  GObject *fade;
  MexContent *content;
  MexData *data;

  fade = clutter_animation_get_object (animation);
  content = g_object_get_data (fade, "mex-content");
  data = g_object_get_data (fade, "mex-data");

  clutter_actor_destroy (CLUTTER_ACTOR (fade));

  mex_show_actor (data, actor);
  mex_hide_actor (data, data->explorer);

  mex_content_view_set_content (MEX_CONTENT_VIEW (actor), content);
}

static void
explorer_transition_complete (ClutterAnimation *animation,
                              ClutterActor     *actor)
{
  ClutterActor *explorer;

  explorer = (ClutterActor*)clutter_animation_get_object (animation);

  clutter_actor_set_scale (explorer, 1.0, 1.0);
  clutter_actor_set_opacity (explorer, 255);
}

static void
run_play_transition (MexData      *data,
                     MexContent   *content,
                     ClutterActor *actor,
                     gboolean      fade_out)
{
  ClutterActor *fade;
  gfloat x, y, width, height, stage_width, stage_height;
  ClutterColor black = {0, 0, 0, 255};
  ClutterActor *focused, *a;
  MxFocusManager *manager;
  gint duration = 500;

  clutter_actor_get_size (CLUTTER_ACTOR (data->stage), &stage_width,
                          &stage_height);

  fade = clutter_rectangle_new_with_color (&black);
  g_object_set_data (G_OBJECT (fade), "mex-content", content);
  g_object_set_data (G_OBJECT (fade), "mex-data", data);
  clutter_actor_set_opacity (fade, 128);
  clutter_actor_set_anchor_point_from_gravity (fade, CLUTTER_GRAVITY_CENTER);
  clutter_actor_set_rotation (fade, CLUTTER_X_AXIS, -80, 0, 0, 0);

  /* find the content box that has been activated */
  manager = mx_focus_manager_get_for_stage (data->stage);
  a = (ClutterActor*) mx_focus_manager_get_focused (manager);
  while (a)
    {
      if (MEX_IS_CONTENT_BOX (a))
        break;
      a = clutter_actor_get_parent (a);
    }

  if (a)
    {
      clutter_actor_get_transformed_position (a, &x, &y);
      clutter_actor_get_size (CLUTTER_ACTOR (a), &width, &height);

      clutter_actor_set_position (fade, x + width / 2, y + height / 2);
      clutter_actor_set_size (CLUTTER_ACTOR (fade), width, height);
    }
  else
    {
      /* show the actor immediately if no content box was found */
      mex_hide_actor (data, data->explorer);
      mex_show_actor (data, actor);
      mex_content_view_set_content (MEX_CONTENT_VIEW (actor), content);

      clutter_actor_destroy (fade);

      return;
    }

  clutter_container_add (CLUTTER_CONTAINER (data->stage), fade, NULL);
  clutter_actor_raise_top (fade);

  g_object_set (data->explorer, "scale-gravity", CLUTTER_GRAVITY_CENTER, NULL);
  clutter_actor_animate (data->explorer, CLUTTER_EASE_IN_OUT_CUBIC, duration,
                         "scale-x", 0.9, "scale-y", 0.9,
                         "signal-after::completed",
                         explorer_transition_complete, actor,
                         "opacity", 128, NULL);

  clutter_actor_animate (fade, CLUTTER_EASE_OUT_CUBIC, duration,
                         "opacity", 255,
                         "x", stage_width / 2.0,
                         "y", stage_height / 2.0,
                         "width", stage_width,
                         "height", stage_height,
                         "rotation-angle-x", (gdouble) 0,
                         "signal-after::completed", play_transition_complete,
                         actor,
                         NULL);
}

static void
mex_play_from_last_cb (MxAction *action, MexData *data)
{
  MexContent *content = mex_action_get_content (action);
  MexModel *model = mex_action_get_context (action);

  if (!content)
    {
      g_warning (G_STRLOC ": Play initiated on action with "
                 "no associated content");
      return;
    }

  g_object_set (content, "last-position-start", TRUE, NULL);
  mex_content_view_set_context (MEX_CONTENT_VIEW (data->player), model);

  /* hide other actors */
  mex_hide_actor (data, data->slide_show);

  /* show the video player */
  run_play_transition (data, content, data->player, TRUE);
}

static void
mex_play_from_begin_cb (MxAction *action, MexData *data)
{
  MexContent *content = mex_action_get_content (action);
  MexModel *model = mex_action_get_context (action);

  if (!content)
    {
      g_warning (G_STRLOC ": Play initiated on action with "
                 "no associated content");
      return;
    }

  g_object_set (content, "last-position-start", FALSE, NULL);
  mex_content_view_set_context (MEX_CONTENT_VIEW (data->player), model);

  /* hide other actors */
  mex_hide_actor (data, data->slide_show);

  /* show the video player */
  run_play_transition (data, content, data->player, TRUE);
}

static void
mex_player_content_set_externally_cb (MexData *data)
{
  /* hide other actors */
  mex_hide_actor (data, data->explorer);
  mex_hide_actor (data, data->slide_show);

  /* show the video player */
  mex_show_actor (data, data->player);
}

static void
mex_show_cb (MxAction *action, MexData *data)
{
  MexContent *content = mex_action_get_content (action);
  MexModel *model = mex_action_get_context (action);

  if (!content)
    {
      g_warning (G_STRLOC ": Play initiated on action with "
                 "no associated content");
      return;
    }

  /* start the slide show */
  mex_content_view_set_context (MEX_CONTENT_VIEW (data->slide_show), model);

  /* hide other actors */
  mex_hide_actor (data, data->player);

  /* stop any playing video, even in idle mode */
  mex_player_set_idle_mode (MEX_PLAYER (data->player), FALSE);

  /* show the slide show */
  run_play_transition (data, content, data->slide_show, FALSE);
}

static gboolean
mex_is_toplevel_plugin_model (MexData  *data,
                              MexModel *model)
{
  if (g_hash_table_lookup (data->model_to_provider, model))
    return TRUE;
  else
    return FALSE;
}

static const MexModelInfo *
mex_get_toplevel_model_info (MexData  *data,
                             MexModel *model)
{
  GList *m, *models;

  const MexModelInfo *info = NULL;
  MexModelManager *manager = mex_model_manager_get_default ();

  if (model && MEX_IS_PROXY_MODEL (model))
    model = mex_proxy_model_get_model (MEX_PROXY_MODEL (model));

  if (model && (info = mex_model_manager_get_model_info (manager, model)))
    return info;

  models = mex_explorer_get_models (MEX_EXPLORER (data->explorer));

  for (m = models; m; m = m->next)
    {
      MexModel *child_model = mex_model_get_model (m->data);
      const MexModelInfo *m_info =
        mex_model_manager_get_model_info (manager, child_model);
      if (m_info)
        info = m_info;
    }

  g_list_free (models);

  if (!info && data->toplevel_plugin_model)
    info = mex_model_manager_get_model_info (manager,
                                             data->toplevel_plugin_model);

  return info;
}

static void
mex_grilo_open_folder_cb (MxAction *action,
                          MexData  *data)
{
  GrlMedia *media;
  MexFeed *feed;
  GrlMediaSource *source = NULL;
  gchar *filter = NULL;
  MexModelInfo *model_info;
  const MexModelInfo *parent_model_info;
  MexModelManager *manager;

  MexProgram *program = MEX_PROGRAM (mex_action_get_content (action));

  feed = mex_program_get_feed (program);

  if (mex_is_toplevel_plugin_model (data, MEX_MODEL (feed)))
    {
      data->toplevel_plugin_model = mex_model_get_model (MEX_MODEL (feed));
      data->toplevel_plugin_model_depth =
        mex_explorer_get_depth (MEX_EXPLORER (data->explorer));
    }

  g_object_get (G_OBJECT (feed), "grilo-source", &source, NULL);

  if (MEX_IS_GRILO_TRACKER_FEED (feed))
      g_object_get (G_OBJECT (feed), "tracker-filter", &filter, NULL);

  media = mex_grilo_program_get_grilo_media (MEX_GRILO_PROGRAM (program));

  /* copy the "parent" model's info */
  manager = mex_model_manager_get_default ();
  parent_model_info = mex_model_manager_get_model_info (manager, MEX_MODEL (feed));
  model_info = mex_model_info_copy (parent_model_info);

  /* check if the model was the alt_model, and set alt_model of the new model
   * accordingly */
  if (parent_model_info->alt_model == MEX_MODEL (feed))
    {
      model_info->alt_model = parent_model_info->model;
      model_info->alt_model_active = TRUE;
    }

  /* FIXME: if only we had g_object_clone().. */

  if (filter)
    {
      feed = g_object_new (G_OBJECT_TYPE (feed),
                         "grilo-source", source,
                         "grilo-box", media,
                         "tracker-filter", filter,
                         NULL);
      g_free (filter);
    }
  else
    {
      feed = g_object_new (G_OBJECT_TYPE (feed),
                         "grilo-source", source,
                         "grilo-box", media,
                         NULL);
    }

  /* register the new model with the model manager */
  model_info->alt_model = MEX_MODEL (feed);
  mex_model_manager_add_model (manager, model_info);
  mex_model_info_free (model_info);

  if (source)
    g_object_unref (source);

  data->folder_opened = TRUE;

  mex_grilo_feed_browse (MEX_GRILO_FEED (feed), 0, G_MAXINT);
  mex_header_activated_cb (MEX_EXPLORER (data->explorer),
                           MEX_MODEL (feed), data);
}

static void
mex_header_activated_cb (MexExplorer *explorer,
                         MexModel    *model,
                         MexData     *data)
{
  MexModel *current_model;
  MexModelProvider *plugin;

  current_model = mex_explorer_get_model (explorer);

  if (current_model == mex_explorer_get_root_model (explorer))
    data->toplevel_model = mex_explorer_get_focused_model (explorer);
  else if (mex_is_toplevel_plugin_model (data, model))
    {
      data->toplevel_plugin_model = mex_model_get_model (model);
      data->toplevel_plugin_model_depth =
        mex_explorer_get_depth (MEX_EXPLORER (data->explorer));
    }

  /* Let plugins override the default behaviour of activating models */
  plugin = g_hash_table_lookup (data->model_to_provider, model);
  if (plugin && mex_model_provider_model_activated (plugin, model))
    return;

  /* If the aggregate model only houses one model, push that model instead */
  if (MEX_IS_AGGREGATE_MODEL (model))
    {
      const GList *models =
        mex_aggregate_model_get_models (MEX_AGGREGATE_MODEL (model));

      /* only one model */
      if (models && !models->next)
        {
          mex_header_activated_cb (explorer, models->data, data);
          return;
        }

      if (models)
        mex_explorer_push_model (explorer, g_object_ref (model));
    }
  else
    {
      /* For non-aggregate models, you get a grid representation, which we
       * pack sorting chrome around. Use a proxy model so that when we change
       * the sort function of the model, we don't affect views on other pages.
       */
      MexModel *new_model = mex_proxy_model_new ();
      mex_proxy_model_set_model (MEX_PROXY_MODEL (new_model), model);
      mex_explorer_push_model (explorer, new_model);
    }
}


static void
mex_container_repopulating_cb (MxActorManager *manager,
                               gulong          id,
                               ClutterActor   *container,
                               ClutterActor   *actor,
                               ClutterActor   *our_container)
{
  /* Fade in the container now it's repopulating */
  if (container == our_container)
    {
      ClutterActor *parent = clutter_actor_get_parent (container);

      clutter_actor_set_opacity (container, 0x00);
      clutter_actor_show (container);
      clutter_actor_animate (container,
                             CLUTTER_EASE_OUT_CUBIC, 250,
                             "opacity", 0xff,
                             NULL);

      if (MEX_IS_SCROLL_VIEW (parent))
        mex_scroll_view_set_indicators_hidden (MEX_SCROLL_VIEW (parent), FALSE);

      g_signal_handlers_disconnect_by_func (manager,
                                            mex_container_repopulating_cb,
                                            container);
    }
}

static void
mex_hide_repopulating_container (ClutterActor *container)
{
  ClutterActor *parent = clutter_actor_get_parent (container);
  ClutterActor *stage = clutter_actor_get_stage (container);
  MxActorManager *manager =
    mx_actor_manager_get_for_stage (CLUTTER_STAGE (stage));

  g_signal_connect (manager, "actor-added",
                    G_CALLBACK (mex_container_repopulating_cb), container);

  clutter_actor_hide (container);

  if (MEX_IS_SCROLL_VIEW (parent))
    mex_scroll_view_set_indicators_hidden (MEX_SCROLL_VIEW (parent), TRUE);
}

static void
mex_notify_depth_cb (MexExplorer *explorer,
                     GParamSpec  *pspec,
                     MexData     *data)
{
  guint depth;
  MexModel *model;
  ClutterContainer *container;

  depth = mex_explorer_get_depth (explorer);

  if (depth == 2)
    mex_info_bar_show_buttons (MEX_INFO_BAR (data->info_bar), FALSE);
  if (depth == 1)
    mex_info_bar_show_buttons (MEX_INFO_BAR (data->info_bar), TRUE);

  if (data->toplevel_plugin_model &&
      (mex_explorer_get_depth (explorer) <= data->toplevel_plugin_model_depth))
    data->toplevel_plugin_model = NULL;

  /* Set the sort function for non-grid containers */
  model = mex_explorer_get_model (explorer);
  container = mex_explorer_get_container_for_model (explorer, model);

  if (container && !MEX_IS_GRID_VIEW (container))
    {
      data->using_alt = FALSE;
      mex_model_set_sort_func (model, mex_model_sort_smart_cb,
                               GINT_TO_POINTER (FALSE));
    }
}

static void
mex_show_actor (MexData      *data,
                ClutterActor *actor)
{
  /* make the actor visible */
  clutter_actor_show (actor);

  /* enable the actor */
  if (MX_IS_WIDGET (actor))
    mx_widget_set_disabled (MX_WIDGET (actor), FALSE);

  /* make sure the player is disabled if it is not the current widget */
  if (actor != data->player)
      mx_widget_set_disabled (MX_WIDGET (data->player), TRUE);

  if (actor == data->explorer)
    {
      /* main menu view */
      mx_widget_set_disabled (MX_WIDGET (data->layout), FALSE);
      clutter_actor_set_opacity (data->layout, 0xff);
    }
  else if (actor == data->player)
    {
      mex_player_set_idle_mode (MEX_PLAYER (data->player), FALSE);
    }
  else
    clutter_actor_animate (actor, ALPHA, DURATION,
                           "opacity", 0xff,
                           NULL);

  /* give the actor key focus */
  mex_push_focus (MX_FOCUSABLE (actor));

  /* raise the actor above any others (e.g. the player) */
  clutter_actor_raise_top (actor);

  /* keep track of the current actor */
  data->current_actor = actor;
}

static void
mex_hide_actor (MexData      *data,
                ClutterActor *actor)
{
  if (MX_IS_WIDGET (actor))
    mx_widget_set_disabled (MX_WIDGET (actor), TRUE);

  if (actor == data->explorer)
    {
      /* main menu view */
      mx_widget_set_disabled (MX_WIDGET (data->layout), TRUE);
      clutter_actor_set_opacity (data->layout, 0);
    }
  else if (actor == data->player)
    {
      /* don't hide the player, just lower it and set idle mode */
      mex_player_set_idle_mode (MEX_PLAYER (data->player), TRUE);
      clutter_actor_lower_bottom (data->player);
    }
  else
    clutter_actor_hide (actor);
}


static void
mex_show_home_screen (MexData *data)
{
  MexModel *model;

  /* hide the slide show */
  mex_hide_actor (data, data->slide_show);

  /* hide the video player */
  mex_hide_actor (data, data->player);

  /* hide current tool */
  if (data->current_tool)
    mex_hide_actor (data, data->current_tool);

  /* show the home screen (explorer) */
  mex_show_actor (data, data->explorer);

  /* hide any dialogs created by the info-bar */
  if (mex_info_bar_dialog_visible (MEX_INFO_BAR (data->info_bar)))
    mex_info_bar_close_dialog (MEX_INFO_BAR (data->info_bar));

  /* go back to the root model */
  mex_explorer_pop_to_root (MEX_EXPLORER (data->explorer));

  model = g_hash_table_lookup (data->model_from_category, "videos");

  if (!model)
    g_warning ("Could not find videos model");

  mex_explorer_set_focused_model (MEX_EXPLORER (data->explorer), model);
}

void
mex_go_back_transition_complete (ClutterAnimation *animation,
                gpointer *data)
{
  ClutterActor *actor = (ClutterActor*) clutter_animation_get_object (animation);
  clutter_actor_destroy (actor);
}
void
mex_go_back_transition_first_stage (ClutterAnimation *animation,
                MexData *data)
{
  MxFocusManager *manager;
  ClutterActor *actor;
  ClutterActor *focused;
  gfloat x, y, width, height;

  actor = (ClutterActor*) clutter_animation_get_object (animation);

  /* hide the slide show */
  mex_hide_actor (data, data->slide_show);

  /* hide the video player */
  mex_hide_actor (data, data->player);

  /* hide any tools */
  if (data->current_tool)
    mex_hide_actor (data, data->current_tool);

  /* show main layout */
  mex_show_actor (data, data->explorer);


  /* ensure the rectangle is still above everything else */
  clutter_actor_raise_top (actor);

  manager = mx_focus_manager_get_for_stage (data->stage);
  focused = (ClutterActor*) mx_focus_manager_get_focused (manager);

  /* find the center of the focused actor */
  if (focused)
    {
      clutter_actor_get_transformed_position (focused, &x, &y);
      clutter_actor_get_size (focused, &width, &height);

      x = x + width / 2.0;
      y = y + height / 2.0;

      clutter_actor_animate (actor, CLUTTER_EASE_IN_SINE, 500,
                             "width", 200.f,
                             "height", 150.f,
                             "x", x,
                             "y", y,
                             "opacity", (guchar) 60,
                             "rotation-angle-x", (gdouble) -90,
                             "signal-after::completed",
                             mex_go_back_transition_complete, NULL,
                             NULL);
    }
  else
    {
      /* simply fade in the menu if there was no focused actor */

      clutter_actor_animate (actor, CLUTTER_EASE_IN_SINE, 500,
                             "opacity", (guchar) 0,
                             "signal-after::completed",
                             mex_go_back_transition_complete, NULL,
                             NULL);

    }
}

static void
mex_go_back (MexData *data)
{
  MexContent *content;
  ClutterActor *transition;
  gfloat width, height;


  /* if the current actor is the explorer, try to move up the hierarchy */
  if (clutter_actor_get_opacity (data->layout) == 0xff)
    {
      /* hide any dialogs created by the info-bar */
      if (mex_info_bar_dialog_visible (MEX_INFO_BAR (data->info_bar)))
        mex_info_bar_close_dialog (MEX_INFO_BAR (data->info_bar));

      if (mex_explorer_get_root_model (MEX_EXPLORER (data->explorer))
          != mex_explorer_get_model (MEX_EXPLORER (data->explorer)))
        {
          mex_explorer_pop_model (MEX_EXPLORER (data->explorer));
          return;
        }
    }
  else
    {
      /* move explorer to current content */
      if (data->current_actor == data->slide_show)
        content = mex_content_view_get_content (MEX_CONTENT_VIEW (data->slide_show));
      else if (data->current_actor == data->player)
        content = mex_content_view_get_content (MEX_CONTENT_VIEW (data->player));
      else
        content = NULL;

      if (content)
        mex_explorer_focus_content (MEX_EXPLORER (data->explorer),
                                    content);

      transition = clutter_rectangle_new_with_color (CLUTTER_COLOR_Black);
      clutter_actor_get_size (CLUTTER_ACTOR (data->stage), &width,
                              &height);
      clutter_actor_set_anchor_point_from_gravity (transition,
                                                   CLUTTER_GRAVITY_CENTER);
      clutter_actor_set_size (transition, width, height);
      clutter_actor_set_position (transition, width / 2, height / 2);
      clutter_actor_set_opacity (transition, 0);

      clutter_container_add_actor (CLUTTER_CONTAINER (data->stage),
                                   transition);
      clutter_actor_raise_top (transition);

      clutter_actor_animate (transition, CLUTTER_EASE_OUT_CUBIC, 200,
                             "opacity", (guchar) 255,
                             "signal-after::completed",
                             mex_go_back_transition_first_stage, data,
                             NULL);
    }
}

static gboolean
mex_hide_volume_control (MexData *data)
{
  mex_info_bar_show_notifications (MEX_INFO_BAR (data->info_bar), TRUE);

  clutter_actor_animate (data->volume_control, ALPHA, DURATION,
                         "opacity", 0x00, NULL);
  return FALSE;
}

static void
mex_show_volume_control (MexData *data)
{
  mex_info_bar_show_notifications (MEX_INFO_BAR (data->info_bar), FALSE);

  clutter_actor_raise_top (data->volume_control);

  clutter_actor_animate (data->volume_control, ALPHA, DURATION,
                         "opacity", 0xff, NULL);

  if (data->hide_volume_source)
    g_source_remove (data->hide_volume_source);

  data->hide_volume_source =
    g_timeout_add_seconds (2, (GSourceFunc) mex_hide_volume_control, data);
}

static void
on_volume_changed (ClutterActor *volume,
                   GParamSpec   *pspec,
                   MexData      *data)
{
  mex_show_volume_control (data);
}

static void
mex_enable_touch_events (MexData  *data,
                         gboolean  on)
{
  MxSettings *settings;

  opt_touch = on;
  mex_explorer_set_touch_mode (MEX_EXPLORER (data->explorer), on);

  /* FIXME: This value is arbitrary and should be set by the platform */
  if (on)
    {
      settings = mx_settings_get_default ();
      g_object_set (G_OBJECT (settings), "drag-threshold", (guint)10, NULL);
    }
}

static gboolean
hide_cursor (gpointer user_data)
{
  MexData *data = user_data;

  clutter_stage_hide_cursor (data->stage);
  data->cursor_timeout_source = 0;

  return FALSE;
}

static gboolean
mex_captured_event_cb (ClutterActor *actor,
                       ClutterEvent *event,
                       MexData      *data)
{
  gboolean handled, fullscreen;
  ClutterKeyEvent *key_event;

  /* Motion events are used to show/hide the cursor when the application is
   * full-screened */
  if (event->type == CLUTTER_MOTION)
    {
      gboolean fullscreen, cursor_visible;

      fullscreen = clutter_stage_get_fullscreen (data->stage);
      g_object_get (data->stage, "cursor-visible", &cursor_visible, NULL);
      if (fullscreen && !cursor_visible)
        {
          clutter_stage_show_cursor (data->stage);

          if (data->cursor_timeout_source)
            g_source_remove (data->cursor_timeout_source);
          data->cursor_timeout_source = g_timeout_add (2000, hide_cursor, data);
        }
    }


  /* Don't respond to any mouse events until we are in "touch mode". This mode
   * is triggered by a BUTTON_RELEASE event. */
  if (!opt_touch)
    {
      switch (event->type)
        {
        case CLUTTER_BUTTON_PRESS:
        case CLUTTER_BUTTON_RELEASE:
          mex_enable_touch_events (data, TRUE);
          break;

        case CLUTTER_ENTER:
        case CLUTTER_LEAVE:
        case CLUTTER_MOTION:
        case CLUTTER_SCROLL:
          return TRUE;
        default:
          break;
        }
    }

  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;

  /* Prevent bringing up "interupting" UI if the error dialog is present */
  if (data->error_dialog && CLUTTER_ACTOR_IS_VISIBLE (data->error_dialog))
    return FALSE;

  key_event = (ClutterKeyEvent *)event;

  /* Check for keys that should always work first */
  switch (key_event->keyval)
    {
    case CLUTTER_KEY_Left :
    case CLUTTER_KEY_Right :
    case CLUTTER_KEY_Up :
    case CLUTTER_KEY_Down :
      mex_enable_touch_events (data, FALSE);
      break;

    case MEX_KEY_HOME :
      /* stop all playing content and show the home screen */
      mex_show_home_screen (data);
      return TRUE;

    case MEX_KEY_BACK :
      /* move back */
      mex_go_back (data);
      return TRUE;

    case CLUTTER_KEY_AudioRaiseVolume :
      mex_volume_control_volume_up (MEX_VOLUME_CONTROL (data->volume_control));
      mex_show_volume_control (data);
      return TRUE;

    case CLUTTER_KEY_AudioLowerVolume :
      mex_volume_control_volume_down (MEX_VOLUME_CONTROL (data->volume_control));
      mex_show_volume_control (data);
      return TRUE;

    case CLUTTER_KEY_AudioMute :
      mex_volume_control_volume_mute (MEX_VOLUME_CONTROL (data->volume_control));
      mex_show_volume_control (data);
      return TRUE;

      /* The play button more often than not is a play/pause button, there is
       * no way of telling which it is so for now so I'm going to say the play
       * button is a play pause.
       */
    case CLUTTER_KEY_AudioPlay:
      if (clutter_media_get_playing (data->media))
        mex_player_pause (MEX_PLAYER (data->player));
      else
        mex_player_play (MEX_PLAYER (data->player));
      return TRUE;

    case CLUTTER_KEY_AudioPause:
      mex_player_pause (MEX_PLAYER (data->player));
      return TRUE;

    case CLUTTER_KEY_AudioStop:
      mex_player_stop (MEX_PLAYER (data->player));
      return TRUE;

    case CLUTTER_KEY_AudioRewind:
      mex_player_rewind (MEX_PLAYER (data->player));
      return TRUE;

    case CLUTTER_KEY_AudioForward:
      mex_player_forward (MEX_PLAYER (data->player));
      return TRUE;

    case CLUTTER_KEY_AudioNext:
      mex_player_next (MEX_PLAYER (data->player));
      return TRUE;

    case CLUTTER_KEY_AudioPrev:
      mex_player_previous (MEX_PLAYER (data->player));
      return TRUE;

    case CLUTTER_KEY_F11 :
      mex_toggle_fullscreen ();
      return TRUE;
    }

  /* Handle the rest of the shortcuts/bindings */
  handled = clutter_binding_pool_activate (data->bindings,
                                           key_event->keyval,
                                           key_event->modifier_state,
                                           G_OBJECT (actor));

  return handled;
}

static gboolean
mex_event_cb (ClutterActor *actor,
              ClutterEvent *event,
              MexData      *data)
{
  ClutterKeyEvent *key_event;

  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;

  key_event = (ClutterKeyEvent *)event;

  switch (key_event->keyval)
    {
    case CLUTTER_KEY_f:
      mex_toggle_fullscreen ();
      return TRUE;
    }

  return FALSE;
}

static void
on_stage_actived (ClutterStage *stage,
                  MexData      *data)
{
  mex_mmkeys_grab_keys (mex_mmkeys_get_default ());
}

static void
on_stage_deactived (ClutterStage *stage,
                    MexData      *data)
{
  mex_mmkeys_ungrab_keys (mex_mmkeys_get_default ());
}

static void
on_fullscreen_set (ClutterStage *stage,
                   GParamSpec   *pspec,
                   MexData      *data)
{
  gboolean fullscreen;

  g_object_get (stage, "fullscreen-set", &fullscreen, NULL);

  if (fullscreen)
    {
      clutter_stage_hide_cursor (stage);
    }
  else
    {
      /* if we have time out to hide the cursor in fullscreen, disable it */
      if (data->cursor_timeout_source)
        {
          g_source_remove (data->cursor_timeout_source);
          data->cursor_timeout_source = 0;
        }

      clutter_stage_show_cursor (stage);
    }
}

static void
mex_remove_model_provider_cb (MexData *data,
                              GObject *old_object)
{
  g_hash_table_remove (data->model_to_provider, old_object);
}

static void
mex_remove_plugin_model_info_cb (MexModelInfo *info)
{
  mex_model_manager_remove_model (mex_model_manager_get_default (),
                                  info->model);
  mex_model_info_free (info);
}

static void
mex_plugin_present_model_cb (GObject      *plugin,
                             MexModelInfo *info,
                             MexData      *data)
{
  MexExplorer *explorer = MEX_EXPLORER (data->explorer);

  /* Add the model to the model manager and set the top-level plugin model */
  mex_model_manager_add_model (mex_model_manager_get_default (), info);

  /* Activate the model */
  mex_header_activated_cb (explorer, info->model, data);
  mex_show_actor (data, data->explorer);

  /* Use a weak reference on the created page to remove the model from the
   * model manager.
   */
  g_object_weak_ref (G_OBJECT (mex_explorer_get_container_for_model (
                                 explorer, mex_explorer_get_model (explorer))),
                     (GWeakNotify)mex_remove_plugin_model_info_cb,
                     mex_model_info_copy (info));
}

static void
mex_refresh_root_model (MexData *data)
{
  MexModel *root;
  GList *c, *categories;
  MexModelManager *manager;

  root = mex_explorer_get_root_model (MEX_EXPLORER (data->explorer));
  mex_aggregate_model_clear (MEX_AGGREGATE_MODEL (root));
  g_hash_table_remove_all (data->model_from_category);

  manager = mex_model_manager_get_default ();

  categories = mex_model_manager_get_categories (manager);
  for (c = categories; c; c = c->next)
    {
      GList *m, *models;
      MexModel *aggregate;
      MexModelCategoryInfo *c_info = c->data;

      /* Create a new aggregate model for this category */
      aggregate = mex_aggregate_model_new ();
      mex_model_set_sort_func (MEX_MODEL (aggregate),
                               mex_model_sort_smart_cb,
                               GINT_TO_POINTER (FALSE));

      /* prevent the length display in the search column */
      if (!g_strcmp0 (c_info->name, "search"))
        g_object_set (aggregate, "display-item-count", FALSE, NULL);

      g_object_set (G_OBJECT (aggregate),
                    "title", c_info->display_name,
                    "icon-name", c_info->icon_name,
                    "placeholder-text", c_info->placeholder_text, NULL);
      g_hash_table_insert (data->model_from_category,
                           g_strdup (c_info->name), aggregate);
      mex_aggregate_model_add_model (MEX_AGGREGATE_MODEL (root), aggregate);

      /* Add appropriate models to this category */
      models = mex_model_manager_get_models_for_category (manager,
                                                          c_info->name);


      for (m = models; m; m = m->next)
        {
          mex_aggregate_model_add_model (MEX_AGGREGATE_MODEL (aggregate),
                                         g_object_ref (m->data));
        }

      g_list_free (models);
    }
  g_list_free (categories);
}

static void
mex_model_added_cb (MexModelManager    *manager,
                    const MexModelInfo *info,
                    MexData            *data)
{
  /* FIXME: We don't yet support re-ordering in MexAggregateModel */
  MexModel *aggregate = g_hash_table_lookup (data->model_from_category,
                                             info->category);

  if (aggregate)
    {
      mex_aggregate_model_add_model (MEX_AGGREGATE_MODEL (aggregate),
                                     info->model);

      if (MEX_IS_MODEL_PROVIDER (info->userdata))
        {
          /* Add a mapping from the model back to the provider */
          g_hash_table_insert (data->model_to_provider, info->model, info->userdata);
          g_object_weak_ref (G_OBJECT (info->model),
                             (GWeakNotify)mex_remove_model_provider_cb,
                             data);
        }
    }
}

static void
mex_model_removed_cb (MexModelManager *manager,
                      MexModel        *model,
                      const gchar     *category,
                      MexData         *data)
{
  MexModel *aggregate = g_hash_table_lookup (data->model_from_category,
                                             category);

  if (aggregate)
    mex_aggregate_model_remove_model (MEX_AGGREGATE_MODEL (aggregate), model);
}

static void
mex_categories_changed_cb (MexModelManager *manager,
                           MexData         *data)
{
  mex_refresh_root_model (data);
}

static void
mex_dialog_hidden_cb (ClutterActor *dialog,
                      GParamSpec   *pspec,
                      ClutterActor *actor)
{
  MexApplet *applet = g_object_get_data (G_OBJECT (actor), "mex-applet");

  mx_bin_set_child (MX_BIN (dialog), NULL);
  clutter_actor_destroy (dialog);

  mex_applet_closed (applet, actor);
}


static void
mex_applet_request_close_cb (MexApplet    *applet,
                             ClutterActor *actor,
                             MexData      *data)
{
  ClutterActor *parent = clutter_actor_get_parent (actor);

  if (MX_IS_DIALOG (parent))
    {
      if (CLUTTER_ACTOR_IS_VISIBLE (parent))
        {
          mex_hide_actor (data, parent);
          g_signal_connect (parent, "notify::visible",
                            G_CALLBACK (mex_dialog_hidden_cb), actor);
        }
      return;
    }

  if (data->current_applet == actor)
    data->current_applet = NULL;
  else
    g_warning (G_STRLOC ": An applet has requested the closing of an "
               "unknown actor.");

  mex_hide_actor (data, actor);
}

static void
mex_applet_present_actor_cb (MexApplet                  *applet,
                             MexAppletPresentationFlags  flags,
                             ClutterActor               *actor,
                             MexData                    *data)
{
  if (!actor)
    return;

  g_object_set_data (G_OBJECT (actor), "mex-applet", applet);

  /* Dialogs are a bit different to normal applets, as they don't
   * displace anything.
   */
  if (flags & MEX_APPLET_PRESENT_DIALOG)
    {
      ClutterActor *dialog = mx_dialog_new ();

      mx_dialog_set_transient_parent (MX_DIALOG (dialog),
                                      data->stack);
      mx_bin_set_child (MX_BIN (dialog), actor);

      clutter_actor_show (dialog);

      return;
    }

  /* TODO: Handle the opaque flag by stopping playback and fading out
   *       the video-texture.
   */


  data->current_applet = actor;
  clutter_actor_set_opacity (actor, 0x00);

  if (!clutter_actor_get_parent (actor))
    clutter_container_add_actor (CLUTTER_CONTAINER (data->stack), actor);

  mex_show_actor (data, actor);
}

/*
 * Tool
 */
static void
mex_tool_remove_actor_cb (MexToolProvider *provider,
                          ClutterActor    *actor,
                          MexData         *data)
{
  if (data->current_tool_provider == provider && data->current_tool == actor)
    {
      data->current_tool = NULL;
      data->current_tool_provider = NULL;
    }
  else
    {
      g_warning (G_STRLOC ": Receveived a 'remove-actor' signal on an unkown "
                 "tool/provider");
    }

  mex_hide_actor (data, actor);
}

static void
mex_tool_present_actor_cb (MexToolProvider *provider,
                           ClutterActor    *actor,
                           MexData         *data)
{
  if (G_UNLIKELY (actor == NULL))
    return;

  if (data->current_tool)
    {
      if (data->current_tool != actor)
        mex_hide_actor (data, data->current_tool);

      data->current_tool = NULL;
      data->current_tool_provider = NULL;
    }

  data->current_tool_provider = provider;

  data->current_tool = actor;
  clutter_actor_set_opacity (actor, 0x00);

  if (!clutter_actor_get_parent (actor))
    clutter_container_add_actor (CLUTTER_CONTAINER (data->stack), actor);

  mex_show_actor (data, actor);

  /* hide other actors */
  mex_hide_actor (data, data->explorer);
  mex_hide_actor (data, data->slide_show);
  mex_hide_actor (data, data->player);
}

static void
mex_plugin_loaded_cb (MexPluginManager *plugin_manager,
                      GObject          *plugin,
                      MexData          *data)
{
  if (MEX_IS_MODEL_PROVIDER (plugin))
    {
      GList *m;
      MexModelManager *manager = mex_model_manager_get_default ();
      const GList *models =
        mex_model_provider_get_models (MEX_MODEL_PROVIDER (plugin));

      for (m = (GList *)models; m; m = m->next)
        {
          MexModelInfo *info = m->data;
          mex_model_manager_add_model (manager, info);

          /* Add a mapping from the model back to the provider */
          g_hash_table_insert (data->model_to_provider, info->model, plugin);
          g_object_weak_ref (G_OBJECT (info->model),
                             (GWeakNotify)mex_remove_model_provider_cb,
                             data);
        }

      g_signal_connect (plugin, "present-model",
                        G_CALLBACK (mex_plugin_present_model_cb), data);
    }

  if (MEX_IS_TOOL_PROVIDER (plugin))
    {
      GList *t, *k;
      const GList *tools =
        mex_tool_provider_get_tools (MEX_TOOL_PROVIDER (plugin));
      const GList *keys =
        mex_tool_provider_get_bindings (MEX_TOOL_PROVIDER (plugin));

      for (t = (GList *)tools; t; t = t->next)
        {
          ClutterActor *tool = t->data;
          clutter_container_add_actor (CLUTTER_CONTAINER (data->tool_area),
                                       tool);
        }

      g_signal_connect (plugin, "present-actor",
                        G_CALLBACK (mex_tool_present_actor_cb), data);
      g_signal_connect (plugin, "remove-actor",
                        G_CALLBACK (mex_tool_remove_actor_cb), data);

      for (k = (GList *)keys; k; k = k->next)
        {
          MexToolProviderBinding *binding = k->data;
          clutter_binding_pool_install_action (data->bindings,
                                               binding->action_name,
                                               binding->key_val,
                                               binding->modifiers,
                                               binding->callback,
                                               binding->data,
                                               binding->notify);
        }

    }

  if (MEX_IS_ACTION_PROVIDER (plugin))
    {
      GList *a;
      MexActionManager *manager = mex_action_manager_get_default ();
      const GList *actions =
        mex_action_provider_get_actions (MEX_ACTION_PROVIDER (plugin));

      for (a = (GList *)actions; a; a = a->next)
        {
          MexActionInfo *info = a->data;
          g_object_set_data (G_OBJECT (info->action), "mex-data", data);
          mex_action_manager_add_action (manager, info);
        }
    }

  if (MEX_IS_APPLET_PROVIDER (plugin))
    {
      GList *a;
      MexAppletManager *manager = mex_applet_manager_get_default ();
      const GList *applets =
        mex_applet_provider_get_applets (MEX_APPLET_PROVIDER (plugin));

      for (a = (GList *)applets; a; a = a->next)
        {
          mex_applet_manager_add_applet (manager, MEX_APPLET (a->data));
          g_signal_connect (a->data, "request-close",
                            G_CALLBACK (mex_applet_request_close_cb), data);
          g_signal_connect (a->data, "present-actor",
                            G_CALLBACK (mex_applet_present_actor_cb), data);
        }
    }
}

static void
mex_notify_download_queue_length_cb (MexDownloadQueue *queue,
                                     GParamSpec       *pspec,
                                     MexData          *data)
{
  static gint prev_length = 0;
  gint cur_length;

  cur_length = mex_download_queue_get_queue_length (queue);

  if (cur_length > 0 && prev_length == 0)
    mex_push_busy (data);
  else if (cur_length == 0 && prev_length > 0)
    mex_pop_busy (data);

  prev_length = cur_length;
}

static void
mex_spinner_faded_cb (MxSpinner *spinner)
{
  mx_spinner_set_animating (spinner, FALSE);
}

static void
mex_pop_busy (MexData *data)
{
  g_return_if_fail (data->busy_count > 0);

  data->busy_count--;

  if (data->busy_count == 0)
    {
      clutter_actor_animate (data->spinner, CLUTTER_EASE_OUT_QUAD, 250,
                             "opacity", 0x00,
                             "signal-swapped::completed",
                               mex_spinner_faded_cb,
                               data->spinner,
                             NULL);
    }
}

static void
mex_push_busy (MexData *data)
{
  data->busy_count++;

  if (data->busy_count == 1)
    {
      clutter_actor_detach_animation (data->spinner);
      mx_spinner_set_animating (MX_SPINNER (data->spinner), TRUE);

      clutter_actor_raise_top (data->spinner);

      clutter_actor_animate (data->spinner, CLUTTER_EASE_OUT_QUAD, 250,
                             "opacity", 0xff,
                             NULL);
    }
}

static void
start_service_reply (DBusGProxy *proxy,
                     guint       success,
                     GError     *error,
                     gpointer    user_data)
{
  if (error)
    {
      g_warning ("org.freedesktop.DBus.StartServiceByName() failed: %s",
                 error->message);
      return;
    }
}

static void
auto_start_dbus_service (const gchar *name)
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (connection == NULL) {
    g_warning ("Failed to open connection to DBus: %s", error->message);
    g_error_free (error);
    return;
  }

  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  org_freedesktop_DBus_start_service_by_name_async (proxy,
                                                    name, 0,
                                                    start_service_reply, NULL);
}

static gboolean
request_dbus_name (const gchar *name)
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
  guint32 request_status;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (connection == NULL) {
    g_warning ("Failed to open connection to DBus: %s", error->message);
    g_error_free (error);
    return FALSE;
  }

  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (proxy, name,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_status,
                                          &error)) {
    g_warning ("Failed to request name: %s", error->message);
    g_error_free (error);
    return FALSE;
  }

  return request_status == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
}

static void
rebinder_configure_run (MxWindow *window)
{
#if HAVE_REBINDER
  const gchar dbus_name[] = MEX_REBINDER_CONFIGURE_DBUS_INTERFACE;
  Rebinder the_rebinder;

  if (request_dbus_name (dbus_name) == FALSE)
    {
      g_message ("Could not request DBus name: %s", dbus_name);
      return;
    }

  /* we expect to be build against clutter-glx */
  the_rebinder.dpy = clutter_x11_get_default_display ();

  the_rebinder.config = rebinder_configure (&the_rebinder,
					    window,
					    TRUE,
					    opt_fullscreen);

  clutter_main ();

  rebinder_configure_free (the_rebinder.config);
#endif /* HAVE_REBINDER */
}

#if HAVE_REBINDER
static void
rebinder_quit (void)
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
  guint32 request_status;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (connection == NULL)
    {
      g_warning ("Failed to open connection to DBus: %s", error->message);
      g_error_free (error);
      return;
    }

  proxy = dbus_g_proxy_new_for_name (connection,
                                     MEX_REBINDER_DBUS_SERVICE,
                                     MEX_REBINDER_DBUS_PATH,
                                     MEX_REBINDER_DBUS_INTERFACE);
  if (proxy == NULL)
    {
      g_message ("Could not create proxy for %s", MEX_REBINDER_DBUS_SERVICE);
      return;
    }

  dbus_g_proxy_call_no_reply (proxy, "Quit", G_TYPE_INVALID);

  g_object_unref (proxy);
  dbus_g_connection_unref (connection);
}
#endif

static void
out_of_box (MexData *data)
{
  MexSettings *settings;
  GError *error = NULL;
  gchar *out_box_done;
  const gchar oob[] = "out-of-box-done";

  MexNotificationSource *source;
  MexNotification *help_message;

  settings = mex_settings_get_default ();
  out_box_done = mex_settings_find_config_file (settings, oob);

  if (out_box_done != NULL)
    {
      g_free (out_box_done);
      return;
    }

  rebinder_configure_run (data->window);

  /* Add a help hint for usage of the columns */
  if (data->info_bar)
    {
      mex_info_bar_new_notification (MEX_INFO_BAR (data->info_bar),
                                     _("Activate the column headers for more content"),
                                     15);
    }

  /* All out of box actions are complete, create an empty file to keep
   * the state.
   */
  out_box_done = g_build_filename (mex_settings_get_config_dir (settings),
                                   oob,
                                   NULL);
  g_file_set_contents (out_box_done, NULL, 0, &error);
  if (error)
    {
      g_warning ("could not create %s: %s", oob, error->message);
      g_error_free (error);
    }
  g_free (out_box_done);
}

static void
_close_screen_dialog_cb (MxAction *action, gpointer user_data)
{
  clutter_main_quit ();
}

/*
 * Check the screen resolution and issue a warning if we're running in a mode
 * with less than 600 pixels of height available.
 */
static void
check_resolution (MexData *data)
{
#if HAVE_CLUTTER_X11
  Screen *screen;
  ClutterActor *dialog, *layout, *title, *label;
  MxAction *action;

  /*
   * We're making the big assumption here that we're running fullscreen on the
   * default screen of the default display.  Anything else is a world of pain...
   */
  screen = ScreenOfDisplay (clutter_x11_get_default_display (),
                            clutter_x11_get_default_screen ());

  /* Issue a warning if we've not got 600 pixels of height to play with */
  if (HeightOfScreen (screen) >= 600)
    return;

  dialog = mx_dialog_new ();
  mx_stylable_set_style_class (MX_STYLABLE (dialog), "MtvInfoBarDialog");

  layout = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (layout), 10);
  mx_table_set_row_spacing (MX_TABLE (layout), 30);
  mx_bin_set_child (MX_BIN (dialog), layout);

  title = mx_label_new_with_text ("Screen Too Small");
  mx_stylable_set_style_class (MX_STYLABLE (title), "DialogHeader");
  mx_table_add_actor (MX_TABLE (layout), CLUTTER_ACTOR (title), 0, 0);

  label = mx_label_new_with_text (_("This application requires at least 600 vertical pixels."));
  mx_table_add_actor (MX_TABLE (layout), CLUTTER_ACTOR (label), 1, 0);

  action = mx_action_new_full ("close", _("Close"),
                               G_CALLBACK (_close_screen_dialog_cb),
                               NULL);
  mx_dialog_add_action (MX_DIALOG (dialog), action);

  clutter_actor_show (dialog);
  mx_dialog_set_transient_parent (MX_DIALOG (dialog), CLUTTER_ACTOR (data->stage));
  mex_push_focus (MX_FOCUSABLE (dialog));
#endif
}

static void
mex_go_back_cb (MxAction *action,
                MexData  *data)
{
  mex_go_back (data);
}

static void
cleanup_before_exit (void)
{
#if HAVE_REBINDER
  rebinder_quit ();
#endif
}

static MxApplication *application_for_signal;
static void
on_int_term_signaled (int sig)
{
  cleanup_before_exit ();
  mx_application_quit (application_for_signal);
}

static GOptionEntry entries[] =
{
  { "full-screen", 'f', 0, G_OPTION_ARG_NONE, &opt_fullscreen,
    "Start full screen", NULL },
  { "show-version", 0, 0, G_OPTION_ARG_NONE, &opt_version,
    "Dispay the version", NULL },
  { "ignore-resolution", 'r', 0, G_OPTION_ARG_NONE, &opt_ignore_res,
    "Don't warn if the screen size isn't sufficient", NULL },
  { "touch-mode", 't', 0, G_OPTION_ARG_NONE, &opt_touch,
    "Enable touch-screen mode", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &opt_file,
    "File to play", NULL },
  { NULL }
};

static const gchar *play_action_mimetypes[] = { "video/", "x-mex/media", "x-mex/tv", NULL };
static const gchar *listen_action_mimetypes[] = { "audio/", NULL };
static const gchar *resume_action_mimetypes[] = { "video/", "audio/", "x-mex/media", NULL };
static const gchar *folder_action_mimetypes[] = { "x-grl/box", NULL };
static const gchar *show_action_mimetypes[] = { "image/", NULL };
static const gchar *back_action_mimetypes[] = { "x-mex/back", NULL };


int
main (int argc, char **argv)
{
  gchar *settings;
  MxWindow *window;
  MxApplication *app;
  MexModel *root_model;
  GOptionContext *context;
  MexModelManager *mmanager;
  MexPluginManager *pmanager;
  MexActionManager *amanager;
  GrlPluginRegistry *registry;
  gchar *tmp;

  GError *error = NULL;
  MexData data = { 0, };

  MexActionInfo play = { 0, };
  MexActionInfo play_from_last = { 0, };
  MexActionInfo play_from_begin = { 0, };
  MexActionInfo listen_from_begin = { 0, };
  MexActionInfo listen = { 0, };
  MexActionInfo open_folder = { 0, };
  MexActionInfo show = { 0, };
  MexActionInfo back = { 0, };

  /* FIXME: Replace this with a configuration file */
  MexModelCategoryInfo search = { "search", _("Search"), "icon-panelheader-search", 0, "" };
  MexModelCategoryInfo videos = { "videos", _("Videos"), "icon-panelheader-videos", 20, _("Connect an external drive or update your network settings to see Videos here.") };
  MexModelCategoryInfo live = { "live", _("Live TV"), "icon-panelheader-tv", 25, "" };
  MexModelCategoryInfo pictures = { "pictures", _("Photos"), "icon-panelheader-photos", 30, _("Connect an external drive or update your network settings to see Photos here.") };
  MexModelCategoryInfo music = { "music", _("Music"), "icon-panelheader-music", 40, _("Connect an external drive or update your network settings to see Music here.") };
  MexModelCategoryInfo queue = { "queue", _("Queue"), "icon-panelheader-queue", 50, _("This is your custom playlist.  Select Add to queue from the info menu on any video file to add it here.") };

#ifdef HAVE_CLUTTER_CEX100
  const ClutterColor black = { 0x00, 0x00, 0x00, 0x00 };
#else
  const ClutterColor black = { 0x00, 0x00, 0x00, 0xff };
#endif /* HAVE_CLUTTER_CEX100 */

  /*
   * Workaround SQLite Tracker backend problems by relying on the bus
   * backend.
   */
  g_setenv ("TRACKER_SPARQL_BACKEND", "bus", TRUE);

  g_thread_init (NULL);
  g_type_init ();

  /* Options */
  context = g_option_context_new ("- The Media Explorer UI");

  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("Failed to parse options: %s\n", error->message);
      exit (1);
    }
  g_option_context_free (context);

  /* Initialisation */
  mex_init (&argc, &argv);

#ifdef USE_PLAYER_CLUTTER_GST
  clutter_gst_init (&argc, &argv);
#endif

  clutter_set_font_flags (clutter_get_font_flags () & ~CLUTTER_FONT_MIPMAPPING);

  /* log domain */
  MEX_LOG_DOMAIN_INIT (main_log_domain, "main");

  /* grilo */
  mex_grilo_init (&argc, &argv);

  registry = grl_plugin_registry_get_default ();

  settings = mex_settings_find_config_file (mex_settings_get_default (),
                                            "grilo-system.conf");
  if (settings)
    grl_plugin_registry_add_config_from_file (registry, settings, NULL);
  g_free (settings);


  settings = mex_settings_find_config_file (mex_settings_get_default (),
                                            "mex.conf");
  /* Load plugins */

  if (settings)
    {
      GKeyFile *mex_conf;
      gchar **enabled_plugins;

      mex_conf = g_key_file_new ();

      g_key_file_load_from_file (mex_conf,
                                 settings,
                                 G_KEY_FILE_NONE,
                                 NULL);

      enabled_plugins = g_key_file_get_string_list (mex_conf,
                                                    "grilo-plugins",
                                                    "enabled",
                                                    NULL,
                                                    NULL);
      g_key_file_free (mex_conf);

      if (enabled_plugins)
        {
          gint i;

          for (i=0; enabled_plugins[i]; i++)
            {
              if (!grl_plugin_registry_load_by_id (registry,
                                                   enabled_plugins[i],
                                                   &error))
                {
                  g_warning ("Tried to load specified grilo plugin: %s but failed: %s",
                             enabled_plugins[i],
                             (error) ? error->message : "");

                  if (error)
                    g_clear_error (&error);
                }
              else
                {
                  MEX_DEBUG ("loaded: %s plugin", enabled_plugins[i]);
                }
            }
          g_strfreev (enabled_plugins);
        }
      g_free (settings);
    }
  else
    {
      MEX_DEBUG ("No mex.conf found, loading default plugins");

      /* Tracker is our first choice of plugin */
      if (!grl_plugin_registry_load_by_id (registry, "grl-tracker", NULL))
        {
          /* try and load the upnp and filesystem plugins instead */
         if (!grl_plugin_registry_load_by_id (registry, "grl-upnp", NULL) &&
             !grl_plugin_registry_load_by_id (registry, "grl-filesystem", NULL))
           {
             g_warning ("Could not load fallback plugins\n \
                        please check that grilo-plugins has been correctly \
                        installed");
           }
        }
    }

  /* Auto start the rebinder */
#if HAVE_REBINDER
  auto_start_dbus_service (MEX_REBINDER_DBUS_INTERFACE);
#endif /* HAVE_REBINDER */

  /* Create application */
  app = mx_application_new (&argc, &argv, "Media Explorer",
                            MX_APPLICATION_SINGLE_INSTANCE);
  mex_style_load_default ();

  if (error)
    {
      g_warning (G_STRLOC ": Error loading style: %s", error->message);
      g_clear_error (&error);
    }

#ifdef HAVE_CLUTTER_CEX100
  /* If we're on CEX100, use the default stage and make it semi-transparent */
  data.stage = CLUTTER_STAGE (clutter_stage_get_default ());
  clutter_stage_set_use_alpha (data.stage, TRUE);
  data.window = window = mx_window_new_with_clutter_stage (data.stage);
  mx_application_add_window (app, window);
#else
  data.window = window = mx_application_create_window (app);
  data.stage = mx_window_get_clutter_stage (window);
#endif

  mex_set_main_window (window);

  g_signal_connect (data.stage, "activate",
                    G_CALLBACK (on_stage_actived), &data);
  g_signal_connect (data.stage, "deactivate",
                    G_CALLBACK (on_stage_deactived), &data);
  g_signal_connect (data.stage, "notify::fullscreen-set",
                    G_CALLBACK (on_fullscreen_set), &data);
  on_fullscreen_set (data.stage, NULL, &data);

  /* Must set color after set use_alpha */
  clutter_stage_set_color (data.stage, &black);

  data.info_bar = mex_info_bar_get_default ();

  g_signal_connect_swapped (data.info_bar, "close-request",
                            G_CALLBACK (mx_application_quit), app);
  /* Out of the box experience */
  out_of_box (&data);

  /* Create bindings pool */
  data.bindings = clutter_binding_pool_new ("Media Explorer");

  /* Create base widgets */
  data.layout = mx_box_layout_new ();
  clutter_actor_set_name (data.layout, "main-layout");
  data.tool_area = mx_box_layout_new ();
  clutter_actor_set_name (data.tool_area, "tool-area");
  data.explorer = mex_explorer_new ();
  clutter_actor_set_name (data.explorer, "main-explorer");
  g_signal_connect (data.explorer, "notify::depth",
                    G_CALLBACK (mex_notify_depth_cb), &data);
  g_signal_connect (data.explorer, "header-activated",
                    G_CALLBACK (mex_header_activated_cb), &data);
  data.spinner = mx_spinner_new ();
  clutter_actor_set_name (data.spinner, "main-spinner");
  mx_spinner_set_animating (MX_SPINNER (data.spinner), FALSE);
  clutter_actor_set_opacity (data.spinner, 0x00);

  /* Hook onto the download-queue length property notification to
   * indicate we're busy
   */
  g_signal_connect (mex_download_queue_get_default (), "notify::queue-length",
                    G_CALLBACK (mex_notify_download_queue_length_cb), &data);

  /* Pack shell into layout and set to expand */
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (data.layout),
                                 MX_ORIENTATION_VERTICAL);



  /* Pack info bar into layout */
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (data.layout),
                                           data.info_bar,
                                           0, "expand", FALSE,
                                           "x-fill", TRUE,
                                           "y-fill", FALSE, NULL);

  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (data.layout),
                                           data.explorer,
                                           1, "expand", TRUE,
                                           NULL);


  /* A stack is the top level actor in the window */
  data.stack = mx_stack_new ();

  /* It's possible that MexPlayer does not provide an actor that will display
   * the video. This happens on STB hardware when the video is displayed in
   * another frame buffer. */
  data.player = (ClutterActor *) mex_player_get_default ();
  g_signal_connect_swapped (data.player, "close-request",
                            G_CALLBACK (mex_go_back), &data);
  g_signal_connect_swapped (data.player, "open-request",
                            G_CALLBACK (mex_player_content_set_externally_cb),
                            &data);

  clutter_container_add_actor (CLUTTER_CONTAINER (data.stack),
                               data.player);

  data.media = mex_player_get_clutter_media (MEX_PLAYER (data.player));
  g_signal_connect (data.media, "error",
                    G_CALLBACK (mex_player_media_error_cb), &data);
  g_signal_connect (data.media, "notify::buffer-fill",
                    G_CALLBACK (mex_player_media_notify_buffer_fill_cb), &data);

  clutter_container_add_actor (CLUTTER_CONTAINER (data.stack), data.layout);

  /* Pack spinner into stack */
  clutter_container_add_actor (CLUTTER_CONTAINER (data.stack), data.spinner);
  mx_stack_child_set_x_fill (MX_STACK (data.stack), data.spinner, FALSE);
  mx_stack_child_set_y_fill (MX_STACK (data.stack), data.spinner, FALSE);
  mx_stack_child_set_x_align (MX_STACK (data.stack), data.spinner,
                              MX_ALIGN_END);
  mx_stack_child_set_y_align (MX_STACK (data.stack), data.spinner,
                              MX_ALIGN_START);

  data.volume_control = mex_volume_control_new ();
  clutter_actor_set_opacity (data.volume_control, 0x00);

  /* Pack volume control into stack */
  clutter_container_add_actor (CLUTTER_CONTAINER (data.stack), data.volume_control);
  mx_stack_child_set_x_fill (MX_STACK (data.stack), data.volume_control, FALSE);
  mx_stack_child_set_y_fill (MX_STACK (data.stack), data.volume_control, FALSE);
  mx_stack_child_set_x_align (MX_STACK (data.stack), data.volume_control,
                              MX_ALIGN_MIDDLE);
  mx_stack_child_set_y_align (MX_STACK (data.stack), data.volume_control,
                              MX_ALIGN_START);

  /* listen for notify:volume to show the volume controls when the volume
   * changes (whatever the source of that change is, possibly out of process */
  g_signal_connect (data.volume_control, "notify::volume",
                    G_CALLBACK (on_volume_changed), &data);


  data.slide_show = mex_slide_show_new ();
  g_signal_connect_swapped (data.slide_show, "close-request",
                            G_CALLBACK (mex_go_back), &data);
  clutter_container_add_actor (CLUTTER_CONTAINER (data.stack), data.slide_show);
  clutter_actor_hide (data.slide_show);

  if (opt_version == TRUE)
    {
      /* Pack the version in the stack and align it to the bottom left */
      data.version = mx_label_new_with_text ("v" MEX_VERSION_GIT);

      clutter_container_add_actor (CLUTTER_CONTAINER (data.stack),
                                   data.version);
      mx_stack_child_set_x_fill (MX_STACK (data.stack), data.version, FALSE);
      mx_stack_child_set_y_fill (MX_STACK (data.stack), data.version, FALSE);
      mx_stack_child_set_x_align (MX_STACK (data.stack), data.version,
                                  MX_ALIGN_START);
      mx_stack_child_set_y_align (MX_STACK (data.stack), data.version,
                                  MX_ALIGN_END);
    }

  /* Set the stack as the window child */
  mx_window_set_child (window, data.stack);

  /* Resize and display window */
  mx_window_set_has_toolbar (window, FALSE);
  mx_window_set_window_size (window, 1280, 720);
  mx_window_show (window);

  if (opt_fullscreen)
    mx_window_set_fullscreen (window, TRUE);

  /* Attach event handler to stage */
  g_signal_connect (data.stage, "captured-event",
                    G_CALLBACK (mex_captured_event_cb), &data);

  g_signal_connect (data.stage, "event",
                    G_CALLBACK (mex_event_cb), &data);

  /* Create root model */
  root_model = mex_aggregate_model_new ();
  mex_model_set_sort_func (MEX_MODEL (root_model),
                           mex_model_sort_smart_cb,
                           GINT_TO_POINTER (FALSE));
  mex_explorer_set_root_model (MEX_EXPLORER (data.explorer), root_model);

  /* Create top-level model hash-table */
  data.model_from_category = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                    g_free, g_object_unref);

  /* Create model->provider mapping hash-table */
  data.model_to_provider = g_hash_table_new (NULL, NULL);

  /* Create the 'Up folder' shortcut content */
  data.folder_content =
    g_object_ref_sink (MEX_CONTENT (mex_program_new (NULL)));
  mex_content_set_metadata (data.folder_content,
                            MEX_CONTENT_METADATA_TITLE,
                            _("Back"));
  mex_content_set_metadata (data.folder_content,
                            MEX_CONTENT_METADATA_MIMETYPE,
                            "x-mex/back");

  tmp = g_strconcat ("file://", mex_get_data_dir (),
                     "/shell/style/folder-tile-up.png", NULL);
  mex_content_set_metadata (data.folder_content,
                            MEX_CONTENT_METADATA_STILL, tmp);
  g_free (tmp);

  /* Add the actions */

  /* Play actions */
  play_from_last.action =
    mx_action_new_full ("play-from-last", _("Resume"),
                        G_CALLBACK (mex_play_from_last_cb), &data);
  mx_action_set_icon (play_from_last.action, "media-watch-mex");
  play_from_last.mime_types = (gchar **)resume_action_mimetypes;

  /* This is the default action for this mime-type, so it should have the
   * highest priority. Play from beginning / Watch have slightly lower
   * priority.
   */
  play_from_last.priority = G_MAXINT;

  play.action =
    mx_action_new_full ("play", _("Watch"),
                        G_CALLBACK (mex_play_from_begin_cb), &data);
  mx_action_set_icon (play.action, "media-watch-mex");
  play.mime_types = (gchar **)play_action_mimetypes;
  play.priority = G_MAXINT - 1;

  play_from_begin.action =
    mx_action_new_full ("play-from-begin", _("Watch from start"),
                        G_CALLBACK (mex_play_from_begin_cb), &data);
  mx_action_set_icon (play_from_begin.action, "media-watch-from-beginning-mex");
  play_from_begin.mime_types = (gchar **)play_action_mimetypes;
  play_from_begin.priority = G_MAXINT - 1;

  /* View action (for pictures) */
  show.action = mx_action_new_full ("show", _("View"),
                                    G_CALLBACK (mex_show_cb), &data);
  mx_action_set_icon (show.action, "media-watch-mex");
  show.mime_types = (gchar **)show_action_mimetypes;
  show.priority = G_MAXINT;

  /* Open folder action */
  open_folder.action =
    mx_action_new_full ("open-grilo-folder", _("Open folder"),
                        G_CALLBACK (mex_grilo_open_folder_cb), &data);
  mx_action_set_icon (open_folder.action, "user-home-highlight-mex");
  open_folder.mime_types = (gchar **)folder_action_mimetypes;
  open_folder.priority = G_MAXINT;

  /* Listen action (for audio) */
  listen.action =
    mx_action_new_full ("listen", _("Listen"),
                        G_CALLBACK (mex_play_from_begin_cb), &data);
  mx_action_set_icon (listen.action, "media-watch-mex");
  listen.mime_types = (gchar **)listen_action_mimetypes;
  listen.priority = G_MAXINT;

  listen_from_begin.action =
    mx_action_new_full ("listen-from-begin", _("Listen from start"),
                        G_CALLBACK (mex_play_from_begin_cb), &data);
  mx_action_set_icon (listen_from_begin.action,
                      "media-watch-from-beginning-mex");
  listen_from_begin.mime_types = (gchar **)listen_action_mimetypes;
  listen_from_begin.priority = G_MAXINT - 1;

  /* View action (for pictures) */
  show.action = mx_action_new_full ("show", _("View"),
                                    G_CALLBACK (mex_show_cb), &data);
  mx_action_set_icon (show.action, "media-watch-mex");
  show.mime_types = (gchar **)show_action_mimetypes;
  show.priority = G_MAXINT;

  /* Go back action */
  back.action =
    mx_action_new_full ("go-back", _("Go back"),
                        G_CALLBACK (mex_go_back_cb), &data);
  back.mime_types = (gchar **)back_action_mimetypes;
  back.priority = G_MAXINT;

  amanager = mex_action_manager_get_default ();
  mex_action_manager_add_action (amanager, &play_from_last);
  mex_action_manager_add_action (amanager, &play_from_begin);
  mex_action_manager_add_action (amanager, &play);
  mex_action_manager_add_action (amanager, &show);
  mex_action_manager_add_action (amanager, &open_folder);
  mex_action_manager_add_action (amanager, &back);
  mex_action_manager_add_action (amanager, &listen);
  mex_action_manager_add_action (amanager, &listen_from_begin);

  /* Add the default categories to the model manager */
  mmanager = mex_model_manager_get_default ();
  mex_model_manager_add_category (mmanager, &search);
  mex_model_manager_add_category (mmanager, &live);
  mex_model_manager_add_category (mmanager, &videos);
  mex_model_manager_add_category (mmanager, &music);
  mex_model_manager_add_category (mmanager, &pictures);
  mex_model_manager_add_category (mmanager, &queue);

  /* Populate interface by loading plugins */
  pmanager = mex_plugin_manager_get_default ();
  g_signal_connect (pmanager, "plugin-loaded",
                    G_CALLBACK (mex_plugin_loaded_cb), &data);
  mex_plugin_manager_refresh (pmanager);

  /* Hook onto the model manager signals and refresh */
  g_signal_connect (mmanager, "model-added",
                    G_CALLBACK (mex_model_added_cb), &data);
  g_signal_connect (mmanager, "model-removed",
                    G_CALLBACK (mex_model_removed_cb), &data);
  g_signal_connect (mmanager, "categories-changed",
                    G_CALLBACK (mex_categories_changed_cb), &data);
  mex_refresh_root_model (&data);

  /* Present interface */
  mex_show_home_screen (&data);

  if (!opt_ignore_res)
    check_resolution (&data);
  if (opt_touch)
    mex_enable_touch_events (&data, TRUE);

  if (opt_file)
    {
      MexContent *first_content = NULL;
      MexModel *queue_model;
      GList *queue_models = NULL;
      gboolean first_done = FALSE;
      gint i;

      queue_models = mex_model_manager_get_models_for_category (mmanager,
                                                               "queue");

      queue_model = MEX_MODEL (queue_models->data);

      for (i=0; opt_file[i]; i++)
        {
          gchar *uri;
          MexContent *content;

          uri = g_filename_to_uri (opt_file[i], NULL, NULL);
          content = mex_content_from_uri (uri);
          g_free (uri);

          if (content)
            {
              if (!first_done)
                first_content = content;
              mex_model_add_content (queue_model, content);
            }
        }

      mex_content_view_set_context (MEX_CONTENT_VIEW (data.player),
                                    queue_model);
      mex_content_view_set_content (MEX_CONTENT_VIEW (data.player),
                                    first_content);

      mex_player_content_set_externally_cb (&data);
    }

  application_for_signal = app;
  signal (SIGINT, on_int_term_signaled);
  signal (SIGTERM, on_int_term_signaled);

  mx_application_run (app);

  cleanup_before_exit ();

#ifdef MEX_ENABLE_DEBUG
  /* When we are building the application in debug mode let's try to cleanly
   * remove everything to make valgrind and our debug plugin more useful */
  if (error)
    g_error_free (error);

  g_object_unref (amanager);

  g_object_unref (app);

  gst_deinit ();
  mex_deinit ();
#endif

  return 0;
}

