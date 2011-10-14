/*
 * Simple MPEG/DVB parser to achieve network/service information without initial tuning data
 *
 * Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * The author can be reached at: handygewinnspiel AT gmx DOT de
 *
 * The project's page is http://wirbel.htpc-forum.de/w_scan/index2.html
 */



/* this file is shared between w_scan and the VDR plugin wirbelscan.
 * For details on both of them see http://wirbel.htpc-forum.de
 */


#ifndef _EXTENDED_DVBFRONTEND_H_
#define _EXTENDED_DVBFRONTEND_H_

#include <linux/dvb/frontend.h>

/******************************************************************************
 * definitions which are missing in <linux/dvb/frontend.h>
 *
 *****************************************************************************/

#ifndef fe_polarization // 300468 v181  6.2.13.2 Satellite delivery system descriptor
typedef enum fe_polarization {
        POLARIZATION_HORIZONTAL,
        POLARIZATION_VERTICAL,
        POLARIZATION_CIRCULAR_LEFT,
        POLARIZATION_CIRCULAR_RIGHT,
} fe_polarization_t;
#endif

#ifndef fe_west_east_flag       // 300468 v181  6.2.13.2 Satellite delivery system descriptor
typedef enum fe_west_east_flag {
        EAST_FLAG,
        WEST_FLAG,
} fe_west_east_flag_t;
#endif

#ifndef fe_interleave   // 300468 v181  6.2.13.4 Terrestrial delivery system descriptor
typedef enum fe_interleaver {
        INTERLEAVE_NATIVE,
        INTERLEAVE_IN_DEPTH,
        INTERLEAVE_AUTO,
} fe_interleave_t;
#endif

#ifndef fe_alpha        // 300468 v181  6.2.13.4 Terrestrial delivery system descriptor
typedef enum fe_alpha {
        ALPHA_1,
        ALPHA_2,
        ALPHA_4,
        ALPHA_AUTO,
} fe_alpha_t;
#endif

#ifndef fe_priority     // 300468 v181  6.2.13.4 Terrestrial delivery system descriptor
typedef enum fe_priority {
        PRIORITY_HP,
        PRIORITY_LP,
        PRIORITY_AUTO,
} fe_priority_t;
#endif

#ifndef fe_time_slicing // 300468 v181  6.2.13.4 Terrestrial delivery system descriptor
typedef enum fe_time_slicing {
        TIME_SLICING_ON,
        TIME_SLICING_OFF,
        TIME_SLICING_AUTO,
} fe_time_slicing_t;
#endif

#ifndef fe_mpe_fce      // 300468 v181  6.2.13.4 Terrestrial delivery system descriptor
typedef enum fe_mpe_fce {
        MPE_FCE_ON,
        MPE_FCE_OFF,
        MPE_FCE_AUTO,
} fe_mpe_fce_t;
#endif


/* since Linux DVB API v5 'struct dvb_qpsk_parameters' in frontend.h
 * is no longer able to store all information related to a
 * DVB-S frontend. Some information still missing at all in v5.
 */
struct extended_dvb_qpsk_parameters {
        __u32                   symbol_rate;                    /* symbols per second */
        fe_code_rate_t          fec_inner;                      /* inner forward error correction */
        fe_modulation_t         modulation_type;
        fe_pilot_t              pilot;                          /* not shure about this one. */
        fe_rolloff_t            rolloff;
        fe_delivery_system_t    modulation_system;
        fe_polarization_t       polarization;                   /* urgently missing in frontend.h */
        __u32                   orbital_position;
        fe_west_east_flag_t     west_east_flag;
        __u8                    scrambling_sequence_selector;   /* 6.2.13.3 S2 satellite delivery system descriptor */
        __u8                    multiple_input_stream_flag;     /* 6.2.13.3 S2 satellite delivery system descriptor */
        __u32                   scrambling_sequence_index;      /* 6.2.13.3 S2 satellite delivery system descriptor */
        __u8                    input_stream_identifier;        /* 6.2.13.3 S2 satellite delivery system descriptor */
};

/* since Linux DVB API v5 'struct dvb_qam_parameters' in frontend.h
 * is no longer able to store all information related to a
 * DVB-C frontend.
 */
struct extended_dvb_qam_parameters {
        __u32                   symbol_rate;    /* symbols per second */
        fe_code_rate_t          fec_inner;      /* inner forward error correction */
        fe_modulation_t         modulation;     /* modulation type */
        fe_delivery_system_t    delivery_system;
        __u32                   fec_outer;      /* not supported at all */
};

/* since Linux DVB API v5 'struct dvb_vsb_parameters' in frontend.h
 * is no longer able to store all information related to a
 * ATSC frontend.
 */
struct extended_dvb_vsb_parameters {
        fe_modulation_t         modulation;     /* modulation type */
        fe_code_rate_t          fec_inner;      /* forward error correction */
        fe_delivery_system_t    delivery_system;
        __u32                   fec_outer;      /* not supported at all */
};

/* since Linux DVB API v5 'struct dvb_ofdm_parameters' in frontend.h
 * is no longer able to store all information related to a
 * DVB-T frontend. Some information still missing at all in v5.
 */
struct  extended_dvb_ofdm_parameters {
        fe_bandwidth_t          bandwidth;
        fe_code_rate_t          code_rate_HP;   /* high priority stream code rate */
        fe_code_rate_t          code_rate_LP;   /* low priority stream code rate */
        fe_modulation_t         constellation;  /* modulation type */
        fe_transmit_mode_t      transmission_mode;
        fe_guard_interval_t     guard_interval;
        fe_hierarchy_t          hierarchy_information;
        fe_delivery_system_t    delivery_system;
        fe_alpha_t              alpha;          /* only defined in w_scan (see above) */
        fe_interleave_t         interleaver;    /* only defined in w_scan (see above) */
        fe_priority_t           priority;       /* only defined in w_scan (see above) */
        fe_time_slicing_t       time_slicing;   /* only defined in w_scan (see above) */
        fe_mpe_fce_t            mpe_fce;        /* only defined in w_scan (see above) */        
};

/******************************************************************************
 * intended to be used similar to struct dvb_frontend_parameter
 *
 *****************************************************************************/
struct  extended_dvb_frontend_parameters {
        __u32 frequency;                        /* QAM/OFDM/ATSC: abs. frequency in Hz */
                                                /* QPSK: intermediate frequency in kHz */
        fe_spectral_inversion_t inversion;
        union {
                struct extended_dvb_qpsk_parameters qpsk;
                struct extended_dvb_qam_parameters  qam;
                struct extended_dvb_ofdm_parameters ofdm;
                struct extended_dvb_vsb_parameters  vsb;
        } u;
};

#endif
