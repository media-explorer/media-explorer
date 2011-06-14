/*
 * Mex - a media explorer
 *
 * Copyright © , 2010, 2011 Intel Corporation.
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

#include "mex-info-panel.h"
#include "mex-shadow.h"
#include "mex-player.h"
#include "mex-metadata-utils.h"
#include "mex-enum-types.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>

#include <config.h>
#include <glib/gi18n-lib.h>

static void mex_content_view_iface_init (MexContentViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexInfoPanel, mex_info_panel, MX_TYPE_FRAME,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_content_view_iface_init))
#define INFO_PANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_INFO_PANEL, MexInfoPanelPrivate))

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

  MxLabel *metadata_row1;

  MexContent *content;

  MexInfoPanelMode mode;

  GList *video_metadata_template;
  GList *image_metadata_template;
  GList *music_metadata_template;
};

static MexContent*
mex_info_panel_get_content (MexContentView *view)
{
    MexInfoPanel *self = MEX_INFO_PANEL (view);
    MexInfoPanelPrivate *priv = self->priv;

    return priv->content;
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
  /* not implementing model/context */
  iface->set_content = mex_info_panel_set_content;
  iface->get_content = mex_info_panel_get_content;
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

  if (priv->content)
    {
      g_object_unref (priv->content);
      priv->content = NULL;
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
  MexPlayer *player;

  player = mex_player_get_default ();

  mex_content_view_set_content (MEX_CONTENT_VIEW (player), priv->content);
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

      priv->watch_button =
        CLUTTER_ACTOR (clutter_script_get_object (priv->script, "watch-button"));

      priv->buttons_container =
        CLUTTER_ACTOR (clutter_script_get_object (priv->script,
                                                  "buttons"));

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
    {
      g_warning ("Could not load info panel: %s", err->message);
      g_clear_error (&err);
      return;
    }

  root = CLUTTER_ACTOR (clutter_script_get_object (priv->script,
                                                   "info-panel-container"));

  priv->metadata_row1 =
    MX_LABEL (clutter_script_get_object (priv->script, "row1-metadata"));

  mx_bin_set_child (MX_BIN (self), root);

  if (priv->mode == MEX_INFO_PANEL_MODE_FULL)
    clutter_actor_add_effect (root, CLUTTER_EFFECT (mex_shadow_new ()));
  else
    mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);
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
        break;
    }
  if (human)
    return human;

  return NULL;
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

      if (!info->value)
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
          g_string_append (string, human_text);
          g_free (human_text);
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
mex_info_panel_set_content (MexContentView *view, MexContent *content)
{
  MexInfoPanel *self = MEX_INFO_PANEL (view);
  MexInfoPanelPrivate *priv = self->priv;

  const gchar *mimetype;
  const gchar *title = NULL;

  ClutterActor *thumbnail, *queue_button;

  if (priv->content)
    g_object_unref (priv->content);

  priv->content = g_object_ref (content);

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
      thumbnail =
        CLUTTER_ACTOR (clutter_script_get_object (priv->script, "thumbnail"));
      queue_button =
        CLUTTER_ACTOR (clutter_script_get_object (priv->script,
                                                  "add-to-queue-button"));

      mex_content_view_set_content (MEX_CONTENT_VIEW (thumbnail), content);
      mex_content_view_set_content (MEX_CONTENT_VIEW (queue_button), content);

      if ((title = mex_content_get_metadata (content,
                                             MEX_CONTENT_METADATA_TITLE)))
        {
          GObject *label = clutter_script_get_object (priv->script,
                                                      "content-title");
          mx_label_set_text (MX_LABEL (label), title);
        }
    }
}

static void
mex_info_panel_init (MexInfoPanel *self)
{
  self->priv = INFO_PANEL_PRIVATE (self);

  /* Template for the metadata that we want to display for images */

  GList *image_metadata_template = NULL;

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
  GList *video_metadata_template = NULL;

  video_metadata_template =
    g_list_append (video_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_DATE,
                                          _("Date added: "),
                                          0));
  video_metadata_template =
    g_list_append (video_metadata_template,
                   mex_metadata_info_new (MEX_CONTENT_METADATA_LAST_POSITION,
                                          _("Resume from: "),
                                          0));
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

  GList *music_metadata_template = NULL;

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
                   mex_metadata_info_new (MEX_CONTENT_METADATA_LAST_POSITION,
                                          _("Resume from: "),
                                          0));
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

