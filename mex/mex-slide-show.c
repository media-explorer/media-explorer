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

#include "mex-slide-show.h"

#include "mex-main.h"
#include "mex-private.h"
#include "mex-program.h"
#include "mex-utils.h"
#include "mex-scroll-view.h"
#include "mex-shadow.h"

#include "mex-content-tile.h"
#include "mex-view-model.h"
#include "mex-content-proxy.h"
#include "mex-download-queue.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <stdlib.h>
#include <string.h>

static void mx_focusable_iface_init (MxFocusableIface *iface);
static void mex_slide_show_content_view_iface_init (MexContentViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexSlideShow, mex_slide_show, MX_TYPE_FRAME,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_slide_show_content_view_iface_init))

#define SLIDE_SHOW_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SLIDE_SHOW, \
                                                        MexSlideShowPrivate))

enum
{
  PROP_0,
};

struct _MexSlideShowPrivate
{
  ClutterScript *script;
  MexModel      *model;
  MexProxy      *proxy;
  MexContent    *content;

  ClutterActor  *image;
  ClutterActor  *controls;
  ClutterActor  *info_panel;
  ClutterActor  *fit_to_screen_button;
  ClutterActor  *current_tile;

  /* TODO: We could probably find something better... */
  MxFocusable   *last_focused_item;

  ClutterState  *state;

  guint slideshow_source;
  guint controls_timeout;

  guint playing : 1;
  guint info_visible : 1;
  guint controls_prev_visible : 1;

  gpointer download_id;
};

enum
{
  CLOSE_REQUEST,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };


static void reset_controls_timeout (MexSlideShow *show);

static void download_queue_completed (MexDownloadQueue *queue,
                                      const gchar      *uri,
                                      const gchar      *buffer,
                                      gsize             count,
                                      const GError     *error,
                                      gpointer          user_data);

static void mex_slide_show_set_playing (MexSlideShow *slideshow,
                                        gboolean      playing);

static void mex_slide_show_show (MexSlideShow *self);

/* MxFocusable Implementation */
static MxFocusable*
mex_slide_show_accept_focus (MxFocusable *focusable,
                             MxFocusHint  focus_hint)
{
  MexSlideShowPrivate *priv = MEX_SLIDE_SHOW (focusable)->priv;
  MxFocusable *button;

  /* If we have a previously select item while switching back and
     forth from controls to info panel, refocus that item. */
  if (priv->last_focused_item)
    return mx_focusable_accept_focus (priv->last_focused_item, focus_hint);

  /* Otherwise, always focus the play/pause button. */
  button = MX_FOCUSABLE (clutter_script_get_object (priv->script,
                                                    "play-pause-button"));

  return mx_focusable_accept_focus (button, focus_hint);
}

static MxFocusable*
mex_slide_show_move_focus (MxFocusable *focusable,
                           MxFocusDirection direction,
                           MxFocusable *old_focus)
{
  if (direction == MX_FOCUS_DIRECTION_UP)
    clutter_state_set_state (MEX_SLIDE_SHOW (focusable)->priv->state,
                             "slideshow");
  return NULL;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = mex_slide_show_accept_focus;
  iface->move_focus = mex_slide_show_move_focus;
}

/* MexContentView Implementation */


static MexModel*   mex_slide_show_get_model (MexContentView *view);
static void        mex_slide_show_set_model (MexContentView *view,
                                            MexModel       *model);

static void        mex_slide_show_set_content (MexContentView *view,
                                               MexContent     *content);
static MexContent* mex_slide_show_get_content (MexContentView *view);

static void
mex_slide_show_content_view_iface_init (MexContentViewIface *iface)
{
  iface->get_content = mex_slide_show_get_content;
  iface->set_content = mex_slide_show_set_content;

  iface->get_context = mex_slide_show_get_model;
  iface->set_context = mex_slide_show_set_model;
}

static gboolean
allowed_content (MexContent *content)
{
  const gchar *mimetype;

  mimetype = mex_content_get_metadata (MEX_CONTENT (content),
                                       MEX_CONTENT_METADATA_MIMETYPE);

  return (strncmp (mimetype, "image/", 6) == 0);
}

static GQuark
image_rotation_quark ()
{
  static GQuark result;

  if (!result)
    result = g_quark_from_static_string ("image-rotation");

  return result;
}

