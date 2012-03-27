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

#include <stdlib.h>

#ifdef USE_PLAYER_CLUTTER_GST
#include <clutter-gst/clutter-gst.h>
#include <mex/mex-media-dbus-bridge.h>
#else
#ifdef USE_PLAYER_DBUS
#include <mex/mex-player-client.h>
#else
#ifdef USE_PLAYER_SURFACE
#include <mex/mex-media-dbus-bridge.h>
#include <mex/mex-surface-player.h>
#else
#error Unexpected player setup
#endif
#endif
#endif

#include "mex-player.h"

#include "mex-main.h"
#include "mex-background-manager.h"
#include "mex-private.h"
#include "mex-program.h"
#include "mex-content-view.h"
#include "mex-content-tile.h"
#include "mex-media-controls.h"
#include "mex-screensaver.h"
#include "mex-info-panel.h"
#include "mex-log.h"
#include "mex-utils.h"

#define MEX_LOG_DOMAIN_DEFAULT  player_log_domain
MEX_LOG_DOMAIN(player_log_domain);

static void mex_player_content_view_iface_init (MexContentViewIface *iface);
static void mex_player_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexPlayer, mex_player, MX_TYPE_STACK,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_player_content_view_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mex_player_focusable_iface_init))

#define PLAYER_PRIVATE(o)				      \
	  (G_TYPE_INSTANCE_GET_PRIVATE ((o),		      \
					MEX_TYPE_PLAYER,      \
					MexPlayerPrivate))


#define   GST_PLAY_FLAG_VIS (1 << 3)

struct _MexPlayerPrivate
{
#if defined(USE_PLAYER_CLUTTER_GST) || defined(USE_PLAYER_SURFACE)
  MexMediaDBUSBridge *bridge;
#endif
  ClutterMedia *media;
  MexContent   *content;
  MexModel     *model;

  ClutterActor *controls;
  ClutterActor *related_tile;
  ClutterActor *info_panel;

  guint hide_controls_source;

  guint info_visible : 1;
  guint controls_visible : 1;
  guint controls_prev_visible : 1;

  guint at_eos : 1;

  guint disable_media_controls : 1;

  gdouble position;
  gdouble current_position;
  guint   duration;

  MexScreensaver *screensaver;
};

enum
{
  CLOSE_REQUEST,
  OPEN_REQUEST,

  LAST_SIGNAL
};

enum
{
  PROP_0,
};

static guint signals[LAST_SIGNAL] = { 0, };

static gboolean mex_player_set_controls_visible (MexPlayer *player,
                                                 gboolean   visible);


static void mex_get_stream_cb (MexProgram   *program,
                               const gchar  *url,
                               const GError *error,
                               gpointer      user_data);

static void save_old_content (MexPlayer *player);

static void media_eos_cb (ClutterMedia *media,
                          MexPlayer    *player);

static void media_playing_cb (ClutterMedia *media,
                              GParamSpec   *pspec,
                              MexPlayer    *player);

static void media_update_progress (GObject    *gobject,
                                   GParamSpec *pspec,
                                   MexPlayer  *player);

static void media_uri_changed_cb (GObject    *gobject,
                                  GParamSpec *pspec,
                                  MexPlayer  *player);

/* MxFocusable implementation */
static MxFocusable *
mex_player_move_focus (MxFocusable      *focusable,
                       MxFocusDirection  direction,
                       MxFocusable      *old_focus)
{
  if (direction == MX_FOCUS_DIRECTION_UP)
    mex_player_set_controls_visible (MEX_PLAYER (focusable), FALSE);

  return NULL;
}

static void
mex_player_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_player_move_focus;
}

