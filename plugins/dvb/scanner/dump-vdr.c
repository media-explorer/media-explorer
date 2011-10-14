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
#include <string.h>
#include "scan.h"
#include "extended_frontend.h"
#include "dump-vdr.h"
#include "satellites.h"

struct cTr {
        const char * sat_name;
        const char * vdr_sat_name;
};

static struct cTr translations[] = {
        {  "S4E8",    "S5E"},
        {  "S7E0",    "S7E"},
        {  "S9E0",    "S9E"},
        { "S10E0",   "S10E"},
        { "S13E0",   "S13E"},
        { "S16E0",   "S16E"},
        { "S19E2", "S19.2E"},
        { "S21E6", "S21.6E"},
        { "S23E5", "S23.5E"},
        { "S25E5", "S25.5E"},
        { "S26EX",   "S26E"},
        { "S28E2", "S28.2E"},
        { "S28E5", "S28.5E"},
        { "S31E5", "S31.5E"},
        { "S32E9",   "S33E"},
        { "S33E0",   "S33E"},
        { "S35E9",   "S36E"},
        { "S36E0",   "S36E"},
        { "S38E0",   "S38E"},
        { "S39E0",   "S39E"},
        { "S40EX",   "S40E"},
        { "S42E0",   "S42E"},
        { "S45E0",   "S45E"},
        { "S49E0",   "S49E"},
        { "S53E0",   "S53E"},
        { "S57E0",   "S56E"},
        { "S57EX",   "S57E"},
        { "S60EX",   "S60E"},
        { "S62EX",   "S62E"},
        { "S64E2",   "S64E"},
        { "S68EX", "S68.5E"},
        { "S70E5", "S70.5E"},
        { "S72EX",   "S72E"},
        { "S75EX",   "S75E"},
        { "S76EX", "S76.5E"},
        { "S78E5", "S78.5E"},
        { "S80EX",   "S80E"},
        { "S83EX",   "S83E"},
        { "S87E5", "S87.5E"},
        { "S88EX",   "S88E"},
        { "S90EX",   "S90E"},
        { "S91E5", "S91.5E"},
        { "S93E5", "S93.5E"},
        { "S95E0",   "S95E"},
        { "S96EX", "S96.5E"},
        {"S100EX","S100.5E"},
        {"S105EX","S105.5E"},
        {"S108EX",  "S108E"},
        {"S140EX",  "S140E"},
        {"S160E0",  "S160E"},
        {  "S0W8",    "S1W"},
        {  "S4W0",    "S4W"},
        {  "S5WX",    "S5W"},
        {  "S7W0",    "S7W"},
        {  "S8W0",    "S8W"},
        { "S11WX",   "S11W"},
        { "S12W5", "S12.5W"},
        { "S14W0",   "S14W"},
        { "S15W0",   "S15W"},
        { "S18WX",   "S18W"},
        { "S22WX",   "S22W"},
        { "S24WX", "S24.5W"},
        { "S27WX", "S27.5W"},
        { "S30W0",   "S30W"}};
#define TR_COUNT(x) (sizeof(x)/sizeof(struct cTr))

/******************************************************************************
 * translate short names used by w_scan into VDR satellite identifiers. 
 *
 *****************************************************************************/

static const char * short_name_to_vdr_name(const char * satname) {
unsigned int i;
for (i = 0; i < TR_COUNT(translations); i++)
   if (! strcmp(satname, translations[i].sat_name))
      return translations[i].vdr_sat_name;
return satname; //fallback.
}

/******************************************************************************
 * translate VDR satellite identifiers into short names used by w_scan. 
 *
 *****************************************************************************/

const char * vdr_name_to_short_name(const char * satname) {
unsigned int i;
for (i = 0; i < TR_COUNT(translations); i++)
   if (! strcmp(satname, translations[i].vdr_sat_name))
      return translations[i].sat_name;
return "unknown satellite"; //fallback.
}

/******************************************************************************
 * translate linuxtv inversions to VDR inversion identifiers. 
 *
 *****************************************************************************/

const char * vdr_inversion_name(int inversion) {
        switch(inversion) {
                case INVERSION_OFF: return "0";
                case INVERSION_ON:  return "1";
                default:            return "999";
                }
}

/******************************************************************************
 * translate linuxtv forw err corr to VDR fec identifiers. 
 *
 *****************************************************************************/

