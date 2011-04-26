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


#include "mex-media-controls.h"
#include "mex-view-model.h"
#include "mex.h"

static void mex_media_controls_sort_items (MexMediaControls *self);

static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexMediaControls, mex_media_controls, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init))

#define MEDIA_CONTROLS_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MEDIA_CONTROLS, MexMediaControlsPrivate))

enum
{
  PROP_0,

  PROP_MEDIA,
  PROP_PLAYING_QUEUE
};

struct _MexMediaControlsPrivate
{
  ClutterMedia *media;

  ClutterActor *vbox;
  ClutterActor *slider;
  ClutterActor *queue_button;

  ClutterScript *script;

  MxAction *play_pause_action;
  MxAction *stop_action;
  MxAction *add_to_queue_action;

  MexContent *content;

  ClutterEffect *vertical_effect;
  ClutterEffect *horizontal_effect;

  guint key_press_timeout;
  guint long_press_activated : 1;
  guint increment : 1;
  guint key_press_count;

  GCompareDataFunc sort_func;
  gpointer sort_data;

  MexProxy *proxy;

  guint is_queue_model : 1;

  MexViewModel *model;
};

enum
{
  STOPPED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };


/* MxFocusableIface */

static MxFocusable *
mex_media_controls_move_focus (MxFocusable      *focusable,
                               MxFocusDirection  direction,
                               MxFocusable      *from)
{
  return NULL;
}

static MxFocusable *
mex_media_controls_accept_focus (MxFocusable *focusable,
                                 MxFocusHint  hint)
{
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (focusable)->priv;

  return mx_focusable_accept_focus (MX_FOCUSABLE (priv->vbox),
                                    MX_FOCUS_HINT_FIRST);
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_media_controls_move_focus;
  iface->accept_focus = mex_media_controls_accept_focus;
}

/* Actor implementation */

static void
mex_media_controls_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MexMediaControls *self = MEX_MEDIA_CONTROLS (object);

  switch (property_id)
    {
    case PROP_MEDIA:
      g_value_set_object (value, mex_media_controls_get_media (self));
      break;

    case PROP_PLAYING_QUEUE:
      g_value_set_boolean (value, mex_media_controls_get_playing_queue (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_media_controls_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MexMediaControls *self = MEX_MEDIA_CONTROLS (object);

  switch (property_id)
    {
    case PROP_MEDIA:
      mex_media_controls_set_media (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_media_controls_dispose (GObject *object)
{
  MexMediaControls *self = MEX_MEDIA_CONTROLS (object);
  MexMediaControlsPrivate *priv = self->priv;

  if (priv->proxy)
    {
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->media)
    mex_media_controls_set_media (self, NULL);

  if (priv->script)
    {
      g_object_unref (priv->script);
      priv->script = NULL;
    }

  if (priv->vbox)
    {
      clutter_actor_destroy (priv->vbox);
      priv->vbox = NULL;
    }

  G_OBJECT_CLASS (mex_media_controls_parent_class)->dispose (object);
}

static void
mex_media_controls_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_media_controls_parent_class)->finalize (object);
}

static void
mex_media_controls_get_preferred_width (ClutterActor *actor,
                                        gfloat        for_height,
                                        gfloat       *min_width_p,
                                        gfloat       *nat_width_p)
{
  MxPadding padding;
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (for_height >= 0)
    for_height = MAX (0, for_height - padding.top - padding.bottom);

  clutter_actor_get_preferred_width (priv->vbox,
                                     for_height,
                                     min_width_p,
                                     nat_width_p);

  if (min_width_p)
    *min_width_p += padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p += padding.left + padding.right;
}

static void
mex_media_controls_get_preferred_height (ClutterActor *actor,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *nat_height_p)
{
  MxPadding padding;
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (for_width >= 0)
    for_width = MAX (0, for_width - padding.left - padding.right);

  clutter_actor_get_preferred_height (priv->vbox,
                                     for_width,
                                     min_height_p,
                                     nat_height_p);

  if (min_height_p)
    *min_height_p += padding.top + padding.bottom;
  if (nat_height_p)
    *nat_height_p += padding.top + padding.bottom;
}

static void
mex_media_controls_allocate (ClutterActor           *actor,
                             const ClutterActorBox  *box,
                             ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_media_controls_parent_class)->
    allocate (actor, box, flags);

  mx_widget_get_available_area (MX_WIDGET (actor), box, &child_box);
  clutter_actor_allocate (priv->vbox, &child_box, flags);
}

static void
mex_media_controls_paint (ClutterActor *actor)
{
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_media_controls_parent_class)->paint (actor);

  clutter_actor_paint (priv->vbox);
}

static void
mex_media_controls_pick (ClutterActor       *actor,
                         const ClutterColor *color)
{
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_media_controls_parent_class)->pick (actor, color);

  clutter_actor_paint (priv->vbox);
}

