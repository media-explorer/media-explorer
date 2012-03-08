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

#include "mex-private.h"
#include "mex-enum-types.h"
#include "mex-content.h"

#include "mex-dvbt-channel.h"

static void mex_content_iface_init (MexContentIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexDVBTChannel,
                         mex_dvbt_channel,
                         MEX_TYPE_CHANNEL,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                mex_content_iface_init))


#define DVBT_CHANNEL_PRIVATE(o)                         \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                    \
                                MEX_TYPE_DVBT_CHANNEL,  \
                                MexDVBTChannelPrivate))

enum
{
  PROP_0,

  PROP_SERVICE_ID,
  PROP_PMT,

  PROP_FREQUENCY,
  PROP_INVERSION,
  PROP_BANDWIDTH,
  PROP_CODE_RATE_HP,
  PROP_CODE_RATE_LP,
  PROP_MODULATION,
  PROP_TRANSMISSION_MODE,
  PROP_GUARD,
  PROP_HIERARCHY
};

struct _MexDVBTChannelPrivate
{
  gchar *uri;

  guint frequency;  /* in kHz */
  MexDvbInversion inversion;
  MexDvbBandwidth bandwidth;
  MexDvbCodeRate code_rate_hp, code_rate_lp;
  MexDvbModulation modulation;
  MexDvbTransmissionMode transmission_mode;
  MexDvbGuard guard;
  MexDvbHierarchy hierarchy;

  gchar *service_id;
  guint pmt;
};

static void
on_service_id_changed (MexDVBTChannel *channel,
                       GParamSpec     *spec,
                       gpointer        data)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_free (priv->uri);
  priv->uri = g_strdup_printf ("dvb://%s", priv->service_id);

  g_object_notify (G_OBJECT (channel), "uri");
}

/*
 * Content implementation
 */

static const char *
mex_dvbt_channel_get_metadata (MexContent         *content,
                               MexContentMetadata  key)
{
  MexDVBTChannel *channel = MEX_DVBT_CHANNEL (content);
  MexDVBTChannelPrivate *priv = channel->priv;
  MexContentIface *iface_class, *parent_iface_class;

  switch (key)
    {
    case MEX_CONTENT_METADATA_URL:
      return priv->uri;
    default:
      /* Chain up to the parent implementation of that method */
      iface_class = MEX_CONTENT_GET_IFACE (channel);
      parent_iface_class = g_type_interface_peek_parent (iface_class);

      return parent_iface_class->get_metadata (content, key);
    }
}
static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->get_metadata = mex_dvbt_channel_get_metadata;
}

/*
 * GObject implementation
 */

