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

#include <gio/gio.h>

#include "mex-channel.h"
#include "mex-channel-provider.h"
#include "mex-log.h"

#include "mex-uri-channel-provider.h"

#define MEX_LOG_DOMAIN_DEFAULT  channel_log_domain
MEX_LOG_DOMAIN_EXTERN(channel_log_domain);

static void
mex_channel_provider_iface_init (MexChannelProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexUriChannelProvider,
                         mex_uri_channel_provider,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CHANNEL_PROVIDER,
                                                mex_channel_provider_iface_init))

#define URI_CHANNEL_PROVIDER_PRIVATE(o)                           \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                  MEX_TYPE_URI_CHANNEL_PROVIDER,  \
                                  MexUriChannelProviderPrivate))

enum
{
  PROP_0,

  PROP_CONFIG_FILE
};

struct _MexUriChannelProviderPrivate
{
  GPtrArray *channels;

  gchar *config_file;
};

static gboolean
parse_line (MexUriChannelProvider *provider,
            const gchar           *line)
{
  MexUriChannelProviderPrivate *priv = provider->priv;
  gchar **fields, *name, *uri;
  MexChannel *channel;

  fields = g_strsplit (line, "|", 0);
  if (!(fields[0] && fields[1]))
    {
      MEX_WARNING ("Invalid channel definition in %s: %s",
                   priv->config_file, line);
      g_strfreev (fields);
      return FALSE;
    }

  name = fields[0];
  uri = fields[1];

  channel = g_object_new (MEX_TYPE_CHANNEL,
                          "name", name,
                          "uri", uri,
                          NULL);
  g_ptr_array_add (priv->channels, channel);
  g_strfreev (fields);

  return TRUE;
}

static gboolean
parse_config (MexUriChannelProvider *provider)
{
  MexUriChannelProviderPrivate *priv = provider->priv;
  GFileInputStream *input;
  GDataInputStream *data;
  gboolean result = TRUE;
  GError *error = NULL;
  gchar *line;
  GFile *file;

  if (priv->channels)
    g_ptr_array_free (priv->channels, TRUE);
  priv->channels = g_ptr_array_new_with_free_func (g_object_unref);

  file = g_file_new_for_path (priv->config_file);
  input = g_file_read (file, NULL, &error);
  if (G_UNLIKELY (error))
    {
      MEX_WARNING ("Could not read config file %s: %s",
                   priv->config_file, error->message);
      g_clear_error (&error);
      goto read_error;
    }

  data = g_data_input_stream_new (G_INPUT_STREAM (input));

  line = g_data_input_stream_read_line (data, NULL, NULL, &error);
  while (line)
    {
      parse_line (provider, line);
      g_free (line);
      line = g_data_input_stream_read_line (data, NULL, NULL, &error);
    }
  if (G_UNLIKELY (error))
    {
      MEX_WARNING ("Could not read line: %s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (data);
  g_object_unref (input);
read_error:
  g_object_unref (file);
  return result;
}

/*
 * MexChannelProvider implementation
 */

static guint
mex_uri_channel_provider_get_n_channels (MexChannelProvider *provider)
{
  MexUriChannelProvider *uri_provider = MEX_URI_CHANNEL_PROVIDER (provider);

  return uri_provider->priv->channels->len;
}

static const GPtrArray *
mex_uri_channel_provider_get_channels (MexChannelProvider *provider)
{
  MexUriChannelProvider *uri_provider = MEX_URI_CHANNEL_PROVIDER (provider);

  return uri_provider->priv->channels;
}

static void
mex_channel_provider_iface_init (MexChannelProviderInterface *iface)
{
  iface->get_n_channels = mex_uri_channel_provider_get_n_channels;
  iface->get_channels = mex_uri_channel_provider_get_channels;
}

/*
 * GObject implementation
 */

static void
mex_uri_channel_provider_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  MexUriChannelProvider *provider = MEX_URI_CHANNEL_PROVIDER (object);
  MexUriChannelProviderPrivate *priv = provider->priv;

  switch (property_id)
    {
    case PROP_CONFIG_FILE:
      g_value_set_string (value, priv->config_file);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_uri_channel_provider_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  MexUriChannelProvider *provider = MEX_URI_CHANNEL_PROVIDER (object);

  switch (property_id)
    {
    case PROP_CONFIG_FILE:
      mex_uri_channel_provider_set_config_file (provider,
                                                g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_uri_channel_provider_finalize (GObject *object)
{
  MexUriChannelProvider *provider = MEX_URI_CHANNEL_PROVIDER (object);
  MexUriChannelProviderPrivate *priv = provider->priv;

  g_free (priv->config_file);

  G_OBJECT_CLASS (mex_uri_channel_provider_parent_class)->finalize (object);
}

static void
mex_uri_channel_provider_class_init (MexUriChannelProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexUriChannelProviderPrivate));

  object_class->get_property = mex_uri_channel_provider_get_property;
  object_class->set_property = mex_uri_channel_provider_set_property;
  object_class->finalize = mex_uri_channel_provider_finalize;

  pspec = g_param_spec_string ("config-file",
			       "Config file",
			       "Channel configuration file",
			       "channels-uri.conf",
			       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CONFIG_FILE, pspec);
}

static void
mex_uri_channel_provider_init (MexUriChannelProvider *self)
{
  self->priv = URI_CHANNEL_PROVIDER_PRIVATE (self);
}

MexChannelProvider *
mex_uri_channel_provider_new (const gchar *config_file)
{
  return g_object_new (MEX_TYPE_URI_CHANNEL_PROVIDER,
                       "config-file", config_file,
                       NULL);
}

const gchar *
mex_uri_channel_provider_get_config_file (MexUriChannelProvider *provider)
{
  g_return_val_if_fail (MEX_IS_URI_CHANNEL_PROVIDER (provider), NULL);

  return provider->priv->config_file;
}

gboolean
mex_uri_channel_provider_set_config_file (MexUriChannelProvider *provider,
                                          const gchar           *config_file)
{
  MexUriChannelProviderPrivate *priv;

  g_return_val_if_fail (MEX_IS_URI_CHANNEL_PROVIDER (provider), FALSE);
  priv = provider->priv;

  g_free (priv->config_file);
  priv->config_file = g_strdup (config_file);

  return parse_config (provider);
}