static void
mex_media_controls_map (ClutterActor *actor)
{
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_media_controls_parent_class)->map (actor);

  clutter_actor_map (priv->vbox);
}

static void
mex_media_controls_unmap (ClutterActor *actor)
{
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_media_controls_parent_class)->unmap (actor);

  clutter_actor_unmap (priv->vbox);
  g_object_set (G_OBJECT (priv->model), "model", NULL, NULL);
}

static void
mex_media_controls_class_init (MexMediaControlsClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexMediaControlsPrivate));

  object_class->get_property = mex_media_controls_get_property;
  object_class->set_property = mex_media_controls_set_property;
  object_class->dispose = mex_media_controls_dispose;
  object_class->finalize = mex_media_controls_finalize;

  actor_class->get_preferred_width = mex_media_controls_get_preferred_width;
  actor_class->get_preferred_height = mex_media_controls_get_preferred_height;
  actor_class->allocate = mex_media_controls_allocate;
  actor_class->paint = mex_media_controls_paint;
  actor_class->pick = mex_media_controls_pick;
  actor_class->map = mex_media_controls_map;
  actor_class->unmap = mex_media_controls_unmap;

  pspec = g_param_spec_object ("media",
                               "Media",
                               "The ClutterMedia object the controls apply to.",
                               G_TYPE_OBJECT,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MEDIA, pspec);


  signals[STOPPED] = g_signal_new ("stopped",
                                   G_TYPE_FROM_CLASS (klass),
                                   G_SIGNAL_RUN_LAST,
                                   0, NULL, NULL,
                                   g_cclosure_marshal_VOID__VOID,
                                   G_TYPE_NONE, 0);
}

static void
mex_media_controls_play_cb (MxButton         *toggle,
                            MexMediaControls *self)
{
  MexMediaControlsPrivate *priv = self->priv;

  if (priv->media)
    clutter_media_set_playing (priv->media,
                               !clutter_media_get_playing (priv->media));
}

static void
mex_media_controls_stop_cb (MxButton         *toggle,
                            MexMediaControls *self)
{
  MexMediaControlsPrivate *priv = self->priv;

  if (priv->media)
    clutter_media_set_playing (priv->media, FALSE);

  g_signal_emit (self, signals[STOPPED], 0);
}

static void
slider_value_changed_cb (MxSlider         *slider,
                         GParamSpec       *pspec,
                         MexMediaControls *controls)
{
  MexMediaControlsPrivate *priv = controls->priv;

  if (priv->media)
    clutter_media_set_progress (priv->media,
                                mx_slider_get_value (MX_SLIDER (priv->slider)));
}

static gboolean
key_press_timeout_cb (gpointer data)
{
  MexMediaControlsPrivate *priv = MEX_MEDIA_CONTROLS (data)->priv;
  gdouble duration;
  gdouble change;
  gfloat progress;

  priv->long_press_activated = TRUE;

  priv->key_press_count++;

  duration = clutter_media_get_duration (priv->media);
  progress = clutter_media_get_progress (priv->media);

  if (priv->key_press_count >= 10)
    change = 60;
  else
    change = 10;

  if (priv->increment)
    progress = MIN (1.0, ((duration * progress) + change) / duration);
  else
    progress = MAX (0.0, ((duration * progress) - change) / duration);

  clutter_media_set_progress (priv->media, progress);

  return TRUE;
}

