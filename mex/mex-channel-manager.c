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

#include <glib/gi18n.h>

#include "mex-generic-model.h"
#include "mex-model-manager.h"

#include "mex-channel-manager.h"

G_DEFINE_TYPE (MexChannelManager, mex_channel_manager, G_TYPE_OBJECT)

#define CHANNEL_MANAGER_PRIVATE(o)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o),                      \
                                  MEX_TYPE_CHANNEL_MANAGER, \
                                  MexChannelManagerPrivate))

struct _MexChannelManagerPrivate
{
  GPtrArray *channels;
  MexModel *channels_model;
  MexLogoProvider *logo_provider; /* For now a single logo provider */
};

static void
ensure_logos (MexChannelManager *manager)
{
  MexChannelManagerPrivate *priv = manager->priv;
  guint i;

  /* To query the URL of the logos we need channels and a LogoProvider */
  if (priv->channels->len == 0)
    return;

  if (priv->logo_provider == NULL)
    return;

  for (i = 0; i < priv->channels->len; i++)
    {
      MexChannel *channel = g_ptr_array_index (priv->channels, i);
      gchar *logo_uri;

      logo_uri = mex_logo_provider_get_channel_logo (priv->logo_provider,
                                                     channel);
      mex_channel_set_logo_uri (channel, logo_uri);
      g_free (logo_uri);
    }
}

/*
 * GObject implementation
 */

static void
mex_channel_manager_get_property (GObject    *object,
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
mex_channel_manager_set_property (GObject      *object,
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
mex_channel_manager_dispose (GObject *object)
{
  MexChannelManager *manager = MEX_CHANNEL_MANAGER (object);
  MexChannelManagerPrivate *priv = manager->priv;

  if (priv->logo_provider)
    {
      g_object_unref (priv->logo_provider);
      priv->logo_provider = NULL;
    }

  if (priv->channels_model)
    {
      g_object_unref (priv->channels_model);
      priv->channels_model = NULL;
    }

  G_OBJECT_CLASS (mex_channel_manager_parent_class)->dispose (object);
}

static void
mex_channel_manager_finalize (GObject *object)
{
  MexChannelManager *manager = MEX_CHANNEL_MANAGER (object);
  MexChannelManagerPrivate *priv = manager->priv;

  g_ptr_array_free (priv->channels, TRUE);

  G_OBJECT_CLASS (mex_channel_manager_parent_class)->finalize (object);
}

static void
mex_channel_manager_class_init (MexChannelManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexChannelManagerPrivate));

  object_class->get_property = mex_channel_manager_get_property;
  object_class->set_property = mex_channel_manager_set_property;
  object_class->dispose = mex_channel_manager_dispose;
  object_class->finalize = mex_channel_manager_finalize;
}

static void
mex_channel_manager_init (MexChannelManager *self)
{
  MexChannelManagerPrivate *priv;
  MexModelManager *manager;

  self->priv = priv = CHANNEL_MANAGER_PRIVATE (self);

  priv->channels = g_ptr_array_new_with_free_func (g_object_unref);

  manager = mex_model_manager_get_default ();
  priv->channels_model = mex_generic_model_new (_("TV"), "icon-panelheader-tv");
  g_object_set (G_OBJECT (priv->channels_model), "category", "live", NULL);

  mex_model_manager_add_model (manager, priv->channels_model);
}

MexChannelManager *
mex_channel_manager_get_default (void)
{
  static MexChannelManager *singleton = NULL;

  if (G_LIKELY (singleton))
    return singleton;

  singleton = g_object_new (MEX_TYPE_CHANNEL_MANAGER, NULL);
  return singleton;
}

const GPtrArray *
mex_channel_manager_get_channels (MexChannelManager *manager)
{
  g_return_val_if_fail (MEX_IS_CHANNEL_MANAGER (manager), NULL);

  return manager->priv->channels;
}

static void
add_channels_from_ptr_array (MexChannelManager *manager,
                             const GPtrArray   *channels)
{
  MexChannelManagerPrivate *priv = manager->priv;
  guint previous_len;
  guint i;

  previous_len = priv->channels->len;
  g_ptr_array_set_size (priv->channels, previous_len + channels->len);

  for (i = 0; i < channels->len; i++)
    {
      MexChannel *channel = g_ptr_array_index (channels, i);

      priv->channels->pdata[previous_len + i] = g_object_ref (channel);
      mex_model_add_content (priv->channels_model, MEX_CONTENT (channel));
    }
}

void
mex_channel_manager_add_provider (MexChannelManager  *manager,
                                  MexChannelProvider *provider)
{
  const GPtrArray *channels;

  g_return_if_fail (MEX_IS_CHANNEL_MANAGER (manager));
  g_return_if_fail (MEX_IS_CHANNEL_PROVIDER (provider));

  channels = mex_channel_provider_get_channels (provider);

  add_channels_from_ptr_array (manager, channels);

  ensure_logos (manager);
}

void
mex_channel_manager_add_channels (MexChannelManager *manager,
                                  const GPtrArray   *channels)
{
  g_return_if_fail (MEX_IS_CHANNEL_MANAGER (manager));
  g_return_if_fail (channels != NULL);

  add_channels_from_ptr_array (manager, channels);

  ensure_logos (manager);
}

guint
mex_channel_manager_get_n_channels (MexChannelManager *manager)
{
  g_return_val_if_fail (MEX_IS_CHANNEL_MANAGER (manager), 0);

  return manager->priv->channels->len;
}

gint
mex_channel_manager_get_channel_position (MexChannelManager *manager,
                                          MexChannel        *channel)
{
  MexChannelManagerPrivate *priv;
  MexChannel *c;
  guint i;

  g_return_val_if_fail (MEX_IS_CHANNEL_MANAGER (manager), -1);
  priv = manager->priv;

  for (i = 0; i < priv->channels->len; i++)
    {
      c = g_ptr_array_index (priv->channels, i);

      if (c == channel)
        return i;
    }

  return -1;
}

void
mex_channel_manager_add_logo_provider (MexChannelManager *manager,
                                       MexLogoProvider   *provider)
{
  MexChannelManagerPrivate *priv;

  g_return_if_fail (MEX_IS_CHANNEL_MANAGER (manager));
  g_return_if_fail (MEX_IS_LOGO_PROVIDER (provider));
  priv = manager->priv;

  priv->logo_provider = g_object_ref (provider);

  ensure_logos (manager);
}