static GQuark
image_fit_quark ()
{
  static GQuark result;

  if (!result)
    result = g_quark_from_static_string ("image-fit-option");

  return result;
}

static void
mex_slide_show_real_set_content (MexSlideShow *show,
                                 MexContent   *content)
{
  MexSlideShowPrivate *priv = MEX_SLIDE_SHOW (show)->priv;
  ClutterActor *title;
  GError *err = NULL;
  const gchar *url;
  GList *list, *l;
  ClutterContainer *container;
  MexDownloadQueue *queue;

  if (content == priv->content)
    return;

  url = mex_content_get_metadata (content, MEX_CONTENT_METADATA_STREAM);

  if (!url)
    {
      g_warning ("Slide show: NULL url");
      return;
    }

  if (priv->content) {
    mex_content_set_last_used_metadatas (priv->content);
    mex_content_save_metadata (priv->content);
    g_object_unref (priv->content);
    priv->content = NULL;
  }
  priv->content = content;
  g_object_ref (priv->content);

  queue = mex_download_queue_get_default ();

  if (priv->download_id)
    mex_download_queue_cancel (queue, priv->download_id);

  priv->download_id = mex_download_queue_enqueue (queue, url,
                                                  download_queue_completed,
                                                  show);

  if (err)
    {
      g_warning ("Slide show could not load \"%s\"", url);
      g_clear_error (&err);
    }

  title = (ClutterActor*) clutter_script_get_object (priv->script,
                                                     "title-label");

  mx_label_set_text (MX_LABEL (title),
                     mex_content_get_metadata (content,
                                               MEX_CONTENT_METADATA_TITLE));

  container = CLUTTER_CONTAINER (clutter_script_get_object (priv->script,
                                                            "photo-strip"));
  list = clutter_container_get_children (container);

  for (l = list; l; l = g_list_next (l))
    {
      /* mark the current content tile as active and make sure others are
       * not marked active */

      if (mex_content_view_get_content (l->data) == content)
        mx_stylable_style_pseudo_class_add (MX_STYLABLE (l->data), "active");
      else
        mx_stylable_style_pseudo_class_remove (MX_STYLABLE (l->data), "active");
    }

  g_list_free (list);
}

static void
mex_slide_show_get_property (GObject    *object,
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
mex_slide_show_set_property (GObject      *object,
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
mex_slide_show_dispose (GObject *object)
{
  MexSlideShow *slide_show = MEX_SLIDE_SHOW (object);
  MexSlideShowPrivate *priv = slide_show->priv;

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

  if (priv->script)
    {
      g_object_unref (priv->script);
      priv->script = NULL;
    }

  if (priv->proxy)
    {
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  G_OBJECT_CLASS (mex_slide_show_parent_class)->dispose (object);
}

static void
mex_slide_show_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_slide_show_parent_class)->finalize (object);
}

static void
mex_slide_show_unmap (ClutterActor *actor)
{
  MexSlideShowPrivate *priv = MEX_SLIDE_SHOW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_slide_show_parent_class)->unmap (actor);

  if (priv->proxy)
    g_object_set (G_OBJECT (priv->proxy), "model", NULL, NULL);
}

static void
mex_slide_show_class_init (MexSlideShowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexSlideShowPrivate));

  object_class->get_property = mex_slide_show_get_property;
  object_class->set_property = mex_slide_show_set_property;
  object_class->dispose = mex_slide_show_dispose;
  object_class->finalize = mex_slide_show_finalize;

  actor_class->unmap = mex_slide_show_unmap;

  signals[CLOSE_REQUEST] = g_signal_new ("close-request",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}

static gint
get_content_rotation (MexContent *content, gboolean *is_unset)
{
  gint angle;

  angle = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (content),
                                               image_rotation_quark ()));

  if (angle == 0) {
    if (is_unset)
      *is_unset = TRUE;
    return 0;
  }

  if (is_unset)
    *is_unset = FALSE;

  return angle - 1;
}

static void
set_content_rotation (MexContent *content, gint angle)
{
  g_object_set_qdata (G_OBJECT (content),
                      image_rotation_quark (),
                      GINT_TO_POINTER (angle + 1));
}

