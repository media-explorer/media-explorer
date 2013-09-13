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

#include "mex-info-panel.h"
#include "mex-shadow.h"
#include "mex-player.h"
#include "mex-metadata-utils.h"
#include "mex-enum-types.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>

#include <glib/gi18n-lib.h>

#include <gst/gst.h>
#include <gst/tag/tag.h>

#include <clutter-gst/clutter-gst.h>
#include <clutter-gst/clutter-gst-player.h>

static void mex_content_view_iface_init (MexContentViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexInfoPanel, mex_info_panel, MX_TYPE_FRAME,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_content_view_iface_init))
#define INFO_PANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_INFO_PANEL, MexInfoPanelPrivate))

#define GET_ACTOR(name) \
  (CLUTTER_ACTOR (clutter_script_get_object (priv->script, name)))

#define GET_LABEL(name) \
  (MX_LABEL (clutter_script_get_object (priv->script, name)))

#define GET_OBJECT(name) \
  (clutter_script_get_object (priv->script, name))

enum
{
  PROP_0,

  PROP_MODE
};

struct _MexInfoPanelPrivate
{
  ClutterScript *script;
  ClutterActor *buttons_container;
  ClutterActor *watch_button;
  ClutterActor *audio_combo_box;
  ClutterActor *subtitle_combo_box;

  gboolean audio_combo_box_changed_from_media;
  gboolean subtitle_combo_box_changed_from_media;

  MxLabel *metadata_row1;

  ClutterMedia *media;
  MexContent *content;
  MexModel   *model;

  MexInfoPanelMode mode;

  guint content_handler_id;

  GList *video_metadata_template;
  GList *image_metadata_template;
  GList *music_metadata_template;
};

static void
free_string_list (GList *l)
{
  while (l)
    {
      g_free (l->data);
      l = g_list_delete_link (l, l);
    }
}

static gchar *
get_stream_description (GstTagList *tags,
                        gint        track_num)
{
  gchar *description = NULL;

  if (tags)
    {

      gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &description);

      if (description)
        {
          const gchar *language = gst_tag_get_language_name (description);

          if (language)
            {
              g_free (description);
              description = g_strdup (language);
            }
        }

      if (!description)
        gst_tag_list_get_string (tags, GST_TAG_CODEC, &description);
    }

  if (!description)
    {
      /* In this context Tracks is either an audio track or a subtitles
       * track */
      description = g_strdup_printf (_("Track %d"), track_num);
    }

  return description;
}

static GList *
get_streams_descriptions (GList *tags_list)
{
  GList *descriptions = NULL, *l;
  gint track_num = 1;

  for (l = tags_list; l; l = g_list_next (l))
    {
      GstTagList *tags = l->data;
      gchar *description;

      description = get_stream_description (tags, track_num);
      track_num++;

      descriptions = g_list_prepend (descriptions, description);
    }

  return g_list_reverse (descriptions);
}

static MexContent*
mex_info_panel_get_content (MexContentView *view)
{
    MexInfoPanel *self = MEX_INFO_PANEL (view);
    MexInfoPanelPrivate *priv = self->priv;

    return priv->content;
}

static MexModel *
mex_info_panel_get_context (MexContentView *view)
{
  MexInfoPanelPrivate *priv = INFO_PANEL_PRIVATE (view);

  return priv->model;
}

static void
mex_info_panel_set_context (MexContentView *view, MexModel *context)
{
  MexInfoPanelPrivate *priv = INFO_PANEL_PRIVATE (view);

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }
  if (context)
    priv->model = g_object_ref (context);
}

typedef enum
{
  NONE,

  IMAGE,
  VIDEO,
  MUSIC,

  LAST
} MexInfoPanelMime;

static void mex_info_panel_set_content (MexContentView *self,
                                        MexContent *content);
static void
mex_content_view_iface_init (MexContentViewIface *iface)
{
  iface->set_content = mex_info_panel_set_content;
  iface->get_content = mex_info_panel_get_content;
  iface->set_context = mex_info_panel_set_context;
  iface->get_context = mex_info_panel_get_context;
}

