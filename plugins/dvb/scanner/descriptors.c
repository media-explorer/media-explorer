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
 *
 * added 20090303 -wk-
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/dvb/frontend.h>
#include "scan.h"
#include "descriptors.h"
#include "atsc_psip_section.h"
#include "char-coding.h"

#define hd(d)  hexdump(__FUNCTION__, d + 2, d[1])


/******************************************************************************
 * returns minimum repetition rates as specified in ETR211 4.4.1 and 4.4.2
 * and 13818-1 C.9 Bandwidth Utilization and Signal Acquisition Time
 *****************************************************************************/

int repetition_rate(fe_type_t frontend_type, enum table_id table) {

switch (frontend_type) {
        case FE_QAM:
        case FE_QPSK:
                // ETR211 4.4.1 Satellite and cable delivery systems
                switch (table) {
                        case TABLE_PAT:
                        case TABLE_CAT:
                        case TABLE_PMT:
                        case TABLE_TSDT:
                                // see 13818-1 C.9 Bandwidth Utilization and Signal Acquisition Time
                                // FIXME: i did not understand fully
                                // but i seems to be (1/1 .. [1/25] .. 1/100) sec
                                // no hard spec.. :-(
                                return 1;
                        case TABLE_SDT_ACT:
                        case TABLE_EIT_ACT:
                        case TABLE_EIT_SCHEDULE_ACT_50 ... TABLE_EIT_SCHEDULE_ACT_5F:
                                return 2;
                        case TABLE_NIT_ACT:
                        case TABLE_NIT_OTH:
                        case TABLE_BAT:
                        case TABLE_SDT_OTH:
                        case TABLE_EIT_OTH:
                                return 10;
                        case TABLE_EIT_SCHEDULE_OTH_60 ... TABLE_EIT_SCHEDULE_OTH_60:
                        case TABLE_TDT:
                        case TABLE_TOT:
                                return 30;
                        default:
                                debug("table id 0x%.02X no repetition rate defined.\n",
                                        table);
                                return 30;
                        }
                break;
        case FE_OFDM:
                // ETR211 4.4.2 Terrestrial delivery systems
                switch (table) {
                        case TABLE_PAT:
                        case TABLE_CAT:
                        case TABLE_PMT:
                        case TABLE_TSDT:
                                // see 13818-1 C.9 Bandwidth Utilization and Signal Acquisition Time
                                // FIXME: i did not understand fully
                                // but i seems to be (1/1 .. [1/25] .. 1/100) sec
                                // no hard spec.. :-(
                                return 1;
                        case TABLE_NIT_ACT:
                        case TABLE_NIT_OTH:
                        case TABLE_BAT:
                        case TABLE_SDT_OTH:
                        case TABLE_EIT_SCHEDULE_ACT_50 ... TABLE_EIT_SCHEDULE_ACT_5F:
                                return 12;
                        case TABLE_SDT_ACT:
                        case TABLE_EIT_ACT:
                                return 2;
                        case TABLE_EIT_OTH:
                                return 20;
                        case TABLE_EIT_SCHEDULE_OTH_60 ... TABLE_EIT_SCHEDULE_OTH_60:
                                return 60;
                        case TABLE_TDT:
                        case TABLE_TOT:
                                return 30;
                        default:
                                debug("table id 0x%.02X no repetition rate defined.\n",
                                        table);
                                return 30;
                        }
                break;
        case FE_ATSC:
                switch (table) {
                        case TABLE_PAT:
                        case TABLE_CAT:
                        case TABLE_PMT:
                        case TABLE_TSDT:
                                // see 13818-1 C.9 Bandwidth Utilization and Signal Acquisition Time
                                // FIXME: i did not understand fully
                                // but i seems to be (1/1 .. [1/25] .. 1/100) sec
                                // no hard spec.. :-(
                                return 1;
                        default:
                                /* FIXME: i dont have *any* information about atsc
                                 * repetition rates. This should not break anything,
                                 * but may be we will loose performance or services..
                                 * these are the values mkrufky put in.
                                 * Probably the same values as above..?
                                 */
                                debug("table id 0x%.02X no repetition rate defined.\n",
                                        table);
                                return 5;
                        }
                return 5;
        default:
                fatal("undefined frontend type.\n");
        }
};