static void
rotate_clicked_cb (ClutterActor *button,
                   MexSlideShow *self)
{
  MexSlideShowPrivate *priv = self->priv;
  gint angle;

  angle = get_content_rotation (priv->content, NULL);

  if (angle == 0)
    {
      angle = 360;
      mx_image_set_image_rotation (MX_IMAGE (priv->image), angle);
    }

  angle -= 90;

  set_content_rotation (priv->content, angle);

  clutter_actor_animate (priv->image, CLUTTER_EASE_OUT_SINE, 250,
                         "image-rotation", (gfloat) angle, NULL);
}

static void play_pause_action_cb (MxAction *action, MexSlideShow *slide_show);

static void
update_tile_position (ClutterActor           *tile,
                      ClutterActorBox        *box,
                      ClutterAllocationFlags  flags,
                      MexScrollView          *scrollview)
{
  ClutterGeometry geo;

  clutter_actor_get_allocation_geometry (tile, &geo);
  mex_scroll_view_ensure_visible (scrollview, &geo);
}

static gboolean
mex_slide_show_move (MexSlideShow *slideshow,
                     gint          pos)
{
  MexSlideShowPrivate *priv = MEX_SLIDE_SHOW (slideshow)->priv;
  MexContent *content;
  ClutterContainer *container;
  GList *list, *l;
  gint idx;

  idx = mex_model_index (priv->model, priv->content);

  idx += pos;

  /* find the next allowed content in the model */
  while ((content = mex_model_get_content (priv->model, idx)))
    {
      if (allowed_content (content))
        break;
      else
        idx++;
    }


  if (content)
    {
      ClutterActor *scrollview;
      container = CLUTTER_CONTAINER (clutter_script_get_object (priv->script,
                                                                "photo-strip"));

      scrollview = clutter_actor_get_parent (CLUTTER_ACTOR (container));

      mex_content_view_set_content (MEX_CONTENT_VIEW (priv->info_panel),
                                    content);

      list = clutter_container_get_children (container);

      /* find the actor in the photo strip that represents the new content that
       * will be displayed */
      for (l = list; l; l = g_list_next (l))
        {
          if (mex_content_view_get_content (l->data) == content)
            {
              /* disconnect old signal handler */
              if (priv->current_tile)
                g_signal_handlers_disconnect_by_func (priv->current_tile,
                                                      update_tile_position,
                                                      scrollview);
              priv->current_tile = l->data;

              /* move the actor into the centre of the strip */
              update_tile_position (priv->current_tile, NULL, 0,
                                    MEX_SCROLL_VIEW (scrollview));

              mex_slide_show_real_set_content (slideshow, content);

              /* set the actor as the 'prior' focus */
              mx_focusable_move_focus (MX_FOCUSABLE (container),
                                       MX_FOCUS_DIRECTION_OUT,
                                       l->data);

              /* ensure the tile is always in the middle */
              g_signal_connect (priv->current_tile, "allocation-changed",
                                G_CALLBACK (update_tile_position), scrollview);
              g_object_add_weak_pointer (G_OBJECT (priv->current_tile),
                                         (gpointer *) &priv->current_tile);
              break;
            }
        }

      g_list_free (list);

      return TRUE;
    }
  else
    {
      /* cannot move, so stop the slide show from playing */
      mex_slide_show_set_playing (slideshow, FALSE);

      return FALSE;
    }
}

static gboolean
slide_show_timeout (gpointer slideshow)
{
  return mex_slide_show_move (slideshow, 1);
}

static void
mex_slide_show_set_playing (MexSlideShow *slideshow,
                            gboolean      playing)
{
  MexSlideShowPrivate *priv = slideshow->priv;
  MxStylable *button;
  const gchar *name;

  priv->playing = playing;

  if (playing)
    {
      name = "MediaPause";

      priv->slideshow_source = g_timeout_add_seconds (5,
                                                      slide_show_timeout,
                                                      slideshow);
    }
  else
    {
      name = "MediaPlay";

      if (priv->slideshow_source)
        g_source_remove (priv->slideshow_source);
      priv->slideshow_source = 0;
    }


  button = MX_STYLABLE (clutter_script_get_object (priv->script,
                                                   "play-pause-button"));

  mx_stylable_set_style_class (button, name);
}