static gboolean
slider_captured_event (MxSlider         *slider,
                       ClutterEvent     *event,
                       MexMediaControls *controls)
{
  MexMediaControlsPrivate *priv = controls->priv;


  /* cancel the long press timeout when a key is released */
  if (event->type == CLUTTER_KEY_RELEASE)
    {
      if (priv->key_press_timeout)
        {
          g_source_remove (priv->key_press_timeout);
          priv->key_press_timeout = 0;
          priv->long_press_activated = FALSE;

          priv->key_press_count = 0;
        }
    }

  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;

  /* handle just left and right key events */
  if (event->key.keyval == CLUTTER_KEY_Left)
    priv->increment = FALSE;
  else if (event->key.keyval == CLUTTER_KEY_Right)
    priv->increment  = TRUE;
  else
    return FALSE;


  /* start the key press timeout if required */
  if (!priv->key_press_timeout)
    {
      priv->long_press_activated = FALSE;

      priv->key_press_timeout = g_timeout_add (250, key_press_timeout_cb,
                                               controls);
      key_press_timeout_cb (controls);
    }

  return TRUE;
}

static void
notify_vertical_changed_cb (MxAdjustment     *adjustment,
                            MexMediaControls *controls)
{
  MexMediaControlsPrivate *priv = controls->priv;
  gint bottom;
  gdouble value, max, page_size;

  max = mx_adjustment_get_upper (adjustment);
  value = mx_adjustment_get_value (adjustment);
  page_size = mx_adjustment_get_page_size (adjustment);

  bottom = MIN (50, (max - value - page_size));

  mx_fade_effect_set_border (MX_FADE_EFFECT (priv->vertical_effect),
                             0, 0, bottom, 0);
}

static void
notify_vertical_value_cb (MxAdjustment     *adjustment,
                          GParamSpec       *pspec,
                          MexMediaControls *controls)
{
  notify_vertical_changed_cb (adjustment, controls);
}


static void
notify_horizontal_changed_cb (MxAdjustment     *adjustment,
                              MexMediaControls *controls)
{
  MexMediaControlsPrivate *priv = controls->priv;
  gint left, right;
  gdouble value, min, max, page_size;

  min = mx_adjustment_get_lower (adjustment);
  max = mx_adjustment_get_upper (adjustment);
  value = mx_adjustment_get_value (adjustment);
  page_size = mx_adjustment_get_page_size (adjustment);

  left = MIN (136, (value - min));
  right = MIN (136, (max - value - page_size));

  mx_fade_effect_set_border (MX_FADE_EFFECT (priv->horizontal_effect),
                             0, right, 0, left);
}

static void
notify_horizontal_value_cb (MxAdjustment     *adjustment,
                            GParamSpec       *pspec,
                            MexMediaControls *controls)
{
  notify_horizontal_changed_cb (adjustment, controls);
}

static void
mex_media_controls_update_header (MexMediaControls *self)
{
  MexMediaControlsPrivate *priv = self->priv;
  ClutterActor *label;

  label = (ClutterActor*) clutter_script_get_object (priv->script,
                                                     "title-label");

  mx_label_set_text (MX_LABEL (label),
                     mex_content_get_metadata (priv->content,
                                               MEX_CONTENT_METADATA_TITLE));
}

static void
mex_media_controls_replace_content (MexMediaControls *self,
                                    MexContent       *content)
{
  MexPlayer *player;
  MxScrollable *related_box;
  MxAdjustment *adjustment;
  gdouble upper;

  MexMediaControlsPrivate *priv = self->priv;

  player = mex_player_get_default ();

  mex_content_view_set_content (MEX_CONTENT_VIEW (player), content);

  priv->content = content;
  mex_media_controls_update_header (self);
  mex_content_view_set_content (MEX_CONTENT_VIEW (priv->queue_button),
                                content);

  mex_push_focus ((MxFocusable*) clutter_script_get_object (priv->script,
                                              "play-pause-button"));

  related_box = (MxScrollable *)clutter_script_get_object (priv->script,
                                                           "related-box");
  mx_scrollable_get_adjustments (MX_SCROLLABLE (related_box),
                                 &adjustment, NULL);

  mx_adjustment_get_values (adjustment, NULL, NULL, &upper,
                            NULL, NULL, NULL);

  mx_adjustment_set_value (adjustment, upper);
  mx_scrollable_set_adjustments (MX_SCROLLABLE (related_box),
                                 adjustment,
                                 NULL);
}

static gboolean
key_press_event_cb (ClutterActor    *actor,
                    ClutterKeyEvent *event,
                    gpointer         user_data)
{
  MexMediaControls *self = MEX_MEDIA_CONTROLS (user_data);

  if (event->keyval == MEX_KEY_OK)
    {
      MexContent *content =
        mex_content_view_get_content (MEX_CONTENT_VIEW (actor));
      mex_media_controls_replace_content (self, content);

      return TRUE;
    }

  return FALSE;
}

