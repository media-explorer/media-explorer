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

#include <mex.h>

#include "mex-torrent.h"

static void mex_content_iface_init (MexContentIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexTorrent,
                         mex_torrent,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                mex_content_iface_init))

#define TORRENT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TORRENT, MexTorrentPrivate))

enum
{
  PROP_0,

  PROP_NAME,
  PROP_SIZE
};

struct _MexTorrentPrivate
{
  gchar *name;
  gint64 size;
};

/*
 * MexContent implementation
 */

static GParamSpec *
content_get_property (MexContent         *content,
                      MexContentMetadata  key)
{
  /* TODO */
  return NULL;
}

static const char *
content_get_metadata (MexContent         *content,
                      MexContentMetadata  key)
{
  MexTorrent *torrent = MEX_TORRENT (content);
  MexTorrentPrivate *priv = torrent->priv;

  switch (key)
  {
    case MEX_CONTENT_METADATA_TITLE:
      return priv->name;
    case MEX_CONTENT_METADATA_MIMETYPE:
      return "x-mex-torrent";
    default:
      return NULL;
  }

  return NULL;
}

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->get_property = content_get_property;
  iface->get_metadata = content_get_metadata;
}

/*
 * GObject implementation
 */

static void
mex_torrent_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  MexTorrent *torrent = MEX_TORRENT (object);
  MexTorrentPrivate *priv = torrent->priv;

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_SIZE:
      g_value_set_int64 (value, priv->size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_torrent_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  MexTorrent *torrent = MEX_TORRENT (object);
  MexTorrentPrivate *priv = torrent->priv;

  switch (property_id)
    {
    case PROP_NAME:
      g_free (priv->name);
      priv->name = g_strdup (g_value_get_string (value));
      break;
    case PROP_SIZE:
      priv->size = g_value_get_int64 (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_torrent_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_torrent_parent_class)->finalize (object);
}

static void
mex_torrent_class_init (MexTorrentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexTorrentPrivate));

  object_class->get_property = mex_torrent_get_property;
  object_class->set_property = mex_torrent_set_property;
  object_class->finalize = mex_torrent_finalize;

  pspec = g_param_spec_string ("name",
			       "Name",
			       "Torrent",
			       NULL,
			       G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_NAME, pspec);

  pspec = g_param_spec_int64 ("size",
                              "Size",
                              "Size (in bytes) of the torrent",
                              0, G_MAXINT64,
                              0,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SIZE, pspec);
}

static void
mex_torrent_init (MexTorrent *self)
{
  self->priv = TORRENT_PRIVATE (self);
}

MexTorrent *
mex_torrent_new (void)
{
  return g_object_new (MEX_TYPE_TORRENT, NULL);
}
