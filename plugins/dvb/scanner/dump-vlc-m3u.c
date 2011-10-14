/*
 * Simple MPEG/DVB parser to achieve network/service information without initial tuning data
 *
 * Copyright (C) 2006, 2007, 2008, 2010, 2011 Winfried Koehler
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
 * The project's page is http://wirbel.htpc-forum.de/w_scan/idx2.html
 */

/* 20110702 --wk */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extended_frontend.h"
#include "scan.h"
#include "dump-vlc-m3u.h"
#include "lnb.h"


struct cChrReplacement {
        char chr;
        const char * newStr;
        uint8_t len;
};


// http://de.selfhtml.org/html/referenz/zeichen.htm
// http://www.gemeinde-michendorf.de/homepage/8sonstiges/entity.php
struct cChrReplacement chrs[] = {
        {'"', "&#34;" , 5},
        {'&', "&#38;" , 5},
        {'<', "&#60;" , 5},
        {'>', "&#62;" , 5},
        {'¡', "&#161;", 6},
        {'¢', "&#162;", 6}, 
        {'£', "&#163;", 6}, 
        {'¤', "&#164;", 6},
        {'¥', "&#165;", 6},
        {'¦', "&#166;", 6},
        {'§', "&#167;", 6},
        {'¨', "&#168;", 6},
        {'©', "&#169;", 6},
        {'ª', "&#170;", 6},
        {'«', "&#171;", 6},
        {'¬', "&#172;", 6},
        {'­', "&#173;", 6},
        {'®', "&#174;", 6},
        {'¯', "&#175;", 6},
        {'°', "&#176;", 6},
        {'±', "&#177;", 6},
        {'²', "&#178;", 6},
        {'³', "&#179;", 6},
        {'´', "&#180;", 6},
        {'µ', "&#181;", 6},
        {'¶', "&#182;", 6},
        {'·', "&#183;", 6},
        {'¸', "&#184;", 6},
        {'¹', "&#185;", 6},
        {'º', "&#186;", 6},
        {'»', "&#187;", 6},
        {'¼', "&#188;", 6},
        {'½', "&#189;", 6},
        {'¾', "&#190;", 6},
        {'¿', "&#191;", 6},
        {'À', "&#192;", 6},
        {'Á', "&#193;", 6},
        {'Â', "&#194;", 6},
        {'Ã', "&#195;", 6},
        {'Ä', "&#196;", 6},
        {'Å', "&#197;", 6},
        {'Æ', "&#198;", 6},
        {'Ç', "&#199;", 6},
        {'È', "&#200;", 6},
        {'É', "&#201;", 6},
        {'Ê', "&#202;", 6},
        {'Ë', "&#203;", 6},
        {'Ì', "&#204;", 6},
        {'Í', "&#205;", 6},
        {'Î', "&#206;", 6},
        {'Ï', "&#207;", 6},
        {'Ð', "&#208;", 6},
        {'Ñ', "&#209;", 6},
        {'Ò', "&#210;", 6},
        {'Ó', "&#211;", 6},
        {'Ô', "&#212;", 6},
        {'Õ', "&#213;", 6},
        {'Ö', "&#214;", 6},
        {'×', "&#215;", 6},
        {'Ø', "&#216;", 6},
        {'Ù', "&#217;", 6},
        {'Ú', "&#218;", 6},
        {'Û', "&#219;", 6},
        {'Ü', "&#220;", 6},
        {'Ý', "&#221;", 6},
        {'Þ', "&#222;", 6},
        {'ß', "&#223;", 6},
        {'à', "&#224;", 6},
        {'á', "&#225;", 6},
        {'â', "&#226;", 6},
        {'ã', "&#227;", 6},
        {'ä', "&#228;", 6},
        {'å', "&#229;", 6},
        {'æ', "&#230;", 6},
        {'ç', "&#231;", 6},
        {'è', "&#232;", 6},
        {'é', "&#233;", 6},
        {'ê', "&#234;", 6},
        {'ë', "&#235;", 6},
        {'ì', "&#236;", 6},
        {'í', "&#237;", 6},
        {'î', "&#238;", 6},
        {'ï', "&#239;", 6},
        {'ð', "&#240;", 6},
        {'ñ', "&#241;", 6},
        {'ò', "&#242;", 6},
        {'ó', "&#243;", 6},
        {'ô', "&#244;", 6},
        {'õ', "&#245;", 6},
        {'ö', "&#246;", 6},
        {'÷', "&#247;", 6},
        {'ø', "&#248;", 6},
        {'ù', "&#249;", 6},
        {'ú', "&#250;", 6},
        {'û', "&#251;", 6},
        {'ü', "&#252;", 6},
        {'ý', "&#253;", 6},
        {'þ', "&#254;", 6},
        {'ÿ', "&#255;", 6},
        {'ß', "&#946;", 6},
        {'µ', "&#956;", 6}, 
        {'‘', "&#8216;",7},
        {'/', "&#8260;",7},
};