static void
mex_dvbt_channel_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MexDVBTChannelPrivate *priv = MEX_DVBT_CHANNEL (object)->priv;

  switch (property_id)
    {
    case PROP_SERVICE_ID:
      g_value_set_string (value, priv->service_id);
      break;
    case PROP_PMT:
      g_value_set_uint (value, priv->pmt);
      break;

    case PROP_FREQUENCY:
      g_value_set_uint (value, priv->frequency);
      break;
    case PROP_INVERSION:
      g_value_set_enum (value, priv->inversion);
      break;
    case PROP_BANDWIDTH:
      g_value_set_enum (value, priv->bandwidth);
      break;
    case PROP_CODE_RATE_HP:
      g_value_set_enum (value, priv->code_rate_hp);
      break;
    case PROP_CODE_RATE_LP:
      g_value_set_enum (value, priv->code_rate_lp);
      break;
    case PROP_MODULATION:
      g_value_set_enum (value, priv->modulation);
      break;
    case PROP_TRANSMISSION_MODE:
      g_value_set_enum (value, priv->transmission_mode);
      break;
    case PROP_GUARD:
      g_value_set_enum (value, priv->guard);
      break;
    case PROP_HIERARCHY:
      g_value_set_enum (value, priv->hierarchy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_dvbt_channel_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MexDVBTChannelPrivate *priv = MEX_DVBT_CHANNEL (object)->priv;

  switch (property_id)
    {
    case PROP_SERVICE_ID:
      g_free (priv->service_id);
      priv->service_id = g_value_dup_string (value);
      break;
    case PROP_PMT:
      priv->pmt = g_value_get_uint (value);
      break;

    case PROP_FREQUENCY:
      priv->frequency = g_value_get_uint (value);
      break;
    case PROP_INVERSION:
      priv->inversion = g_value_get_enum (value);
      break;
    case PROP_BANDWIDTH:
      priv->bandwidth = g_value_get_enum (value);
      break;
    case PROP_CODE_RATE_HP:
      priv->code_rate_hp = g_value_get_enum (value);
      break;
    case PROP_CODE_RATE_LP:
      priv->code_rate_lp = g_value_get_enum (value);
      break;
    case PROP_MODULATION:
      priv->modulation = g_value_get_enum (value);
      break;
    case PROP_TRANSMISSION_MODE:
      priv->transmission_mode = g_value_get_enum (value);
      break;
    case PROP_GUARD:
      priv->guard = g_value_get_enum (value);
      break;
    case PROP_HIERARCHY:
      priv->hierarchy = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_dvbt_channel_finalize (GObject *object)
{
  MexDVBTChannelPrivate *priv = MEX_DVBT_CHANNEL (object)->priv;

  g_free (priv->service_id);
  g_free (priv->uri);

  G_OBJECT_CLASS (mex_dvbt_channel_parent_class)->finalize (object);
}

static void
mex_dvbt_channel_class_init (MexDVBTChannelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexDVBTChannelPrivate));

  object_class->get_property = mex_dvbt_channel_get_property;
  object_class->set_property = mex_dvbt_channel_set_property;
  object_class->finalize = mex_dvbt_channel_finalize;

  pspec = g_param_spec_string ("service-id",
                               "Service ID",
                               "Service ID",
                               NULL,
                               MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SERVICE_ID, pspec);

  pspec = g_param_spec_uint ("pmt",
                             "PMT",
                             "Program Map Table",
                             0, G_MAXUINT, 0,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PMT, pspec);

  pspec = g_param_spec_uint ("frequency",
                             "Frequency",
                             "Frequency of the channel (in kHz)",
                             0, G_MAXUINT, 0,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FREQUENCY, pspec);

  pspec = g_param_spec_enum ("inversion",
                             "Inversion",
                             "Inversion",
                             MEX_TYPE_DVB_INVERSION,
                             MEX_DVB_INVERSION_AUTO,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_INVERSION, pspec);

  pspec = g_param_spec_enum ("bandwidth",
                             "Bandwidth",
                             "Bandwidth",
                             MEX_TYPE_DVB_BANDWIDTH,
                             MEX_DVB_BANDWIDTH_AUTO,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_BANDWIDTH, pspec);

  pspec = g_param_spec_enum ("code-rate-hp",
                             "Code Rate HP",
                             "Code Rate HP",
                             MEX_TYPE_DVB_CODE_RATE,
                             MEX_DVB_CODE_RATE_AUTO,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CODE_RATE_HP, pspec);

  pspec = g_param_spec_enum ("code-rate-lp",
                             "Code Rate LP",
                             "Code Rate LP",
                             MEX_TYPE_DVB_CODE_RATE,
                             MEX_DVB_CODE_RATE_AUTO,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CODE_RATE_LP, pspec);

  pspec = g_param_spec_enum ("modulation",
                             "Modulation",
                             "Modulation",
                             MEX_TYPE_DVB_MODULATION,
                             MEX_DVB_MODULATION_AUTO,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MODULATION, pspec);

  pspec = g_param_spec_enum ("transmission-mode",
                             "Transmission Mode",
                             "Transmission Mode",
                             MEX_TYPE_DVB_TRANSMISSION_MODE,
                             MEX_DVB_TRANSMISSION_MODE_AUTO,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TRANSMISSION_MODE, pspec);

  pspec = g_param_spec_enum ("guard",
                             "Guard",
                             "Guard",
                             MEX_TYPE_DVB_GUARD,
                             MEX_DVB_GUARD_AUTO,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_GUARD, pspec);

  pspec = g_param_spec_enum ("hierarchy",
                             "Hierarchy",
                             "Hierarchy",
                             MEX_TYPE_DVB_HIERARCHY,
                             MEX_DVB_HIERARCHY_AUTO,
                             MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_HIERARCHY, pspec);
}

static void
mex_dvbt_channel_init (MexDVBTChannel *self)
{
  self->priv = DVBT_CHANNEL_PRIVATE (self);

  g_signal_connect (self, "notify::service-id",
                    G_CALLBACK (on_service_id_changed), NULL);
}

MexChannel *
mex_dvbt_channel_new (void)
{
  return g_object_new (MEX_TYPE_DVBT_CHANNEL, NULL);
}

const gchar *
mex_dvbt_channel_get_service_id (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), NULL);

  return channel->priv->service_id;
}

void
mex_dvbt_channel_set_service_id (MexDVBTChannel *channel,
                                 const gchar    *service_id)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  g_free (priv->service_id);
  priv->service_id  = g_strdup (service_id);

  g_object_notify (G_OBJECT (channel), "service-id");
}

guint
mex_dvbt_channel_get_pmt (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), 0);

  return channel->priv->pmt;
}