static gboolean
button_release_event_cb (ClutterActor       *actor,
                         ClutterButtonEvent *event,
                         gpointer            user_data)
{
  MexMediaControls *self = MEX_MEDIA_CONTROLS (user_data);
  MexContent *content = mex_content_view_get_content (MEX_CONTENT_VIEW (actor));

  mex_media_controls_replace_content (self, content);

  return TRUE;
}

static void
tile_focus_in_cb (MxBin *actor)
{
  ClutterActorMeta *shadow;

  shadow = (ClutterActorMeta*) clutter_actor_get_effect (CLUTTER_ACTOR (actor),
                                                         "shadow");
  clutter_actor_meta_set_enabled (shadow, TRUE);

  shadow =
    (ClutterActorMeta*) clutter_actor_get_effect (mx_bin_get_child (actor),
                                                  "shadow");
  clutter_actor_meta_set_enabled (shadow, FALSE);
}

static void
tile_focus_out_cb (MxBin *actor)
{
  ClutterActorMeta *shadow;

  shadow = (ClutterActorMeta*) clutter_actor_get_effect (CLUTTER_ACTOR (actor),
                                                         "shadow");
  clutter_actor_meta_set_enabled (shadow, FALSE);

  shadow =
    (ClutterActorMeta*) clutter_actor_get_effect (mx_bin_get_child (actor),
                                                  "shadow");
  clutter_actor_meta_set_enabled (shadow, TRUE);

}

static void
tile_created_cb (MexProxy *proxy,
                 GObject  *content,
                 GObject  *object,
                 gpointer  controls)
{
  const gchar *mime_type;
  ClutterEffect *effect;
  ClutterColor color = { 0, 0, 0, 60 };

  /* filter out folders */
  mime_type = mex_content_get_metadata (MEX_CONTENT (content),
                                        MEX_CONTENT_METADATA_MIMETYPE);

  if (g_strcmp0 (mime_type, "x-grl/box") == 0)
    {
      g_signal_stop_emission_by_name (proxy, "object-created");
      return;
    }

  mex_tile_set_important (MEX_TILE (object), TRUE);
  clutter_actor_set_reactive (CLUTTER_ACTOR (object), TRUE);

  g_object_set (object, "thumb-height", 140, "thumb-width", 250, NULL);

  g_signal_connect (object, "key-press-event", G_CALLBACK (key_press_event_cb),
                    controls);
  g_signal_connect (object, "button-release-event",
                    G_CALLBACK (button_release_event_cb), controls);

  effect = g_object_new (MEX_TYPE_SHADOW,
                         "radius-x", 15,
                         "radius-y", 15,
                         "color", &color,
                         "enabled", FALSE,
                         NULL);
  clutter_actor_add_effect_with_name (CLUTTER_ACTOR (object), "shadow", effect);

  effect = g_object_new (MEX_TYPE_SHADOW,
                         "radius-x", 15,
                         "radius-y", 15,
                         "color", &color,
                         NULL);
  clutter_actor_add_effect_with_name (mx_bin_get_child (MX_BIN (object)),
                                      "shadow", effect);

  g_signal_connect (object, "focus-in", G_CALLBACK (tile_focus_in_cb), NULL);
  g_signal_connect (object, "focus-out", G_CALLBACK (tile_focus_out_cb), NULL);
  tile_focus_out_cb (MX_BIN (object));


  mex_media_controls_sort_items (controls);
}