/* MexContentView implementation */
static void
mex_player_set_content (MexContentView *view,
                        MexContent     *content)
{
  MexPlayerPrivate *priv = MEX_PLAYER (view)->priv;

  if (priv->model)
    mex_media_controls_set_content (MEX_MEDIA_CONTROLS (priv->controls),
                                    content, priv->model);

  if (priv->related_tile)
    {
      g_object_unref (priv->related_tile);
      priv->related_tile = NULL;
    }

  if (content)
    {
      const gchar *sposition, *sduration, *mimetype;

      mimetype = mex_content_get_metadata (content,
                                           MEX_CONTENT_METADATA_MIMETYPE);

      /* disable the media controls when playing dvd video */
      priv->disable_media_controls = !g_strcmp0 (mimetype, "video/dvd");


      if (priv->disable_media_controls && CLUTTER_IS_ACTOR (priv->media))
        {
          clutter_actor_grab_key_focus (CLUTTER_ACTOR (priv->media));
          clutter_actor_set_reactive (CLUTTER_ACTOR (priv->media),
                                      priv->disable_media_controls);
        }

      if (priv->content)
        {
          save_old_content (MEX_PLAYER (view));
          g_object_unref (priv->content);
          priv->content = NULL;
        }

      priv->content = g_object_ref_sink (content);

      sposition = mex_content_get_metadata (content,
                                            MEX_CONTENT_METADATA_LAST_POSITION);
      sduration = mex_content_get_metadata (content,
                                            MEX_CONTENT_METADATA_DURATION);


      if (sduration &&
          !mex_media_controls_get_playing_queue (MEX_MEDIA_CONTROLS (priv->controls)))
        priv->duration = atoi (sduration);
      else
        priv->duration = 0;

      if (priv->duration > 0)
        {
          if (sposition)
            {
              int position = atoi (sposition);
              priv->position = (gdouble) position / (gdouble) priv->duration;
            }
          else
            {
              priv->position = 0.0;
            }
        }

      if (MEX_IS_PROGRAM (content))
        {
          mex_program_get_stream (MEX_PROGRAM (content),
                                  mex_get_stream_cb,
                                  view);
        }
      else
        {
          const gchar *uri;

          uri = mex_content_get_metadata (content,
                                          MEX_CONTENT_METADATA_STREAM);
          mex_get_stream_cb (NULL, uri, NULL, view);
        }

      if (priv->info_visible)
        {
          clutter_actor_animate (priv->info_panel, CLUTTER_EASE_IN_SINE,
                                 250, "opacity", 0x00, NULL);
          mx_widget_set_disabled (MX_WIDGET (priv->info_panel), TRUE);
          priv->info_visible = FALSE;
        }

      mex_player_set_controls_visible (MEX_PLAYER (view), TRUE);
    }
  else
    {
      if (priv->content)
        {
          save_old_content (MEX_PLAYER (view));
          g_object_unref (priv->content);
          priv->content = NULL;
        }
    }
}

static MexContent *
mex_player_get_content (MexContentView *player)
{
  return MEX_PLAYER (player)->priv->content;
}

static void
mex_player_set_context (MexContentView *player,
                        MexModel *model)
{
  MexPlayerPrivate *priv = MEX_PLAYER (player)->priv;

  if (priv->model)
    g_object_unref (priv->model);

  priv->model = model;

  if (model)
    g_object_ref (model);
}

static MexModel*
mex_player_get_context (MexContentView *view)
{
  return MEX_PLAYER (view)->priv->model;
}

static void
mex_player_content_view_iface_init (MexContentViewIface *iface)
{
  iface->get_content = mex_player_get_content;
  iface->set_content = mex_player_set_content;

  iface->set_context = mex_player_set_context;
  iface->get_context = mex_player_get_context;
}

static void
mex_player_get_property (GObject    *object,
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
mex_player_set_property (GObject      *object,
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
mex_player_dispose (GObject *object)
{
  MexPlayer *player = MEX_PLAYER (object);
  MexPlayerPrivate *priv = player->priv;

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

  if (priv->media)
    {
      g_signal_handlers_disconnect_by_func (priv->media,
                                            media_eos_cb, player);
      g_signal_handlers_disconnect_by_func (priv->media,
                                            media_playing_cb, player);
      g_signal_handlers_disconnect_by_func (priv->media,
                                            media_update_progress, player);
      g_signal_handlers_disconnect_by_func (priv->media,
                                            media_uri_changed_cb, player);

      g_object_unref (priv->media);
      priv->media = NULL;
    }

  if (priv->screensaver)
    {
      g_object_unref (priv->screensaver);
      priv->screensaver = NULL;
    }

  if (priv->related_tile)
    {
      g_object_unref (priv->related_tile);
      priv->related_tile = NULL;
    }

#if defined(USE_PLAYER_CLUTTER_GST) || defined(USE_PLAYER_SURFACE)
  if (priv->bridge)
    {
      g_object_unref (priv->bridge);
      priv->bridge = NULL;
    }
#endif

  G_OBJECT_CLASS (mex_player_parent_class)->dispose (object);
}

static void
mex_player_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_player_parent_class)->finalize (object);
}

