/*
 * Mex - a media explorer
 *
 * Copyright © 2012 Intel Corporation.
 * Copyright © 2012 sleep(5) Ltd.
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

#include "mex-music-player.h"

#include "mex-content-view.h"
#include "mex-metadata-utils.h"
#include "mex-utils.h"
#include "mex-player.h"

#include "mex-main.h"

#include <clutter-gst/clutter-gst.h>

static void mex_music_player_content_view_iface_init (MexContentViewIface *iface);
static void mex_music_player_focusable_iface_init (MxFocusableIface *iface);
static void mex_music_player_notify_cb (ClutterMedia   *media,
                                        GParamSpec     *pspec,
                                        MexMusicPlayer *player);
static void mex_music_player_eos_cb (ClutterMedia *media,
                                     MexMusicPlayer *player);
static void mex_music_player_repeat_toggled (MexMusicPlayer *player,
                                             GParamSpec     *spec,
                                             MxButton       *button);
static void mex_music_player_shuffle_toggled (MexMusicPlayer *player,
                                              GParamSpec     *spec,
                                              MxButton       *button);
static void mex_music_player_slider_notify (MxSlider       *slider,
                                            GParamSpec     *pspec,
                                            MexMusicPlayer *priv);

G_DEFINE_TYPE_WITH_CODE (MexMusicPlayer, mex_music_player, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_music_player_content_view_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mex_music_player_focusable_iface_init))

#define MUSIC_PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MUSIC_PLAYER, MexMusicPlayerPrivate))

struct _MexMusicPlayerPrivate
{
  ClutterScript *script;

  ClutterActor *title_label;
  ClutterActor *subtitle_label;
  ClutterActor *play_button;

  MexContent *content;
  MexModel *model;
  gint current_index;

  ClutterMedia *player;

  ClutterActor *slider;
  gulong slider_notify_id;

  gboolean repeat;

  GArray *shuffle;
};

enum
{
  CLOSE_REQUEST,
  OPEN_REQUEST,

  LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0, };

/* focusable */
static MxFocusable*
mex_music_player_move_focus (MxFocusable      *focusable,
                             MxFocusDirection  direction,
                             MxFocusable      *from)
{
  return NULL;
}

static MxFocusable*
mex_music_player_accept_focus (MxFocusable *focusable,
                               MxFocusHint  hint)
{
  MexMusicPlayerPrivate *priv = MEX_MUSIC_PLAYER (focusable)->priv;

  return mx_focusable_accept_focus (MX_FOCUSABLE (priv->play_button), hint);
}

static void
mex_music_player_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_music_player_move_focus;
  iface->accept_focus = mex_music_player_accept_focus;
}


/* content view */
static void
mex_music_player_set_content (MexContentView *player,
                              MexContent     *content)
{
  MexMusicPlayerPrivate *priv = MEX_MUSIC_PLAYER (player)->priv;
  gchar *album_artist;
  const gchar *uri, *album, *artist, *title;
  ClutterActorIter iter;
  ClutterActor *child, *container;

  if (priv->content)
    g_object_unref (priv->content);

  priv->content = content;

  if (!content)
    return;

  g_object_ref (content);


  /* title */
  title = mex_content_get_metadata (content, MEX_CONTENT_METADATA_TITLE);
  mx_label_set_text (MX_LABEL (priv->title_label), title);


  /* subtitle (album, artist) */
  album = mex_content_get_metadata (content, MEX_CONTENT_METADATA_ALBUM);
  artist = mex_content_get_metadata (content, MEX_CONTENT_METADATA_ARTIST);
  album_artist = g_strconcat (album, ", ", artist , NULL);
  mx_label_set_text (MX_LABEL (priv->subtitle_label), album_artist);
  g_free (album_artist);

  /* uri */
  uri = mex_content_get_metadata (content, MEX_CONTENT_METADATA_STREAM);
  clutter_media_set_uri (priv->player, uri);

  /* find the item in the list */
  container = mex_script_get_actor (priv->script, "tracks");
  clutter_actor_iter_init (&iter, container);
  while (clutter_actor_iter_next (&iter, &child))
    {
      MexContent *button_content;

      button_content = g_object_get_data (G_OBJECT (child), "content");

      mx_button_set_toggled (MX_BUTTON (child), (button_content == content));
    }
}