static void
mex_media_controls_init (MexMediaControls *self)
{
  ClutterActor *actor;
  ClutterScript *script;
  GError *err = NULL;
  MxAdjustment *adjustment;
  ClutterActor *related_box;
  gchar *tmp;

  MexMediaControlsPrivate *priv = self->priv = MEDIA_CONTROLS_PRIVATE (self);

  priv->script = script = clutter_script_new ();

  tmp = g_build_filename (mex_get_data_dir (), "json", "media-controls.json",
                          NULL);
  clutter_script_load_from_file (script, tmp, &err);
  g_free (tmp);

  if (err)
    {
      g_warning ("Could not load media controls interface: %s", err->message);
      g_clear_error (&err);
      return;
    }

  priv->vbox =
    (ClutterActor*) clutter_script_get_object (script, "media-controls");
  clutter_actor_set_parent (priv->vbox, CLUTTER_ACTOR (self));

  /* add shadow to media controls box */
  actor = (ClutterActor *) clutter_script_get_object (script,
                                                      "media-controls-box");
  clutter_actor_add_effect (actor, CLUTTER_EFFECT (mex_shadow_new ()));


  /* vertical fade effect */
  priv->vertical_effect = mx_fade_effect_new ();
  clutter_actor_add_effect (priv->vbox, priv->vertical_effect);
  mx_scrollable_get_adjustments (MX_SCROLLABLE (mx_bin_get_child (MX_BIN (priv->vbox))),
                                 NULL, &adjustment);
  g_signal_connect (adjustment, "changed",
                    G_CALLBACK (notify_vertical_changed_cb), self);
  g_signal_connect (adjustment, "notify::value",
                    G_CALLBACK (notify_vertical_value_cb), self);

  /* horizontal fade effect */
  priv->horizontal_effect = mx_fade_effect_new ();
  related_box = (ClutterActor *) clutter_script_get_object (priv->script,
                                                            "related-box");
  clutter_actor_add_effect (related_box, priv->horizontal_effect);
  mx_scrollable_get_adjustments (MX_SCROLLABLE (related_box), &adjustment,
                                 NULL);
  g_signal_connect (adjustment, "changed",
                    G_CALLBACK (notify_horizontal_changed_cb), self);
  g_signal_connect (adjustment, "notify::value",
                    G_CALLBACK (notify_horizontal_value_cb), self);


  /* slider setup */
  priv->slider = (ClutterActor*) clutter_script_get_object (script, "slider");
  g_signal_connect (priv->slider, "notify::value",
                    G_CALLBACK (slider_value_changed_cb), self);
  g_signal_connect (priv->slider, "captured-event",
                    G_CALLBACK (slider_captured_event), self);

  priv->play_pause_action =
    (MxAction*) clutter_script_get_object (script, "play-pause-action");

  priv->stop_action =
   (MxAction*) clutter_script_get_object (script, "stop-action");

  priv->add_to_queue_action =
   (MxAction*) clutter_script_get_object (script, "add-to-queue-action");

  priv->queue_button =
    (ClutterActor *) clutter_script_get_object (script, "add-to-queue-button");

  g_signal_connect (priv->play_pause_action, "activated",
                    G_CALLBACK (mex_media_controls_play_cb), self);
  g_signal_connect (priv->stop_action, "activated",
                    G_CALLBACK (mex_media_controls_stop_cb), self);
#if 0
  g_signal_connect (priv->add_to_queue_action, "activated",
                    G_CALLBACK (mex_media_controls_add_to_queue_cb), self);
#endif

  /* proxy setup */

  priv->model = MEX_VIEW_MODEL (mex_view_model_new (NULL));
  g_object_ref_sink (G_OBJECT (priv->model));
  /* FIXME: Set an arbitrary 200-item limit as we can't handle large
   *        amounts of actors without massive slow-down.
   */
  mex_view_model_set_limit (priv->model, 200);

  priv->proxy = mex_content_proxy_new (MEX_MODEL (priv->model),
                                       CLUTTER_CONTAINER (related_box),
                                       MEX_TYPE_CONTENT_TILE);

  g_signal_connect (priv->proxy, "object-created", G_CALLBACK (tile_created_cb),
                    self);
}

ClutterActor *
mex_media_controls_new (void)
{
  return g_object_new (MEX_TYPE_MEDIA_CONTROLS, NULL);
}

static void
mex_media_controls_notify_can_seek_cb (ClutterMedia     *media,
                                       GParamSpec       *pspec,
                                       MexMediaControls *self)
{
  MexMediaControlsPrivate *priv = self->priv;
  gboolean can_seek = clutter_media_get_can_seek (media);

  mx_widget_set_disabled (MX_WIDGET (priv->slider), !can_seek);
}

static void
mex_media_controls_notify_playing_cb (ClutterMedia     *media,
                                      GParamSpec       *pspec,
                                      MexMediaControls *self)
{
  MexMediaControlsPrivate *priv = self->priv;
  MxStylable *button;
  const gchar *name;

  if (clutter_media_get_playing (media))
    name = "MediaPause";
  else
    name = "MediaPlay";

  button = MX_STYLABLE (clutter_script_get_object (priv->script,
                                                   "play-pause-button"));

  mx_stylable_set_style_class (button, name);
}

