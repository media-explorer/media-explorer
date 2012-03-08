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

#include "mex-video-grid-view.h"
#include "mex-view-model.h"
#include "mex-content-box.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (MexVideoGridView, mex_video_grid_view, MEX_TYPE_GRID_VIEW)

#define VIDEO_GRID_VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_VIDEO_GRID_VIEW, MexVideoGridViewPrivate))

struct _MexVideoGridViewPrivate
{
  MexModel *model;
};

static void
mex_video_grid_view_show_all (MxAction         *action,
                              MexVideoGridView *view)
{
  MexVideoGridViewPrivate *priv = view->priv;

  mex_view_model_set_filter_by (MEX_VIEW_MODEL (priv->model),
                                MEX_CONTENT_METADATA_NONE, 0, NULL);
  g_object_set (priv->model,
                "title", _("All Items"),
                "skip-ungrouped-items", FALSE,
                NULL);
}

static void
mex_video_grid_view_show_tv_shows (MxAction         *action,
                                 MexVideoGridView *view)
{
  MexVideoGridViewPrivate *priv = view->priv;

  mex_view_model_set_filter_by (MEX_VIEW_MODEL (priv->model),
                                MEX_CONTENT_METADATA_SERIES_NAME,
                                MEX_FILTER_NOT, NULL,
                                MEX_CONTENT_METADATA_NONE);
  g_object_set (priv->model,
                "title", _("TV Shows"),
                "skip-ungrouped-items", TRUE,
                NULL);
}

static void
mex_video_grid_view_show_films (MxAction         *action,
                                MexVideoGridView *view)
{
  MexVideoGridViewPrivate *priv = view->priv;

  mex_view_model_set_filter_by (MEX_VIEW_MODEL (priv->model),
                                MEX_CONTENT_METADATA_YEAR, MEX_FILTER_NOT, NULL,
                                MEX_CONTENT_METADATA_NONE);
  g_object_set (priv->model,
                "title", _("Films"),
                "skip-ungrouped-items", FALSE,
                NULL);
}

static void
mex_video_grid_view_constructed (GObject *object)
{
  MexMenu *menu;
  MxAction *action;
  MexModel *model;

  G_OBJECT_CLASS (mex_video_grid_view_parent_class)->constructed (object);

  menu = mex_grid_view_get_menu (MEX_GRID_VIEW (object));

  /* check if the model is already filtered */
  g_object_get (object, "model", &model, NULL);

  if (model && MEX_IS_VIEW_MODEL (model)
      && mex_view_model_get_is_filtered (MEX_VIEW_MODEL (model)))
    {
      /* if the model is already filtered, don't add new filter items */
      g_object_unref (model);
      return;
    }

  if (model)
    g_object_unref (model);


  /* Add menu items */
  action = mx_action_new_full ("all-items",
                               _("All Items"),
                               G_CALLBACK (mex_video_grid_view_show_all),
                               object);
  mex_menu_add_action (menu, action, MEX_MENU_NONE);

  action = mx_action_new_full ("tv-shows",
                               _("TV Shows"),
                               G_CALLBACK (mex_video_grid_view_show_tv_shows),
                               object);
  mex_menu_add_action (menu, action, MEX_MENU_NONE);

  action = mx_action_new_full ("films",
                               _("Films"),
                               G_CALLBACK (mex_video_grid_view_show_films),
                               object);
  mex_menu_add_action (menu, action, MEX_MENU_NONE);
}

static void
mex_video_grid_view_get_property (GObject    *object,
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
mex_video_grid_view_set_property (GObject      *object,
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
mex_video_grid_view_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_video_grid_view_parent_class)->dispose (object);
}

static void
mex_video_grid_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_video_grid_view_parent_class)->finalize (object);
}

static void
mex_video_grid_view_class_init (MexVideoGridViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexVideoGridViewPrivate));

  object_class->constructed = mex_video_grid_view_constructed;
  object_class->get_property = mex_video_grid_view_get_property;
  object_class->set_property = mex_video_grid_view_set_property;
  object_class->dispose = mex_video_grid_view_dispose;
  object_class->finalize = mex_video_grid_view_finalize;
}

static void
mex_video_grid_view_init (MexVideoGridView *self)
{
  self->priv = VIDEO_GRID_VIEW_PRIVATE (self);
}

ClutterActor *
mex_video_grid_view_new (MexModel *model)
{
  MexVideoGridView *view;

  view = g_object_new (MEX_TYPE_VIDEO_GRID_VIEW, "model", model, NULL);

  view->priv->model = model;

  return (ClutterActor *) view;
}