static void
play_pause_action_cb (MxAction     *action,
                      MexSlideShow *slideshow)
{
  mex_slide_show_set_playing (slideshow, !slideshow->priv->playing);

  /* hide the controls when the slideshow is started */
  clutter_state_set_state (slideshow->priv->state, "slideshow");
}

static void
stop_action_cb (MxAction     *action,
                MexSlideShow *slide_show)
{
  MexSlideShowPrivate *priv = slide_show->priv;

  g_signal_emit (slide_show, signals[CLOSE_REQUEST], 0);

  mx_image_clear (MX_IMAGE (priv->image));

  mex_slide_show_set_playing (slide_show, FALSE);
}

static void
fit_to_screen_toggled_cb (MxButton     *button,
                          GParamSpec   *pspec,
                          MexSlideShow *slide_show)
{
  MexSlideShowPrivate *priv = slide_show->priv;
  gboolean toggled;

  toggled = mx_button_get_toggled (button);

  if (!toggled)
    mx_image_animate_scale_mode (MX_IMAGE (priv->image),
                                 CLUTTER_EASE_OUT_SINE, 250,
                                 MX_IMAGE_SCALE_FIT);
  else
    mx_image_animate_scale_mode (MX_IMAGE (priv->image),
                                 CLUTTER_EASE_OUT_SINE, 250,
                                 MX_IMAGE_SCALE_CROP);


  g_object_set_qdata (G_OBJECT (priv->content), image_fit_quark (),
                      GINT_TO_POINTER ((int) toggled));
}

static gboolean
captured_event_cb (ClutterActor *actor,
                   ClutterEvent *event,
                   gpointer      user_data)
{
  MexSlideShowPrivate *priv = MEX_SLIDE_SHOW (actor)->priv;
  MxFocusable *play_pause;

  if (event->type == CLUTTER_MOTION)
    {
      mex_slide_show_show (MEX_SLIDE_SHOW (actor));
      return FALSE;
    }

  /* ensure that any key press cancels the hiding of the controls, or that
   * if the down key is pressed, the controls are made visible */
  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;


  if (MEX_KEY_INFO (event->key.keyval))
    {
      mex_slide_show_set_playing (MEX_SLIDE_SHOW (actor), FALSE);

      /* toggle the info panel */
      if (g_str_equal (clutter_state_get_state (priv->state), "info"))
        {
          ClutterActor *stage = clutter_actor_get_stage (CLUTTER_ACTOR (priv->controls));
          MxFocusManager *manager =
            mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));

          if (manager)
            priv->last_focused_item = mx_focus_manager_get_focused (manager);

          if (priv->controls_prev_visible)
            clutter_state_set_state (priv->state, "controls");
          else
            {
              clutter_state_set_state (priv->state, "slideshow");
              priv->last_focused_item = NULL;
            }
        }
      else
        {
          if (g_str_equal (clutter_state_get_state (priv->state),
                          "controls"))
            priv->controls_prev_visible = TRUE;
          else
            priv->controls_prev_visible = FALSE;

          mex_content_view_set_content (MEX_CONTENT_VIEW (priv->info_panel),
                                        priv->content);

          clutter_state_set_state (priv->state, "info");
        }

      return TRUE;
    }

  /* if the controls are visible, reset the time out */
  if (priv->controls_timeout)
    {
      reset_controls_timeout (MEX_SLIDE_SHOW (actor));
      return FALSE;
    }

  /* handle key events when neither the controls or info panel are visible */
  switch (event->key.keyval)
    {
    case CLUTTER_KEY_Down:
      mex_slide_show_set_playing (MEX_SLIDE_SHOW (actor), FALSE);
      play_pause =
        (MxFocusable *) clutter_script_get_object (priv->script,
                                                   "play-pause-button");
      clutter_state_set_state (priv->state, "controls");
      mex_push_focus (play_pause);
      return TRUE;

    case CLUTTER_KEY_Left:
      mex_slide_show_set_playing (MEX_SLIDE_SHOW (actor), FALSE);
      mex_slide_show_move (MEX_SLIDE_SHOW (actor), -1);
      return TRUE;

    case CLUTTER_KEY_Right:
      mex_slide_show_set_playing (MEX_SLIDE_SHOW (actor), FALSE);
      mex_slide_show_move (MEX_SLIDE_SHOW (actor), 1);
      return TRUE;
    }

  /* capture all key events to prevent the controls from responding
   * to events while they are hidden */
  return TRUE;
}