static void
mex_media_controls_notify_progress_cb (ClutterMedia     *media,
                                       GParamSpec       *pspec,
                                       MexMediaControls *self)
{
  MexMediaControlsPrivate *priv = self->priv;
  MxLabel *label;
  gchar *text;
  gdouble length, progress_s;
  gfloat progress;
  gint len_h, len_m, len_s, pos_h, pos_m, pos_s;

  progress = clutter_media_get_progress (media);
  length = clutter_media_get_duration (media);

  len_h = length / 3600;
  len_m = (length - (len_h * 3600)) / 60;
  len_s = (length - (len_h * 3600) - (len_m * 60));

  progress_s = length * progress;

  pos_h = progress_s / 3600;
  pos_m = (progress_s - (pos_h * 3600)) / 60;
  pos_s = (progress_s - (pos_h * 3600) - (pos_m * 60));

  g_signal_handlers_block_by_func (priv->slider, slider_value_changed_cb, self);
  mx_slider_set_value (MX_SLIDER (priv->slider), progress);
  g_signal_handlers_unblock_by_func (priv->slider, slider_value_changed_cb,
                                     self);

  if (len_h > 0)
    text = g_strdup_printf ("%02d:%02d:%02d / %02d:%02d:%02d",
                            pos_h, pos_m, pos_s, len_h, len_m, len_s);
  else
    text = g_strdup_printf ("%02d:%02d / %02d:%02d",
                            pos_m, pos_s, len_m, len_s);

  label = (MxLabel*) clutter_script_get_object (priv->script, "progress-label");
  mx_label_set_text (label, text);
  g_free (text);
}

static void
mex_media_controls_notify_download_cb (ClutterMedia     *media,
				       gdouble           start,
				       gdouble           stop,
				       MexMediaControls *self)
{
  MexMediaControlsPrivate *priv = self->priv;

  mx_slider_set_buffer_value (MX_SLIDER (priv->slider), stop);
}

void
mex_media_controls_set_media (MexMediaControls *self,
                              ClutterMedia     *media)
{
  MexMediaControlsPrivate *priv;

  g_return_if_fail (MEX_IS_MEDIA_CONTROLS (self));
  g_return_if_fail (!media || CLUTTER_IS_MEDIA (media));

  priv = self->priv;
  if (priv->media != media)
    {
      if (priv->media)
        {
          g_signal_handlers_disconnect_by_func (priv->media,
                                       mex_media_controls_notify_can_seek_cb,
                                       self);
          g_signal_handlers_disconnect_by_func (priv->media,
                                       mex_media_controls_notify_playing_cb,
                                       self);
          g_signal_handlers_disconnect_by_func (priv->media,
                                       mex_media_controls_notify_progress_cb,
                                       self);
          g_signal_handlers_disconnect_by_func (priv->media,
                                       mex_media_controls_notify_download_cb,
                                       self);

          g_object_unref (priv->media);
          priv->media = NULL;
        }

      if (media)
        {
          priv->media = g_object_ref (media);

          g_signal_connect (media, "notify::can-seek",
                            G_CALLBACK (mex_media_controls_notify_can_seek_cb),
                            self);
          g_signal_connect (media, "notify::playing",
                            G_CALLBACK (mex_media_controls_notify_playing_cb),
                            self);
          g_signal_connect (media, "notify::progress",
                            G_CALLBACK (mex_media_controls_notify_progress_cb),
                            self);
	  g_signal_connect (media, "download-buffering",
			    G_CALLBACK (mex_media_controls_notify_download_cb),
			    self);

          mex_media_controls_notify_can_seek_cb (media, NULL, self);
          mex_media_controls_notify_playing_cb (media, NULL, self);
          mex_media_controls_notify_progress_cb (media, NULL, self);
	  mex_media_controls_notify_download_cb (media, 0.0, 0.0, self);
        }

      g_object_notify (G_OBJECT (self), "media");
    }
}

ClutterMedia *
mex_media_controls_get_media (MexMediaControls *self)
{
  g_return_val_if_fail (MEX_IS_MEDIA_CONTROLS (self), NULL);
  return self->priv->media;
}

gboolean
mex_media_controls_get_playing_queue (MexMediaControls *self)
{
  return self->priv->is_queue_model;
}