static gboolean
mex_hide_controls_cb (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;

  /* timer no longer running */
  priv->hide_controls_source = 0;
  /* if we've timed out then don't bring the controls
     up after the info panel has been visible */
  priv->controls_prev_visible = 0;

  /* hide the controls unless the video is at the end */
  if (!priv->at_eos)
    mex_player_set_controls_visible (player, FALSE);

  return FALSE;
}

static void
mex_player_restart_timer (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;

  if (priv->hide_controls_source)
    g_source_remove (priv->hide_controls_source);

  priv->hide_controls_source =
    g_timeout_add_seconds (12, (GSourceFunc) mex_hide_controls_cb, player);
}


static gboolean
mex_player_captured_event (ClutterActor *actor,
                           ClutterEvent *event)
{
  MexPlayer *self = MEX_PLAYER (actor);
  MexPlayerPrivate *priv = self->priv;

  /* If a mouse button was pressed and the controls aren't visible, show them,
   * otherwise restart the control-showing timer.
   */
  switch (event->type)
    {
    case CLUTTER_BUTTON_PRESS:
    case CLUTTER_MOTION:
      if (!priv->controls_visible && !priv->info_visible)
        mex_player_set_controls_visible (self, TRUE);
      else
        mex_player_restart_timer (MEX_PLAYER (actor));
      break;

    case CLUTTER_BUTTON_RELEASE:
      if (event->button.click_count == 2)
        mex_toggle_fullscreen ();
      else
        mex_player_restart_timer (MEX_PLAYER (actor));
      break;

    default:
      mex_player_restart_timer (MEX_PLAYER (actor));
      break;
    }

  return FALSE;
}

static gboolean
mex_player_key_press_event (ClutterActor    *actor,
                            ClutterKeyEvent *event)
{
  MexPlayer *player = MEX_PLAYER (actor);
  MexPlayerPrivate *priv = player->priv;

  ClutterStage *stage;
  MxFocusManager *fmanager;

  stage = (ClutterStage*) clutter_actor_get_stage (actor);
  fmanager = mx_focus_manager_get_for_stage (stage);

  if (MEX_KEY_INFO (event->keyval))
    {
      MexContent *content;

      content = priv->content;

      if (priv->info_visible)
        {
          /* hide the info panel */
          clutter_actor_animate (priv->info_panel, CLUTTER_EASE_IN_SINE,
                                 250, "opacity", 0x00, NULL);

          mx_widget_set_disabled (MX_WIDGET (priv->info_panel), TRUE);
          mx_widget_set_disabled (MX_WIDGET (priv->controls), FALSE);

          priv->info_visible = FALSE;

          if (priv->controls_prev_visible)
            mex_player_set_controls_visible (player, TRUE);
        }
      else
        {
          MxFocusable *focusable;

          /* if you're pressing info button while the media controls are up
             set them as previously visible */
          if (priv->controls_visible)
            priv->controls_prev_visible = TRUE;

          focusable = mx_focus_manager_get_focused (fmanager);
          if (MEX_IS_CONTENT_TILE (focusable) &&
              priv->controls_prev_visible == TRUE)
            {
              content =
                mex_content_view_get_content (MEX_CONTENT_VIEW (focusable));

              /* to avoid any accidental leak */
              if (priv->related_tile)
                {
                  g_object_unref (priv->related_tile);
                  priv->related_tile = NULL;
                }
              priv->related_tile = g_object_ref (focusable);
            }

          mex_content_view_set_content (MEX_CONTENT_VIEW (priv->info_panel),
                                        content);

          /* show the info panel */
          clutter_actor_animate (priv->info_panel, CLUTTER_EASE_IN_SINE,
                                 250, "opacity", 0xff, NULL);

          mx_widget_set_disabled (MX_WIDGET (priv->info_panel), FALSE);
          mx_widget_set_disabled (MX_WIDGET (priv->controls), TRUE);

          priv->info_visible = TRUE;

          mex_player_set_controls_visible (player, FALSE);

          mex_push_focus (MX_FOCUSABLE (priv->info_panel));
        }

      return TRUE;
    }

  switch (event->keyval)
    {
    case CLUTTER_KEY_Down:
        {
          if (!priv->controls_visible && !priv->info_visible)
            return mex_player_set_controls_visible (player, TRUE);
          break;
        }

    /* Pause the player */
    case CLUTTER_KEY_space:
        {
          if (clutter_media_get_playing (priv->media))
            mex_player_pause (player);
          else
            mex_player_play (player);
          break;
        }
    }

  return FALSE;
}