/******************************************************************************
 * 300468 v181 6.2.32 Service descriptor
 *****************************************************************************/

void parse_service_descriptor (const unsigned char *buf, struct service *s, unsigned user_charset_id) {
        unsigned char len;
        uint i, full_len, short_len, isUtf8;
        uint emphasis_on = 0;
        char * provider_name = NULL;
        char * provider_short_name = NULL;
        char * service_name = NULL;
        char * service_short_name = NULL;
        size_t inbytesleft, outbytesleft;
        char * inbuf = NULL;
        char * outbuf = NULL;

        hd(buf);
        s->type = buf[2];

        buf += 3;
        len = *buf;
        buf++;

        if (s->provider_name) {
                free (s->provider_name);
                s->provider_name = 0;
                }
        if (s->provider_short_name) {
                free (s->provider_short_name);
                s->provider_name = 0;
                }
        full_len = short_len = emphasis_on = 0;
        isUtf8 = (*buf == 0x15); 
        /* count length for short provider name
         * and long provider name
         */
        for (i=0; i < len; i++) {        
                switch(*(buf + i)) {
                        case sb_cc_reserved_80 ... sb_cc_reserved_85:
                        case sb_cc_reserved_88 ... sb_cc_reserved_89:
                        case sb_cc_user_8B ... sb_cc_user_9F:
                        // ETR211 4.6.1 Use of control codes in names
                        case character_cr_lf:
                                continue;
                        case character_emphasis_on:
                                emphasis_on = 1;
                                continue;
                        case character_emphasis_off:
                                emphasis_on = 0;
                                continue;
                        case utf8_cc_start:
                                if (isUtf8 && (i+1 < len)) {
                                   uint16_t utf8_cc;
                                   utf8_cc  = *(buf + i) << 8;
                                   utf8_cc += *(buf + i + 1);

                                   switch(utf8_cc) {
                                         case utf8_character_emphasis_on:
                                              emphasis_on = 1;
                                              i++;
                                              continue;
                                         case utf8_character_emphasis_off:
                                              emphasis_on = 0;
                                              i++;
                                              continue;
                                         default:;
                                         }
                                   }
                        default:
                                if (emphasis_on)
                                   short_len++;
                                full_len++;
                                continue;
                        }
                }
        /* allocating memory and zero-terminating */
        provider_name = calloc (full_len + 1, 1);
        provider_short_name = calloc (short_len + 1, 1);
        full_len = short_len = emphasis_on = 0;
        /* copy data */
        for (i=0; i < len; i++) {
                switch(*(buf + i)) {
                        case sb_cc_reserved_80 ... sb_cc_reserved_85:
                        case sb_cc_reserved_88 ... sb_cc_reserved_89:
                        case sb_cc_user_8B ... sb_cc_user_9F:
                        // ETR211 4.6.1 Use of control codes in names
                        case character_cr_lf:
                                continue;
                        case character_emphasis_on:
                                emphasis_on = 1;
                                continue;
                        case character_emphasis_off:
                                emphasis_on = 0;
                                continue;
                        case utf8_cc_start:
                                if (isUtf8 && (i+1 < len)) {
                                   uint16_t utf8_cc;
                                   utf8_cc  = *(buf + i) << 8;
                                   utf8_cc += *(buf + i + 1);

                                   switch(utf8_cc) {
                                         case utf8_character_emphasis_on:
                                              emphasis_on = 1;
                                              i++;
                                              continue;
                                         case utf8_character_emphasis_off:
                                              emphasis_on = 0;
                                              i++;
                                              continue;
                                         default:;
                                         }
                                   } 
                        default:
                                if (emphasis_on)
                                   provider_short_name[short_len++] = *(buf + i);
                                provider_name[full_len++] = *(buf + i);
                                continue;
                        }
                }
        if (provider_name[0]) {
                inbytesleft = full_len;
                outbytesleft = 4 * full_len + 1;
                s->provider_name = (char *) calloc(outbytesleft, 1);
                inbuf = provider_name;
                outbuf = s->provider_name;
                char_coding(&inbuf, &inbytesleft, &outbuf, &outbytesleft, user_charset_id);
                }

        free(provider_name);

        if (provider_short_name[0]) {
                inbytesleft = short_len;
                outbytesleft = 4 * short_len + 1;
                s->provider_short_name = (char *) calloc(outbytesleft, 1);
                inbuf = provider_short_name;
                outbuf = s->provider_short_name;
                char_coding(&inbuf, &inbytesleft, &outbuf, &outbytesleft, user_charset_id);
                }

        free(provider_short_name);

        buf += len;
        len = *buf;
        buf++;
        if (s->service_name)
                free (s->service_name);
        if (s->service_short_name)
                free (s->service_short_name);
        isUtf8 = (*buf == 0x15);
        /* count length for short service name
         * and long service name
         */
        full_len = short_len = emphasis_on = 0;
        for (i=0; i < len; i++) {
                switch(*(buf + i)) {
                        case sb_cc_reserved_80 ... sb_cc_reserved_85:
                        case sb_cc_reserved_88 ... sb_cc_reserved_89:
                        case sb_cc_user_8B ... sb_cc_user_9F:
                        // ETR211 4.6.1 Use of control codes in names
                        case character_cr_lf:
                                continue;
                        case character_emphasis_on:
                                emphasis_on = 1;
                                continue;
                        case character_emphasis_off:
                                emphasis_on = 0;
                                continue;
                        case utf8_cc_start:
                                if (isUtf8 && (i+1 < len)) {
                                   uint16_t utf8_cc;
                                   utf8_cc  = *(buf + i) << 8;
                                   utf8_cc += *(buf + i + 1);

                                   switch(utf8_cc) {
                                         case utf8_character_emphasis_on:
                                              emphasis_on = 1;
                                              i++;
                                              continue;
                                         case utf8_character_emphasis_off:
                                              emphasis_on = 0;
                                              i++;
                                              continue;
                                         default:;
                                         }
                                   }
                        default:
                                if (emphasis_on)
                                   short_len++;
                                full_len++;
                                continue;
                        }
                }
        /* allocating memory and zero-terminating */
        service_name = calloc (full_len + 1, 1);        
        service_short_name = calloc (short_len + 1, 1);
        full_len = short_len = emphasis_on = 0;
        /* copy data */
        for (i=0; i < len; i++) {
                switch(*(buf + i)) {
                        case sb_cc_reserved_80 ... sb_cc_reserved_85:
                        case sb_cc_reserved_88 ... sb_cc_reserved_89:
                        case sb_cc_user_8B ... sb_cc_user_9F:
                        // ETR211 4.6.1 Use of control codes in names
                        case character_cr_lf:
                                continue;
                        case character_emphasis_on:
                                emphasis_on = 1;
                                continue;
                        case character_emphasis_off:
                                emphasis_on = 0;
                                continue;
                        case utf8_cc_start:
                                if (isUtf8 && (i+1 < len)) {
                                   uint16_t utf8_cc;
                                   utf8_cc  = *(buf + i) << 8;
                                   utf8_cc += *(buf + i + 1);

                                   switch(utf8_cc) {
                                         case utf8_character_emphasis_on:
                                              emphasis_on = 1;
                                              i++;
                                              continue;
                                         case utf8_character_emphasis_off:
                                              emphasis_on = 0;
                                              i++;
                                              continue;
                                         default:;
                                         }
                                   }
                        default:
                                if (emphasis_on)
                                   service_short_name[short_len++] = *(buf + i);
                                service_name[full_len++] = *(buf + i);
                                continue;
                        }
                }
        if (service_name[0]) {
                inbytesleft = full_len;
                outbytesleft = 4 * full_len + 1;
                s->service_name = (char *) calloc(outbytesleft, 1);
                inbuf = service_name;
                outbuf = s->service_name;
                char_coding(&inbuf, &inbytesleft, &outbuf, &outbytesleft, user_charset_id);
                }

        free(service_name);

        if (service_short_name[0]) {
                inbytesleft = short_len;
                outbytesleft = 4 * short_len + 1;
                s->service_short_name = (char *) calloc(outbytesleft, 1);
                inbuf = service_short_name;
                outbuf = s->service_short_name;
                char_coding(&inbuf, &inbytesleft, &outbuf, &outbytesleft, user_charset_id);
                }

        free(service_short_name);

        info("\tservice = %s (%s)\n", s->service_name, s->provider_name);
}

