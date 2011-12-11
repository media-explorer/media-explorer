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

#ifndef __MEX_DVB_DEFINES_H__
#define __MEX_DVB_DEFINES_H__

#include <glib.h>

G_BEGIN_DECLS

/*
 * Those definition are kept in sync with the GStreamer enums defined in dvbsrc.
 */

typedef enum
{
  MEX_DVB_INVERSION_OFF,
  MEX_DVB_INVERSION_ON,
  MEX_DVB_INVERSION_AUTO
} MexDvbInversion;

typedef enum
{
  MEX_DVB_BANDWIDTH_8M,
  MEX_DVB_BANDWIDTH_7M,
  MEX_DVB_BANDWIDTH_6M,
  MEX_DVB_BANDWIDTH_AUTO
} MexDvbBandwidth;

typedef enum
{
  MEX_DVB_CODE_RATE_NONE,
  MEX_DVB_CODE_RATE_1_2,
  MEX_DVB_CODE_RATE_2_3,
  MEX_DVB_CODE_RATE_3_4,
  MEX_DVB_CODE_RATE_4_5,
  MEX_DVB_CODE_RATE_5_6,
  MEX_DVB_CODE_RATE_6_7,
  MEX_DVB_CODE_RATE_7_8,
  MEX_DVB_CODE_RATE_8_9,
  MEX_DVB_CODE_RATE_AUTO
} MexDvbCodeRate;

typedef enum
{
  MEX_DVB_MODULATION_QPSK,
  MEX_DVB_MODULATION_QAM16,
  MEX_DVB_MODULATION_QAM32,
  MEX_DVB_MODULATION_QAM64,
  MEX_DVB_MODULATION_QAM128,
  MEX_DVB_MODULATION_QAM256,
  MEX_DVB_MODULATION_AUTO,
  MEX_DVB_MODULATION_8VSB,
  MEX_DVB_MODULATION_16VSB
} MexDvbModulation;

typedef enum
{
  MEX_DVB_GUARD_32,
  MEX_DVB_GUARD_16,
  MEX_DVB_GUARD_8,
  MEX_DVB_GUARD_4,
  MEX_DVB_GUARD_AUTO
} MexDvbGuard;

typedef enum
{
  MEX_DVB_TRANSMISSION_MODE_2K,
  MEX_DVB_TRANSMISSION_MODE_8K,
  MEX_DVB_TRANSMISSION_MODE_AUTO
} MexDvbTransmissionMode;

typedef enum
{
  MEX_DVB_HIERARCHY_NONE,
  MEX_DVB_HIERARCHY_1,
  MEX_DVB_HIERARCHY_2,
  MEX_DVB_HIERARCHY_4,
  MEX_DVB_HIERARCHY_AUTO
} MexDvbHierarchy;

G_END_DECLS

#endif /* __MEX_DVB_DEFINES_H__ */
