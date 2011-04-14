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
  MxLabel *metadata_row2;

  MexContent *content;

  MexInfoPanelMode mode;
};

static MexContent*
mex_info_panel_get_content (MexContentView *view)
{
    MexInfoPanel *self = MEX_INFO_PANEL (view);
    MexInfoPanelPrivate *priv = self->priv;

    return priv->content;
}

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

  GError *err = NULL;
  MexInfoPanel *self = MEX_INFO_PANEL (object);
  MexInfoPanelPrivate *priv = self->priv;

  priv->script = clutter_script_new ();

  switch (priv->mode)
    {
    case MEX_INFO_PANEL_MODE_FULL:
      clutter_script_load_from_file (priv->script,
                                     PKGJSONDIR "/info-panel-full.json",
                                     &err);

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
      clutter_script_load_from_file (priv->script,
                                     PKGJSONDIR "/info-panel-simple.json",
                                     &err);
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

  priv->metadata_row2 =
    MX_LABEL (clutter_script_get_object (priv->script, "row2-metadata"));

  mx_bin_set_child (MX_BIN (self), root);

  if (priv->mode == MEX_INFO_PANEL_MODE_FULL)
    mex_shadow_new (root);
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

static void
_image_metadata (MexInfoPanel *self)
{
  MexInfoPanelPrivate *priv = self->priv = INFO_PANEL_PRIVATE (self);

  const gchar *date, *creation_date, *model;
  gchar *row1, *row2, *human_date;

  date = mex_content_get_metadata (priv->content,
                                   MEX_CONTENT_METADATA_DATE);
  creation_date = mex_content_get_metadata (priv->content,
                                            MEX_CONTENT_METADATA_CREATION_DATE);
  model = mex_content_get_metadata (priv->content,
                                    MEX_CONTENT_METADATA_CAMERA_MODEL);

  if (creation_date)
    human_date = mex_metadata_humanise_date (creation_date);
  else
    human_date = mex_metadata_humanise_date (date);

  if (!human_date && !model)
    {
      mx_label_set_text (priv->metadata_row1,
                         _("No additional information available"));
    }
  else
    {
      /* Date: human_date | heightxwidth */
      row1 = g_strdup_printf ("%s %s",
                              (human_date) ? _("Date:") : "",
                              (human_date) ? human_date : "");
      g_free (human_date);

      mx_label_set_text (priv->metadata_row1, row1);

      row2 = g_strdup_printf ("%s",  model ? model : _(""));
      mx_label_set_text (priv->metadata_row2, row2);

      g_free (row1);
      g_free (row2);
    }
}

static void
_video_metadata (MexInfoPanel *self)
{
  MexInfoPanelPrivate *priv = self->priv = INFO_PANEL_PRIVATE (self);

  const gchar *date = NULL;
  const gchar *duration = NULL;
  const gchar *synopsis = NULL;
  const gchar *resume_from = NULL;

  gchar *human_duration, *human_date, *temp_text, *human_resume_from;

  date = mex_content_get_metadata (priv->content,
                                   MEX_CONTENT_METADATA_DATE);
  duration = mex_content_get_metadata (priv->content,
                                       MEX_CONTENT_METADATA_DURATION);
  synopsis = mex_content_get_metadata (priv->content,
                                       MEX_CONTENT_METADATA_SYNOPSIS);

  if (priv->mode == MEX_INFO_PANEL_MODE_SIMPLE)
    {
      resume_from =
        mex_content_get_metadata (priv->content,
                                  MEX_CONTENT_METADATA_LAST_POSITION);
      if (resume_from)
        {
          if (g_str_equal (resume_from, "0"))
            resume_from = NULL;
        }
    }

  human_date = mex_metadata_humanise_date (date);
  human_duration = mex_metadata_humanise_duration (duration);
  human_resume_from = mex_metadata_humanise_duration (resume_from);

  if (!human_date && !human_duration && !synopsis && !human_resume_from)
    {
      mx_label_set_text (priv->metadata_row1,
                         _("No additional information available"));
    }
  else
    {
      /* Date: date_value | Duration: duration_value
       *| Resume from: resume_value */

      /* TODO: This method isn't scaling up, refactor needed
       * possibly use glist */
      temp_text = g_strdup_printf ("%s %s %s %s %s %s %s %s",
                                   (human_date) ? _("Date:") : "",
                                   (human_date) ? human_date : "",
                                   (human_duration && human_date) ? "|" : "",
                                   (human_duration) ? _("Duration:") : "",
                                   (human_duration) ? human_duration : "",
                                   (human_resume_from) &&
                                   (human_date || human_duration) ?
                                   "|" : "",
                                   (human_resume_from) ? _("Resume from:") : "",
                                   (human_resume_from) ?
                                   human_resume_from : "");

      mx_label_set_text (priv->metadata_row2, (synopsis) ? synopsis : "");

      mx_label_set_text (priv->metadata_row1, temp_text);

      g_free (temp_text);
      g_free (human_duration);
      g_free (human_date);
      g_free (human_resume_from);
    }
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
  mx_label_set_text (priv->metadata_row2, "");

  if (mimetype)
    {
      if (strncmp (mimetype, "image/", 6) == 0)
        {
          /* set the metadata labels */
          _image_metadata (self);
          if (priv->buttons_container)
            clutter_actor_hide (priv->buttons_container);
        }
      else
        {
          MexContent *player_content;
          MexPlayer *player;

          if (priv->buttons_container)
            clutter_actor_show (priv->buttons_container);

          player = mex_player_get_default ();
          player_content =
            mex_content_view_get_content (MEX_CONTENT_VIEW (player));

          /* if we're displaying info about the current playing content */
          if (priv->watch_button)
            {
              if (content == player_content)
                clutter_actor_hide (priv->watch_button);
              else
                clutter_actor_show (priv->watch_button);
            }

          /* set the metadata labels */
          _video_metadata (self);
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
}

ClutterActor *
mex_info_panel_new (MexInfoPanelMode mode)
{
  return g_object_new (MEX_TYPE_INFO_PANEL, "mode", mode, NULL);
}