const char * vdr_fec_name(int fec) {
        switch(fec) {
                case FEC_NONE:  return "0";
                case FEC_1_2:   return "12";
                case FEC_2_3:   return "23";
                case FEC_3_4:   return "34";
                case FEC_4_5:   return "45";
                case FEC_5_6:   return "56";
                case FEC_6_7:   return "67";
                case FEC_7_8:   return "78";
                case FEC_8_9:   return "89";
                case FEC_3_5:   return "35";
                case FEC_9_10:  return "910";
                default:        return "999";
                }
}

/******************************************************************************
 * translate linuxtv modulation types to VDR > 1.7.0 identifiers. 
 *
 *****************************************************************************/

const char * vdr17_modulation_name(int modulation) {
        switch(modulation) {        
                case QAM_16     : return "16"; 
                case QAM_32     : return "32"; 
                case QAM_64     : return "64"; 
                case QAM_128    : return "128"; 
                case QAM_256    : return "256";
                #ifdef QAM_512
                // not defined in Linux DVB API
                case QAM_512    : return "512";
                #endif
                #ifdef QAM_1024
                // not defined in Linux DVB API
                case QAM_1024   : return "1024";
                #endif
                #ifdef QAM_2048
                // not defined in Linux DVB API
                case QAM_2048   : return "2048";
                #endif
                #ifdef QAM_4096
                // not defined in Linux DVB API
                case QAM_4096   : return "4096";        
                #endif 
                case QAM_AUTO   : return "998";
                case QPSK       : return "2"; 
                case PSK_8      : return "5"; 
                case APSK_16    : return "6";
                case APSK_32    : return "7";
                case VSB_8      : return "10";
                case VSB_16     : return "11";
                default         : return "999";
                }
}

/******************************************************************************
 * translate linuxtv modulation types to VDR < 1.7.0 identifiers. 
 *
 *****************************************************************************/

const char * vdr_modulation_name(int modulation) {
        switch(modulation) {        
                case QPSK       : return "0";
                case QAM_16     : return "16";
                case QAM_32     : return "32";
                case QAM_64     : return "64";
                case QAM_128    : return "128";
                case QAM_256    : return "256";        
                case QAM_AUTO   : return "999";
                default         : return "999";
                }
}

/******************************************************************************
 * translate linuxtv bandwidth values to VDR identifiers. 
 *
 *****************************************************************************/

const char * vdr_bandwidth_name (int bandwidth) {
        switch(bandwidth) {                  
                case BANDWIDTH_8_MHZ : return "8";
                case BANDWIDTH_7_MHZ : return "7";
                case BANDWIDTH_6_MHZ : return "6";
                #ifdef BANDWIDTH_5_MHZ
                // not defined in Linux DVB API
                case BANDWIDTH_5_MHZ : return "5";
                #endif
                default              : return "999";
                }                         
}
                                       
/******************************************************************************
 * translate linuxtv tm values to VDR identifiers. 
 *
 *****************************************************************************/

const char * vdr_transmission_mode_name (int transmission_mode) {
        switch(transmission_mode) {                  
                case TRANSMISSION_MODE_2K : return "2";
                case TRANSMISSION_MODE_8K : return "8";
                #ifdef TRANSMISSION_MODE_4K
                // not defined in Linux DVB API
                case TRANSMISSION_MODE_4K : return "4";
                #endif
                default                   : return "999";
                }                         
}  

/******************************************************************************
 * translate linuxtv guard values to VDR identifiers. 
 *
 *****************************************************************************/

const char * vdr_guard_name (int guard_interval) {
        switch(guard_interval) {                  
                case GUARD_INTERVAL_1_32 : return "32";
                case GUARD_INTERVAL_1_16 : return "16";
                case GUARD_INTERVAL_1_8  : return "8";
                case GUARD_INTERVAL_1_4  : return "4";
                default                  : return "999";
                }                         
}  

/******************************************************************************
 * translate linuxtv hierarchy values to VDR identifiers.
 * what about alpha? hm..
 *****************************************************************************/

const char * vdr_hierarchy_name (int hierarchy) {
        switch(hierarchy) {                  
                case HIERARCHY_NONE     : return "0";
                case HIERARCHY_1        : return "1";
                case HIERARCHY_2        : return "2";
                case HIERARCHY_4        : return "4";
                default                 : return "999";
                }                         
}

/******************************************************************************
 * translate linuxtv rolloffs values to VDR identifiers. 
 *
 *****************************************************************************/

const char * vdr_rolloff_name(int rolloff) {
        switch(rolloff) {
                case ROLLOFF_20:  return "20";
                case ROLLOFF_25:  return "25";
                default:          return "35";
                }
}

/******************************************************************************
 * print "frequency:<params>:symbolrate:" to 'f' in vdr < 1.7.0 format 
 *
 *****************************************************************************/