#define R_COUNT(x) (sizeof(x)/sizeof(struct cChrReplacement))



static int idx = 1;

#define T1 "\t"        
#define T2 "\t\t"
#define T3 "\t\t\t"
#define T4 "\t\t\t\t"
#define T5 "\t\t\t\t\t"
#define fprintf_tab0(v) fprintf(f, "%s", v)
#define fprintf_tab1(v) fprintf(f, "%s%s", T1, v)
#define fprintf_tab2(v) fprintf(f, "%s%s", T2, v)
#define fprintf_tab3(v) fprintf(f, "%s%s", T3, v)
#define fprintf_tab4(v) fprintf(f, "%s%s", T4, v)
#define fprintf_tab5(v) fprintf(f, "%s%s", T5, v)
#define fprintf_pair(p,v) fprintf(f, "%s=%c%s%c", p,34,v,34)

int vlc_inversion(int inversion) {
        switch(inversion) {
                case INVERSION_OFF:             return 0;
                case INVERSION_ON:              return 1;
                default:                        return 2;
                }
}

int vlc_fec(int fec) {
        switch(fec) {
                case FEC_NONE:                  return 0;
                case FEC_1_2:                   return 1;
                case FEC_2_3:                   return 2;
                case FEC_3_4:                   return 3;
                case FEC_4_5:                   return 4;
                case FEC_5_6:                   return 5;
                case FEC_6_7:                   return 6;
                case FEC_7_8:                   return 7;
                case FEC_8_9:                   return 8;
/* not known to vlc-1.1.4. I think this is a bug. What about *DVB-S2* and VLC?
   Even buggier. Where is rolloff, and modulation system to be found in VLC ? :-(((
                case FEC_3_5:                   return ????;
                case FEC_9_10:                  return ????;
*/
                default:                        return 9;
                }
}

int vlc_modulation_OFDM(int modulation) {
        switch(modulation) {
                case QPSK:                      return -1;
                case QAM_AUTO:                  return 0;
                case QAM_16:                    return 16;
                case QAM_32:                    return 32;
                case QAM_64:                    return 64;
                case QAM_128:                   return 128;
                case QAM_256:                   return 256;
                default:                        return 0;
    }
}

int vlc_modulation_ATSC(int modulation) {
        switch(modulation) {
                case QAM_AUTO:                  return 0;
                case QAM_16:                    return 16;
                case QAM_32:                    return 32;
                case QAM_64:                    return 64;
                case QAM_128:                   return 128;
                case QAM_256:                   return 256;
                case VSB_8:                     return 8;
                case VSB_16:                    return 16;
                default:                        return 0;
    }
}

int vlc_modulation_QAM(int modulation) {
        switch(modulation) {
                case QAM_16:                    return 16;
                case QAM_32:                    return 32;
                case QAM_64:                    return 64;
                case QAM_128:                   return 128;
                case QAM_256:                   return 256;
                default:                        return 0;
                }
}

int vlc_modulation_QPSK(int modulation) {
        switch(modulation) {
                case QPSK:                      return -1;
                case QAM_16:                    return 16;
/* the following are not known to VLC-1.1.4:
                case PSK_8:                     return ????; 
                case APSK_16:                   return ????;
                case APSK_32:                   return ????;
*/
                default:                        return 0;
                }
}

int vlc_bandwidth (int bandwidth) {
        switch(bandwidth) {                  
                case BANDWIDTH_8_MHZ:           return 8;
                case BANDWIDTH_7_MHZ:           return 7;
                case BANDWIDTH_6_MHZ:           return 6;
                #ifdef BANDWIDTH_5_MHZ
                // not defined in Linux DVB API
                case BANDWIDTH_5_MHZ:           return 5;
                #endif
                default:                        return 0;
                }                         
}
                                       
int vlc_transmission (int transmission_mode) {
        switch(transmission_mode) {         
                case TRANSMISSION_MODE_2K:      return 2;
                case TRANSMISSION_MODE_8K:      return 8;
                #ifdef TRANSMISSION_MODE_4K
                // not defined in Linux DVB API
                case TRANSMISSION_MODE_4K:      return 4;
                #endif
                default:                        return 0;
                }                         
}  

