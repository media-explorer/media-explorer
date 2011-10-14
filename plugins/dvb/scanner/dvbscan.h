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

#ifndef __HEADER_DVBSCAN_H__
#define __HEADER_DVBSCAN_H__

#include <linux/dvb/frontend.h>


typedef struct init_item {
        const char *    name;
        int             id;
} _init_item;



/********************************************************************
 * DVB-T
 *
 ********************************************************************/

/* convert text to identifiers */
int txt_to_ofdm_bw ( const char * txt );
int txt_to_ofdm_fec ( const char * txt );
int txt_to_ofdm_mod ( const char * txt );
int txt_to_ofdm_transmission ( const char * txt );
int txt_to_ofdm_guard ( const char * txt );
int txt_to_ofdm_hierarchy ( const char * txt );

/*convert identifier to text */
const char * ofdm_bw_to_txt ( int id );
const char * ofdm_fec_to_txt ( int id );
const char * ofdm_mod_to_txt ( int id );
const char * ofdm_transmission_to_txt ( int id );
const char * ofdm_guard_to_txt ( int id );
const char * ofdm_hierarchy_to_txt ( int id );

/********************************************************************
 * DVB-C
 *
 ********************************************************************/

/* convert text to identifiers */
int txt_to_qam_fec ( const char * txt );
int txt_to_qam_mod ( const char * txt );

/*convert identifier to text */
const char * qam_fec_to_txt ( int id );
const char * qam_mod_to_txt ( int id );

/********************************************************************
 * ATSC
 *
 ********************************************************************/

/* convert text to identifiers */
int txt_to_atsc_mod ( const char * txt );

/*convert identifier to text */
const char * atsc_mod_to_txt ( int id );

/********************************************************************
 * DVB-S
 *
 ********************************************************************/

/* convert text to identifiers */
int txt_to_qpsk_delivery_system ( const char * txt );
int txt_to_qpsk_pol ( const char * txt );
int txt_to_qpsk_fec ( const char * txt );
int txt_to_qpsk_rolloff ( const char * txt );
int txt_to_qpsk_mod ( const char * txt );

/*convert identifier to text */
const char * qpsk_delivery_system_to_txt ( int id );
const char * qpsk_pol_to_txt ( int id );
const char * qpsk_fec_to_txt ( int id );
const char * qpsk_rolloff_to_txt ( int id );
const char * qpsk_mod_to_txt ( int id );

/********************************************************************
 * non-frontend specific part
 *
 ********************************************************************/

/* convert text to identifiers */
int txt_to_fe_type ( const char * txt );

/*convert identifier to text */
const char * fe_type_to_txt ( int id );


#endif