void dump_param_vdr_1_2(FILE * f, struct extended_dvb_frontend_parameters * p, struct w_scan_flags * flags) {

switch (flags->fe_type) {
        case FE_ATSC:
                fatal("VDR 1.2 .. 1.6 doesn't support ATSC.\n"
                        "hint: did you miss '-o 7' ?");

        case FE_QAM:
                fprintf (f, ":%i:",   p->frequency / 1000);
                fprintf (f, "M%s:C:", vdr_modulation_name(p->u.qam.modulation));
                fprintf (f, "%i:",    p->u.qam.symbol_rate / 1000);
                break;

        case FE_OFDM:
                fprintf (f, ":%i:", p->frequency / 1000);
                fprintf (f, "I%s", vdr_inversion_name(p->inversion));
                fprintf (f, "B%s", vdr_bandwidth_name(p->u.ofdm.bandwidth));
                fprintf (f, "C%s", vdr_fec_name(p->u.ofdm.code_rate_HP));
                fprintf (f, "D%s", vdr_fec_name(p->u.ofdm.code_rate_LP));
                fprintf (f, "M%s", vdr_modulation_name(p->u.ofdm.constellation));
                fprintf (f, "T%s", vdr_transmission_mode_name(p->u.ofdm.transmission_mode));
                fprintf (f, "G%s", vdr_guard_name(p->u.ofdm.guard_interval));
                fprintf (f, "Y%s", vdr_hierarchy_name(p->u.ofdm.hierarchy_information));
                fprintf (f, ":T:27500:");
                break;

        case FE_QPSK:
                fprintf (f, ":%i:",   p->frequency / 1000);
                switch (p->u.qpsk.polarization) {
                        case POLARIZATION_HORIZONTAL:
                                fprintf (f, "h");
                                break;                        
                        case POLARIZATION_VERTICAL:
                                fprintf (f, "v");
                                break;
                        case POLARIZATION_CIRCULAR_LEFT:
                                fprintf (f, "l");
                                break;
                        case POLARIZATION_CIRCULAR_RIGHT:
                                fprintf (f, "r");
                                break;
                        default:
                                fatal("Unknown Polarization %d\n", p->u.qpsk.polarization);
                        }
                fprintf (f, "C%s:", vdr_fec_name(p->u.qpsk.fec_inner));
                fprintf(f, "%s:",
                        short_name_to_vdr_name(satellite_to_short_name(flags->list_id)));
                fprintf (f, "%i:",    p->u.qpsk.symbol_rate / 1000);                
                break;
        default:;
        };
}

/******************************************************************************
 * print "frequency:<params>:symbolrate:" to 'f' in vdr >= 1.7.4 format 
 * NOTE: 1.7.0 .. 1.7.3 not supported here.
 *****************************************************************************/

void dump_param_vdr_1_7(FILE * f, struct extended_dvb_frontend_parameters * p, struct w_scan_flags * flags) {

switch (flags->fe_type) {
        case FE_ATSC:
                fprintf (f, ":%i:",   p->frequency / 1000);                
                fprintf (f, "M%s:A:", vdr17_modulation_name(p->u.vsb.modulation));
                fprintf (f, "%i:",    p->u.qam.symbol_rate / 1000);
                break;

        case FE_QAM:
                fprintf (f, ":%i:",   p->frequency / 1000);
                fprintf (f, "M%s:C:", vdr17_modulation_name(p->u.qam.modulation));
                fprintf (f, "%i:",    p->u.qam.symbol_rate / 1000);
                break;

        case FE_OFDM:
                fprintf (f, ":%i:", p->frequency / 1000);
                fprintf (f, "I%s", vdr_inversion_name(p->inversion));
                fprintf (f, "B%s", vdr_bandwidth_name(p->u.ofdm.bandwidth));
                fprintf (f, "C%s", vdr_fec_name(p->u.ofdm.code_rate_HP));
                fprintf (f, "D%s", vdr_fec_name(p->u.ofdm.code_rate_LP));
                fprintf (f, "M%s", vdr17_modulation_name(p->u.ofdm.constellation));
                fprintf (f, "T%s", vdr_transmission_mode_name(p->u.ofdm.transmission_mode));
                fprintf (f, "G%s", vdr_guard_name(p->u.ofdm.guard_interval));
                fprintf (f, "Y%s", vdr_hierarchy_name(p->u.ofdm.hierarchy_information));
                fprintf (f, ":T:27500:");
                break;

        case FE_QPSK:
                fprintf (f, ":%i:",   p->frequency / 1000);
                switch (p->u.qpsk.polarization) {
                        case POLARIZATION_HORIZONTAL:
                                fprintf (f, "h");
                                break;
                        case POLARIZATION_VERTICAL:
                                fprintf (f, "v");
                                break;
                        case POLARIZATION_CIRCULAR_LEFT:
                                fprintf (f, "l");
                                break;
                        case POLARIZATION_CIRCULAR_RIGHT:
                                fprintf (f, "r");
                                break;
                        default:
                                fatal("Unknown Polarization %d\n", p->u.qpsk.polarization);
                        }
                fprintf (f, "C%s", vdr_fec_name(p->u.qpsk.fec_inner));
                switch (p->u.qpsk.modulation_system) {
                        case SYS_DVBS2:
                                fprintf (f, "M%sO%sS1:",
                                        vdr17_modulation_name(p->u.qpsk.modulation_type),
                                        vdr_rolloff_name(p->u.qpsk.rolloff));
                                break;
                        default:
                                /* DVB-S always r = 0.35 according to specs
                                 * but vdr specifies 'O0' here (should be O35),
                                 * modulation is fix QPSK = 'M2'.
                                 */
                                fprintf (f, "M2O0S0:");
                        }

                fprintf(f, "%s:",
                        short_name_to_vdr_name(satellite_to_short_name(flags->list_id)));
                fprintf (f, "%i:",    p->u.qpsk.symbol_rate / 1000);                
                break;

        default:;
        };
}