static void
image_loaded (MxImage      *image,
              MexSlideShow *show)
{
  MexSlideShowPrivate *priv = show->priv;
  const gchar *orientation;
  gint angle;
  gboolean fit, rotation_unset;

  angle = get_content_rotation (priv->content, &rotation_unset);

  if (rotation_unset)
    {
      orientation = mex_content_get_metadata (priv->content,
                                              MEX_CONTENT_METADATA_ORIENTATION);
      if (orientation)
        angle = atoi (orientation);
    }

  mx_image_set_image_rotation (MX_IMAGE (priv->image), angle);

  set_content_rotation (priv->content, angle);

  fit = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (priv->content),
                                             image_fit_quark ()));
  mx_image_set_scale_mode (MX_IMAGE (priv->image), (fit) ? MX_IMAGE_SCALE_CROP :
                           MX_IMAGE_SCALE_FIT);
  mx_button_set_toggled (MX_BUTTON (priv->fit_to_screen_button), fit);
}

static gboolean
controls_timeout (ClutterActor *slideshow)
{
  MexSlideShowPrivate *priv = MEX_SLIDE_SHOW (slideshow)->priv;

  priv->last_focused_item = NULL;
  clutter_state_set_state (priv->state, "slideshow");

  priv->controls_timeout = 0;

  return FALSE;
}

static void
reset_controls_timeout (MexSlideShow *show)
{
  MexSlideShowPrivate *priv = show->priv;

  if (priv->controls_timeout)
    {
      g_source_remove (priv->controls_timeout);
      priv->controls_timeout = 0;
    }

  /* set the timeout to hide the controls if they are now visible */
  priv->controls_timeout =
    g_timeout_add_seconds (5, (GSourceFunc) controls_timeout, show);
}

static void
state_notify_cb (ClutterState *state,
                 GParamSpec   *pspec,
                 MexSlideShow *show)
{
  MexSlideShowPrivate *priv = show->priv;
  const gchar *newstate;

  newstate = clutter_state_get_state (state);

  if (g_str_equal (newstate, "controls"))
    {
      /* set the timeout to hide the controls if they are now visible */
      reset_controls_timeout (show);
    }
  else
    {
      /* controls are not visible, so remove any existing timeout */
      if (priv->controls_timeout)
        {
          g_source_remove (priv->controls_timeout);
          priv->controls_timeout = 0;
        }
    }
}


static void
mex_slide_show_hide (MexSlideShow *self)
{
  clutter_state_set_state (self->priv->state, "slideshow");
}

static void
mex_slide_show_show (MexSlideShow *self)
{
  reset_controls_timeout (self);
  clutter_state_set_state (self->priv->state, "controls");
}

static gboolean
button_press_event_cb (ClutterActor *image,
                       ClutterEvent *event,
                       MexSlideShow *self)
{
  mex_slide_show_show (self);
  return FALSE;
}

static gboolean
button_release_event_cb (ClutterActor *image,
                         ClutterEvent *event,
                         MexSlideShow *self)
{
  if (event->button.click_count == 2)
    mex_toggle_fullscreen ();
  return TRUE;
}