static void
mex_info_panel_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MexInfoPanel *self = MEX_INFO_PANEL (object);
  MexInfoPanelPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, priv->mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_info_panel_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MexInfoPanel *self = MEX_INFO_PANEL (object);
  MexInfoPanelPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_MODE:
      priv->mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_info_panel_dispose (GObject *object)
{
  MexInfoPanel *self = MEX_INFO_PANEL (object);
  MexInfoPanelPrivate *priv = self->priv;

  if (priv->content_handler_id > 0)
    {
      g_signal_handler_disconnect (priv->content, priv->content_handler_id);
      priv->content_handler_id = 0;
    }

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

  while (priv->image_metadata_template)
    {
      mex_metadata_info_free (priv->image_metadata_template->data);
      priv->image_metadata_template = g_list_delete_link (
        priv->image_metadata_template, priv->image_metadata_template);
    }

  while (priv->video_metadata_template)
    {
      mex_metadata_info_free (priv->video_metadata_template->data);
      priv->video_metadata_template = g_list_delete_link (
        priv->video_metadata_template, priv->video_metadata_template);
    }

  G_OBJECT_CLASS (mex_info_panel_parent_class)->dispose (object);
}

static void
mex_info_panel_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_info_panel_parent_class)->finalize (object);
}

static void
_watch_button_pressed_cb (ClutterActor *actor, MexInfoPanel *self)
{
  MexInfoPanelPrivate *priv = self->priv = INFO_PANEL_PRIVATE (self);

  mex_content_open (priv->content, priv->model);
}

static void
audio_combo_box_notify (MxComboBox   *box,
                        GParamSpec   *pspec,
                        MexInfoPanel *panel)
{
#ifdef USE_PLAYER_CLUTTER_GST
  MexInfoPanelPrivate *priv = panel->priv;
  GstTagList *tags;
  ClutterGstPlayer *player;
  gchar *description;
  GList *list;
  gchar *title;
  gint index_;

  index_ = mx_combo_box_get_index (box);

  /* index is -1 when the custom text is set */
  if (index_ < 0)
    return;

  if (!CLUTTER_GST_IS_PLAYER (priv->media))
    return;

  player = CLUTTER_GST_PLAYER (priv->media);

  /* Avoid looping when the change come from the ClutterMedia */
  if (priv->audio_combo_box_changed_from_media)
    priv->audio_combo_box_changed_from_media = FALSE;
  else
    clutter_gst_player_set_audio_stream (player, index_);

  /* audio track */
  list = clutter_gst_player_get_audio_streams (player);
  tags = (GstTagList *) g_list_nth_data (list, index_);
  description = get_stream_description (tags, index_ + 1);
  title = g_strdup_printf (_("Audio (%s)"), description);
  g_free (description);

  mx_combo_box_set_active_text (MX_COMBO_BOX (priv->audio_combo_box), title);

  g_free (title);
#endif
}

static void
subtitle_combo_box_notify (MxComboBox   *box,
                           GParamSpec   *pspec,
                           MexInfoPanel *panel)
{
#ifdef USE_PLAYER_CLUTTER_GST
  MexInfoPanelPrivate *priv = panel->priv;
  ClutterGstPlayer *player;
  GList *list;
  gchar *title;
  gint index_;

  index_ = mx_combo_box_get_index (box);

  /* index is -1 when the custom text is set */
  if (index_ < 0)
    return;

  if (!CLUTTER_GST_IS_PLAYER (priv->media))
    return;

  player = CLUTTER_GST_PLAYER (priv->media);

  /* Avoid looping when the change come from the ClutterMedia */
  if (priv->subtitle_combo_box_changed_from_media)
    priv->subtitle_combo_box_changed_from_media = FALSE;
  else
    clutter_gst_player_set_subtitle_track (player, index_ - 1);

  /* audio track */
  if (index_ == 0)
    {
      title = g_strdup_printf (_("Subtitles (None)"));
    }
  else
    {
      GstTagList *tags;
      gchar *description;

      list = clutter_gst_player_get_subtitle_tracks (player);
      tags = (GstTagList *) g_list_nth_data (list, index_ - 1);
      description = get_stream_description (tags, index_);
      title = g_strdup_printf (_("Subtitles (%s)"), description);
      g_free (description);
    }

  mx_combo_box_set_active_text (MX_COMBO_BOX (priv->subtitle_combo_box), title);

  g_free (title);
#endif
}

