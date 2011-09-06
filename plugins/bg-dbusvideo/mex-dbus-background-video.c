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

#include "mex-dbus-background-video.h"

#include <mex/mex-background.h>
#include <mex/mex-player-client.h>
#include <mex/mex-utils.h>

static void mex_dbus_background_video_background_iface_init (MexBackgroundIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexDbusBackgroundVideo,
                         mex_dbus_background_video,
                         CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_BACKGROUND,
                                                mex_dbus_background_video_background_iface_init))

#define BACKGROUND_VIDEO_PRIVATE(o)                                  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                 \
                                MEX_TYPE_DBUS_BACKGROUND_VIDEO,      \
                                MexDbusBackgroundVideoPrivate))

struct _MexDbusBackgroundVideoPrivate
{
  gboolean      active;
  const gchar  *name;
  gchar        *video_url;
  ClutterMedia *media;
};

static void eos_cb (ClutterMedia *media, MexDbusBackgroundVideo *background);

static void
mex_dbus_background_video_get_property (GObject    *object,
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
mex_dbus_background_video_set_property (GObject      *object,
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
mex_dbus_background_video_dispose (GObject *object)
{
  MexDbusBackgroundVideoPrivate *priv = MEX_DBUS_BACKGROUND_VIDEO (object)->priv;

  if (priv->media)
    {
      if (priv->active)
        clutter_media_set_playing (priv->media, FALSE);
      g_object_unref (priv->media);
      priv->media = NULL;
    }

  G_OBJECT_CLASS (mex_dbus_background_video_parent_class)->finalize (object);
}

static void
mex_dbus_background_video_finalize (GObject *object)
{
  MexDbusBackgroundVideoPrivate *priv = MEX_DBUS_BACKGROUND_VIDEO (object)->priv;

  g_free (priv->video_url);

  G_OBJECT_CLASS (mex_dbus_background_video_parent_class)->finalize (object);
}

static void
mex_dbus_background_video_constructed (GObject *object)
{
  MexDbusBackgroundVideoPrivate *priv = MEX_DBUS_BACKGROUND_VIDEO (object)->priv;

  if (G_OBJECT_CLASS (mex_dbus_background_video_parent_class)->constructed)
    G_OBJECT_CLASS (mex_dbus_background_video_parent_class)->constructed (object);

  /* Don't start playing until we're activated */
  clutter_media_set_playing (CLUTTER_MEDIA (object), FALSE);
  clutter_actor_set_opacity (CLUTTER_ACTOR (object), 0);
}

static void
mex_dbus_background_video_class_init (MexDbusBackgroundVideoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexDbusBackgroundVideoPrivate));

  object_class->get_property = mex_dbus_background_video_get_property;
  object_class->set_property = mex_dbus_background_video_set_property;
  object_class->dispose = mex_dbus_background_video_dispose;
  object_class->finalize = mex_dbus_background_video_finalize;
}

static void
mex_dbus_background_video_init (MexDbusBackgroundVideo *self)
{
  MexDbusBackgroundVideoPrivate *priv;

  self->priv = priv = BACKGROUND_VIDEO_PRIVATE (self);

  priv->video_url = g_strconcat ("file://", mex_get_data_dir (),
                                 "/style/background-loop.mkv", NULL);

  priv->media = (ClutterMedia *) mex_player_client_new ();

  priv->name = "dbusvideo";

  g_signal_connect (priv->media, "eos", G_CALLBACK (eos_cb), self);
}

MexDbusBackgroundVideo *
mex_dbus_background_video_new (void)
{
  return g_object_new (MEX_TYPE_DBUS_BACKGROUND_VIDEO, NULL);
}

static void
mex_dbus_background_video_set_active (MexBackground *self,
                                      gboolean       active)
{
  MexDbusBackgroundVideoPrivate *priv = MEX_DBUS_BACKGROUND_VIDEO (self)->priv;

  priv->active = active;
  if (active)
    {
      g_signal_connect (priv->media, "eos", G_CALLBACK (eos_cb), self);
      clutter_media_set_uri (CLUTTER_MEDIA (priv->media), priv->video_url);
      clutter_media_set_playing (CLUTTER_MEDIA (priv->media), TRUE);
    }
  else
    {
      g_signal_handlers_disconnect_by_func (priv->media,
                                            G_CALLBACK (eos_cb), self);
      clutter_media_set_playing (CLUTTER_MEDIA (priv->media), FALSE);
      clutter_media_set_uri (CLUTTER_MEDIA (priv->media), NULL);
    }
}

static const gchar *
mex_dbus_background_video_get_name (MexBackground *self)
{
  MexDbusBackgroundVideoPrivate *priv = MEX_DBUS_BACKGROUND_VIDEO (self)->priv;

  return priv->name;
}

static void
mex_dbus_background_video_background_iface_init (MexBackgroundIface *iface)
{
  iface->set_active = mex_dbus_background_video_set_active;
  iface->get_name = mex_dbus_background_video_get_name;
}

static void
eos_cb (ClutterMedia *media, MexDbusBackgroundVideo *background)
{
  MexDbusBackgroundVideoPrivate *priv = background->priv;

  if (!priv->active)
    return;

  clutter_media_set_progress (media, 0);
  clutter_media_set_playing (media, TRUE);
}
