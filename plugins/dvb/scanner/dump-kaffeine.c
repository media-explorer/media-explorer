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
#include <string.h>
#include "extended_frontend.h"
#include "scan.h"
#include "dump-kaffeine.h"
#include "satellites.h"

int kaffeine_chnum=1;

struct cTr {
        const char * sat_name;
        const char * kaffeine_sat_name;
};

static struct cTr translations[] = {
        {  "S4E8", "Sirius-5.0E"},
        {  "S7E0", "EutelsatW3A-7.0E"},         //userdef
        {  "S9E0", "Eurobird9-9.0E"},
        { "S10E0", "EutelsatW1-10.0E"},         //userdef
        { "S13E0", "Hotbird-13.0E"},
        { "S16E0", "EutelsatW2-16E"},           //userdef
        { "S19E2", "Astra-19.2E"},
        { "S21E6", "EutelsatW6-21.6E"},         //userdef
        { "S23E5", "Astra-23.5E"},              //userdef
        { "S25E5", "Eurobird2-25.5E"},          //userdef
        { "S26EX", "Badr-26.0E"},               //userdef
        { "S28E2", "Astra-28.2E"},
        { "S28E5", "Eurobird1-28.5E"},
        { "S31E5", "Astra-31.5E"},              //userdef
        { "S32E9", "Intel802-32.9E"},           //userdef
        { "S33E0", "Eurobird3-33.0E"},          //userdef
        { "S35E9", "EutelsatW4-35.9E"},         //userdef
        { "S36E0", "Eutelsat-Sesat-36.0E"},     //userdef
        { "S38E0", "Paksat1-38.0E"},            //userdef
        { "S39E0", "Hellas2-39.0E"},            //userdef
        { "S40EX", "ExpressAM1-40.0E"},
        { "S41E9", "Turksat-42.0E"},
        { "S45E0", "Intelsat12-45.0E"},         //userdef
        { "S49E0", "Yamal202-49.0E"},           //userdef
        { "S53E0", "ExpressAM22-53.0E"},
        { "S57E0", "Bonum1-57.0E"},             //userdef
        { "S57EX", "NSS703-57.0E"},             //userdef
        { "S60EX", "Intel904-60.0E"},
        { "S62EX", "Intel902-62.0E"},           //userdef
        { "S64E2", "Intel906-64.2E"},           //userdef
        { "S68EX", "Intel710-68.0E"},           //userdef
        { "S70E5", "EutelsatW5-70.5E"},         //userdef
        { "S72EX", "Intel4-72.0E"},
        { "S75EX", "ABS1-75.0E"},
        { "S76EX", "Telstar10-76.0E"},          //userdef
        { "S78E5", "Thaicom2-78.5E"},           //userdef
        { "S80EX", "ExpressAM2-80.0E"},
        { "S83EX", "Insat2E-83.0E"},            //userdef
        { "S87E5", "Chinastar1-87.5E"},         //userdef
        { "S88EX", "ST1-88.0E"},                //userdef
        { "S90EX", "Yamal201-90.0E"},
        { "S91E5", "Measat3-91.5E"},            //userdef
        { "S93E5", "Insat3A-93.5E"},            //userdef
        { "S95E0", "NSS6-95.0E"},               //userdef
        { "S96EX", "ExpressAM33-96.0E"},        //userdef
        {"S100EX", "AsiaSat2-100.0E"},          //userdef
        {"S105EX", "AsiaSat3S_C-105.5E"},
        {"S108EX", "Telkom1-108.0E"},           //userdef
        {"S140EX", "ExpressAm3-140.0E"},        //userdef
        {"S160E0", "OptusD1-160.0E"},           //userdef
        {  "S0W8", "Thor-1.0W"},
        {  "S4W0", "Amos-4w"},
        {  "S5WX", "Atlantic-Bird-3-5.0W"},     //userdef
        {  "S7W0", "Nilesat101+102-7.0W"},
        {  "S8W0", "Telecom2-8.0W"},
        { "S11WX", "Express-3A-11.0W"},
        { "S12W5", "Atlantic-Bird-1-12.5W"},
        { "S14W0", "Express-A4-15.0W"},         //userdef
        { "S15W0", "Telstar12-15.0W"},
        { "S18WX", "Intelsat-901-18.0W"},       //userdef
        { "S22WX", "NSS-7-22.0W"},
        { "S24WX", "Intelsat-905-24.5W"},
        { "S27WX", "Intelsat-907-27.5W"},
        { "S30W0", "Hispasat-30.0W"}};