static void
mex_info_panel_constructed (GObject *object)
{
  ClutterActor *root;
  gchar *tmp;

  GError *err = NULL;
  MexInfoPanel *self = MEX_INFO_PANEL (object);
  MexInfoPanelPrivate *priv = self->priv;

  priv->script = clutter_script_new ();

  switch (priv->mode)
    {
    case MEX_INFO_PANEL_MODE_FULL:
      tmp = g_build_filename (mex_get_data_dir (), "json",
                              "info-panel-full.json", NULL);
      clutter_script_load_from_file (priv->script, tmp, &err);
      g_free (tmp);

      priv->watch_button = GET_ACTOR ("watch-button");
      priv->buttons_container = GET_ACTOR ("buttons");

      g_signal_connect (priv->watch_button, "clicked",
                        G_CALLBACK (_watch_button_pressed_cb),
                        self);

      mx_stylable_set_style_class (MX_STYLABLE (self), "Full");
      break;

    case MEX_INFO_PANEL_MODE_SIMPLE:
      tmp = g_build_filename (mex_get_data_dir (), "json",
                              "info-panel-simple.json", NULL);
      clutter_script_load_from_file (priv->script, tmp, &err);
      g_free (tmp);

      mx_stylable_set_style_class (MX_STYLABLE (self), "Simple");
      break;

    default:
      g_error (G_STRLOC ": Unrecognised display mode");
      break;
    }

  if (err)
    g_error ("Could not load info panel: %s", err->message);

  root = GET_ACTOR ("info-panel-container");

  priv->metadata_row1 = GET_LABEL ("row1-metadata");

  clutter_actor_add_child (CLUTTER_ACTOR (self), root);

  if (priv->mode == MEX_INFO_PANEL_MODE_FULL)
    {
      /* hide the combo boxes for the audio streams and subtitles tracks
       * by default. Their visibility is handled by listening to some
       * ClutterMedia/ClutterGstVideoTexture signals */
      priv->audio_combo_box = GET_ACTOR ("audio-streams-choice");
      clutter_actor_hide (priv->audio_combo_box);

      priv->subtitle_combo_box = GET_ACTOR ("subtitle-tracks-choice");
      clutter_actor_hide (priv->subtitle_combo_box);

      clutter_actor_add_effect (root, CLUTTER_EFFECT (mex_shadow_new ()));

      g_signal_connect (priv->audio_combo_box, "notify::index",
                        G_CALLBACK (audio_combo_box_notify), self);

      g_signal_connect (priv->subtitle_combo_box, "notify::index",
                        G_CALLBACK (subtitle_combo_box_notify), self);
    }
}

static void
mex_info_panel_class_init (MexInfoPanelClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexInfoPanelPrivate));

  object_class->get_property = mex_info_panel_get_property;
  object_class->set_property = mex_info_panel_set_property;
  object_class->dispose = mex_info_panel_dispose;
  object_class->finalize = mex_info_panel_finalize;
  object_class->constructed = mex_info_panel_constructed;

  pspec = g_param_spec_enum ("mode",
                             "Mode",
                             "Display mode for the info panel.",
                             MEX_TYPE_INFO_PANEL_MODE,
                             MEX_INFO_PANEL_MODE_FULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MODE, pspec);

}

static gchar *
_append_human (const gchar *raw,
               MexContentMetadata key)
{
  gchar *human;

  switch (key)
    {
      case MEX_CONTENT_METADATA_DATE:
      case MEX_CONTENT_METADATA_CREATION_DATE:
        human = mex_metadata_humanise_date (raw);
        break;

      case MEX_CONTENT_METADATA_DURATION:
      case MEX_CONTENT_METADATA_LAST_POSITION:
        human = mex_metadata_humanise_duration (raw);
        break;

      default:
        human = NULL;
        break;
    }

  return human;
}