void parse_ca_identifier_descriptor (const unsigned char *buf, struct service *s) {
        unsigned char len = buf [1];
        unsigned int i;

        buf += 2;

        if (len > sizeof(s->ca_id)) {
                len = sizeof(s->ca_id);
                warning("too many CA system ids\n");
                }
        memcpy(s->ca_id, buf, len);
        s->ca_num=0;
        for (i = 0; i < len / sizeof(s->ca_id[0]); i++) {
                int id = ((s->ca_id[i] & 0x00FF) << 8) + ((s->ca_id[i] & 0xFF00) >> 8);
                s->ca_id[i] = id;
                moreverbose("\tCA ID\t: PID 0x%04x\n", s->ca_id[i]);
                s->ca_num++;
                }
}

void parse_ca_descriptor (const unsigned char *buf, struct service *s) {
        unsigned char descriptor_length = buf [1];
        int CA_system_ID;
        int found=0;
        int i;

        buf += 2;

        if (descriptor_length < 4)
                return;
        CA_system_ID = (buf[0] << 8) | buf[1];

        for (i=0; i<s->ca_num; i++)
                if (s->ca_id[i] == CA_system_ID)
                        found++;

        if (!found) {
                if (s->ca_num + 1 >= CA_SYSTEM_ID_MAX)
                        warning("TOO MANY CA SYSTEM IDs.\n");
                else {
                        moreverbose("\tCA ID\t: PID 0x%04x\n", CA_system_ID);
                        s->ca_id[s->ca_num]=CA_system_ID;
                        s->ca_num++;
                        }
                }
} 