static MexContent *
mex_music_player_get_content (MexContentView *player)
{
  return MEX_MUSIC_PLAYER (player)->priv->content;
}

static void
mex_music_player_item_clicked (MxButton       *button,
                               MexMusicPlayer *player)
{
  MexContent *content;

  content = g_object_get_data (G_OBJECT (button), "content");

  mex_music_player_set_content (MEX_CONTENT_VIEW (player), content);
  mex_music_player_play (player);
}

static void
mex_music_player_set_context (MexContentView *player,
                              MexModel       *model)
{
  MexMusicPlayerPrivate *priv = MEX_MUSIC_PLAYER (player)->priv;
  ClutterActor *box, *button;
  MexContent *content;
  gint i;

  if (priv->model)
    g_object_unref (priv->model);

  priv->model = model;

  if (model)
    g_object_ref (model);

  box = mex_script_get_actor (priv->script, "tracks");
  clutter_actor_remove_all_children (box);

  for (i = 0; (content = mex_model_get_content (model, i)); i++)
    {
      const gchar *title;

      title = mex_content_get_metadata (content, MEX_CONTENT_METADATA_TITLE);
      button = mx_button_new_with_label (title);
      mx_stylable_set_style_class (MX_STYLABLE (button), "Track");
      mx_button_set_is_toggle (MX_BUTTON (button), TRUE);
      g_object_set_data (G_OBJECT (button), "content", content);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (mex_music_player_item_clicked), player);

      mx_bin_set_fill (MX_BIN (button), TRUE, TRUE);
      mx_button_set_icon_position (MX_BUTTON (button), MX_POSITION_RIGHT);

      clutter_actor_add_child (box, button);
    }
}

static MexModel*
mex_music_player_get_context (MexContentView *view)
{
  return MEX_MUSIC_PLAYER (view)->priv->model;
}



static void
mex_music_player_content_view_iface_init (MexContentViewIface *iface)
{
  iface->get_content = mex_music_player_get_content;
  iface->set_content = mex_music_player_set_content;

  iface->set_context = mex_music_player_set_context;
  iface->get_context = mex_music_player_get_context;
}

static void
mex_music_player_get_property (GObject    *object,
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
mex_music_player_set_property (GObject      *object,
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
mex_music_player_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_music_player_parent_class)->dispose (object);
}

static void
mex_music_player_finalize (GObject *object)
{
  MexMusicPlayerPrivate *priv = MEX_MUSIC_PLAYER (object)->priv;

  if (priv->shuffle)
    {
      g_array_free (priv->shuffle, TRUE);
      priv->shuffle = NULL;
    }

  G_OBJECT_CLASS (mex_music_player_parent_class)->finalize (object);
}

static void
mex_music_player_paint (ClutterActor *actor)
{
  MexMusicPlayerPrivate *priv = MEX_MUSIC_PLAYER (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_music_player_parent_class)->paint (actor);

  clutter_actor_paint (mex_script_get_actor (priv->script, "box"));
}