static void
_set_metadata (MexInfoPanel *self, MexInfoPanelMime mime)
{
  MexInfoPanelPrivate *priv = self->priv = INFO_PANEL_PRIVATE (self);
  GList *temp = NULL;
  GList *target = NULL;
  gint i;

  GString *string;
  string = g_string_new ("");

  switch (mime)
    {
      case IMAGE:
        target = priv->image_metadata_template;
        break;

      case VIDEO:
        target = priv->video_metadata_template;
        break;

      case MUSIC:
        target = priv->music_metadata_template;
        break;

      default:
        target = priv->video_metadata_template;
    }

  mex_metadata_get_metadata (&target, priv->content);

  i = 0;

  for (temp = target; temp; temp = temp->next)
    {
      MexMetadataInfo *info;
      info = temp->data;

      if (!info->value ||
          !mex_metadata_info_get_visible (info, info->value))
        continue;

      /* Add the separator after the first iteration has occurred (i>0) */
      if (i > 0)
        {
          g_string_append (string, " | ");
        }

      /* field name */
      g_string_append (string, info->key_string);

      /* values that we want re-written in a more human form */
      if (info->key == MEX_CONTENT_METADATA_DATE ||
          info->key ==  MEX_CONTENT_METADATA_CREATION_DATE ||
          info->key == MEX_CONTENT_METADATA_DURATION ||
          info->key == MEX_CONTENT_METADATA_LAST_POSITION)
        {
          gchar *human_text = _append_human (info->value, info->key);
          if (human_text)
            {
              g_string_append (string, human_text);
              g_free (human_text);
            }
        }
      else
        {
         g_string_append (string, info->value);
        }
      i++;
    }

  mx_label_set_text (priv->metadata_row1, string->str);

  g_string_free (string, TRUE);
}

static void
_unset_content (MexInfoPanel *self)
{
  MexInfoPanelPrivate *priv = self->priv;

  if (priv->content)
    {
      if (priv->content_handler_id > 0)
        g_signal_handler_disconnect (priv->content, priv->content_handler_id);

      g_object_unref (priv->content);
      priv->content = NULL;
    }
}

static void
_content_changed_cb (MexContent     *content,
                     GParamSpec     *pspec,
                     MexContentView *view)
{
  /* Refresh the content */
  _unset_content (MEX_INFO_PANEL (view));
  mex_info_panel_set_content (view, content);
}

static void
mex_info_panel_set_content (MexContentView *view, MexContent *content)
{
  MexInfoPanel *self = MEX_INFO_PANEL (view);
  MexInfoPanelPrivate *priv = self->priv;

  const gchar *mimetype;
  const gchar *title = NULL;

  ClutterActor *thumbnail, *queue_button;

  if (priv->content == content)
    return;


  _unset_content (MEX_INFO_PANEL (view));

  priv->content = g_object_ref (content);

  priv->content_handler_id =
    g_signal_connect (content, "notify",
                      G_CALLBACK (_content_changed_cb),
                      view);

  mimetype = mex_content_get_metadata (content, MEX_CONTENT_METADATA_MIMETYPE);

  mx_label_set_text (priv->metadata_row1, "");

  if (mimetype)
    {
      if (strncmp (mimetype, "image/", 6) == 0)
        {
          /* We don't want any buttons on the image info panel */
          if (priv->buttons_container)
              clutter_actor_hide (priv->buttons_container);
          _set_metadata (self, IMAGE);
        }
      else
        {
          /* Set the metadata info and update the watch button label */
          if (strncmp (mimetype, "video/", 6) == 0)
            {
              if (priv->watch_button)
                mx_button_set_label (MX_BUTTON (priv->watch_button),
                                     _("Watch"));
              _set_metadata (self, VIDEO);
            }
          else if (strncmp (mimetype, "audio/", 6) == 0)
            {
              if (priv->watch_button)
                mx_button_set_label (MX_BUTTON (priv->watch_button),
                                     _("Listen"));
              _set_metadata (self, MUSIC);
            }

          /* for all other than the image mime type we want to have buttons */
          if (priv->buttons_container)
            {
              MexContent *player_content;
              MexPlayer *player;

              clutter_actor_show (priv->buttons_container);

              player = mex_player_get_default ();
              player_content =
                mex_content_view_get_content (MEX_CONTENT_VIEW (player));

              /* if we're displaying info about the current playing content */
              if (content == player_content)
                clutter_actor_hide (priv->watch_button);
              else
                clutter_actor_show (priv->watch_button);
            }
        }
    }

  if (priv->mode == MEX_INFO_PANEL_MODE_FULL)
    {
      thumbnail = GET_ACTOR ("thumbnail");
      queue_button = GET_ACTOR ("add-to-queue-button");

      mex_content_view_set_content (MEX_CONTENT_VIEW (thumbnail), content);
      mex_content_view_set_content (MEX_CONTENT_VIEW (queue_button), content);

      if ((title = mex_content_get_metadata (content,
                                             MEX_CONTENT_METADATA_TITLE)))
        {
          GObject *label = GET_OBJECT ("content-title");
          mx_label_set_text (MX_LABEL (label), title);
        }
    }
}