void parse_iso639_language_descriptor (const unsigned char *buf, struct service *s) {
        unsigned int lang_count = buf[1] / 4;
        unsigned int i;
        buf += 2;
        for (i = 0; i < lang_count; i++) {
                // ISO_639_language_code 24 bslbf
                memcpy(s->audio_lang[s->audio_num], buf, 3);                
/*              switch (buf[3]) { // audio_type 8 bslbf, seems to be wrong all over the place
                        case 1: // clean effects, program element has no language
                                break;
                        case 2: // hearing impaired, program element is prepared for the hearing impaired
                                break;
                        case 3: // visual_impaired_commentary, program element is prepared for the visually impaired viewer
                                break;
                        default:
                                info("unhandled audio_type.\n");
                        }*/
                buf += 4;
                }
}

void parse_network_name_descriptor (const unsigned char *buf, struct transponder *t) {
        unsigned char len = buf[1];
        //hd(buf);
        if (t == NULL) {
                info("%s: transponder == NULL\n", __FUNCTION__);
                return;
                }
        if (t->network_name)
                free (t->network_name);
        t->network_name = (char *) malloc(len + 1);
        memcpy(t->network_name, buf + 2, len);
        t->network_name[len] = '\0';
        if (!t->network_name[0]) {
                free (t->network_name);
                t->network_name = 0;
                }
}

static long bcd32_to_cpu (const int b0, const int b1, const int b2, const int b3) {
        return ((b0 >> 4) & 0x0f) * 10000000 + (b0 & 0x0f) * 1000000 +
               ((b1 >> 4) & 0x0f) * 100000   + (b1 & 0x0f) * 10000 +
               ((b2 >> 4) & 0x0f) * 1000     + (b2 & 0x0f) * 100 +
               ((b3 >> 4) & 0x0f) * 10       + (b3 & 0x0f);
}

void parse_S2_satellite_delivery_system_descriptor(const unsigned char *buf, void * dummy) {
        hd(buf);
        /* FIXME: finding that descriptor means that we're dealing with two
         * transponders on the same freq. I'm not shure now what to do this case.
         */
        //scrambling_sequence_selector 1 bslbf
        //scrambling_sequence_selector = (buf[2] & 0x80) >> 7;
        //multiple_input_stream_flag 1 bslbf
        //multiple_input_stream_flag = (buf[2] & 0x40) >> 6;
        //backwards_compatibility_indicator 1 bslbf
        //backwards_compatibility_indicator = (buf[2] & 0x20) >> 5;
        //reserved_future_use 5 bslbf
        //buf += 3;
        //if (scrambling_sequence_selector == 1) {
        //              Reserved 6 bslbf
        //              scrambling_sequence_index 18 uimsbf
        //              scrambling_sequence_index = (*buf++ & 0x03) << 16;
        //              scrambling_sequence_index |= *buf++ << 8;
        //              scrambling_sequence_index |= *buf++;
        //              }
        //if (multiple_input_stream_flag == 1) {
        //              input_stream_identifier 8 uimsbf
        //              input_stream_identifier = *buf++;
        //}
        verbose("S2_satellite_delivery_system_descriptor(skipped.)\n");
}