static void
mex_music_player_allocate (ClutterActor           *actor,
                           const ClutterActorBox  *box,
                           ClutterAllocationFlags  flags)
{
  MexMusicPlayerPrivate *priv = MEX_MUSIC_PLAYER (actor)->priv;
  ClutterActorBox child_box;
  ClutterActor *child;
  gfloat w, h;
  MxPadding padding;

  CLUTTER_ACTOR_CLASS (mex_music_player_parent_class)->allocate (actor, box,
                                                                 flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  child = mex_script_get_actor (priv->script, "box");
  clutter_actor_get_preferred_size (child, NULL, NULL, &w, &h);

  child_box.x1 = padding.left + (box->x2 - box->x1 - padding.left - padding.right) / 2 - w / 2;
  child_box.y1 = (box->y2 - box->y1 - padding.bottom) - h;
  child_box.x2 = child_box.x1 + w;
  child_box.y2 = child_box.y1 + h;
  clutter_actor_allocate (child, &child_box, flags);
}

static gboolean
mex_music_player_captured_event (ClutterActor *actor,
                                 ClutterEvent *event)
{
  MexMusicPlayerPrivate *priv = MEX_MUSIC_PLAYER (actor)->priv;
  ClutterKeyEvent *kevent;


  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;

  kevent = (ClutterKeyEvent *) event;

  if (MEX_KEY_INFO (kevent->keyval))
    {
      ClutterActor *tracks;

      tracks = mex_script_get_actor (priv->script, "tracks-scrollview");

      if (CLUTTER_ACTOR_IS_VISIBLE (tracks))
        clutter_actor_hide (tracks);
      else
        clutter_actor_show (tracks);

      return TRUE;
    }

  return FALSE;
}

static void
mex_music_player_class_init (MexMusicPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexMusicPlayerPrivate));

  object_class->get_property = mex_music_player_get_property;
  object_class->set_property = mex_music_player_set_property;
  object_class->dispose = mex_music_player_dispose;
  object_class->finalize = mex_music_player_finalize;

  actor_class->paint = mex_music_player_paint;
  actor_class->allocate = mex_music_player_allocate;

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

static void
mex_music_player_init (MexMusicPlayer *self)
{
  GError *error = NULL;
  ClutterActor *box;
  MexMusicPlayerPrivate *priv;
  ClutterActor *button;

  priv = self->priv = MUSIC_PLAYER_PRIVATE (self);

  priv->script = clutter_script_new ();
  clutter_script_load_from_resource (priv->script,
                                     "/org/media-explorer/MediaExplorer/json/"
                                     "music-player.json",
                                     &error);

  if (error)
    {
      g_error (G_STRLOC " %s", error->message);
      g_clear_error (&error);
    }

  box = mex_script_get_actor (priv->script, "box");
  g_assert (box);

  clutter_actor_add_child (CLUTTER_ACTOR (self), box);


  /* labels */
  priv->title_label = mex_script_get_actor (priv->script, "title-label");
  priv->subtitle_label = mex_script_get_actor (priv->script, "subtitle-label");

  /* play */
  priv->play_button = mex_script_get_actor (priv->script, "play-button");
  g_signal_connect_swapped (priv->play_button, "clicked",
                            G_CALLBACK (mex_music_player_play_toggle), self);

  /* next */
  button = mex_script_get_actor (priv->script, "next-button");
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (mex_music_player_next), self);

  /* previous */
  button = mex_script_get_actor (priv->script, "previous-button");
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (mex_music_player_previous), self);

  /* stop */
  button = mex_script_get_actor (priv->script, "stop-button");
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (mex_music_player_quit), self);

  /* repeat */
  button = mex_script_get_actor (priv->script, "repeat-button");
  g_signal_connect_swapped (button, "notify::toggled",
                            G_CALLBACK (mex_music_player_repeat_toggled), self);

  button = mex_script_get_actor (priv->script, "shuffle-button");
  g_signal_connect_swapped (button, "notify::toggled",
                            G_CALLBACK (mex_music_player_shuffle_toggled), self);


  /* player */
  priv->player = (ClutterMedia *) clutter_gst_video_texture_new ();
  g_signal_connect (priv->player, "notify",
                    G_CALLBACK (mex_music_player_notify_cb), self);
  g_signal_connect (priv->player, "eos",
                    G_CALLBACK (mex_music_player_eos_cb), self);

  /* slider */
  priv->slider = mex_script_get_actor (priv->script, "progress-slider");
  priv->slider_notify_id = g_signal_connect (priv->slider, "notify::value",
                                             G_CALLBACK (mex_music_player_slider_notify),
                                             self);


  g_signal_connect (self, "captured-event",
                    G_CALLBACK (mex_music_player_captured_event), NULL);
}