static gboolean
resume_visible_cb (const gchar *value, gpointer user_data)
{
  if (value && (atoi (value) > 0))
    return TRUE;
  return FALSE;
}

static void
mex_info_panel_init (MexInfoPanel *self)
{
  GList *image_metadata_template = NULL;
  GList *video_metadata_template = NULL;
  GList *music_metadata_template = NULL;

  self->priv = INFO_PANEL_PRIVATE (self);

  /* Template for the metadata that we want to display for images */

  image_metadata_template =
    g_list_append (image_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_CREATION_DATE,
                                          _("Date added: "),
                                          0));
  image_metadata_template =
    g_list_append (image_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_DATE,
                                          _("Date taken: "),
                                          1));
  image_metadata_template =
    g_list_append (image_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_CAMERA_MODEL,
                                          _("Camera model: "),
                                          0));
  image_metadata_template =
    g_list_append (image_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_WIDTH,
                                          _("Width: "),
                                          0));
  image_metadata_template =
    g_list_append (image_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_HEIGHT,
                                          _("Height: "),
                                          0));

  self->priv->image_metadata_template = image_metadata_template;


  /* Template for the metadata we want to display for videos */
  video_metadata_template =
    g_list_append (video_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_DATE,
                                          _("Date added: "),
                                          0));
  video_metadata_template =
    g_list_append (video_metadata_template,
                   mex_metadata_info_new_with_visibility (
                                MEX_CONTENT_METADATA_LAST_POSITION,
                                _("Resume from: "),
                                0,
                                resume_visible_cb,
                                self));
  video_metadata_template =
    g_list_append (video_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_DURATION,
                                          _("Duration: "),
                                          1));
  video_metadata_template =
    g_list_append (video_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_SYNOPSIS,
                                          _("Description: "),
                                          0));

  self->priv->video_metadata_template = video_metadata_template;

  music_metadata_template =
    g_list_append (music_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_ARTIST,
                                          _("Artist: "),
                                          0));
  music_metadata_template =
    g_list_append (music_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_ALBUM,
                                          _("Album: "),
                                          0));
  music_metadata_template =
    g_list_append (music_metadata_template,
                   mex_metadata_info_new_with_visibility (
                                MEX_CONTENT_METADATA_LAST_POSITION,
                                _("Resume from: "),
                                0,
                                resume_visible_cb,
                                self));
  music_metadata_template =
    g_list_append (music_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_DURATION,
                                          _("Duration: "),
                                          1));
  music_metadata_template =
    g_list_append (music_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_PLAY_COUNT,
                                          _("Plays: "),
                                          0));
  music_metadata_template =
    g_list_append (music_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_DATE,
                                          _("Date added: "),
                                          0));

  self->priv->music_metadata_template = music_metadata_template;

}

ClutterActor *
mex_info_panel_new (MexInfoPanelMode mode)
{
  return g_object_new (MEX_TYPE_INFO_PANEL, "mode", mode, NULL);
}

static void
on_media_audio_streams_changed (ClutterMedia *media,
                                GParamSpec   *pspsec,
                                MexInfoPanel *panel)
{
  ClutterGstPlayer *player = CLUTTER_GST_PLAYER (media);
  MexInfoPanelPrivate *priv = panel->priv;
  GList *streams, *l, *descriptions;
  gint n_streams;

  streams = clutter_gst_player_get_audio_streams (player);
  n_streams = g_list_length (streams);

  /* no need to display the audio stream combox box if there's no more than 1
   * stream */
  if (n_streams <= 1)
    {
      mx_combo_box_remove_all (MX_COMBO_BOX (priv->audio_combo_box));
      clutter_actor_hide (priv->audio_combo_box);
      return;
    }

  mx_combo_box_remove_all (MX_COMBO_BOX (priv->audio_combo_box));

  descriptions = get_streams_descriptions (streams);
  for (l = descriptions; l; l = g_list_next (l))
    {
      gchar *description = l->data;

      mx_combo_box_append_text (MX_COMBO_BOX (priv->audio_combo_box),
                                description);
    }
  free_string_list (descriptions);

  clutter_actor_show (priv->audio_combo_box);
}