int vlc_guard (int guard_interval) {
        switch(guard_interval) {          
                case GUARD_INTERVAL_1_32:       return 32;
                case GUARD_INTERVAL_1_16:       return 16;
                case GUARD_INTERVAL_1_8:        return 8;
                case GUARD_INTERVAL_1_4:        return 4;
                default:                        return 0;
                }                        
}  

int vlc_hierarchy (int hierarchy) {
        switch(hierarchy) {                  
                case HIERARCHY_NONE:            return -1;
                case HIERARCHY_1:               return 1;
                case HIERARCHY_2:               return 2;
                case HIERARCHY_4:               return 4;
                default:                        return 0;
                }                         
}

void vlc_file_header(FILE *f, uint16_t adapter, uint16_t frontend, struct w_scan_flags * flags, struct lnb_types_st *lnbp)
{
        fprintf (f, "%s\n", "#EXTM3U");

        fprintf (f, "#EXTVLCOPT:dvb-adapter=%i\n", adapter);

        switch (flags->fe_type) {
        case FE_QPSK: 
                fprintf (f, "#EXTVLCOPT:dvb-lnb-lof1=%lu\n", lnbp->low_val);
                fprintf (f, "#EXTVLCOPT:dvb-lnb-lof2=%lu\n", lnbp->high_val);
                fprintf (f, "#EXTVLCOPT:dvb-lnb-slof=%lu\n", lnbp->switch_val);
                break;
        default:;
        };


}

void vlc_dump_dvb_parameters (FILE * f, struct extended_dvb_frontend_parameters * p, struct w_scan_flags * flags, struct lnb_types_st * lnbp)
{
        switch (flags->fe_type) {
        case FE_ATSC:
                fprintf (f, "#EXTVLCOPT:dvb-frequency=%i\n",    p->frequency);
                fprintf (f, "#EXTVLCOPT:dvb-modulation=%i\n",   vlc_modulation_ATSC(p->u.vsb.modulation));
                /* NOTE: VLC is not honoring srate here. Most probably cable clear-qam tuning will fail
                 *       in several cases, where the frontend doesnt support 'AUTO'.
                 */
                break;
        case FE_QAM:
                fprintf (f, "#EXTVLCOPT:dvb-frequency=%i\n",    p->frequency);
                fprintf (f, "#EXTVLCOPT:dvb-inversion=%i\n",    vlc_inversion(p->inversion));
                /* VLC defaults to srate = 6875000 ??!
                 * NOTE: quite *uncommon* value for European DVB-C; Looks like a VLC bug. This value will
                 *       avoid DVB-C tuning for about 75% of European DVB-C users.
                 *       VLC should default to srate = 6900000 and QAM_64 or QAM_AUTO (QAM_AUTO is done.)
                 */
                fprintf (f, "#EXTVLCOPT:dvb-srate=%i\n",        p->u.qam.symbol_rate); 
                fprintf (f, "#EXTVLCOPT:dvb-fec=%i\n",          vlc_fec(p->u.qam.fec_inner));
                fprintf (f, "#EXTVLCOPT:dvb-modulation=%i\n",   vlc_modulation_QAM(p->u.qam.modulation));
                break;
        case FE_OFDM:
                fprintf (f, "#EXTVLCOPT:dvb-frequency=%i\n",    p->frequency);
                fprintf (f, "#EXTVLCOPT:dvb-inversion=%i\n",    vlc_inversion(p->inversion));
                fprintf (f, "#EXTVLCOPT:dvb-bandwidth=%i\n",    vlc_bandwidth(p->u.ofdm.bandwidth));
                fprintf (f, "#EXTVLCOPT:dvb-code-rate-hp=%i\n", vlc_fec(p->u.ofdm.code_rate_HP));
                fprintf (f, "#EXTVLCOPT:dvb-code-rate-lp=%i\n", vlc_fec(p->u.ofdm.code_rate_LP));
                fprintf (f, "#EXTVLCOPT:dvb-modulation=%i\n",   vlc_modulation_OFDM(p->u.ofdm.constellation));
                fprintf (f, "#EXTVLCOPT:dvb-transmission=%i\n", vlc_transmission(p->u.ofdm.transmission_mode));
                fprintf (f, "#EXTVLCOPT:dvb-guard=%i\n",        vlc_guard(p->u.ofdm.guard_interval));
                fprintf (f, "#EXTVLCOPT:dvb-hierarchy=%i\n",    vlc_hierarchy(p->u.ofdm.hierarchy_information));
                break;
        case FE_QPSK:
                fprintf (f, "#EXTVLCOPT:dvb-frequency=%i\n",    p->frequency);
                fprintf (f, "#EXTVLCOPT:dvb-inversion=%i\n",    vlc_inversion(p->inversion));
                fprintf (f, "#EXTVLCOPT:dvb-srate=%i\n",        p->u.qpsk.symbol_rate);
                fprintf (f, "#EXTVLCOPT:dvb-fec=%i\n",          vlc_fec(p->u.qpsk.fec_inner));
                switch (p->u.qpsk.polarization) {
                        case POLARIZATION_HORIZONTAL:
                        case POLARIZATION_CIRCULAR_LEFT:
                                fprintf (f, "#EXTVLCOPT:dvb-voltage=18\n");
                                break;
                        default:
                                fprintf (f, "#EXTVLCOPT:dvb-voltage=13\n");
                                break;                                
                        }
                fprintf (f, "#EXTVLCOPT:dvb-tone=%i\n", p->frequency >= lnbp->switch_val);

                if ((flags->sw_pos & 0xF) < 0xF)
                        fprintf (f, "#EXTVLCOPT:dvb-satno=%i\n", flags->sw_pos & 0xF);

                break;
        default:
                fatal("Unknown frontend type %d\n", flags->fe_type);
        };
}