static void
mex_music_player_slider_notify (MxSlider       *slider,
                                GParamSpec     *pspec,
                                MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv = player->priv;

  clutter_media_set_progress (priv->player,
                              mx_slider_get_value (MX_SLIDER (priv->slider)));
}

static void
mex_music_player_update_progress (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv = player->priv;
  ClutterActor *label, *slider;
  gchar *text;
  gdouble progress, duration;

  progress = clutter_media_get_progress (priv->player);
  duration = clutter_media_get_duration (priv->player);

  label = mex_script_get_actor (priv->script, "progress-label");
  slider = mex_script_get_actor (priv->script, "progress-slider");

  g_signal_handler_block (slider, priv->slider_notify_id);
  mx_slider_set_value (MX_SLIDER (slider), progress);
  g_signal_handler_unblock (slider, priv->slider_notify_id);

  text = mex_metadata_utils_create_progress_string (progress, duration);
  mx_label_set_text (MX_LABEL (label), text);
  g_free (text);
}

static void
mex_music_player_update_playing (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv = player->priv;

  if (clutter_media_get_playing (priv->player))
    mx_stylable_set_style_class (MX_STYLABLE (priv->play_button), "MediaPause");
  else
    mx_stylable_set_style_class (MX_STYLABLE (priv->play_button), "MediaPlay");
}

gboolean
mex_music_player_is_playing (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv;

  g_return_val_if_fail (MEX_IS_MUSIC_PLAYER (player), FALSE);
  priv = player->priv;

  return clutter_media_get_playing (priv->player);
}

void
mex_music_player_play (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv;

  g_return_if_fail (MEX_IS_MUSIC_PLAYER (player));
  priv = player->priv;

  clutter_media_set_playing (priv->player, TRUE);
}

void
mex_music_player_play_toggle (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv;

  g_return_if_fail (MEX_IS_MUSIC_PLAYER (player));
  priv = player->priv;

  clutter_media_set_playing (priv->player,
                             !clutter_media_get_playing (priv->player));
}

void
mex_music_player_stop (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv;

  g_return_if_fail (MEX_IS_MUSIC_PLAYER (player));
  priv = player->priv;

  clutter_media_set_playing (priv->player, FALSE);
}

void
mex_music_player_quit (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv;

  g_return_if_fail (MEX_IS_MUSIC_PLAYER (player));
  priv = player->priv;

  clutter_media_set_playing (priv->player, FALSE);

  g_signal_emit (player, signals[CLOSE_REQUEST], 0);
}

static void
mex_music_player_update_index (MexMusicPlayer *player,
                               gint            new_index)
{
  MexMusicPlayerPrivate *priv = player->priv;
  gint content_index, length;
  MexContent *new_content;

  length = mex_model_get_length (priv->model);

  if (priv->repeat)
    {
      if (new_index >= length)
        new_index = 0;
      else if (new_index < 0)
        new_index = length - 1;
    }
  else
    {
      if (new_index >= length)
        new_index = length - 1;
      else if (new_index < 0)
        new_index = 0;
    }

  priv->current_index = new_index;

  if (priv->shuffle)
    content_index = g_array_index (priv->shuffle, gint, priv->current_index);
  else
    content_index = priv->current_index;

  new_content = mex_model_get_content (priv->model, content_index);

  mex_music_player_set_content (MEX_CONTENT_VIEW (player), new_content);
}

void
mex_music_player_next (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv;

  g_return_if_fail (MEX_IS_MUSIC_PLAYER (player));
  priv = player->priv;

  mex_music_player_update_index (player, priv->current_index + 1);
}

void
mex_music_player_previous (MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv;

  g_return_if_fail (MEX_IS_MUSIC_PLAYER (player));
  priv = player->priv;

  mex_music_player_update_index (player, priv->current_index - 1);
}

static void
mex_music_player_repeat_toggled (MexMusicPlayer *player,
                                 GParamSpec     *pspec,
                                 MxButton       *repeat_button)
{
  MexMusicPlayerPrivate *priv = player->priv;

  priv->repeat = mx_button_get_toggled (repeat_button);
}


