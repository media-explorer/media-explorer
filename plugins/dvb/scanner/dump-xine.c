/*
 * Simple MPEG/DVB parser to achieve network/service information without initial tuning data
 *
 * Copyright (C) 2006, 2007, 2008, 2009, 2011 Winfried Koehler 
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

/* 20110702 --wk */

#include <stdio.h>
#include <stdlib.h>
#include "extended_frontend.h"
#include "scan.h"
#include "dump-xine.h"

const char * xine_inversion_name(int inversion) {
        switch(inversion) {
                case INVERSION_OFF: return "INVERSION_OFF";
                case INVERSION_ON:  return "INVERSION_ON";
                default:            return "INVERSION_AUTO";
                }
}

const char * xine_fec_name(int fec) {
        switch(fec) {
                case FEC_NONE:  return "FEC_NONE";
                case FEC_1_2:   return "FEC_1_2";
                case FEC_2_3:   return "FEC_2_3";
                case FEC_3_4:   return "FEC_3_4";
                case FEC_4_5:   return "FEC_4_5";
                case FEC_5_6:   return "FEC_5_6";
                case FEC_6_7:   return "FEC_6_7";
                case FEC_7_8:   return "FEC_7_8";
                case FEC_8_9:   return "FEC_8_9";
                case FEC_3_5:   return "FEC_3_5";
                case FEC_9_10:  return "FEC_9_10";
                default:        return "FEC_AUTO";
                }
}

const char * xine_modulation_name(int modulation) {
        switch(modulation) {
                case QPSK       : return "QPSK";
                case QAM_16     : return "QAM_16";
                case QAM_32     : return "QAM_32";
                case QAM_64     : return "QAM_64";
                case QAM_128    : return "QAM_128";
                case QAM_256    : return "QAM_256";
                case QAM_AUTO   : return "QAM_AUTO";
                case VSB_8      : return "8VSB";
                case VSB_16     : return "16VSB";
                case PSK_8      : return "PSK_8";
                case APSK_16    : return "APSK_16";
                case APSK_32    : return "APSK_32";
                case DQPSK      : return "DQPSK";
                default         : return "QAM_AUTO";
                }
}

const char * xine_bandwidth_name (int bandwidth) {
        switch(bandwidth) {                  
                case BANDWIDTH_8_MHZ : return "BANDWIDTH_8_MHZ";
                case BANDWIDTH_7_MHZ : return "BANDWIDTH_7_MHZ";
                case BANDWIDTH_6_MHZ : return "BANDWIDTH_6_MHZ";
                #ifdef BANDWIDTH_5_MHZ
                // not defined in Linux DVB API
                case BANDWIDTH_5_MHZ : return "BANDWIDTH_5_MHZ";
                #endif
                default              : return "BANDWIDTH_AUTO";
                }                         
}
                                       
const char * xine_transmission_mode_name (int transmission_mode) {
        switch(transmission_mode) {                  
                case TRANSMISSION_MODE_2K : return "TRANSMISSION_MODE_2K";
                case TRANSMISSION_MODE_8K : return "TRANSMISSION_MODE_8K";
                #ifdef TRANSMISSION_MODE_4K
                // not defined in Linux DVB API
                case TRANSMISSION_MODE_4K : return "TRANSMISSION_MODE_4K";
                #endif
                default                   : return "TRANSMISSION_MODE_AUTO";
                }                         
}  

const char * xine_guard_name (int guard_interval) {
        switch(guard_interval) {                  
                case GUARD_INTERVAL_1_32 : return "GUARD_INTERVAL_1_32";
                case GUARD_INTERVAL_1_16 : return "GUARD_INTERVAL_1_16";
                case GUARD_INTERVAL_1_8  : return "GUARD_INTERVAL_1_8";
                case GUARD_INTERVAL_1_4  : return "GUARD_INTERVAL_1_4";
                default                  : return "GUARD_INTERVAL_AUTO";
                }                         
}  

const char * xine_hierarchy_name (int hierarchy) {
        switch(hierarchy) {                  
                case HIERARCHY_NONE     : return "HIERARCHY_NONE";
                case HIERARCHY_1        : return "HIERARCHY_1";
                case HIERARCHY_2        : return "HIERARCHY_2";
                case HIERARCHY_4        : return "HIERARCHY_4";
                default                 : return "HIERARCHY_AUTO";
                }                         
}


void xine_dump_dvb_parameters (FILE *f, struct extended_dvb_frontend_parameters *p, struct w_scan_flags * flags)
{
        switch (flags->fe_type) {
        case FE_ATSC:
                fprintf (f, "%i:", p->frequency);
                fprintf (f, "%s",  xine_modulation_name(p->u.vsb.modulation));
                break;
        case FE_QAM:
                fprintf (f, "%i:", p->frequency);
                fprintf (f, "%s:", xine_inversion_name(p->inversion));
                fprintf (f, "%i:", p->u.qpsk.symbol_rate);
                fprintf (f, "%s:", xine_fec_name(p->u.qpsk.fec_inner));
                fprintf (f, "%s",  xine_modulation_name(p->u.qam.modulation));
                break;
        case FE_OFDM:
                fprintf (f, "%i:", p->frequency);
                fprintf (f, "%s:", xine_inversion_name(p->inversion));
                fprintf (f, "%s:", xine_bandwidth_name(p->u.ofdm.bandwidth));
                fprintf (f, "%s:", xine_fec_name(p->u.ofdm.code_rate_HP));
                fprintf (f, "%s:", xine_fec_name(p->u.ofdm.code_rate_LP));
                fprintf (f, "%s:", xine_modulation_name(p->u.ofdm.constellation));
                fprintf (f, "%s:", xine_transmission_mode_name(p->u.ofdm.transmission_mode));
                fprintf (f, "%s:", xine_guard_name(p->u.ofdm.guard_interval));
                fprintf (f, "%s",  xine_hierarchy_name(p->u.ofdm.hierarchy_information));
                break;
        case FE_QPSK:
                fprintf (f, "%i:", p->frequency / 1000);
                switch (p->u.qpsk.polarization) {
                        case POLARIZATION_HORIZONTAL:
                                fprintf (f, "h:");
                                break;
                        case POLARIZATION_VERTICAL:
                                fprintf (f, "v:");
                                break;
                        case POLARIZATION_CIRCULAR_LEFT:
                                fprintf (f, "l:");
                                break;
                        case POLARIZATION_CIRCULAR_RIGHT:
                                fprintf (f, "r:");
                                break;
                        default:
                                fatal("Unknown Polarization %d\n", p->u.qpsk.polarization);
                        }

                if (flags->rotor_position > 0)
                        fprintf (f, "%i:", flags->rotor_position);
                else
                        fprintf (f, "0:");

                fprintf (f, "%i", p->u.qpsk.symbol_rate / 1000);
                break;
        default:
                fatal("Unknown frontend type %d\n", flags->fe_type);
        };
}

void xine_dump_service_parameter_set (FILE * f, 
                                struct service * s,
                                struct transponder * t,
                                struct w_scan_flags * flags)
{
        if (s->video_pid || s->audio_pid[0]) {
                if (s->provider_name)
                        fprintf (f, "%s(%s):", s->service_name, s->provider_name);
                else
                        fprintf (f, "%s:", s->service_name);
                xine_dump_dvb_parameters (f, &t->param, flags);
                fprintf (f, ":%i:%i:%i", s->video_pid, s->audio_pid[0], s->service_id);
                /* what about AC3 audio here && multiple audio pids? see also: dump_mplayer.c/h */
                fprintf (f, "\n");
                }
}
