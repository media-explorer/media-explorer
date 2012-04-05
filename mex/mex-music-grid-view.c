/*
 * Mex - a media explorer
 *
 * Copyright © 2012 Intel Corporation.
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

#include "mex-music-grid-view.h"
#include "mex-view-model.h"
#include "mex-content-box.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (MexMusicGridView, mex_music_grid_view, MEX_TYPE_GRID_VIEW)

#define MUSIC_GRID_VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MUSIC_GRID_VIEW, MexMusicGridViewPrivate))

struct _MexMusicGridViewPrivate
{
  MexModel *model;
};


static void
mex_music_grid_view_select_artist (MxAction         *action,
                                   MexMusicGridView *view)
{
  MexMusicGridViewPrivate *priv = view->priv;
  const gchar *album_name;

  album_name = mx_action_get_display_name (action);

  mex_view_model_set_group_by (MEX_VIEW_MODEL (priv->model),
                               MEX_CONTENT_METADATA_ALBUM);
  mex_view_model_set_filter_by (MEX_VIEW_MODEL (priv->model),
                                MEX_CONTENT_METADATA_ARTIST, MEX_FILTER_EQUAL,
                                album_name, MEX_CONTENT_METADATA_NONE);

  mex_menu_pop (mex_grid_view_get_menu (MEX_GRID_VIEW (view)));

  g_object_set (priv->model,
                "title", album_name,
                "skip-ungrouped-items", FALSE,
                NULL);
}

static void
mex_music_grid_view_show_artists (MxAction         *action,
                                  MexMusicGridView *view)
{
  MexMusicGridViewPrivate *priv = view->priv;
  gint i;
  ClutterActor *header, *layout;
  MexMenu *menu;
  MexContent *content;
  MexModel *model;
  gunichar letter = 0;

  /* create a new menu */
  menu = mex_grid_view_get_menu (MEX_GRID_VIEW (view));
  mex_menu_push (menu);

  /* get layout */
  layout = (ClutterActor*) mex_menu_get_layout (menu);


  /* add header */
  header = mx_label_new_with_text (_("Artist"));
  mx_stylable_set_style_class (MX_STYLABLE (header), "Header");
  mx_label_set_y_align (MX_LABEL (header), MX_ALIGN_MIDDLE);
  mx_box_layout_insert_actor (MX_BOX_LAYOUT (layout), header, 0);

  /* create the model to get artists */
  model = mex_view_model_new (mex_model_get_model (priv->model));
  g_object_set (model, "skip-ungrouped-items", TRUE, NULL);
  mex_view_model_set_order_by (MEX_VIEW_MODEL (model),
                               MEX_CONTENT_METADATA_ARTIST, FALSE);
  mex_view_model_set_group_by (MEX_VIEW_MODEL (model),
                               MEX_CONTENT_METADATA_ARTIST);

  /* create album list */
  for (i = 0; (content = mex_model_get_content (model, i)); i++)
    {
      const gchar *title;
      gchar *normalised;


      title = mex_content_get_metadata (content, MEX_CONTENT_METADATA_TITLE);

      /* normalise the title */
      normalised = g_utf8_normalize (title, -1, G_NORMALIZE_ALL);

      if (!normalised)
        continue;

      /* get the first character */
      if (letter != g_utf8_get_char (normalised))
        {
          gchar display_name[7];
          gint length;

          letter = g_utf8_get_char (normalised);

          length = g_unichar_to_utf8 (letter, display_name);
          display_name[length] = '\0';

          mex_menu_add_section_header (menu, display_name);
        }

      g_free (normalised);
      normalised = NULL;

      action = mx_action_new_full (title, title,
                                   G_CALLBACK (mex_music_grid_view_select_artist),
                                   view);
      mex_menu_add_action (menu, action, MEX_MENU_NONE);
    }

  g_object_unref (model);
}

static void
mex_music_grid_view_show_albums (MxAction         *action,
                                 MexMusicGridView *view)
{
  MexMusicGridViewPrivate *priv = view->priv;

  mex_view_model_set_filter_by (MEX_VIEW_MODEL (priv->model),
                                MEX_CONTENT_METADATA_NONE, 0, NULL);
  mex_view_model_set_group_by (MEX_VIEW_MODEL (priv->model),
                               MEX_CONTENT_METADATA_ALBUM);
  g_object_set (priv->model,
                "title", _("All Albums"),
                "skip-ungrouped-items", TRUE,
                NULL);
}

static void
mex_music_grid_view_show_tracks (MxAction         *action,
                                 MexMusicGridView *view)
{
  MexMusicGridViewPrivate *priv = view->priv;

  mex_view_model_set_filter_by (MEX_VIEW_MODEL (priv->model),
                                MEX_CONTENT_METADATA_NONE, 0, NULL);
  mex_view_model_set_group_by (MEX_VIEW_MODEL (priv->model), 0);
  g_object_set (priv->model,
                "title", _("All Tracks"),
                "skip-ungrouped-items", FALSE,
                NULL);
}

static void
mex_music_grid_view_constructed (GObject *object)
{
  MexMenu *menu;
  MxAction *action;
  MexGrid *grid;

  G_OBJECT_CLASS (mex_music_grid_view_parent_class)->constructed (object);

  menu = mex_grid_view_get_menu (MEX_GRID_VIEW (object));

  /* Add menu items */
  action = mx_action_new_full ("artists",
                               _("Artists"),
                               G_CALLBACK (mex_music_grid_view_show_artists),
                               object);
  mex_menu_add_action (menu, action, MEX_MENU_RIGHT);

  action = mx_action_new_full ("albums",
                               _("Albums"),
                               G_CALLBACK (mex_music_grid_view_show_albums),
                               object);
  mex_menu_add_action (menu, action, MEX_MENU_NONE);

  action = mx_action_new_full ("tracks",
                               _("Tracks"),
                               G_CALLBACK (mex_music_grid_view_show_tracks),
                               object);
  mex_menu_add_action (menu, action, MEX_MENU_NONE);

  /* set custom grid properties */
  grid = mex_grid_view_get_grid (MEX_GRID_VIEW (object));
  g_object_set (grid, "stride", 4, "tile-ratio", 1.0, NULL);
}

static void
mex_music_grid_view_get_property (GObject    *object,
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
mex_music_grid_view_set_property (GObject      *object,
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
mex_music_grid_view_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_music_grid_view_parent_class)->dispose (object);
}

static void
mex_music_grid_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_music_grid_view_parent_class)->finalize (object);
}

static void
mex_music_grid_view_class_init (MexMusicGridViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexMusicGridViewPrivate));

  object_class->constructed = mex_music_grid_view_constructed;
  object_class->get_property = mex_music_grid_view_get_property;
  object_class->set_property = mex_music_grid_view_set_property;
  object_class->dispose = mex_music_grid_view_dispose;
  object_class->finalize = mex_music_grid_view_finalize;
}

static void
mex_music_grid_view_init (MexMusicGridView *self)
{
  self->priv = MUSIC_GRID_VIEW_PRIVATE (self);
}

ClutterActor *
mex_music_grid_view_new (MexModel *model)
{
  MexMusicGridView *view;

  view = g_object_new (MEX_TYPE_MUSIC_GRID_VIEW, "model", model, NULL);

  view->priv->model = model;

  /* show all albums by default */
  mex_music_grid_view_show_albums (NULL, view);

  return (ClutterActor *) view;
}