static gint
mex_music_player_random_sort (gconstpointer a,
                              gconstpointer b)
{
  /* return a random sort value */
  return g_random_int_range (-1, 2);
}

static void
mex_music_player_shuffle_toggled (MexMusicPlayer *player,
                                  GParamSpec     *spec,
                                  MxButton       *button)
{
  MexMusicPlayerPrivate *priv = player->priv;
  gint i;

  if (mx_button_get_toggled (button))
    {
      gint length;

      length = mex_model_get_length (priv->model);

      priv->shuffle = g_array_sized_new (FALSE, FALSE, sizeof (gint), length);

      for (i = 0; i < length; i++)
        g_array_insert_val (priv->shuffle, i, i);

      /* shuffle the list */
      g_array_sort (priv->shuffle, mex_music_player_random_sort);
    }
  else
    {
      if (priv->shuffle)
        {
          g_array_free (priv->shuffle, TRUE);
          priv->shuffle = NULL;
        }
    }
}

static void
mex_music_player_notify_cb (ClutterMedia   *media,
                            GParamSpec     *pspec,
                            MexMusicPlayer *player)
{
  if (g_str_equal (pspec->name, "progress"))
    mex_music_player_update_progress (player);
  else if (g_str_equal (pspec->name, "playing"))
    mex_music_player_update_playing (player);
}


static void
mex_music_player_eos_cb (ClutterMedia   *media,
                         MexMusicPlayer *player)
{
  MexMusicPlayerPrivate *priv = player->priv;
  gint old_index = priv->current_index;

  mex_music_player_update_index (player, priv->current_index + 1);

  /* play if the index has changed, or if repeat is set */
  if (priv->current_index != old_index || priv->repeat)
    mex_music_player_play (player);
}

MexMusicPlayer *
mex_music_player_get_default (void)
{
  static MexMusicPlayer *singleton = NULL;

  if (singleton)
    return singleton;

  singleton = g_object_new (MEX_TYPE_MUSIC_PLAYER, NULL);
  return singleton;
}

ClutterMedia *
mex_music_player_get_clutter_media (MexMusicPlayer *player)
{
  g_return_val_if_fail (MEX_IS_MUSIC_PLAYER (player), NULL);

  return player->priv->player;
}

void
mex_music_player_seek_us (MexMusicPlayer *player, gint64 seek_offset_us)
{
  MexMusicPlayerPrivate *priv;
  gdouble duration_us, progress, new_progress;

  g_return_if_fail (MEX_IS_MUSIC_PLAYER (player));

  priv = player->priv;

  duration_us = clutter_media_get_duration (priv->player) * 1000000;
  progress = clutter_media_get_progress (priv->player) * duration_us;

  new_progress = (progress + seek_offset_us) / duration_us;

  if (new_progress < 0.0)
    mex_music_player_previous (player);
  else if (new_progress > 1.0)
    mex_music_player_next (player);
  else
    clutter_media_set_progress (priv->player, new_progress);
}

void
mex_music_player_set_uri (MexMusicPlayer *player, const gchar *uri)
{
  MexMusicPlayerPrivate *priv;
  char *old_uri;
  gboolean playing;
  gboolean uri_set = FALSE;

  g_return_if_fail (MEX_IS_MUSIC_PLAYER (player));

  priv = player->priv;

  old_uri = clutter_media_get_uri (priv->player);

  if (!old_uri && !uri)
    return;

  playing = mex_music_player_is_playing (player);

  if (!old_uri || (uri && strcmp (uri, old_uri)))
    {
      uri_set = TRUE;
      clutter_media_set_uri (priv->player, uri);
    }

  if (playing && !uri)
    {
      /* we are no longer playing, I guess */
      g_signal_emit (player, signals[CLOSE_REQUEST], 0);
    }
  else if (!playing && uri && uri_set)
    {
      g_signal_emit (player, signals[OPEN_REQUEST], 0);
    }

  g_free (old_uri);
}