static void
on_media_audio_stream_changed (ClutterMedia *media,
                                GParamSpec   *pspsec,
                                MexInfoPanel *panel)
{
  ClutterGstPlayer *player = CLUTTER_GST_PLAYER (media);
  MexInfoPanelPrivate *priv = panel->priv;
  gint index_;

  priv->audio_combo_box_changed_from_media = TRUE;

  index_ = clutter_gst_player_get_audio_stream (player);
  mx_combo_box_set_index (MX_COMBO_BOX (priv->audio_combo_box), index_);
}

static void
on_media_subtitle_tracks_changed (ClutterMedia *media,
                                  GParamSpec   *pspsec,
                                  MexInfoPanel *panel)
{
  ClutterGstPlayer *player = CLUTTER_GST_PLAYER (media);
  MexInfoPanelPrivate *priv = panel->priv;
  GList *tracks, *l, *descriptions;
  gint n_tracks;

  tracks = clutter_gst_player_get_subtitle_tracks (player);

  mx_combo_box_remove_all (MX_COMBO_BOX (priv->subtitle_combo_box));

  /* no need to display the subtitle combo box if there's no subtitles */
  n_tracks = g_list_length (tracks);
  if (n_tracks == 0)
    {
      clutter_actor_hide (priv->subtitle_combo_box);
      return;
    }

  /* Add a "None" option to disable subtitles */

  /* TRANSLATORS: In this context, None is used to disable subtitles in the
   * list of choices for subtitles */
  mx_combo_box_append_text (MX_COMBO_BOX (priv->subtitle_combo_box),
                            _("None"));

  /* TRANSLATORS: In this context, track is a subtitles track */
  descriptions = get_streams_descriptions (tracks);
  for (l = descriptions; l; l = g_list_next (l))
    {
      gchar *description = l->data;

      mx_combo_box_append_text (MX_COMBO_BOX (priv->subtitle_combo_box),
                                description);
    }
  free_string_list (descriptions);

  clutter_actor_show (priv->subtitle_combo_box);
}

static void
on_media_subtitle_track_changed (ClutterMedia *media,
                                 GParamSpec   *pspsec,
                                 MexInfoPanel *panel)
{
  ClutterGstPlayer *player = CLUTTER_GST_PLAYER (media);
  MexInfoPanelPrivate *priv = panel->priv;
  gint index_;

  priv->subtitle_combo_box_changed_from_media = TRUE;

  index_ = clutter_gst_player_get_subtitle_track (player);

  /* +1 here as we add "None" in the combo box to be able to disable subs */
  mx_combo_box_set_index (MX_COMBO_BOX (priv->subtitle_combo_box), index_ + 1);
}

void
mex_info_panel_set_media (MexInfoPanel *panel,
                          ClutterMedia *media)
{
  MexInfoPanelPrivate *priv = panel->priv;

  g_return_if_fail (MEX_IS_INFO_PANEL (panel));
  g_return_if_fail (CLUTTER_IS_MEDIA (media));

  /* we only do something with the info from ClutterMedia in full mode */
  if (priv->mode != MEX_INFO_PANEL_MODE_FULL)
    return;

  if (priv->media)
    {
      g_signal_handlers_disconnect_by_func (priv->media,
                                            on_media_audio_streams_changed,
                                            panel);
      g_signal_handlers_disconnect_by_func (priv->media,
                                            on_media_subtitle_tracks_changed,
                                            panel);
    }

  priv->media = media;

  if (media)
    {
      g_signal_connect (priv->media, "notify::audio-streams",
                        G_CALLBACK (on_media_audio_streams_changed), panel);
      g_signal_connect (priv->media, "notify::audio-stream",
                        G_CALLBACK (on_media_audio_stream_changed), panel);

      g_signal_connect (priv->media, "notify::subtitle-tracks",
                        G_CALLBACK (on_media_subtitle_tracks_changed), panel);
      g_signal_connect (priv->media, "notify::audio-stream",
                        G_CALLBACK (on_media_subtitle_track_changed), panel);
    }
}