void parse_satellite_delivery_system_descriptor(const unsigned char *buf,
                        struct transponder *t, fe_spectral_inversion_t inversion) {
        if (t == NULL)
                return;
        hd(buf);
        t->type = FE_QPSK;
        t->param.inversion = inversion;
        /* frequency is coded in GHz, where the decimal point occurs after the
         * third character (e.g. 011,75725 GHz).
         */
        t->param.frequency = bcd32_to_cpu (buf[2], buf[3], buf[4], buf[5]);
        t->param.frequency *= 10;
        //orbital_position 16 bslbf
        t->param.u.qpsk.orbital_position = (buf[6] << 8) | buf[7];
        //west_east_flag 1 bslbf
        t->param.u.qpsk.west_east_flag = (buf[8] & 0x80) >> 7;
        //polarization 2 bslbf
        switch ((buf[8] & 0x60) >> 5) {
                case 0: t->param.u.qpsk.polarization = POLARIZATION_HORIZONTAL;         break;
                case 1: t->param.u.qpsk.polarization = POLARIZATION_VERTICAL;           break;
                case 2: t->param.u.qpsk.polarization = POLARIZATION_CIRCULAR_LEFT;      break;
                case 3: t->param.u.qpsk.polarization = POLARIZATION_CIRCULAR_RIGHT;     break;
                default:
                        fatal("polarization decoding failed: %d\n", (buf[8] & 0x60) >> 5);
                }
        switch ((buf[8] & 0x18) >> 3) {
                case 0: t->param.u.qpsk.rolloff = ROLLOFF_35;   break;
                case 1: t->param.u.qpsk.rolloff = ROLLOFF_25;   break;
                case 2: t->param.u.qpsk.rolloff = ROLLOFF_20;   break;
                case 3:        
                        warning("reserved rolloff value 3 found\n");
                        t->param.u.qpsk.rolloff = ROLLOFF_AUTO;
                        break;
                default:
                        fatal("rolloff decoding failed: %d\n", (buf[8] & 0x18) >> 3);
                }
        switch ((buf[8] & 0x04) >> 2) {
                case 0: t->param.u.qpsk.modulation_system = SYS_DVBS;   break;
                case 1: t->param.u.qpsk.modulation_system = SYS_DVBS2;  break;
                default:
                        t->param.u.qpsk.modulation_system = SYS_DVBS;
                }
        //modulation_type 2 bslbf
        switch (buf[8] & 0x03) {
                case 1: t->param.u.qpsk.modulation_type = QPSK;         break;
                case 2: t->param.u.qpsk.modulation_type = PSK_8;        break;
                case 3: t->param.u.qpsk.modulation_type = QAM_16;       break;
                default:
                        t->param.u.qpsk.modulation_type = QAM_AUTO;
                }
        if (t->param.u.qpsk.modulation_type == PSK_8)
                t->param.u.qpsk.modulation_system = SYS_DVBS2;
        //symbol_rate 28 bslbf
        t->param.u.qpsk.symbol_rate = bcd32_to_cpu (buf[9], buf[10], buf[11], buf[12] & 0xF0);
        t->param.u.qpsk.symbol_rate *= 10;
        //FEC_inner 4 bslbf
        switch (buf[12] & 0x0F) {
                case 1: t->param.u.qpsk.fec_inner = FEC_1_2;    break;
                case 2: t->param.u.qpsk.fec_inner = FEC_2_3;    break;
                case 3: t->param.u.qpsk.fec_inner = FEC_3_4;    break;
                case 4: t->param.u.qpsk.fec_inner = FEC_5_6;    break;
                case 5: t->param.u.qpsk.fec_inner = FEC_7_8;    break;
                case 6: t->param.u.qpsk.fec_inner = FEC_8_9;    break;
                case 7: t->param.u.qpsk.fec_inner = FEC_3_5;    break;
                case 8: t->param.u.qpsk.fec_inner = FEC_4_5;    break;
                case 9: t->param.u.qpsk.fec_inner = FEC_9_10;   break;
                case 15:t->param.u.qpsk.fec_inner = FEC_NONE;   break;
                default:
                        verbose("\t%s: undefined inner fec %u\n",
                        __FUNCTION__, buf[12] & 0x0F);
                        t->param.u.qpsk.fec_inner = FEC_AUTO;
                }
        /* some NIT's are broken. */
        if ((t->param.u.qpsk.modulation_type == PSK_8) ||
            (t->param.u.qpsk.rolloff == ROLLOFF_25) ||
            (t->param.u.qpsk.rolloff == ROLLOFF_20) ||
            (t->param.u.qpsk.fec_inner == FEC_9_10) ||
            (t->param.u.qpsk.fec_inner == FEC_3_5)) {
                verbose("\t%s: fixing broken NIT, setting modulation_system to DVB-S2.\n",
                    __FUNCTION__);
                t->param.u.qpsk.modulation_system = SYS_DVBS2;
                }
}