void vlc_dump_service_parameter_set (FILE *f, 
                                const char *service_name,
                                const char *provider_name,
                                struct extended_dvb_frontend_parameters *p,
                                uint16_t video_pid,
                                uint16_t *audio_pid,
                                uint16_t service_id,
                                struct w_scan_flags * flags,
                                struct lnb_types_st *lnbp)
{
        if (video_pid || audio_pid[0]) {
                if (provider_name)
                        fprintf (f, "#EXTINF:0,%s;%s\n", service_name, provider_name);
                else
                        fprintf (f, "#EXTINF:0,%s\n", service_name);
                vlc_dump_dvb_parameters (f, p, flags, lnbp);
                //fprintf (f, "#EXTVLCOPT:sout-ts-pid-video=%i\n", video_pid);
                //fprintf (f, "#EXTVLCOPT:sout-ts-pid-audio=%i\n", audio_pid[0]); // FIXME: should get preferred LANG audio pid
                fprintf (f, "#EXTVLCOPT:program=%i\n", service_id);
                fprintf (f, "%s\n", "dvb://");
                }
}

const char * convert_str(const char * serviceName)
{
        static char buf[512];
        memset(&buf[0], 0, 512);
        int len = strlen(serviceName);
        int pi, po = 0;
        unsigned i;

        for (pi=0; pi<len; pi++) {
                int found = 0;
                for (i=0; i<R_COUNT(chrs); i++) {
                        if (serviceName[pi] == chrs[i].chr) {
                                found++;
                                strcpy(&buf[po], chrs[i].newStr);
                                po += chrs[i].len;
                                break;
                                }
                        }
                if (! found)
                        strcpy(&buf[po++], &serviceName[pi]);
                }
        return &buf[0];        
}

void vlc_xspf_prolog(FILE * f, uint16_t adapter, uint16_t frontend, struct w_scan_flags * flags, struct lnb_types_st * lnbp)
{
        fprintf_tab0("<?");
        fprintf_pair("xml version", "1.0");
        fprintf_tab0(" ");
        fprintf_pair("encoding", "UTF-8");
        fprintf_tab0("?>\n");
        fprintf_tab1("<playlist ");
        fprintf_pair("version", "1");
        fprintf_tab0(" ");
        fprintf_pair("xmlns", "http://xspf.org/ns/0/");
        fprintf_tab0(" ");
        fprintf_pair("xmlns:vlc", "http://www.videolan.org/vlc/playlist/ns/0/");
        fprintf_tab0(">\n");
        fprintf_tab2("<title>DVB Playlist</title>\n");
        fprintf_tab2("<trackList>\n");

        idx = 1;
}

void vlc_xspf_epilog(FILE *f)
{
        fprintf_tab2("</trackList>\n");

        fprintf_tab1("</playlist>\n");
}

