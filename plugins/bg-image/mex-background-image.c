/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#include "mex-background-image.h"

#include <mx/mx.h>

#include <mex/mex-background.h>

static void mex_background_image_background_iface_init (MexBackgroundIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexBackgroundImage,
                         mex_background_image,
                         MX_TYPE_STACK,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_BACKGROUND,
                                                mex_background_image_background_iface_init))

#define BACKGROUND_IMAGE_PRIVATE(o)                             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_BACKGROUND_IMAGE,      \
                                MexBackgroundImagePrivate))

struct _MexBackgroundImagePrivate
{
  gchar        *image_url;
  const gchar  *name;
};

static void
mex_background_image_get_property (GObject    *object,
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
mex_background_image_set_property (GObject      *object,
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
mex_background_image_finalize (GObject *object)
{
  MexBackgroundImagePrivate *priv = MEX_BACKGROUND_IMAGE (object)->priv;

  g_free (priv->image_url);

  G_OBJECT_CLASS (mex_background_image_parent_class)->finalize (object);
}

static void
mex_background_image_class_init (MexBackgroundImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexBackgroundImagePrivate));

  object_class->get_property = mex_background_image_get_property;
  object_class->set_property = mex_background_image_set_property;
  object_class->finalize = mex_background_image_finalize;
}

static void
mex_background_image_init (MexBackgroundImage *self)
{
  MexBackgroundImagePrivate *priv;
  GError *error = NULL;
  ClutterActor *image;

  self->priv = priv = BACKGROUND_IMAGE_PRIVATE (self);

  priv->name = "image";
  priv->image_url =
    g_strconcat (mex_settings_get_config_dir (mex_settings_get_default ()),
                 "/mex-background.png", NULL);

  g_debug ("%s %s", mex_settings_get_config_dir (mex_settings_get_default ()),
                 "/mex-background.png");

  image = clutter_texture_new_from_file (priv->image_url, &error);

  if (error)
    {
      g_warning ("Error loading bg texture: %s", error->message);
      g_error_free (error);
    }
  else
    {
      clutter_container_add_actor (CLUTTER_CONTAINER (self), image);
    }
}


MexBackgroundImage *
mex_background_image_new (void)
{
  return g_object_new (MEX_TYPE_BACKGROUND_IMAGE, NULL);
}

static void
mex_background_image_set_active (MexBackground *self,
                                 gboolean       active)
{
  return;
}

static const gchar*
mex_background_image_get_name (MexBackground *self)
{
  MexBackgroundImagePrivate *priv = MEX_BACKGROUND_IMAGE (self)->priv;

  return priv->name;
}

static void
mex_background_image_background_iface_init (MexBackgroundIface *iface)
{
  iface->set_active = mex_background_image_set_active;
  iface->get_name = mex_background_image_get_name;
}