static void
mex_player_class_init (MexPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexPlayerPrivate));

  object_class->get_property = mex_player_get_property;
  object_class->set_property = mex_player_set_property;
  object_class->dispose = mex_player_dispose;
  object_class->finalize = mex_player_finalize;

  actor_class->key_press_event = mex_player_key_press_event;
  actor_class->captured_event = mex_player_captured_event;

  signals[CLOSE_REQUEST] = g_signal_new ("close-request",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);

  signals[OPEN_REQUEST] = g_signal_new ("open-request",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);
}

static gboolean
mex_player_set_controls_visible (MexPlayer *player,
                                 gboolean   visible)
{
  MexPlayerPrivate *priv = player->priv;
  gfloat pos;
  ClutterStage *stage;
  MxFocusManager *fmanager;

  if (priv->disable_media_controls)
    visible = FALSE;

  stage = (ClutterStage*) clutter_actor_get_stage (CLUTTER_ACTOR (player));

  if (stage)
    fmanager = mx_focus_manager_get_for_stage (stage);
  else
    fmanager = NULL;


  pos = clutter_actor_get_height (priv->controls);
  if (visible)
    {
      priv->controls_prev_visible = FALSE;
      priv->controls_visible = TRUE;

      mx_widget_set_disabled (MX_WIDGET (priv->controls), FALSE);

      clutter_actor_animate (priv->controls, CLUTTER_EASE_IN_SINE, 250,
                             "opacity", 0xff,
                             "anchor-y", 0.0,
                             NULL);
      mex_media_controls_set_disabled (MEX_MEDIA_CONTROLS (priv->controls),
                                       FALSE);

      mex_player_restart_timer (player);

      if (priv->related_tile)
        {
          if (fmanager)
            mx_focus_manager_push_focus_with_hint (fmanager,
                                                   MX_FOCUSABLE (priv->related_tile),
                                                   MX_FOCUS_HINT_PRIOR);

          g_object_unref (priv->related_tile);
          priv->related_tile = NULL;
        }
      else
        {
          if (fmanager)
            mx_focus_manager_push_focus (fmanager, MX_FOCUSABLE (priv->controls));
        }
    }

  if (!visible)
    {
      priv->controls_visible = FALSE;
      clutter_actor_animate (priv->controls, CLUTTER_EASE_IN_SINE, 250,
                             "opacity", 0x00,
                             "anchor-y", -pos,
                             NULL);
      mex_media_controls_set_disabled (MEX_MEDIA_CONTROLS (priv->controls),
                                       TRUE);

      if (priv->hide_controls_source)
        {
          g_source_remove (priv->hide_controls_source);
          priv->hide_controls_source = 0;
        }
    }

  return TRUE;
}

static void
controls_stopped_cb (MexMediaControls *controls,
                     MexPlayer        *player)
{
  save_old_content (player);

  if (player->priv->hide_controls_source)
    {
      g_source_remove (player->priv->hide_controls_source);
      player->priv->hide_controls_source = 0;
    }

  g_signal_emit (player, signals[CLOSE_REQUEST], 0);
}

static void
media_eos_cb (ClutterMedia *media,
              MexPlayer    *player)
{
  MexPlayerPrivate *priv = player->priv;
  MexContent *enqueued_content;

  priv->position = 0.0;

  /* Check to see if we have enqueued content and if so play it next */
  enqueued_content =
    mex_media_controls_get_enqueued (MEX_MEDIA_CONTROLS (priv->controls),
                                     priv->content);

  /* set the control visible */
  clutter_actor_animate (priv->info_panel, CLUTTER_EASE_IN_SINE,
                         250, "opacity", 0x00, NULL);
  mex_player_set_controls_visible (player, TRUE);

  if (enqueued_content)
    {
      mex_player_set_content (MEX_CONTENT_VIEW (player), enqueued_content);
    }
  else
    {
      mex_screensaver_uninhibit (priv->screensaver);

      clutter_media_set_progress (media, priv->position);
      clutter_media_set_playing (media, FALSE);

      priv->current_position = 0.0;
      priv->at_eos = TRUE;
    }

  /* focus the related content */
  mex_media_controls_focus_content (MEX_MEDIA_CONTROLS (priv->controls),
                                    priv->content);
}