void vlc_dump_dvb_parameters_as_xspf (FILE * f, struct extended_dvb_frontend_parameters * p, struct w_scan_flags * flags, struct lnb_types_st * lnbp)
{
        switch (flags->fe_type) {
        case FE_ATSC:
                fprintf (f, "%s", "atsc://");
                fprintf (f, "frequency=%i,",      p->frequency);
                fprintf (f, "modulation=%i",      vlc_modulation_ATSC(p->u.vsb.modulation));
                /* NOTE: VLC is not honoring srate here. Most probably cable clear-qam tuning will fail
                 *       in several cases, where the frontend doesnt support 'AUTO'.
                 */
                break;
        case FE_QAM:
                fprintf (f, "%s", "dvb-c://");
                fprintf (f, "frequency=%i,",            p->frequency);
                fprintf (f, "inversion=%i,",            vlc_inversion(p->inversion));
                /* VLC defaults to srate = 6875000 ??!
                 * NOTE: quite *uncommon* value for European DVB-C; Looks like a VLC bug. This value will
                 *       avoid DVB-C tuning for about 75% of European DVB-C users.
                 *       VLC should default to srate = 6900000 and QAM_64 or QAM_AUTO (QAM_AUTO is done.)
                 */
                fprintf (f, "srate=%i,",                p->u.qam.symbol_rate); 
                fprintf (f, "fec=%i,",                  vlc_fec(p->u.qam.fec_inner));
                fprintf (f, "modulation=%i",            vlc_modulation_QAM(p->u.qam.modulation));
                break;
        case FE_OFDM:
                fprintf (f, "%s",                       "dvb-t://");
                fprintf (f, "frequency=%i,",            p->frequency);
                fprintf (f, "inversion=%i,",            vlc_inversion(p->inversion));
                fprintf (f, "bandwidth=%i,",            vlc_bandwidth(p->u.ofdm.bandwidth));
                fprintf (f, "code-rate-hp=%i,",         vlc_fec(p->u.ofdm.code_rate_HP));
                fprintf (f, "code-rate-lp=%i,",         vlc_fec(p->u.ofdm.code_rate_LP));
                fprintf (f, "modulation=%i,",           vlc_modulation_OFDM(p->u.ofdm.constellation));
                fprintf (f, "transmission=%i,",         vlc_transmission(p->u.ofdm.transmission_mode));
                fprintf (f, "guard=%i,",                vlc_guard(p->u.ofdm.guard_interval));
                fprintf (f, "hierarchy=%i",             vlc_hierarchy(p->u.ofdm.hierarchy_information));
                break;
        case FE_QPSK:
                /* NOTE: VLC (1.1.4, 20101113) doesnt seem to support DVB-S2 at all, DVB-S only. */
                fprintf (f, "%s", "dvb-s://");
                fprintf (f, "frequency=%i,",            p->frequency);
                fprintf (f, "inversion=%i,",            vlc_inversion(p->inversion));
                fprintf (f, "srate=%i,",                p->u.qpsk.symbol_rate);
                fprintf (f, "fec=%i,",                  vlc_fec(p->u.qpsk.fec_inner));
                switch (p->u.qpsk.polarization) {
                        case POLARIZATION_HORIZONTAL:
                        case POLARIZATION_CIRCULAR_LEFT:
                                fprintf (f, "voltage=18,\n");
                                break;
                        default:
                                fprintf (f, "voltage=13,\n");
                                break;                                
                        }
                fprintf (f, "tone=%i", p->frequency >= lnbp->switch_val);

                if ((flags->sw_pos & 0xF) < 0xF)
                        fprintf (f, ",satno=%i", flags->sw_pos & 0xF);

                break;
        default:
                fatal("Unknown frontend type %d\n", flags->fe_type);
        };
}

void vlc_dump_service_parameter_set_as_xspf (FILE * f,
                                struct service * s,
                                struct transponder * t,
                                struct w_scan_flags * flags,
                                struct lnb_types_st *lnbp)
{
        fprintf_tab3("<track>\n");
        fprintf (f, "%s%s%.4d. %s%s\n", T4, "<title>", idx++, convert_str(s->service_name), "</title>");
        fprintf (f, "%s%s",             T4, "<location>");

        vlc_dump_dvb_parameters_as_xspf(f, &t->param, flags, lnbp);

        fprintf (f, "%s\n", "</location>");
        fprintf_tab4("<extension ");
        fprintf_pair("application", "http://www.videolan.org/vlc/playlist/0");
        fprintf_tab0(">\n");
        fprintf (f, "%s%s%d%s\n", T5, "<vlc:id>", idx, "</vlc:id>");
        fprintf (f, "%s%s%d%s\n", T5, "<vlc:option>program=", s->service_id, "</vlc:option>");
        fprintf_tab4("</extension>\n");
        fprintf_tab3("</track>\n");
}