#ifndef FEC_RS_204_208  //FIXME: as soon as defined in Linux DVB API, insert correct name here.
        #define FEC_RS_204_208  FEC_AUTO
#endif

void parse_cable_delivery_system_descriptor (const unsigned char *buf, struct transponder *t,
                                                fe_spectral_inversion_t inversion) {
        if (t == NULL)
                return;
        hd(buf);
        t->type = FE_QAM;
        t->param.u.qam.delivery_system = SYS_DVBC_ANNEX_AC;
        t->param.inversion = inversion;
        /*frequency is coded in MHz, where the decimal occurs after the fourth
        character (e.g. 0312,0000 MHz).
        */
        t->param.frequency = bcd32_to_cpu (buf[2], buf[3], buf[4], buf[5]);
        t->param.frequency *= 100;
        //t->reserved_future_use = (buf[6] << 4) | ((buf[7] & 0xf0) >> 4); 
        //FEC_outer 4 bslbf -> not used by linuxtv dvb api. WHY?
        switch (buf[7] & 0x0f) {
                case 1: t->param.u.qam.fec_outer = FEC_NONE;       break;
                case 2: t->param.u.qam.fec_outer = FEC_RS_204_208; break;
                default:
                        info("undefined outer fec\n");
                        t->param.u.qam.fec_outer = FEC_AUTO;
                }
        //modulation 8 bslbf
        switch (buf[8]) {
                case 1: t->param.u.qam.modulation = QAM_16;  break;
                case 2: t->param.u.qam.modulation = QAM_32;  break;
                case 3: t->param.u.qam.modulation = QAM_64;  break;
                case 4: t->param.u.qam.modulation = QAM_128; break;
                case 5: t->param.u.qam.modulation = QAM_256; break;
                default:
                        info("undefined modulation\n");
                        t->param.u.qam.modulation = QAM_AUTO;
                }
        //symbol_rate 28 bslbf
        t->param.u.qam.symbol_rate = \
                bcd32_to_cpu (buf[9], buf[10], buf[11], buf[12] & 0xf0);
        t->param.u.qam.symbol_rate *= 10; 
        //FEC_inner 4 bslbf
        switch (buf[12] & 0x0f) {
                case 1: t->param.u.qam.fec_inner = FEC_1_2;   break;
                case 2: t->param.u.qam.fec_inner = FEC_2_3;   break;
                case 3: t->param.u.qam.fec_inner = FEC_3_4;   break;
                case 4: t->param.u.qam.fec_inner = FEC_5_6;   break;
                case 5: t->param.u.qam.fec_inner = FEC_7_8;   break;
                case 6: t->param.u.qam.fec_inner = FEC_8_9;   break;
                case 7: t->param.u.qam.fec_inner = FEC_3_5;   break;
                case 8: t->param.u.qam.fec_inner = FEC_4_5;   break;
                case 9: t->param.u.qam.fec_inner = FEC_9_10;  break;
                case 15: t->param.u.qam.fec_inner = FEC_NONE; break;
                default:
                        info("undefined inner fec\n");
                        t->param.u.qam.fec_inner = FEC_AUTO;
                }
}

