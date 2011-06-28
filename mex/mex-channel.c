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


/**
 * SECTION:mex-channel
 * @short_description: A TV channel
 *
 * FIXME
 */

#include "mex-content.h"
#include "mex-log.h"

#include "mex-channel.h"

#define MEX_LOG_DOMAIN_DEFAULT  channel_log_domain
MEX_LOG_DOMAIN(channel_log_domain);

static void mex_content_iface_init (MexContentIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexChannel,
                         mex_channel,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                mex_content_iface_init))

#define CHANNEL_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_CHANNEL, MexChannelPrivate))

enum
{
  PROP_0,

  PROP_NAME,
  PROP_URI,
  PROP_THUMBNAIL_URI,
  PROP_LOGO_URI
};

struct _MexChannelPrivate
{
  gchar *name;
  gchar *uri;
  gchar *thumbnail_uri;
  gchar *logo_uri;
};

/*
 * MexContent implementation
 */

static const char *
mex_channel_get_metadata (MexContent         *content,
                          MexContentMetadata  key)
{
  MexChannel *channel = MEX_CHANNEL (content);
  MexChannelPrivate *priv = channel ->priv;

  switch (key)
    {
    case MEX_CONTENT_METADATA_TITLE:
      return priv->name;
      break;
    case MEX_CONTENT_METADATA_STREAM:
      return priv->uri;
    case MEX_CONTENT_METADATA_STILL:
      return priv->thumbnail_uri;
    case MEX_CONTENT_METADATA_MIMETYPE:
      return "x-mex-channel";
    case MEX_CONTENT_METADATA_STATION_LOGO:
      return priv->logo_uri;
    default:
      g_warning ("Can't provide metadata for %s on a MexChannel",
                 mex_content_metadata_key_to_string (key));
    }

  return NULL;
}

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->get_metadata = mex_channel_get_metadata;
}

/*
 * GObject implementation
 */

static void
mex_channel_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  MexChannel *channel = MEX_CHANNEL (object);
  MexChannelPrivate *priv = channel->priv;

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;
    case PROP_THUMBNAIL_URI:
      g_value_set_string (value, priv->thumbnail_uri);
      break;
    case PROP_LOGO_URI:
      g_value_set_string (value, priv->logo_uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_channel_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  MexChannel *channel = MEX_CHANNEL (object);

  switch (property_id)
    {
    case PROP_NAME:
      mex_channel_set_name (channel, g_value_get_string (value));
      break;
    case PROP_URI:
      mex_channel_set_uri (channel, g_value_get_string (value));
      break;
    case PROP_THUMBNAIL_URI:
      mex_channel_set_thumbnail_uri (channel, g_value_get_string (value));
      break;
    case PROP_LOGO_URI:
      mex_channel_set_logo_uri (channel, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_channel_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_channel_parent_class)->dispose (object);
}

static void
mex_channel_finalize (GObject *object)
{
  MexChannel *channel = MEX_CHANNEL (object);
  MexChannelPrivate *priv = channel->priv;

  g_free (priv->name);
  g_free (priv->uri);
  g_free (priv->thumbnail_uri);
  g_free (priv->logo_uri);

  G_OBJECT_CLASS (mex_channel_parent_class)->finalize (object);
}

static void
mex_channel_class_init (MexChannelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexChannelPrivate));

  object_class->get_property = mex_channel_get_property;
  object_class->set_property = mex_channel_set_property;
  object_class->dispose = mex_channel_dispose;
  object_class->finalize = mex_channel_finalize;

  pspec = g_param_spec_string ("name",
			       "Name",
			       "Channel name",
			       "Unnamed",
			       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_NAME, pspec);

  pspec = g_param_spec_string ("uri",
			       "URI",
			       "Channel Uri",
			       NULL,
			       G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_URI, pspec);

  pspec = g_param_spec_string ("thumbnail-uri",
			       "Thumbnail URI",
			       "The URI of a thumbnail of the channel",
			       NULL,
			       G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_THUMBNAIL_URI, pspec);

  pspec = g_param_spec_string ("logo-uri",
			       "Logo URI",
			       "The URI of the channel logo",
			       NULL,
			       G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_LOGO_URI, pspec);
}

static void
mex_channel_init (MexChannel *self)
{
  self->priv = CHANNEL_PRIVATE (self);
}

MexChannel *
mex_channel_new (void)
{
  return g_object_new (MEX_TYPE_CHANNEL, NULL);
}

/*
 * Accessors
 */

const gchar *
mex_channel_get_name (MexChannel *channel)
{
  g_return_val_if_fail (MEX_IS_CHANNEL (channel), NULL);

  return channel->priv->name;
}

void
mex_channel_set_name (MexChannel  *channel,
                      const gchar *name)
{
  MexChannelPrivate *priv;

  g_return_if_fail (MEX_IS_CHANNEL (channel));
  g_return_if_fail (name);
  priv = channel->priv;

  g_free (priv->name);
  priv->name = g_strdup (name);

  g_object_notify (G_OBJECT (channel), "name");
}

const gchar *
mex_channel_get_uri (MexChannel *channel)
{
  g_return_val_if_fail (MEX_IS_CHANNEL (channel), NULL);

  return channel->priv->uri;
}

void
mex_channel_set_uri (MexChannel  *channel,
                     const gchar *uri)
{
  MexChannelPrivate *priv;

  g_return_if_fail (MEX_IS_CHANNEL (channel));
  priv = channel->priv;

  g_free (priv->uri);
  priv->uri = g_strdup (uri);

  g_object_notify (G_OBJECT (channel), "uri");
}


const gchar *
mex_channel_get_thumbnail_uri (MexChannel *channel)
{
  g_return_val_if_fail (MEX_IS_CHANNEL (channel), NULL);

  return channel->priv->thumbnail_uri;
}

void
mex_channel_set_thumbnail_uri (MexChannel  *channel,
                               const gchar *uri)
{
  MexChannelPrivate *priv;

  g_return_if_fail (MEX_IS_CHANNEL (channel));
  g_return_if_fail (uri);
  priv = channel->priv;

  g_free (priv->thumbnail_uri);
  priv->thumbnail_uri = g_strdup (uri);

  g_object_notify (G_OBJECT (channel), "thumbnail-uri");
}

const gchar *
mex_channel_get_logo_uri (MexChannel *channel)
{
  g_return_val_if_fail (MEX_IS_CHANNEL (channel), NULL);

  return channel->priv->logo_uri;
}

void
mex_channel_set_logo_uri (MexChannel  *channel,
                          const gchar *uri)
{
  MexChannelPrivate *priv;

  g_return_if_fail (MEX_IS_CHANNEL (channel));
  g_return_if_fail (uri);
  priv = channel->priv;

  g_free (priv->logo_uri);
  priv->logo_uri = g_strdup (uri);

  g_object_notify (G_OBJECT (channel), "logo-uri");
}