#define TR_COUNT(x) (sizeof(x)/sizeof(struct cTr))

/******************************************************************************
 * identifiers not yet assigned:
 *
 *  "Intelsat-903-34.5W""Intelsat-11-43.0W""Intelsat-3R-43.0W""Intelsat-6B-43.0W"
 *  "AMC2-85w""AMC3-87w""Galaxy25-97w""IA5-97w""BrasilSat-B2-65.0W"
 *  "PAS-43.0W""Intelsat-1R-45.0W""Amazonas-61.0W""Nahuel-1-71.8W""SBS6-74w"
 *  "Intelsat-705-50.0W""Intelsat-707-53.0W""Intelsat-805-55.5W" "Intelsat-9-58.0W"
 *  "NSS-10-37.5W"  "NSS-806-40.5W" "OptusC1-156E""Estrela-do-Sul-63.0W"
 *  "BrasilSat-B3-84.0W""Galaxy3C-95w""Galaxy27-129w""Satmex-6-113.0W"
 *  "BrasilSat-B4-70.0W""AMC6-72w""BrasilSat-B1-75.0W""AMC5-79w""AMC9-83w"
 *  "Galaxy28-89w""IA7-129wIA8-89w""Galaxy11-91w""Galaxy26-93w""IA6-93w"
 *  "AMC4-101w""AMC1-103w""Anik-F1-107.3W""Satmex-5-116.8W""Galaxy10R-123w"
 *****************************************************************************/



/******************************************************************************
 * translate short names used by w_scan into kaffeine satellite identifiers. 
 *
 *****************************************************************************/

static const char * short_name_to_kaffeine_name(const char * satname) {
unsigned int i;
for (i = 0; i < TR_COUNT(translations); i++)
   if (! strcmp(satname, translations[i].sat_name))
      return translations[i].kaffeine_sat_name;
return satname; //fallback.
}


/******************************************************************************
 * print complete kaffeine channels.dvb line from service params. 
 *
 *****************************************************************************/

void kaffeine_dump_service_parameter_set (FILE * f,
                                struct service * s,
                                struct transponder * t,
                                struct w_scan_flags * flags)
{
  int i;

  if (kaffeine_chnum == 1) {
        /* write header */
        fprintf (f, "# kaffeine channels.dvb automatically generated by w_scan\n");
        fprintf (f, "# see (http://wirbel.htpc-forum.de/w_scan/index2.html)\n");
        fprintf (f, "# TV(C)/RA(C)|name|vpid|apids|ttpid|sid|tsid|{S/C/T/A}"
                    "|freq|sr|pol|fec_H|inv|mod|fec_L|bw|tm|gi|h|num|subpids|category|nid\n");
        }
  fprintf(f, "%s|", ((s->scrambled==0) && (s->video_pid >0))?"TV":
                    ((s->scrambled==0) && (s->video_pid==0))?"RA":
                    (s->video_pid)?"TVC":"RAC");
  fprintf(f, "%s|", s->service_name);
  fprintf(f, "%i(2)|", s->video_pid);
  fprintf(f, "%i,", s->audio_pid[0]);
  for (i = 1; i < s->audio_num; i++)
        fprintf(f,"%i,",s->audio_pid[i]);
  for (i = 0; i < s->ac3_num; i++)
        fprintf (f, "%i(ac3),", s->ac3_pid[i]);
  fprintf(f,"|");
  fprintf(f, "%d|",s->teletext_pid);
  fprintf(f, "%d|",s->service_id);
  fprintf(f, "%d|",t->pids.transport_stream_id);
  switch (flags->fe_type) {
        case FE_ATSC: /* ATSC  */
                fprintf(f, "Atsc|");
                break;
        case FE_QAM:  /* DVB-C */
                fprintf(f, "Cable|");
                break;
        case FE_OFDM: /* DVB-T */
                fprintf(f, "Terrestrial|");
                break;
        case FE_QPSK: /* DVB-S(2), i.e. "SAstra-19.2E" */
                fprintf(f, "S%s|",
                short_name_to_kaffeine_name(satellite_to_short_name(flags->list_id)));
                break;
        default:;
        }
  fprintf(f, "%i|", t->param.frequency / 1000);            /* 6 digits      */
  fprintf(f, "%i|", t->param.u.qam.symbol_rate / 1000);    /* 4 or 5 digits */