void parse_terrestrial_delivery_system_descriptor (const unsigned char *buf,
                        struct transponder *t, fe_spectral_inversion_t inversion) {
        struct extended_dvb_ofdm_parameters *o;

        if (t == NULL) return;
        hd(buf);
        o = &t->param.u.ofdm;
        t->type = FE_OFDM;
        o->delivery_system = SYS_DVBT;
        t->param.inversion = inversion;
        //center_frequency 32 bslbf
        t->param.frequency  = (buf[2] << 24) | (buf[3] << 16); 
        t->param.frequency |= (buf[4] << 8)  |  buf[5];
        //10Hz steps
        t->param.frequency *= 10;
        //bandwidth 3 bslbf
        switch (buf[6] >> 5) {
                case 0: o->bandwidth = BANDWIDTH_8_MHZ; break;
                case 1: o->bandwidth = BANDWIDTH_7_MHZ; break;
                case 2: o->bandwidth = BANDWIDTH_6_MHZ; break;
                #ifdef BANDWIDTH_5_MHZ
                // every time the same: not defined in linuxtv dvb api :-(
                case 3: o->bandwidth = BANDWIDTH_5_MHZ; break;
                #endif
                default:
                        info("undefined bandwidth value found.\n");
                        o->bandwidth = BANDWIDTH_8_MHZ;
                }
        // priority 1 bslbf,    DVB headers dont have a field priority as defined by EN300468 v1.8.1 :-(((         
        o->priority = (buf[6] >> 4) & 0x1;
        //Time_Slicing_indicator 1 bslbf
        o->time_slicing = (buf[6] >> 3) & 0x1;
        //MPE-FEC_indicator 1 bslbf
        o->mpe_fce = (buf[6] >> 2) & 0x1;
        //reserved_future_use 2 bslbf
        //t->ext_param.reserved_future_use = buf[6] & 0x3;
        //constellation 2 bslbf
        switch (buf[7] >> 6) {
                case 0: o->constellation = QPSK;   break;
                case 1: o->constellation = QAM_16; break;
                case 2: o->constellation = QAM_64; break;
                default:
                        info("undefined constellation value found.\n");
                        o->constellation = QAM_AUTO;
                }
        //hierarchy_information 3 bslbf
        switch ((buf[7] >> 3) & 0x7) {
                // what about alpha here?
                case 0: o->hierarchy_information = HIERARCHY_NONE; break; //non-hierarchical, native interleaver
                case 1: o->hierarchy_information = HIERARCHY_1;    break; //alpha = 1, native interleaver
                case 2: o->hierarchy_information = HIERARCHY_2;    break; //alpha = 2, native interleaver
                case 3: o->hierarchy_information = HIERARCHY_4;    break; //alpha = 4, native interleaver
                case 4: o->hierarchy_information = HIERARCHY_NONE; break; //non-hierarchical, in-depth interleaver
                case 5: o->hierarchy_information = HIERARCHY_1;    break; //alpha = 1, in-depth interleaver
                case 6: o->hierarchy_information = HIERARCHY_2;    break; //alpha = 2, in-depth interleaver
                case 7: o->hierarchy_information = HIERARCHY_4;    break; //alpha = 4, in-depth interleaver
                default:
                        o->hierarchy_information = HIERARCHY_NONE;
                }        
        //code_rate-HP_stream 3 bslbf
        switch (buf[7] & 0x7) {
                case 0: o->code_rate_HP = FEC_1_2; break;
                case 1: o->code_rate_HP = FEC_2_3; break;
                case 2: o->code_rate_HP = FEC_3_4; break;
                case 3: o->code_rate_HP = FEC_5_6; break;
                case 4: o->code_rate_HP = FEC_7_8; break;
                default:
                        info("undefined coderate HP\n");
                        o->code_rate_HP = FEC_AUTO;
                }
        //code_rate-LP_stream 3 bslbf
        switch ((buf[8] >> 5) & 0x7) {
                case 0: o->code_rate_LP = FEC_1_2; break;
                case 1: o->code_rate_LP = FEC_2_3; break;
                case 2: o->code_rate_LP = FEC_3_4; break;
                case 3: o->code_rate_LP = FEC_5_6; break;
                case 4: o->code_rate_LP = FEC_7_8; break;
                default:
                        info("undefined coderate LP\n");
                        o->code_rate_LP = FEC_AUTO;
                }
        if (o->hierarchy_information == HIERARCHY_NONE)
                o->code_rate_LP = FEC_NONE;
        //guard_interval 2 bslbf
        switch ((buf[8] >> 3) & 0x3) {
                case 0: o->guard_interval = GUARD_INTERVAL_1_32; break;
                case 1: o->guard_interval = GUARD_INTERVAL_1_16; break;
                case 2: o->guard_interval = GUARD_INTERVAL_1_8;  break;
                case 3: o->guard_interval = GUARD_INTERVAL_1_4;  break;
                default:;
                }
        //transmission_mode 2 bslbf
        switch ((buf[8] >> 1) & 0x3) {
                case 0: o->transmission_mode = TRANSMISSION_MODE_2K; break;
                case 1: o->transmission_mode = TRANSMISSION_MODE_8K; break;
                #ifdef TRANSMISSION_MODE_4K
                // every time the same: not defined in linuxtv dvb api :-(
                case 2: o->transmission_mode = TRANSMISSION_MODE_4K; break;
                #endif
                default:
                        info("undefined transmission mode\n");
                        o->transmission_mode = TRANSMISSION_MODE_AUTO;
                }
        //other_frequency_flag 1 bslbf
        t->other_frequency_flag = (buf[8] & 0x01);
        //reserved_future_use 32 bslbf
}