static void
media_playing_cb (ClutterMedia *media,
                  GParamSpec   *pspec,
                  MexPlayer    *player)
{
  MexPlayerPrivate *priv = player->priv;

  mex_screensaver_inhibit (priv->screensaver);

  /* reset at_eos flag if a video is now playing */
  if (clutter_media_get_playing (media))
    priv->at_eos = FALSE;
}

static void
save_old_content (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;
  guint position;
  gchar str[20];

  if (!priv->content)
    return;

  if (priv->duration)
    {
      priv->position = priv->current_position;
      position = (guint) (priv->position * priv->duration);

      if (position > 0)
        {
          snprintf (str, sizeof (str), "%u", position);
          mex_content_set_metadata (priv->content,
                                    MEX_CONTENT_METADATA_LAST_POSITION,
                                    str);
        }
  }

  mex_content_set_last_used_metadatas (priv->content);
  mex_content_save_metadata (priv->content);
}

static void
media_uri_changed_cb (GObject *object, GParamSpec *spec, MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;
  gchar *uri;

  if ((uri = clutter_media_get_uri (CLUTTER_MEDIA (object))))
    {
      MexContent *ctrls_content;

      ctrls_content =
        mex_media_controls_get_content (MEX_MEDIA_CONTROLS (priv->controls));

      if (!ctrls_content)
        {
          mex_player_set_uri (player, uri);
        }
      else if (g_strcmp0 (mex_content_get_metadata (ctrls_content, MEX_CONTENT_METADATA_STREAM), uri) != 0)
        {
          mex_player_set_uri (player, uri);
        }
    }
}

static void
media_update_progress (GObject    *gobject,
                       GParamSpec *pspec,
                       MexPlayer  *player)
{
  MexPlayerPrivate *priv = player->priv;

  if (!priv->at_eos)
    priv->current_position = clutter_media_get_progress (priv->media);
}

static void
mex_player_init (MexPlayer *self)
{
  MexPlayerPrivate *priv;

  self->priv = priv = PLAYER_PRIVATE (self);

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

#ifdef USE_PLAYER_CLUTTER_GST
  priv->media = (ClutterMedia *) clutter_gst_video_texture_new ();

  /* We want to keep a reference to the media here to ensure consistency with
   * the D-BUS client interface behaviour
   */
  g_object_ref_sink (priv->media);

  clutter_container_add_actor (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->media));
  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (priv->media), TRUE);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->media),
                               "fit", TRUE, NULL);

  /* Use progressive download when possible. Don't enable that yet, the
   * behaviour of seeking in the non already downloaded part of the stream
   * is not great. Either disable seeking in that case or find out why.*/
#if 0
  video_texture = CLUTTER_GST_VIDEO_TEXTURE (priv->media);
  clutter_gst_video_texture_set_buffering_mode (video_texture,
						CLUTTER_GST_BUFFERING_MODE_DOWNLOAD);
#endif
#else
#ifdef USE_PLAYER_DBUS
  priv->media = (ClutterMedia *) mex_player_client_new ();
#else
#ifdef USE_PLAYER_SURFACE
  priv->media = (ClutterMedia *) mex_surface_player_new ();
#else
#error Unexpected player setup
#endif
#endif
#endif

  g_signal_connect (priv->media, "eos", G_CALLBACK (media_eos_cb), self);
  g_signal_connect (priv->media, "notify::playing",
                    G_CALLBACK (media_playing_cb), self);
  g_signal_connect (priv->media, "notify::progress",
                    G_CALLBACK (media_update_progress), self);
  g_signal_connect (priv->media, "notify::uri",
                    G_CALLBACK (media_uri_changed_cb),
                    self);