  if (flags->fe_type == FE_QPSK)
        fprintf(f, "%s|",
                t->param.u.qpsk.polarization == POLARIZATION_VERTICAL?"v":"h");        
  else
        fprintf(f, "%s|", "v"); /* dummy */

  switch (flags->fe_type) {
        case FE_ATSC: /* ATSC, fall-through */
        case FE_QAM:  /* DVB-C */
                fprintf(f, "-1|"); /* FEC_AUTO -> simply ignored inside VDR */
                break;
        case FE_OFDM: /* DVB-T */
                switch (t->param.u.ofdm.code_rate_HP) {
                case FEC_NONE:fprintf(f,  "0|"); break;
                case FEC_1_2: fprintf(f, "12|"); break;
                case FEC_2_3: fprintf(f, "23|"); break;
                case FEC_3_4: fprintf(f, "34|"); break;
                case FEC_4_5: fprintf(f, "45|"); break;
                case FEC_5_6: fprintf(f, "56|"); break;
                case FEC_6_7: fprintf(f, "67|"); break;
                case FEC_7_8: fprintf(f, "78|"); break;
                case FEC_8_9: fprintf(f, "89|"); break;
                default: fprintf(f, "-1|"); /* FEC_AUTO */
                }
                break;
        case FE_QPSK: /* DVB-S(2) */
                switch (t->param.u.qpsk.fec_inner) {
                case FEC_NONE: fprintf(f,   "0|"); break;
                case FEC_1_2:  fprintf(f,  "12|"); break;
                case FEC_2_3:  fprintf(f,  "23|"); break;
                case FEC_3_4:  fprintf(f,  "34|"); break;
                case FEC_4_5:  fprintf(f,  "45|"); break;
                case FEC_5_6:  fprintf(f,  "56|"); break;
                case FEC_6_7:  fprintf(f,  "67|"); break;
                case FEC_7_8:  fprintf(f,  "78|"); break;
                case FEC_8_9:  fprintf(f,  "89|"); break;
                /* no kaffeine support yet.
                case FEC_9_10: fprintf(f, "910|"); break;
                case FEC_3_5:  fprintf(f,  "35|"); break;
                */
                default: fprintf(f, "-1|"); /* FEC_AUTO */
                }
                break;
        default:;
        }
  switch (t->param.inversion) {
        case INVERSION_OFF: fprintf(f,  "0|"); break;
        case INVERSION_ON : fprintf(f,  "1|"); break;
        default:            fprintf(f, "-1|"); /* INVERSION_AUTO */
        }
  switch (flags->fe_type) { /* modulation */
        case FE_ATSC: /* ATSC */
                switch (t->param.u.vsb.modulation) {
                        case QAM_64:  fprintf(f,  "64|"); break;
                        case QAM_128: fprintf(f, "128|"); break;
                        case QAM_256: fprintf(f, "256|"); break;
                        case VSB_8:   fprintf(f, "108|"); break;
                        case VSB_16:  fprintf(f, "116|"); break;
                        default: fprintf(f, "-1|");
                        }
                fprintf(f, "-1|"); /* FEC_LP    */
                fprintf(f, "-1|"); /* Bandwidth */
                fprintf(f, "-1|"); /* transmission mode */
                fprintf(f, "-1|"); /* guard interval */
                fprintf(f, "-1|"); /* hierarchy */
                break;
        case FE_QAM:  /* DVB-C */
                switch (t->param.u.qam.modulation) {
                        case QAM_16:  fprintf(f,  "16|"); break;
                        case QAM_32:  fprintf(f,  "32|"); break;
                        case QAM_64:  fprintf(f,  "64|"); break;
                        case QAM_128: fprintf(f, "128|"); break;
                        case QAM_256: fprintf(f, "256|"); break;
                        default: fprintf(f, "-1|"); /* QAM_AUTO */
                        }
                fprintf(f, "-1|"); /* FEC_LP    */
                fprintf(f, "-1|"); /* Bandwidth */
                fprintf(f, "-1|"); /* transmission mode */
                fprintf(f, "-1|"); /* guard interval */
                fprintf(f, "-1|"); /* hierarchy */
                break;
        case FE_OFDM: /* DVB-T */
                switch (t->param.u.ofdm.constellation) {
                        case QPSK:    fprintf(f,  "8|");  break;
                        case QAM_16:  fprintf(f, "16|" ); break;
                        case QAM_32:  fprintf(f, "32|" ); break;
                        case QAM_64:  fprintf(f, "64|" ); break;
                        default: fprintf(f, "-1|"); /* QAM_AUTO */
                        }
                if (t->param.u.ofdm.hierarchy_information != HIERARCHY_NONE)
                        switch (t->param.u.ofdm.code_rate_LP) {
                                case FEC_NONE:fprintf(f,  "0|"); break;
                                case FEC_1_2: fprintf(f, "12|"); break;
                                case FEC_2_3: fprintf(f, "23|"); break;
                                case FEC_3_4: fprintf(f, "34|"); break;
                                case FEC_4_5: fprintf(f, "45|"); break;
                                case FEC_5_6: fprintf(f, "56|"); break;
                                case FEC_6_7: fprintf(f, "67|"); break;
                                case FEC_7_8: fprintf(f, "78|"); break;
                                case FEC_8_9: fprintf(f, "89|"); break;
                                default: fprintf(f, "-1|"); /* FEC_AUTO */
                                }
                else
                        fprintf(f, "0|"); /* FEC_NONE, if no hierarchy */
                switch (t->param.u.ofdm.bandwidth) {
                        case BANDWIDTH_8_MHZ: fprintf(f, "8|"); break;
                        case BANDWIDTH_7_MHZ: fprintf(f, "7|"); break;
                        case BANDWIDTH_6_MHZ: fprintf(f, "6|"); break;
                        #ifdef BANDWIDTH_5_MHZ
                        case BANDWIDTH_5_MHZ: fprintf(f, "5|"); break;
                        #endif
                        default: fprintf(f, "-1|"); /* BANDWIDTH_AUTO */
                        }
                switch (t->param.u.ofdm.transmission_mode) {
                        case TRANSMISSION_MODE_8K: fprintf(f, "8|"); break;
                        case TRANSMISSION_MODE_2K: fprintf(f, "2|"); break;
                        #ifdef TRANSMISSION_MODE_4K
                        case TRANSMISSION_MODE_4K: fprintf(f, "4|"); break;
                        #endif
                        default: fprintf(f, "-1|"); /* TRANSMISSION_MODE_AUTO */
                        }
                switch (t->param.u.ofdm.guard_interval) {
                        case GUARD_INTERVAL_1_32: fprintf(f, "32|"); break;
                        case GUARD_INTERVAL_1_16: fprintf(f, "16|"); break;
                        case GUARD_INTERVAL_1_8:  fprintf(f,  "8|"); break;
                        case GUARD_INTERVAL_1_4:  fprintf(f,  "4|"); break;
                        default: fprintf(f, "-1|"); /* GUARD_INTERVAL_AUTO */
                        }
                switch (t->param.u.ofdm.hierarchy_information) { // alpha values???
                        case HIERARCHY_NONE: fprintf(f, "0|"); break;
                        case HIERARCHY_1:    fprintf(f, "1|"); break;
                        case HIERARCHY_2:    fprintf(f, "2|"); break;
                        case HIERARCHY_4:    fprintf(f, "4|"); break;
                        default: fprintf(f, "-1|"); /* HIERARCHY_AUTO */
                        }
                break;
        case FE_QPSK: /* DVB_S/S2 */
                fprintf(f, "-1|"); /* modulation_type. what about rolloff, modulation_system, PSK_8, ... ? */
                fprintf(f, "-1|"); /* FEC_LP    */
                fprintf(f, "-1|"); /* Bandwidth */
                fprintf(f, "-1|"); /* transmission mode */
                fprintf(f, "-1|"); /* guard interval */
                fprintf(f, "-1|"); /* hierarchy */
                break;
        default: break;
        }
  fprintf(f, "%d|",kaffeine_chnum++);                   /* channel number, ascending                    */
  fprintf(f, "%s|","");                                 /* list of subpids? Dont know up to now..       */
  fprintf(f, "%s|","");                                 /* category? Dont know up to now..              */
  fprintf(f, "%u|\n",t->pids.original_network_id);      /* is *onid* or *nid* here needed?              */
}

