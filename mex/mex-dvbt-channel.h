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

#ifndef __MEX_DVBT_CHANNEL_H__
#define __MEX_DVBT_CHANNEL_H__

#include <glib-object.h>

#include <mex/mex-channel.h>
#include <mex/mex-dvb-defines.h>

G_BEGIN_DECLS

#define MEX_TYPE_DVBT_CHANNEL mex_dvbt_channel_get_type()

#define MEX_DVBT_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_DVBT_CHANNEL, MexDVBTChannel))

#define MEX_DVBT_CHANNEL_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                \
                            MEX_TYPE_DVBT_CHANNEL,  \
                            MexDVBTChannelClass))

#define MEX_IS_DVBT_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_DVBT_CHANNEL))

#define MEX_IS_DVBT_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_DVBT_CHANNEL))

#define MEX_DVBT_CHANNEL_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                  \
                              MEX_TYPE_DVBT_CHANNEL,  \
                              MexDVBTChannelClass))

typedef struct _MexDVBTChannel MexDVBTChannel;
typedef struct _MexDVBTChannelClass MexDVBTChannelClass;
typedef struct _MexDVBTChannelPrivate MexDVBTChannelPrivate;

struct _MexDVBTChannel
{
  MexChannel parent;

  MexDVBTChannelPrivate *priv;
};

struct _MexDVBTChannelClass
{
  MexChannelClass parent_class;
};

GType mex_dvbt_channel_get_type (void) G_GNUC_CONST;

MexChannel *              mex_dvbt_channel_new                    (void);
guint                     mex_dvbt_channel_get_frequency          (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_frequency          (MexDVBTChannel *channel,
                                                                   guint           frequency);

guint                     mex_dvbt_channel_get_inversion          (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_inversion          (MexDVBTChannel  *channel,
                                                                   MexDvbInversion  inversion);
MexDvbBandwidth           mex_dvbt_channel_get_bandwidth          (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_bandwidth          (MexDVBTChannel  *channel,
                                                                   MexDvbBandwidth  bandwidth);
MexDvbCodeRate            mex_dvbt_channel_get_code_rate_hp       (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_code_rate_hp       (MexDVBTChannel *channel,
                                                                   MexDvbCodeRate  code_rate_hp);
MexDvbCodeRate            mex_dvbt_channel_get_code_rate_lp       (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_code_rate_lp       (MexDVBTChannel *channel,
                                                                   MexDvbCodeRate  code_rate_lp);
MexDvbModulation          mex_dvbt_channel_get_modulation         (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_modulation         (MexDVBTChannel   *channel,
                                                                   MexDvbModulation  modulation);
MexDvbTransmissionMode    mex_dvbt_channel_get_transmission_mode  (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_transmission_mode  (MexDVBTChannel         *channel,
                                                                   MexDvbTransmissionMode  mode);
MexDvbGuard               mex_dvbt_channel_get_guard              (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_guard              (MexDVBTChannel *channel,
                                                                   MexDvbGuard     guard);
MexDvbHierarchy           mex_dvbt_channel_get_hierarchy          (MexDVBTChannel *channel);
void                      mex_dvbt_channel_set_hierarchy          (MexDVBTChannel  *channel,
                                                                   MexDvbHierarchy  hierarchy);

G_END_DECLS

#endif /* __MEX_DVBT_CHANNEL_H__ */