#if (defined(USE_PLAYER_SURFACE) || defined (USE_PLAYER_CLUTTER_GST))
  {
    GError *error = NULL;
    priv->bridge = mex_media_dbus_bridge_new (priv->media);
    if (!mex_media_dbus_bridge_register (priv->bridge, &error))
      {
        g_warning (G_STRLOC ": Error registering player on D-BUS");
        g_clear_error (&error);
      }
  }
#endif

  /* add info panel */
  priv->info_panel = mex_info_panel_new (MEX_INFO_PANEL_MODE_FULL);
  mx_widget_set_disabled (MX_WIDGET (priv->info_panel), TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->info_panel);
  clutter_container_child_set (CLUTTER_CONTAINER (self), priv->info_panel,
                               "y-fill", FALSE, "y-align", MX_ALIGN_END,
                               NULL);
  clutter_actor_set_opacity (priv->info_panel, 0);
  mex_info_panel_set_media (MEX_INFO_PANEL (priv->info_panel), priv->media);

  /* add media controls */
  priv->controls = mex_media_controls_new ();
  g_signal_connect (priv->controls, "stopped", G_CALLBACK (controls_stopped_cb),
                    self);
  mex_media_controls_set_media (MEX_MEDIA_CONTROLS (priv->controls),
                                priv->media);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->controls);
  clutter_container_child_set (CLUTTER_CONTAINER (self), priv->controls,
                               "y-fill", FALSE, "y-align", MX_ALIGN_END,
                               NULL);

  priv->screensaver = mex_screensaver_new ();
}

MexPlayer *
mex_player_get_default (void)
{
  static MexPlayer *singleton = NULL;

  if (singleton)
    return singleton;

  singleton = g_object_new (MEX_TYPE_PLAYER, NULL);
  return singleton;
}

ClutterMedia *
mex_player_get_clutter_media (MexPlayer *player)
{
  g_return_val_if_fail (MEX_IS_PLAYER (player), NULL);

  return player->priv->media;
}

static void
mex_get_stream_cb (MexProgram   *program,
                   const gchar  *url,
                   const GError *error,
                   gpointer      user_data)
{
  MexPlayer *player = user_data;
  MexPlayerPrivate *priv = player->priv;
  MexGenericContent  *generic_content;
#ifdef USE_PLAYER_CLUTTER_GST
  ClutterGstVideoTexture *video_texture;
#endif

  if (G_UNLIKELY (error))
    {
      g_warning ("Could not play content: %s (%s)", error->message, url);
      return;
    }

  /* ensure that this callback is for the current content */
  if (priv->content != (MexContent*) program)
    return;

#ifdef USE_PLAYER_CLUTTER_GST
  /* We seek at the precise time when the file is local, but we
   * seek to key frame when streaming */
  video_texture = CLUTTER_GST_VIDEO_TEXTURE (priv->media);
  if (g_str_has_prefix (url, "file://"))
    {
      clutter_gst_video_texture_set_seek_flags (video_texture,
                                                CLUTTER_GST_SEEK_FLAG_ACCURATE);
    }
  else
    {
      clutter_gst_video_texture_set_seek_flags (video_texture,
                                                CLUTTER_GST_SEEK_FLAG_NONE);
    }

  /* TODO when we have settings we can configure this feature */

  if (g_str_has_prefix (mex_content_get_metadata (priv->content,
                                                  MEX_CONTENT_METADATA_MIMETYPE),
                        "audio/"))
    {
      GstElement *gst_element, *visual;
      gint gst_flags;

      gst_element = clutter_gst_video_texture_get_pipeline (video_texture);
      g_object_get (G_OBJECT (gst_element), "flags", &gst_flags, NULL);

      gst_flags = (GST_PLAY_FLAG_VIS | gst_flags);

      g_object_set (G_OBJECT (gst_element), "flags", gst_flags, NULL);

      visual = gst_element_factory_make ("libvisual_infinite", NULL);

      if (visual)
        g_object_set (G_OBJECT (gst_element), "vis-plugin", visual, NULL);
    }
#endif

  MEX_DEBUG ("set uri %s", url);
  clutter_media_set_uri (CLUTTER_MEDIA (priv->media), url);
  generic_content = MEX_GENERIC_CONTENT (priv->content);
  if (mex_generic_content_get_last_position_start (generic_content))
    clutter_media_set_progress (CLUTTER_MEDIA (priv->media), priv->position);
  clutter_media_set_playing (CLUTTER_MEDIA (priv->media), TRUE);
}