void parse_frequency_list_descriptor (const unsigned char *buf, struct transponder *t) {
        int n, i;
        typeof(*t->other_f) f;

        if (t == NULL)
                return;
        hd(buf);
        if (t->other_f)
                return;

        n = (buf[1] - 1) / 4;
        if (n < 1 || (buf[2] & 0x03) != 3)
                return;

        t->other_f = calloc(n, sizeof(*t->other_f));
        t->n_other_f = n;
        buf += 3;
        for (i = 0; i < n; i++) {
                f = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
                t->other_f[i] = f * 10;
                buf += 4;
        }
}

/* ATSC PSIP VCT */
void parse_atsc_service_location_descriptor(struct service *s,const unsigned char *buf) {
        struct ATSC_service_location_descriptor d = read_ATSC_service_location_descriptor(buf);
        int i;
        unsigned char *b = (unsigned char *) buf+5;

        s->pcr_pid = d.PCR_PID;
        for (i=0; i < d.number_elements; i++) {
                struct ATSC_service_location_element e = read_ATSC_service_location_element(b);
                switch (e.stream_type) {
                        case iso_iec_13818_1_11172_2_video_stream:
                                s->video_pid = e.elementary_PID;
                                moreverbose("  VIDEO     : PID 0x%04x\n", e.elementary_PID);
                                break;
                        case atsc_a_52b_ac3:
                                if (s->audio_num < AUDIO_CHAN_MAX) {
                                        s->audio_pid[s->audio_num] = e.elementary_PID;
                                        s->audio_lang[s->audio_num][0] = (e.ISO_639_language_code >> 16) & 0xff;
                                        s->audio_lang[s->audio_num][1] = (e.ISO_639_language_code >> 8)  & 0xff;
                                        s->audio_lang[s->audio_num][2] =  e.ISO_639_language_code        & 0xff;
                                        s->audio_num++;
                                }
                                moreverbose("\tAUDIO\t: PID 0x%04x lang: %s\n",e.elementary_PID,s->audio_lang[s->audio_num-1]);

                                break;
                        default:
                                warning("unhandled stream_type: %X\n",e.stream_type);
                                break;
                };
                b += 6;
        }
}

void parse_atsc_extended_channel_name_descriptor(struct service *s, const unsigned char *buf) {
        unsigned char *b = (unsigned char *) buf+2;
        int i,j;
        int num_str = b[0];

        #define uncompressed_string 0x00

        b++;
        for (i = 0; i < num_str; i++) {
                int num_seg = b[3];
                b += 4; /* skip lang code */
                for (j = 0; j < num_seg; j++) {
                        int compression_type = b[0],/* mode = b[1],*/ num_bytes = b[2];

                        switch (compression_type) {
                                case uncompressed_string:
                                        if (s->service_name)
                                                free(s->service_name);
                                        s->service_name = malloc(num_bytes * sizeof(char) + 1);
                                        memcpy(s->service_name,&b[3],num_bytes);
                                        s->service_name[num_bytes] = '\0';
                                        break;
                                default:
                                        warning("compressed strings are not supported yet\n");
                                        break;
                        }
                        b += 3 + num_bytes;
                }
        }
}