static void
mex_slide_show_init (MexSlideShow *self)
{
  MexSlideShowPrivate *priv;
  ClutterActor *actor;
  GError *err = NULL;
  MxAction *action;
  gchar *tmp;

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

  self->priv = priv = SLIDE_SHOW_PRIVATE (self);

  priv->script = clutter_script_new ();

  tmp = g_build_filename (mex_get_data_dir (), "json", "slide-show.json",
                          NULL);
  clutter_script_load_from_file (priv->script, tmp, &err);
  g_free (tmp);

  priv->image = (ClutterActor*) clutter_script_get_object (priv->script,
                                                           "viewer");
  clutter_actor_set_reactive (priv->image, TRUE);
  g_signal_connect (priv->image, "image-loaded", G_CALLBACK (image_loaded),
                    self);
  g_signal_connect (priv->image, "button-press-event",
                    G_CALLBACK (button_press_event_cb), self);
  g_signal_connect (priv->image, "button-release-event",
                    G_CALLBACK (button_release_event_cb), self);

  priv->controls = (ClutterActor*) clutter_script_get_object (priv->script,
                                                              "controls");
  priv->info_panel = (ClutterActor*) clutter_script_get_object (priv->script,
                                                                "info-panel");

  actor = CLUTTER_ACTOR (clutter_script_get_object (priv->script,
                                                    "slide-show"));

  g_signal_connect (self, "captured-event", G_CALLBACK (captured_event_cb),
                    NULL);

  if (!actor)
    g_warning ("Could not load slide show interface");

  mx_bin_set_child (MX_BIN (self), actor);
  mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);

  if (err)
    g_error ("Could not load slide show interface: %s", err->message);

  actor = CLUTTER_ACTOR (clutter_script_get_object (priv->script,
                                                    "rotate-button"));
  g_signal_connect (actor, "clicked", G_CALLBACK (rotate_clicked_cb), self);

  actor = CLUTTER_ACTOR (clutter_script_get_object (priv->script,
                                                    "fit-to-screen-button"));
  g_signal_connect (actor, "notify::toggled",
                    G_CALLBACK (fit_to_screen_toggled_cb), self);
  priv->fit_to_screen_button = actor;

  action = (MxAction *) clutter_script_get_object (priv->script,
                                                   "play-pause-action");

  g_signal_connect (action, "activated", G_CALLBACK (play_pause_action_cb),
                    self);



  action = (MxAction *) clutter_script_get_object (priv->script,
                                                   "stop-action");

  g_signal_connect (action, "activated", G_CALLBACK (stop_action_cb),
                    self);

  /* setup the state */
  priv->state = (ClutterState *) clutter_script_get_object (priv->script,
                                                            "state");
  clutter_state_set_state (priv->state, "slideshow");
  g_signal_connect (priv->state, "notify::state", G_CALLBACK (state_notify_cb),
                    self);

  g_signal_connect (self, "hide", G_CALLBACK (mex_slide_show_hide), NULL);
  g_signal_connect (self, "show", G_CALLBACK (mex_slide_show_show), NULL);
}

ClutterActor *
mex_slide_show_new (void)
{
  return g_object_new (MEX_TYPE_SLIDE_SHOW, NULL);
}

static void
download_queue_completed (MexDownloadQueue *queue,
                          const gchar      *uri,
                          const gchar      *buffer,
                          gsize             count,
                          const GError     *error,
                          gpointer          user_data)
{
  MexSlideShowPrivate *priv = MEX_SLIDE_SHOW (user_data)->priv;
  MxImage *image = MX_IMAGE (priv->image);
  GError *suberror = NULL;
  gfloat width, height;

  priv->download_id = 0;

  if (error)
    {
      g_warning ("Error loading %s: %s", uri, error->message);
      return;
    }

  clutter_actor_get_size (CLUTTER_ACTOR (image), &width, &height);

  /* TODO: Find a way of not having to do a g_memdup here */
  if (!mx_image_set_from_buffer_at_size (image, g_memdup (buffer, count), count,
                                         (GDestroyNotify)g_free, width, height,
                                         &suberror))
    {
      g_warning ("Error loading %s: %s", uri, suberror->message);
      g_error_free (suberror);

      return;
    }

}

static gboolean
tile_focus_in_cb (ClutterActor *actor,
                  gpointer      user_data)
{
  MexContent *content;

  content = mex_content_view_get_content (MEX_CONTENT_VIEW (actor));

  mex_slide_show_real_set_content (MEX_SLIDE_SHOW (user_data), content);

  /* the slide show has been manually moved to a different item, so stop the
   * slide show from playing */
  mex_slide_show_set_playing (MEX_SLIDE_SHOW (user_data), FALSE);

  return FALSE;
}

static void
notify_pseudo_class (MxBin *actor)
{
  ClutterEffect *shadow;
  ClutterActor *enable_shadow, *disable_shadow;

  if (mx_stylable_style_pseudo_class_contains (MX_STYLABLE (actor), "active")
      || mx_stylable_style_pseudo_class_contains (MX_STYLABLE (actor), "focus"))
    {
      enable_shadow = (ClutterActor*) actor;
      disable_shadow = mx_bin_get_child (actor);
    }
  else
    {
      enable_shadow = mx_bin_get_child (actor);
      disable_shadow = (ClutterActor*) actor;
    }

  shadow = clutter_actor_get_effect (enable_shadow, "shadow");
  clutter_actor_meta_set_enabled (CLUTTER_ACTOR_META (shadow), TRUE);

  shadow = clutter_actor_get_effect (disable_shadow, "shadow");
  clutter_actor_meta_set_enabled (CLUTTER_ACTOR_META (shadow), FALSE);
}