void
mex_player_quit (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;

  MEX_DEBUG ("quit");
  save_old_content (player);
  clutter_media_set_uri (CLUTTER_MEDIA (priv->media), NULL);
  clutter_media_set_playing (priv->media, FALSE);
  g_signal_emit (player, signals[CLOSE_REQUEST], 0);
}

void
mex_player_stop (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;

  MEX_DEBUG ("stop");
  save_old_content (player);
  clutter_media_set_uri (CLUTTER_MEDIA (priv->media), NULL);
  clutter_media_set_playing (priv->media, FALSE);
}

void
mex_player_pause (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;

  MEX_DEBUG ("pause");
  clutter_media_set_playing (priv->media, FALSE);
}

void
mex_player_play (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;

  MEX_DEBUG ("play");
  clutter_media_set_playing (priv->media, TRUE);
}

static void
player_forward_rewind (MexPlayer *player, gboolean increment)
{
  MexPlayerPrivate *priv = player->priv;

  gdouble duration;
  gfloat old_progress, new_progress;

  duration = clutter_media_get_duration (priv->media);
  old_progress = clutter_media_get_progress (priv->media);

  /* If/when clutter gst supports trickmode we could implement
   *  that here instead
   * http://bugzilla.clutter-project.org/show_bug.cgi?id=2634
   */

  if (increment)
    new_progress = MIN (1.0, ((duration * old_progress) + 10) / duration);
  else
    new_progress = MAX (0.0, ((duration * old_progress) - 10) / duration);

  mex_player_set_controls_visible (player, TRUE);

  MEX_DEBUG ("rewind %f -> %f", old_progress, new_progress);

  clutter_media_set_progress (priv->media, new_progress);
}

void
mex_player_forward (MexPlayer *player)
{
  player_forward_rewind (player, TRUE);
}

void
mex_player_rewind (MexPlayer *player)
{
  player_forward_rewind (player, FALSE);
}

void
mex_player_next (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;

  clutter_media_set_progress (priv->media, 1.0);
}

void
mex_player_previous (MexPlayer *player)
{
  MexPlayerPrivate *priv = player->priv;

  clutter_media_set_progress (priv->media, 0.0);
}

void
mex_player_seek_us (MexPlayer *player, gint64 seek_offset_us)
{
  MexPlayerPrivate *priv = player->priv;
  gdouble duration_us, progress, new_progress;

  duration_us = clutter_media_get_duration (priv->media) * 1000000;
  progress = clutter_media_get_progress (priv->media) * duration_us;

  new_progress = (progress + seek_offset_us) / duration_us;

  if (new_progress < 0.0)
    mex_player_previous (player);
  else if (new_progress > 1.0)
    mex_player_next (player);
  else
    clutter_media_set_progress (priv->media, new_progress);
}

void
mex_player_set_uri (MexPlayer *player, const gchar *uri)
{
  MexPlayerPrivate *priv = player->priv;

  MexContent *content;

  content = mex_content_from_uri (uri);

  if (content)
    {
      mex_content_view_set_content (MEX_CONTENT_VIEW (player),
                                    content);

      mex_media_controls_set_content (MEX_MEDIA_CONTROLS (priv->controls),
                                      content,
                                      NULL);
    }
  else
    {
      /* Try reallly really hard to play it, if it fails we at least will get
       * gstreamer's error dialog.
       */
      clutter_media_set_uri (priv->media, uri);
    }

  g_signal_emit (player, signals[OPEN_REQUEST], 0);
}

void
mex_player_open (MexPlayer *player)
{
  g_signal_emit (player, signals[OPEN_REQUEST], 0);
}

/**/

static ClutterMedia *
_get_default_media_player (void)
{
  return mex_player_get_clutter_media (mex_player_get_default ());
}

static MexPlayerGetMediaPlayerCb get_media_player_cb = _get_default_media_player;

void
mex_player_set_media_player_callback (MexPlayerGetMediaPlayerCb callback)
{
  g_return_if_fail (callback != NULL);

  get_media_player_cb = callback;
}

ClutterMedia *
mex_player_get_default_media_player (void)
{
  return get_media_player_cb ();
}