/******************************************************************************
 * print complete vdr channels.conf line from service params. 
 *
 *****************************************************************************/

void vdr_dump_service_parameter_set (FILE * f,
                                struct service * s,
                                struct transponder * t,
                                struct w_scan_flags * flags) {
        int i;

        if (! flags->ca_select && s->scrambled)
                return;
        if ((flags->vdr_version < 7) &&
            (flags->fe_type == QPSK) &&
            (t->param.u.qpsk.modulation_system == SYS_DVBS2))
                /* delivery system not supported by vdr version */
                return;

        fprintf (f, "%s", s->service_name);

        if (flags->dump_provider)
                fprintf (f, ";%s", s->provider_name);
        
        switch(flags->vdr_version) { // freq:<params>:source:sr:
                case 2 ... 6:
                        dump_param_vdr_1_2(f, &t->param, flags);
                        break;
                case 7:
                default:
                        dump_param_vdr_1_7(f, &t->param, flags);
                }
                
        fprintf (f, "%i", s->video_pid);

        if (s->video_pid && (s->pcr_pid != s->video_pid))
                fprintf (f, "+%i", s->pcr_pid);

        fprintf (f, ":");

        switch(flags->vdr_version) {
                case 2:
                        fprintf (f, "%i", s->audio_pid[0]);
                        for (i = 1; i < s->audio_num; i++) {
                                if (i) fprintf (f, ",");                                        
                                fprintf (f, ",%i", s->audio_pid[i]);
                                }
                        break;
                default:
                        fprintf (f, "%i", s->audio_pid[0]);
                        if (s->audio_lang && s->audio_lang[0][0])
                                fprintf (f, "=%.4s", s->audio_lang[0]);
                        for (i = 1; i < s->audio_num; i++) {                        
                                fprintf (f, ",%i", s->audio_pid[i]);
                                if (s->audio_lang && s->audio_lang[i][0])
                                        fprintf (f, "=%.4s", s->audio_lang[i]);
                                }
                }

        if (s->ac3_num) {
                fprintf (f, "%s", ";");
                for (i = 0; i < s->ac3_num;) {
                        fprintf (f, "%i", s->ac3_pid[i++]);
                        if (i < s->ac3_num)
                                fprintf (f, "%s", ",");
                        }
                }

        fprintf (f, ":%d:", s->teletext_pid);

        fprintf (f, "%x", s->ca_id[0]);
        for (i = 1; i < s->ca_num; i++) {
                if (s->ca_id[i] == 0) continue;
                fprintf (f, ",%x", s->ca_id[i]);
                }

        if (flags->vdr_version < 3) {
                t->pids.original_network_id = 0;
                t->pids.transport_stream_id = 0;
                }

        fprintf (f, ":%d:%d:%d:0",
                s->service_id,
                (t->pids.transport_stream_id > 0)?t->pids.original_network_id:0,
                t->pids.transport_stream_id);

        if (flags->print_pmt) {
                fprintf (f, ":%d", s->pmt_pid);
                }

        fprintf (f, "%s", "\n");
}