void
mex_dvbt_channel_set_pmt (MexDVBTChannel *channel,
                                guint           pmt)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (pmt == priv->pmt)
    return;

  priv->pmt = pmt;

  g_object_notify (G_OBJECT (channel), "pmt");
}

guint
mex_dvbt_channel_get_frequency (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), 0);

  return channel->priv->frequency;
}

void
mex_dvbt_channel_set_frequency (MexDVBTChannel *channel,
                                guint           frequency)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (frequency == priv->frequency)
    return;

  priv->frequency = frequency;

  g_object_notify (G_OBJECT (channel), "frequency");
}

guint
mex_dvbt_channel_get_inversion (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), MEX_DVB_INVERSION_AUTO);

  return channel->priv->inversion;
}

void
mex_dvbt_channel_set_inversion (MexDVBTChannel  *channel,
                                MexDvbInversion  inversion)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (inversion == priv->inversion)
    return;

  priv->inversion = inversion;

  g_object_notify (G_OBJECT (channel), "inversion");
}

MexDvbBandwidth
mex_dvbt_channel_get_bandwidth (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), MEX_DVB_BANDWIDTH_AUTO);

  return channel->priv->bandwidth;
}

void
mex_dvbt_channel_set_bandwidth (MexDVBTChannel  *channel,
                               MexDvbBandwidth  bandwidth)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (bandwidth == priv->bandwidth)
    return;

  priv->bandwidth = bandwidth;

  g_object_notify (G_OBJECT (channel), "bandwidth");
}

MexDvbCodeRate
mex_dvbt_channel_get_code_rate_hp (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), MEX_DVB_CODE_RATE_AUTO);

  return channel->priv->code_rate_hp;
}

void
mex_dvbt_channel_set_code_rate_hp (MexDVBTChannel *channel,
                                   MexDvbCodeRate  code_rate_hp)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (code_rate_hp == priv->code_rate_hp)
    return;

  priv->code_rate_hp = code_rate_hp;

  g_object_notify (G_OBJECT (channel), "code-rate-hp");
}

MexDvbCodeRate
mex_dvbt_channel_get_code_rate_lp (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), MEX_DVB_CODE_RATE_AUTO);

  return channel->priv->code_rate_lp;
}

void
mex_dvbt_channel_set_code_rate_lp (MexDVBTChannel *channel,
                                   MexDvbCodeRate  code_rate_lp)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (code_rate_lp == priv->code_rate_lp)
    return;

  priv->code_rate_lp = code_rate_lp;

  g_object_notify (G_OBJECT (channel), "code-rate-lp");
}

MexDvbModulation
mex_dvbt_channel_get_modulation (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), MEX_DVB_MODULATION_AUTO);

  return channel->priv->modulation;
}

void
mex_dvbt_channel_set_modulation (MexDVBTChannel   *channel,
                                 MexDvbModulation  modulation)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (modulation == priv->modulation)
    return;

  priv->modulation = modulation;

  g_object_notify (G_OBJECT (channel), "modulation");
}

MexDvbTransmissionMode
mex_dvbt_channel_get_transmission_mode (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel),
                        MEX_DVB_TRANSMISSION_MODE_AUTO);

  return channel->priv->transmission_mode;
}

void
mex_dvbt_channel_set_transmission_mode (MexDVBTChannel         *channel,
                                        MexDvbTransmissionMode  mode)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (mode == priv->transmission_mode)
    return;

  priv->transmission_mode = mode;

  g_object_notify (G_OBJECT (channel), "transmission-mode");
}

MexDvbGuard
mex_dvbt_channel_get_guard (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), MEX_DVB_GUARD_AUTO);

  return channel->priv->guard;
}

void
mex_dvbt_channel_set_guard (MexDVBTChannel *channel,
                            MexDvbGuard     guard)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (guard == priv->guard)
    return;

  priv->guard = guard;

  g_object_notify (G_OBJECT (channel), "guard");
}

MexDvbHierarchy
mex_dvbt_channel_get_hierarchy (MexDVBTChannel *channel)
{
  g_return_val_if_fail (MEX_IS_DVBT_CHANNEL (channel), MEX_DVB_HIERARCHY_AUTO);

  return channel->priv->hierarchy;
}

void
mex_dvbt_channel_set_hierarchy (MexDVBTChannel  *channel,
                                MexDvbHierarchy  hierarchy)
{
  MexDVBTChannelPrivate *priv = channel->priv;

  g_return_if_fail (MEX_IS_DVBT_CHANNEL (channel));

  if (hierarchy == priv->hierarchy)
    return;

  priv->hierarchy = hierarchy;

  g_object_notify (G_OBJECT (channel), "hierarchy");
}