static void
tile_created_cb (MexProxy *proxy,
                 GObject  *content,
                 GObject  *object,
                 gpointer  slideshow)
{
  ClutterEffect *shadow;
  ClutterColor color = { 0, 0, 0, 60 };

  /* remove any content from the slide show that is not an image */
  if (!allowed_content (MEX_CONTENT (content)))
    {
      g_signal_stop_emission_by_name (proxy, "object-created");
      return;
    }

  mex_tile_set_important (MEX_TILE (object), TRUE);

  g_object_set (object, "thumb-height", 81, "thumb-width", 108,
                "header-visible", FALSE, NULL);

  g_signal_connect (object, "focus-in", G_CALLBACK (tile_focus_in_cb),
                    slideshow);

  shadow = g_object_new (MEX_TYPE_SHADOW,
                         "radius-x", 15,
                         "radius-y", 15,
                         "color", &color,
                         NULL);
  clutter_actor_add_effect_with_name (CLUTTER_ACTOR (object), "shadow",
                                      CLUTTER_EFFECT (shadow));
  shadow = g_object_new (MEX_TYPE_SHADOW,
                         "radius-x", 15,
                         "radius-y", 15,
                         "color", &color,
                         NULL);
  clutter_actor_add_effect_with_name (mx_bin_get_child (MX_BIN (object)),
                                      "shadow", CLUTTER_EFFECT (shadow));

  g_signal_connect (object, "notify::style-pseudo-class",
                    G_CALLBACK (notify_pseudo_class), NULL);

  notify_pseudo_class (MX_BIN (object));

  /* ensure the correct item is highlighted in the photo strip */
  mex_slide_show_move (slideshow, 0);
}

static void
mex_slide_show_set_model (MexContentView *view,
                          MexModel       *model)
{
  MexSlideShowPrivate *priv = MEX_SLIDE_SHOW (view)->priv;
  MexModel *orig_model;

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (model)
    {
      ClutterContainer *container;

      container = CLUTTER_CONTAINER (clutter_script_get_object (priv->script,
                                                                "photo-strip"));

      /* remove the existing actors */
      clutter_container_foreach (container,
                                 (ClutterCallback) clutter_actor_destroy,
                                 NULL);

      if (priv->proxy)
        g_object_unref (priv->proxy);

      /* FIXME: Set an arbitrary 200-item limit as we can't handle large
       *        amounts of actors without massive slow-down.
       */
      orig_model = mex_model_get_model (model);
      priv->model = g_object_new (MEX_TYPE_VIEW_MODEL,
                                  "model", orig_model,
                                  "limit", 200,
                                  NULL);

      priv->proxy = mex_content_proxy_new (priv->model,
                                           container,
                                           MEX_TYPE_CONTENT_TILE);
      g_signal_connect (priv->proxy, "object-created",
                        G_CALLBACK (tile_created_cb), view);
    }
}

static MexModel *
mex_slide_show_get_model (MexContentView *view)
{
  return MEX_SLIDE_SHOW (view)->priv->model;
}

static void
mex_slide_show_set_content (MexContentView *view,
                            MexContent     *content)
{
  MexSlideShow *show = MEX_SLIDE_SHOW (view);
  MexSlideShowPrivate *priv = show->priv;

  mx_image_clear (MX_IMAGE (priv->image));

  mex_slide_show_real_set_content (show, content);

  /* ensure the new content is highlighted in the photo strip */
  mex_view_model_stop (MEX_VIEW_MODEL (priv->model));
  mex_view_model_start_at_content (MEX_VIEW_MODEL (priv->model),
                                   content, FALSE);
  mex_slide_show_move (show, 0);

  clutter_state_set_state (show->priv->state, "controls");
}

static MexContent*
mex_slide_show_get_content (MexContentView *view)
{
  return MEX_SLIDE_SHOW (view)->priv->content;
}