void
mex_media_controls_set_content (MexMediaControls *self,
                                MexContent       *content,
                                MexModel         *context)
{
  MexMediaControlsPrivate *priv = self->priv;

  g_return_if_fail (MEX_IS_CONTENT (content));

  priv->content = content;
  priv->is_queue_model = FALSE;

  mex_media_controls_update_header (self);

  /* We may not have a context if we're launched by something like SetUri*/
  if (context)
    {
      MexModel *orig_model;

      /* update the related strip */
      mex_view_model_stop (priv->model);

      orig_model = mex_model_get_model (context);
      g_object_set (G_OBJECT (priv->model), "model", orig_model, NULL);

      mex_view_model_start_at_content (priv->model, priv->content, TRUE);
    }

  /* Work out if the context was a queue */
 if (MEX_IS_PROXY_MODEL (context))
   {
     MexModel *model_from_proxy;
     model_from_proxy = mex_proxy_model_get_model (MEX_PROXY_MODEL (context));

     if (MEX_IS_QUEUE_MODEL (model_from_proxy))
       priv->is_queue_model = TRUE;
   }
  /* we may have an aggregate model if the column was the context */
 else if (MEX_IS_AGGREGATE_MODEL (context))
    {
      MexModel *real_model;
      MexModelManager *model_manager;
      const MexModelInfo *model_info;

      model_manager = mex_model_manager_get_default ();
      real_model =
        mex_aggregate_model_get_model_for_content (MEX_AGGREGATE_MODEL (context),
                                                content);

      model_info = mex_model_manager_get_model_info (model_manager, real_model);

      if (model_info)
        if (g_strcmp0 (model_info->category, "queue") == 0)
            priv->is_queue_model = TRUE;
    }

  /* Update content on the queue button */
  mex_content_view_set_content (MEX_CONTENT_VIEW (priv->queue_button),
                                priv->content);
}

void
mex_media_controls_focus_content (MexMediaControls *self,
                                  MexContent       *content)
{
  MexMediaControlsPrivate *priv = self->priv;
  ClutterContainer *container;
  GList *children, *l;

  container = CLUTTER_CONTAINER (clutter_script_get_object (priv->script,
                                                            "related-box"));

  children = clutter_container_get_children (container);

  for (l = children; l; l = g_list_next (l))
    {
      if (mex_content_view_get_content (l->data) == content)
        {
          mex_push_focus (l->data);
          return;
        }
    }

  return;
}

static void
mex_media_controls_sort_items (MexMediaControls *self)
{
  MexMediaControlsPrivate *priv = self->priv;
  ClutterContainer *container;
  GList *children, *l;

  if (!priv->sort_func)
    return;

  container = CLUTTER_CONTAINER (clutter_script_get_object (priv->script,
                                                            "related-box"));

  children = clutter_container_get_children (container);

  children = g_list_sort_with_data (children, priv->sort_func, priv->sort_data);

  for (l = children; l; l = g_list_next (l))
    {
      clutter_actor_raise_top (l->data);
    }

  g_list_free (children);
}

/**
  * mex_media_controls_get_enqueued:
  * @controls: The MexMediaControls widget
  * @current_content: MexContent that the player is currently playing
  *
  * If the media controls has been given a queue model then return the next
  * MexContent in the queue model.
  *
  * Return value: The next content in the queue or NULL
  */
MexContent *
mex_media_controls_get_enqueued (MexMediaControls *controls,
                                 MexContent *current_content)
{
  MexMediaControlsPrivate *priv;
  MexModel *queue;
  MexContent *content = NULL;

  if (!MEX_IS_MEDIA_CONTROLS (controls) || !MEX_IS_CONTENT (current_content))
    return NULL;

  priv = controls->priv;

  if (priv->is_queue_model == FALSE)
    return NULL;

  queue = mex_proxy_get_model (priv->proxy);
  if (queue)
    {
      gint idx, length;

      idx = mex_model_index (queue, current_content);
      length = mex_model_get_length (queue);

      if (idx++ > length)
       return NULL;

      content = mex_model_get_content (queue, idx);
    }

  return content;
}

void
mex_media_controls_set_sort_func (MexMediaControls *self,
                                  GCompareDataFunc  func,
                                  gpointer          userdata)
{
  MexMediaControlsPrivate *priv;

  g_return_if_fail (MEX_IS_MEDIA_CONTROLS (self));

  priv = self->priv;

  if ((priv->sort_func != func) || (priv->sort_data != userdata))
    {
      priv->sort_func = func;
      priv->sort_data = userdata;

      if (func)
        {
          mex_media_controls_sort_items (self);
        }
    }
}
