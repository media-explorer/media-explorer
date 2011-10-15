/*
 * Simple MPEG/DVB parser to achieve network/service information without initial tuning data
 *
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Winfried Koehler 
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
 *  referred standards:
 *
 *    ETSI EN 300 468 v1.9.1
 *    ETSI TR 101 211
 *    ETSI ETR 211
 *    ITU-T H.222.0 / ISO/IEC 13818-1
 *    http://www.eutelsat.com/satellites/pdf/Diseqc/Reference docs/bus_spec.pdf
 *
 ##############################################################################
 * This is tool is derived from the dvb scan tool,
 * Copyright: Johannes Stezenbach <js@convergence.de> and others, GPLv2 and LGPL
 * (linuxtv-dvb-apps-1.1.0/utils/scan)
 *
 * Differencies:
 * - command line options
 * - detects dvb card automatically
 * - no need for initial tuning data
 * - some adaptions for VDR syntax
 *
 * have phun, wirbel 2006/02/16
 ##############################################################################
 */

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>

#include <linux/dvb/dmx.h>
#include <linux/dvb/version.h>

#include "version.h"
#include "scan.h"
#include "dump-vdr.h"
#include "dump-xine.h"
#include "dump-dvbscan.h"
#include "dump-kaffeine.h"
#include "dump-mplayer.h"
#include "dump-vlc-m3u.h"
#include "dvbscan.h"
#include "parse-dvbscan.h"
#include "countries.h"
#include "satellites.h"
#include "atsc_psip_section.h"
#include "descriptors.h"
#include "lnb.h"
#include "diseqc.h"
#include "iconv_codes.h"
#include "char-coding.h"

static char demux_devname[80];

static struct dvb_frontend_info fe_info = {.type = -1};

int verbosity = 2;      // need signed -> use of fatal()

struct w_scan_flags flags = {
        0,              // readback value w_scan version {YYYYMMDD}
        FE_OFDM,        // frontend type
        ATSC_VSB,       // default for ATSC scan
        0,              // need 2nd generation frontend
        DE,             // country index or sat index
        1,              // tuning speed {1 = fast, 2 = medium, 3 = slow}
        0,              // filter timeout {0 = default, 1 = long} 
        1,              // get_other_nits, atm always
        1,              // add_frequencies, atm always
        1,              // dump_provider, dump also provider name
        6,              // VDR version number, VDR-1.6.x
        0,              // 0 = qam auto, 1 = search qams
        1,              // scan encrypted channels = yes
        -1,             // rotor position, unused
        0x0302,         // assuming DVB API version 3.2
        0xFF,           // switch pos
        0,              // codepage, 0 = UTF-8
        0,              // print pmt
};

static unsigned int modulation_min = 0;         // initialization of modulation loop. QAM64  if FE_QAM
static unsigned int modulation_max = 1;         // initialization of modulation loop. QAM256 if FE_QAM
static unsigned int dvbc_symbolrate_min = 0;    // initialization of symbolrate loop. 6900
static unsigned int dvbc_symbolrate_max = 1;    // initialization of symbolrate loop. 6875
static unsigned int freq_offset_min = 0;        // initialization of freq offset loop. 0 == offset (0), 1 == offset(+), 2 == offset(-)
static unsigned int freq_offset_max = 2;        // initialization of freq offset loop.
static int this_channellist = DVBT_DE;          // w_scan uses by default DVB-t
static unsigned int ATSC_type = ATSC_VSB;       // 20090227: flag type vars shouldnt be signed. 
static unsigned int no_ATSC_PSIP = 0;           // 20090227: initialization was missing, signed -> unsigned                
static unsigned int serv_select = 3;            // 20080106: radio and tv as default (no service/other). 20090227: flag type vars shouldnt be signed. 
static int this_rotor_pos = -1;                 // 20090320: DVB-S/S2, current rotor position
static int committed_switch = 0;                // 20090320: DVB-S/S2, DISEQC committed switch position
static int uncommitted_switch = 0;              // 20090320: DVB-S/S2, DISEQC uncommitted switch position
static struct lnb_types_st this_lnb;            // 20090320: DVB-S/S2, LNB type, initialized in main to 'UNIVERSAL'

time_t start_time = 0;

static enum fe_spectral_inversion caps_inversion        = INVERSION_AUTO;
static enum fe_code_rate caps_fec                       = FEC_AUTO;
static enum fe_modulation caps_qam                      = QAM_AUTO;
static enum fe_modulation this_qam                      = QAM_64;
static enum fe_modulation this_atsc                     = VSB_8;
static enum fe_transmit_mode caps_transmission_mode     = TRANSMISSION_MODE_AUTO;
static enum fe_guard_interval caps_guard_interval       = GUARD_INTERVAL_AUTO;
static enum fe_hierarchy caps_hierarchy                 = HIERARCHY_AUTO;

enum __output_format {
        OUTPUT_VDR,
        OUTPUT_GSTREAMER,
        OUTPUT_PIDS,
        OUTPUT_XINE,
        OUTPUT_DVBSCAN_TUNING_DATA,
        OUTPUT_KAFFEINE,
        OUTPUT_MPLAYER,
        OUTPUT_VLC_M3U,
};

static enum __output_format output_format = OUTPUT_VDR;

int run_time() {
        return time(NULL) - start_time;
}

void hexdump(const char * intro, const unsigned char * buf, int len) {

        int i, j;
        char sbuf[17];

        if (verbosity < 4)
                return;

        memset(&sbuf, 0, 17);

        info("\t===================== %s ", intro);
        for (i = strlen(intro) + 1; i < 50; i++)
                info("=");
        info("\n");
        info("\tlen = %d\n", len);
        for (i = 0; i < len; i++) {
                if ((i % 16) == 0) {
                        info("%s0x%.2X: ",i?"\n\t":"\t",(i / 16) * 16);
                        }
                info("%.2X ", (uint8_t) *(buf + i));
                sbuf[i % 16] = *(buf + i);
                if (((i + 1) % 16) == 0) {
                        // remove non-printable chars
                        for (j = 0; j < 16; j++)
                                if (! ((sbuf[j] > 31)  && (sbuf[j] < 127)))
                                        sbuf[j] = ' ';
                        
                        info(": %s", sbuf);
                        memset(&sbuf, 0, 17);
                        }
                }
        if (len % 16) {
                for (i = 0; i < (len % 16); i++)
                        if (! ((sbuf[i] > 31)  && (sbuf[i] < 127)))
                                sbuf[i] = ' ';
                for (i = (len % 16); i < 16; i++)
                        info("   ");
                info(": %s", sbuf);
                }
        info("\n");
        info("\t========================================================================\n");
}


struct section_buf {
        struct list_head list;
        const char *dmx_devname;
        unsigned int run_once   : 1;
        unsigned int segmented  : 1;    /* segmented by table_id_ext */
        int fd;
        int pid;
        int table_id;
        int table_id_ext;
        int section_version_number;
        uint8_t section_done[32];
        int sectionfilter_done;
        unsigned char buf[1024];
        time_t timeout;
        time_t start_time;
        time_t running_time;
        struct section_buf *next_seg;   /* this is used to handle segmented tables (like NIT-other) */
};

static LIST_HEAD(scanned_transponders);
static LIST_HEAD(new_transponders);
static struct transponder *current_tp;

static void setup_filter (struct section_buf* s, const char *dmx_devname,
                          int pid, int table_id, int table_id_ext,
                          int run_once, int segmented);

static void add_filter (struct section_buf *s);


/* According to the DVB standards, the combination of network_id and
 * transport_stream_id should be unique, but in real life the satellite
 * operators and broadcasters don't care enough to coordinate
 * the numbering. Thus we identify TPs by frequency (scan handles only
 * one satellite at a time). Further complication: Different NITs on
 * one satellite sometimes list the same TP with slightly different
 * frequencies, so we have to search within some bandwidth.
 */
struct transponder *alloc_transponder(uint32_t frequency)
{
        struct transponder *tp = calloc(1, sizeof(*tp));

        tp->param.frequency = frequency;
        tp->updated_by_nit = 0;
        INIT_LIST_HEAD(&tp->list);
        INIT_LIST_HEAD(&tp->services);
        list_add_tail(&tp->list, &new_transponders);
        return tp;
}

static int is_nearly_same_frequency(uint32_t f1, uint32_t f2, enum fe_type type)
{
        uint32_t diff;
        if (f1 == f2)
                return 1;
        diff = (f1 > f2) ? (f1 - f2) : (f2 - f1);
        //FIXME: use symbolrate etc. to estimate bandwidth
        switch(type) {
                case FE_QPSK:
                        // 2MHz
                        if (diff < 2000) {
                                debug("f1 = %u is same TP as f2 = %u (diff=%d)\n", f1, f2, diff);
                                return 1;
                                }
                        break;
                default:
                        // 500kHz
                        if (diff < 500000) {
                                debug("f1 = %u is same TP as f2 = %u (diff=%d)\n", f1, f2, diff);
                                return 1;
                                }
                }
        return 0;
}


int is_different_transponder_deep_scan(struct transponder * a, struct transponder * b, int auto_allowed) {

#define IS_DIFFERENT(A, B, _ALLOW_AUTO_, _AUTO_) ( (A != B)  && (! _ALLOW_AUTO_ || (_ALLOW_AUTO_ && (A != _AUTO_) && (B != _AUTO_)) ) )

        if (a->type != b->type)
                return 1;
        if (! is_nearly_same_frequency(a->param.frequency, b->param.frequency, a->type))
                return 1;
        switch (a->type) {
                case FE_OFDM:
                        if(IS_DIFFERENT(a->param.u.ofdm.constellation,b->param.u.ofdm.constellation,auto_allowed,QAM_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.ofdm.bandwidth,b->param.u.ofdm.bandwidth,auto_allowed,BANDWIDTH_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.ofdm.code_rate_HP,b->param.u.ofdm.code_rate_HP,auto_allowed,FEC_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.ofdm.hierarchy_information,b->param.u.ofdm.hierarchy_information,auto_allowed,HIERARCHY_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.ofdm.code_rate_LP,b->param.u.ofdm.code_rate_LP,auto_allowed,FEC_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.ofdm.transmission_mode,b->param.u.ofdm.transmission_mode,auto_allowed,TRANSMISSION_MODE_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.ofdm.guard_interval,b->param.u.ofdm.guard_interval,auto_allowed,GUARD_INTERVAL_AUTO))
                                return 1;
                        return 0;
                case FE_ATSC:
                        if(IS_DIFFERENT(a->param.u.vsb.modulation,b->param.u.vsb.modulation,auto_allowed,QAM_AUTO))
                                return 1;
                        return 0;
                case FE_QAM:
                        if(IS_DIFFERENT(a->param.u.qam.modulation,b->param.u.qam.modulation,auto_allowed,QAM_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.qam.symbol_rate,b->param.u.qam.symbol_rate,0,0))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.qam.fec_inner,b->param.u.qam.fec_inner,auto_allowed,FEC_AUTO))
                                return 1;
                        return 0;
                case FE_QPSK:
                        if(IS_DIFFERENT(a->param.u.qpsk.symbol_rate,b->param.u.qpsk.symbol_rate,0,0))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.qpsk.modulation_system,b->param.u.qpsk.modulation_system,0,0))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.qpsk.polarization,b->param.u.qpsk.polarization,0,0))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.qpsk.fec_inner,b->param.u.qpsk.fec_inner,auto_allowed,FEC_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.qpsk.rolloff,b->param.u.qpsk.rolloff,auto_allowed,ROLLOFF_AUTO))
                                return 1;
                        if(IS_DIFFERENT(a->param.u.qpsk.modulation_type,b->param.u.qpsk.modulation_type,auto_allowed,HIERARCHY_AUTO))
                                return 1;
                        return 0;
                default:
                        fatal("unimplemented frontend type.\n");
                }
}

static struct transponder *find_transponder(struct transponder * tn, int auto_allowed)
{
        struct list_head *pos;
        struct transponder *tp;
        
        /* check wether develivery system matches frontend type */
        if (tn->type != fe_info.type)
                return NULL;
        /* compare wether tn matches any known transponder,
         * ignoring params set to "AUTO" while comparing
         */
        list_for_each(pos, &scanned_transponders) {
                tp = list_entry(pos, struct transponder, list);
                if (! is_different_transponder_deep_scan(tp, tn, 1))
                        return tp;
        }
        list_for_each(pos, &new_transponders) {
                tp = list_entry(pos, struct transponder, list);
                if (! is_different_transponder_deep_scan(tp, tn, 1))
                        return tp;
        }
        return NULL;
}

/* identify wether tn is already in list of new transponders */
static int is_known_initial_transponder(struct transponder * tn, int auto_allowed)
{
        struct list_head *pos;
        struct transponder *tp;

        list_for_each(pos, &new_transponders) {
                tp = list_entry(pos, struct transponder, list);
                switch (tn->type) {
                        case FE_OFDM:
                        case FE_QAM:
                                if ((tp->type == tn->type) &&
                                    is_nearly_same_frequency(tp->param.frequency, tn->param.frequency, tp->type))
                                        return 1;
                                break;
                        case FE_ATSC:
                                if ((tp->type == tn->type) &&
                                    is_nearly_same_frequency(tp->param.frequency, tn->param.frequency, tp->type) &&
                                   (tp->param.u.vsb.modulation == tn->param.u.vsb.modulation))
                                        return 1;
                                break;
                        case FE_QPSK: 
                                if (! is_different_transponder_deep_scan(tn,tp,auto_allowed))
                                        return 1;
                                break;
                        default:
                                fatal("Unhandled type %d\n", tn->type);
                        }
                }
        return 0;
}

void print_transponder(char * dest, struct transponder * t) {
        memset(dest, 0, sizeof(dest));
        switch (t->type) {
                case FE_OFDM:
                        sprintf(dest, "%-8s f = %6d kHz I%sB%sC%sD%sT%sG%sY%s",
                                xine_modulation_name(t->param.u.ofdm.constellation),
                                t->param.frequency/1000,
                                vdr_inversion_name(t->param.inversion),
                                vdr_bandwidth_name(t->param.u.ofdm.bandwidth),
                                vdr_fec_name(t->param.u.ofdm.code_rate_HP),
                                vdr_fec_name(t->param.u.ofdm.code_rate_LP),
                                vdr_transmission_mode_name(t->param.u.ofdm.transmission_mode),
                                vdr_guard_name(t->param.u.ofdm.guard_interval),
                                vdr_hierarchy_name(t->param.u.ofdm.hierarchy_information));
                        break;
                case FE_ATSC:
                        sprintf(dest, "%-8s f=%d kHz",
                                atsc_mod_to_txt(t->param.u.vsb.modulation),
                                t->param.frequency/1000);
                        break;
                case FE_QAM:
                        sprintf(dest, "%-8s f = %d kHz S%dC%s",
                                xine_modulation_name(t->param.u.qam.modulation),
                                t->param.frequency/1000,
                                t->param.u.qam.symbol_rate/1000,
                                vdr_fec_name(t->param.u.qam.fec_inner));
                        break;
                case FE_QPSK:
                        sprintf(dest, "%-2s f = %d kHz %s SR = %5d %4s 0,%s %5s",
                                qpsk_delivery_system_to_txt(t->param.u.qpsk.modulation_system),
                                t->param.frequency/1000,
                                qpsk_pol_to_txt(t->param.u.qpsk.polarization),
                                t->param.u.qpsk.symbol_rate/1000,
                                qpsk_fec_to_txt(t->param.u.qpsk.fec_inner),
                                qpsk_rolloff_to_txt(t->param.u.qpsk.rolloff),
                                qpsk_mod_to_txt(t->param.u.qpsk.modulation_type));
                
        break;
                default:
                        warning("unimplemented frontend type %d\n", t->type);
                }
}


static void copy_transponder(struct transponder *dest, struct transponder *source)
{
        struct list_head *pos;
        struct service *service;
        memcpy(&dest->pids, &source->pids, sizeof(dest->pids));
        dest->type = source->type;
        memcpy(&dest->param, &source->param, sizeof(dest->param));
        dest->scan_done = source->scan_done;
        dest->last_tuning_failed = source->last_tuning_failed;
        dest->other_frequency_flag = source->other_frequency_flag;
        dest->n_other_f = source->n_other_f;
        // equiv to.. http://www.mail-archive.com/linux-media@vger.kernel.org/msg14655.html 
        // Anssi Hannula <anssi.hann...@iki.fi>
        if (dest->pids.transport_stream_id != source->pids.transport_stream_id) {
                /* propagate change to any already allocated services */
                list_for_each(pos, &dest->services) {
                service = list_entry(pos, struct service, list);
                service->transport_stream_id = source->pids.transport_stream_id;
                }
        }

        if (dest->n_other_f) {
                dest->other_f = calloc(dest->n_other_f, sizeof(uint32_t));
                memcpy(dest->other_f, source->other_f, dest->n_other_f * sizeof(uint32_t));
        } 

        else
                dest->other_f = NULL;
        if (source->network_name != NULL) {
                if (dest->network_name != NULL)
                        free(dest->network_name);
                dest->network_name = (char *) malloc(strlen(source->network_name));
                memcpy(dest->network_name, source->network_name, strlen(source->network_name));
                dest->network_name[strlen(source->network_name)] = '\0';
                }
        else
                dest->network_name = NULL;
}

/* service_ids are guaranteed to be unique within one TP
 * (the DVB standards say theay should be unique within one
 * network, but in real life...)
 */
static struct service *alloc_service(struct transponder *tp, int service_id)
{
        struct service *s = calloc(1, sizeof(*s));
        INIT_LIST_HEAD(&s->list);
        s->service_id = service_id;
        list_add_tail(&s->list, &tp->services);
        return s;
}

static struct service *find_service(struct transponder *tp, int service_id)
{
        struct list_head *pos;
        struct service *s;

        list_for_each(pos, &tp->services) {
                s = list_entry(pos, struct service, list);
                if (s->service_id == service_id)
                        return s;
        }
        return NULL;
}

static int find_descriptor(uint8_t tag, const unsigned char *buf,
                int descriptors_loop_len,
                const unsigned char **desc, int *desc_len)
{
        while (descriptors_loop_len > 0) {
                unsigned char descriptor_tag = buf[0];
                unsigned char descriptor_len = buf[1] + 2;

                if (!descriptor_len) {
                        warning("descriptor_tag == 0x%02x, len is 0\n", descriptor_tag);
                        break;
                }

                if (tag == descriptor_tag) {
                        if (desc)
                                *desc = buf;
                        if (desc_len)
                                *desc_len = descriptor_len;
                        return 1;
                }

                buf += descriptor_len;
                descriptors_loop_len -= descriptor_len;
        }
        return 0;
}

static void parse_descriptors(enum table_id t, const unsigned char *buf,
                              int descriptors_loop_len, void *data, fe_type_t fe_type) {
        while (descriptors_loop_len > 0) {
                unsigned char descriptor_tag = buf[0];
                unsigned char descriptor_len = buf[1] + 2;

                if (!descriptor_len) {
                        debug("descriptor_tag == 0x%02x, len is 0\n", descriptor_tag);
                        break;
                }

                switch (descriptor_tag) {
                        case MHP_application_descriptor:
                        case MHP_application_name_desriptor:
                        case MHP_transport_protocol_descriptor:
                        case dvb_j_application_descriptor:
                        case dvb_j_application_location_descriptor:
                                break;
                        case ca_descriptor: /* 20080106 */
                                if (t == TABLE_PMT)
                                        parse_ca_descriptor (buf, data);        
                                break;        
                        case iso_639_language_descriptor:
                                if (t == TABLE_PMT)
                                        parse_iso639_language_descriptor (buf, data);
                                break;
                        case application_icons_descriptor:
                        case carousel_identifier_descriptor:
                                break;
                        case network_name_descriptor:
                                if (t == TABLE_NIT_ACT)
                                        parse_network_name_descriptor (buf, data);
                                break;
                        case service_list_descriptor:
                        case stuffing_descriptor:
                                break;
                        case satellite_delivery_system_descriptor:
                                if ((fe_type == FE_OFDM) || (fe_type == FE_QAM) || (fe_type == FE_ATSC))
                                        break;
                                if ((t == TABLE_NIT_ACT) || (t == TABLE_NIT_OTH))
                                        parse_satellite_delivery_system_descriptor (buf, data, caps_inversion);
                                break;
                        case cable_delivery_system_descriptor:
                                if ((fe_type == FE_OFDM) || (fe_type == FE_QPSK))
                                        break;
                                if ((t == TABLE_NIT_ACT) || (t == TABLE_NIT_OTH))
                                        parse_cable_delivery_system_descriptor (buf, data, caps_inversion);
                                break;
                        case vbi_data_descriptor:
                        case vbi_teletext_descriptor:
                        case bouquet_name_descriptor:
                                break;
                        case service_descriptor:
                                if ((t == TABLE_SDT_ACT) || (t == TABLE_SDT_OTH))
                                        parse_service_descriptor (buf, data, flags.codepage);
                                break;
                        case country_availability_descriptor:
                        case linkage_descriptor:
                        case nvod_reference_descriptor:
                        case time_shifted_service_descriptor:
                        case short_event_descriptor:
                        case extended_event_descriptor:
                        case time_shifted_event_descriptor:
                        case component_descriptor:
                        case mosaic_descriptor: 
                        case stream_identifier_descriptor:
                                break;
                        case ca_identifier_descriptor:
                                if ((t == TABLE_SDT_ACT) || (t == TABLE_SDT_OTH))
                                        parse_ca_identifier_descriptor (buf, data);
                                break;
                        case content_descriptor:
                        case parental_rating_descriptor:
                        case teletext_descriptor:
                        case telephone_descriptor:
                        case local_time_offset_descriptor:
                        case subtitling_descriptor:
                                break;
                        case terrestrial_delivery_system_descriptor:
                                if ((fe_type == FE_QAM) || (fe_type == FE_QPSK))
                                        break;
                                if ((t == TABLE_NIT_ACT) || (t == TABLE_NIT_OTH))
                                        parse_terrestrial_delivery_system_descriptor (buf, data, caps_inversion);
                                break;
                        case multilingual_network_name_descriptor:
                        case multilingual_bouquet_name_descriptor:
                        case multilingual_service_name_descriptor:
                        case multilingual_component_descriptor:
                        case private_data_specifier_descriptor:
                        case service_move_descriptor:
                        case short_smoothing_buffer_descriptor:
                                break;
                        case frequency_list_descriptor:
                                if ((t == TABLE_NIT_ACT) || (t == TABLE_NIT_OTH))
                                        parse_frequency_list_descriptor (buf, data);
                                break;
                        case partial_transport_stream_descriptor:
                        case data_broadcast_descriptor:
                        case scrambling_descriptor:
                        case data_broadcast_id_descriptor:
                        case transport_stream_descriptor:
                        case dsng_descriptor:
                        case pdc_descriptor:
                        case ac3_descriptor:
                        case ancillary_data_descriptor:
                        case cell_list_descriptor:
                        case cell_frequency_link_descriptor:
                        case announcement_support_descriptor:
                        case application_signalling_descriptor:
                        case service_identifier_descriptor:
                        case service_availability_descriptor:
                        case default_authority_descriptor:
                        case related_content_descriptor:
                        case tva_id_descriptor:
                        case content_identifier_descriptor:
                        case time_slice_fec_identifier_descriptor:
                        case ecm_repetition_rate_descriptor:
                                break;
                        case s2_satellite_delivery_system_descriptor:
                                if ((fe_type == FE_OFDM) || (fe_type == FE_QAM) || (fe_type == FE_ATSC))
                                        break;
                                if ((t == TABLE_NIT_ACT) || (t == TABLE_NIT_OTH))
                                        parse_S2_satellite_delivery_system_descriptor(buf, data);
                                break;
                        case enhanced_ac3_descriptor:
                        case dts_descriptor:
                        case aac_descriptor:
                        case extension_descriptor:
                                break;                
                        case 0x83:
                        case 0xF2: // 0xF2 Private DVB Descriptor  Premiere.de, Content Transmission Descriptor
                                break;                     
                        default:
                                verbosedebug("skip descriptor 0x%02x\n", descriptor_tag);
                        }

                buf += descriptor_len;
                descriptors_loop_len -= descriptor_len;
        }
}


static void parse_pat(const unsigned char *buf, int section_length,
                      int transport_stream_id)
{
        hexdump(__FUNCTION__, buf, section_length);
        while (section_length > 0) {
                struct service *s;
                int service_id = (buf[0] << 8) | buf[1];

                if (service_id == 0) {
                        verbosedebug ("skipping %02x %02x %02x %02x (service_id == 0)\n", buf[0],buf[1],buf[2],buf[3]);
                        buf += 4;               /*  skip nit pid entry... */
                        section_length -= 4;
                        continue;
                }
                /* SDT might have been parsed first... */
                s = find_service(current_tp, service_id);
                if (!s)
                        s = alloc_service(current_tp, service_id);
                s->pmt_pid = ((buf[2] & 0x1f) << 8) | buf[3];
                if (!s->priv && s->pmt_pid) {
                        s->priv = malloc(sizeof(struct section_buf));
                        setup_filter(s->priv, demux_devname,
                                     s->pmt_pid, TABLE_PMT, -1, 1, 0);

                        add_filter (s->priv);
                }

                buf += 4;
                section_length -= 4;
        }
}


static void parse_pmt (const unsigned char *buf, int section_length, int service_id)
{
        int program_info_len;
        struct service *s;
        char msg_buf[14 * AUDIO_CHAN_MAX + 1];
        char *tmp;
        int i;
        hexdump(__FUNCTION__, buf, section_length);
        s = find_service (current_tp, service_id);
        if (!s) {
                error("PMT for service_id 0x%04x was not in PAT\n", service_id);
                return;
        }

        s->pcr_pid = ((buf[0] & 0x1f) << 8) | buf[1];
        program_info_len = ((buf[2] & 0x0f) << 8) | buf[3];

        // 20080106, search PMT program info for CA Ids
        buf +=4;
        section_length -= 4;

        while (program_info_len > 0) {
                int descriptor_length = ((int)buf[1]) + 2;
                parse_descriptors(TABLE_PMT, buf, section_length, s, fe_info.type);
                buf += descriptor_length;
                section_length   -= descriptor_length;
                program_info_len -= descriptor_length;
                }

        while (section_length > 0) {
                int ES_info_len = ((buf[3] & 0x0f) << 8) | buf[4];
                int elementary_pid = ((buf[1] & 0x1f) << 8) | buf[2];

                switch (buf[0]) { // stream type
                case iso_iec_11172_video_stream:
                case iso_iec_13818_1_11172_2_video_stream:
                        moreverbose("  VIDEO     : PID 0x%04x\n", elementary_pid);
                        if (s->video_pid == 0)
                                s->video_pid = elementary_pid;
                        break;
                case iso_iec_11172_audio_stream:
                case iso_iec_13818_3_audio_stream:
                        moreverbose("  AUDIO     : PID 0x%04x\n", elementary_pid);
                        if (s->audio_num < AUDIO_CHAN_MAX) {
                                s->audio_pid[s->audio_num] = elementary_pid;
                                s->audio_num++;
                        }
                        else
                                warning("more than %i audio channels, truncating\n", AUDIO_CHAN_MAX);
                        parse_descriptors (TABLE_PMT, buf + 5, ES_info_len, s, fe_info.type);
                        break;
                case iso_iec_13818_1_private_sections:
                case iso_iec_13818_1_private_data:
                   /* ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data*/

                        if (find_descriptor(teletext_descriptor, buf + 5, ES_info_len, NULL, NULL)) {
                                moreverbose("  TELETEXT  : PID 0x%04x\n", elementary_pid);
                                s->teletext_pid = elementary_pid;
                                break;
                        }
                        else if (find_descriptor(subtitling_descriptor, buf + 5, ES_info_len, NULL, NULL)) {
                                /* Note: The subtitling descriptor can also signal
                                 * teletext subtitling, but then the teletext descriptor
                                 * will also be present; so we can be quite confident
                                 * that we catch DVB subtitling streams only here, w/o
                                 * parsing the descriptor. */
                                moreverbose("  SUBTITLING: PID 0x%04x\n", elementary_pid);
                                s->subtitling_pid = elementary_pid;
                                break;
                        }
                        else if (find_descriptor(ac3_descriptor, buf + 5, ES_info_len, NULL, NULL)) {
                                moreverbose("  AC3       : PID 0x%04x\n", elementary_pid);
                                if (s->ac3_num < AC3_CHAN_MAX) {
                                        s->ac3_pid[s->ac3_num] = elementary_pid;
                                        parse_descriptors (TABLE_PMT, buf + 5, ES_info_len, s, fe_info.type);
                                        s->ac3_num++;
                                }
                                else
                                        warning("more than %i ac3 audio channels, truncating\n",
                                             AC3_CHAN_MAX);
                                break;
                        }
                        else if (find_descriptor(enhanced_ac3_descriptor, buf + 5, ES_info_len, NULL, NULL)) {
                                moreverbose("  EAC3      : PID 0x%04x\n", elementary_pid);
                                if (s->ac3_num < AC3_CHAN_MAX) {
                                        s->ac3_pid[s->ac3_num] = elementary_pid;
                                        parse_descriptors (TABLE_PMT, buf + 5, ES_info_len, s, fe_info.type);
                                        s->ac3_num++;
                                }
                                else
                                        warning("more than %i eac3 audio channels, truncating\n",
                                             AC3_CHAN_MAX);
                                break;
                        }
                        /* we shouldn't reach this one, usually it should be Teletext, Subtitling or AC3 .. */
                        moreverbose("  unknown private data: PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13522_MHEG:
                        /*
                        MHEG-5, or ISO/IEC 13522-5, is part of a set of international standards relating to the
                        presentation of multimedia information, standardized by the Multimedia and Hypermedia Experts Group (MHEG).
                        It is most commonly used as a language to describe interactive television services. */
                        moreverbose("  MHEG      : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_1_Annex_A_DSM_CC:
                        moreverbose("  DSM CC    : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_1_11172_1_auxiliary:
                        moreverbose("  ITU-T Rec. H.222.0 | ISO/IEC 13818-1/11172-1 auxiliary : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_6_type_a_multiproto_encaps:
                        moreverbose("  ISO/IEC 13818-6 Multiprotocol encapsulation    : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_6_type_b:
                        /*
                        Digital storage media command and control (DSM-CC) is a toolkit for control channels associated
                        with MPEG-1 and MPEG-2 streams. It is defined in part 6 of the MPEG-2 standard (Extensions for DSM-CC).
                        DSM-CC may be used for controlling the video reception, providing features normally found
                        on VCR (fast-forward, rewind, pause, etc). It may also be used for a wide variety of other purposes
                        including packet data transport. MPEG-2 ISO/IEC 13818-6 (part 6 of the MPEG-2 standard).

                        DSM-CC defines or extends five distinct protocols:
                        * User-User 
                        * User-Network 
                        * MPEG transport profiles (profiles to the standard MPEG transport protocol ISO/IEC 13818-1 to allow
                                transmission of event, synchronization, download, and other information in the MPEG transport stream)
                        * Download 
                        * Switched Digital Broadcast-Channel Change Protocol (SDB/CCP)
                                Enables a client to remotely switch from channel to channel in a broadcast environment.
                                Used to attach a client to a continuous-feed session (CFS) or other broadcast feed. Sometimes used in pay-per-view.
                        */
                        moreverbose("  DSM-CC U-N Messages : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_6_type_c://DSM-CC Stream Descriptors
                        moreverbose("  ISO/IEC 13818-6 Stream Descriptors : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_6_type_d://DSM-CC Sections (any type, including private data)
                        moreverbose("  ISO/IEC 13818-6 Sections (any type, including private data) : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_1_auxiliary:
                        moreverbose("  ISO/IEC 13818-1 auxiliary : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_7_audio_w_ADTS_transp:
                        moreverbose("  ADTS Audio Stream (usually AAC) : PID 0x%04x\n", elementary_pid);
                        if (output_format == OUTPUT_VDR) break; /* not supported by VDR up to now. */
                        if (s->audio_num < AUDIO_CHAN_MAX) {
                                s->audio_pid[s->audio_num] = elementary_pid;
                                s->audio_num++;
                        }
                        else
                                warning("more than %i audio channels, truncating\n", AUDIO_CHAN_MAX);
                        parse_descriptors (TABLE_PMT, buf + 5, ES_info_len, s, fe_info.type);
                        break;
                case iso_iec_14496_2_visual:
                        moreverbose("  ISO/IEC 14496-2 Visual : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_14496_3_audio_w_LATM_transp:
                        moreverbose("  ISO/IEC 14496-3 Audio with LATM transport syntax as def. in ISO/IEC 14496-3/AMD1 : PID 0x%04x\n", elementary_pid);
                        if (output_format == OUTPUT_VDR) break; /* not supported by VDR up to now. */
                        if (s->audio_num < AUDIO_CHAN_MAX) {
                                s->audio_pid[s->audio_num] = elementary_pid;
                                s->audio_num++;
                        }
                        else
                                warning("more than %i audio channels, truncating\n", AUDIO_CHAN_MAX);
                        parse_descriptors (TABLE_PMT, buf + 5, ES_info_len, s, fe_info.type);
                        break;
                case iso_iec_14496_1_packet_stream_in_PES:
                        moreverbose("  ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_14496_1_packet_stream_in_14996:
                        moreverbose("  ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC 14496 sections : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_6_synced_download_protocol:
                        moreverbose("  ISO/IEC 13818-6 DSM-CC synchronized download protocol : PID 0x%04x\n", elementary_pid);
                        break;
                case metadata_in_PES:
                        moreverbose("  Metadata carried in PES packets using the Metadata Access Unit Wrapper : PID 0x%04x\n", elementary_pid);
                        break;
                case metadata_in_metadata_sections:
                        moreverbose("  Metadata carried in metadata_sections : PID 0x%04x\n", elementary_pid);
                        break;
                case metadata_in_iso_iec_13818_6_data_carous:
                        moreverbose("  Metadata carried in ISO/IEC 13818-6 (DSM-CC) Data Carousel : PID 0x%04x\n", elementary_pid);
                        break;
                case metadata_in_iso_iec_13818_6_obj_carous:
                        moreverbose("  Metadata carried in ISO/IEC 13818-6 (DSM-CC) Object Carousel : PID 0x%04x\n", elementary_pid);
                        break;
                case metadata_in_iso_iec_13818_6_synced_dl:
                        moreverbose("  Metadata carried in ISO/IEC 13818-6 Synchronized Download Protocol using the Metadata Access Unit Wrapper : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_13818_11_IPMP_stream:
                        moreverbose("  IPMP stream (defined in ISO/IEC 13818-11, MPEG-2 IPMP) : PID 0x%04x\n", elementary_pid);
                        break;
                case iso_iec_14496_10_AVC_video_stream:
                        moreverbose("  AVC Video stream, ITU-T Rec. H.264 | ISO/IEC 14496-10 : PID 0x%04x\n", elementary_pid);
                        if (s->video_pid == 0)
                                s->video_pid = elementary_pid;
                        break;
                case atsc_a_52b_ac3:
                        moreverbose("  AC-3 Audio per ATSC A/52B : PID 0x%04x\n", elementary_pid);
                        if ((output_format == OUTPUT_VDR) && (flags.vdr_version < 7))
                           break; /* not supported by VDR-1.2..1.7.13 */
                        if (s->ac3_num < AC3_CHAN_MAX) {
                                s->ac3_pid[s->ac3_num] = elementary_pid;
                                s->ac3_num++;
                        }
                        else
                                warning("more than %i ac3 audio channels, truncating\n", AC3_CHAN_MAX);
                        parse_descriptors (TABLE_PMT, buf + 5, ES_info_len, s, fe_info.type);
                        break;
                default:
                        moreverbose("  OTHER     : PID 0x%04x TYPE 0x%02x\n", elementary_pid, buf[0]);
                }

                buf += ES_info_len + 5;
                section_length -= ES_info_len + 5;
        }


        tmp = msg_buf;
        tmp += sprintf(tmp, "0x%04x (%.4s)", s->audio_pid[0], s->audio_lang[0]);

        if (s->audio_num > AUDIO_CHAN_MAX) {
                warning("more than %i audio channels: %i, truncating to %i\n",
                      AUDIO_CHAN_MAX, s->audio_num, AUDIO_CHAN_MAX);
                s->audio_num = AUDIO_CHAN_MAX;
        }

        for (i=1; i<s->audio_num; i++)
                tmp += sprintf(tmp, ", 0x%04x (%.4s)", s->audio_pid[i], s->audio_lang[i]);

        debug("0x%04x 0x%04x: %s -- %s, pmt_pid 0x%04x, vpid 0x%04x, apid %s\n",
            s->transport_stream_id,
            s->service_id,
            s->provider_name, s->service_name,
            s->pmt_pid, s->video_pid, msg_buf);
}


static void parse_nit (const unsigned char *buf, int section_length, int table_id, int network_id)
{
        char buffer[60];
        int descriptors_loop_len = ((buf[0] & 0x0f) << 8) | buf[1];

        hexdump(__FUNCTION__, buf, section_length);

        if (section_length < descriptors_loop_len + 4)
        {
                warning("section too short: network_id == 0x%04x, section_length == %i, "
                     "descriptors_loop_len == %i\n",
                     network_id, section_length, descriptors_loop_len);
                return;
        }
        // update network_name
        parse_descriptors (table_id, buf + 2, descriptors_loop_len, current_tp, fe_info.type);
        section_length -= descriptors_loop_len + 4;
        buf += descriptors_loop_len + 4;

        while (section_length > 6) {
                int transport_stream_id = (buf[0] << 8) | buf[1];
                struct transponder *t = NULL, tn;

                descriptors_loop_len = ((buf[4] & 0x0f) << 8) | buf[5];

                if (section_length < descriptors_loop_len + 4)
                {
                        warning("section too short: transport_stream_id == 0x%04x, "
                             "section_length == %i, descriptors_loop_len == %i\n",
                             transport_stream_id, section_length,
                             descriptors_loop_len);
                        break;
                }

                debug("transport_stream_id 0x%04x\n", transport_stream_id);

                memset(&tn, 0, sizeof(tn));
                tn.type = -1;
                tn.pids.network_id = network_id;
                tn.pids.original_network_id = (buf[2] << 8) | buf[3];   /* onid patch by Hartmut Birr */
                tn.pids.transport_stream_id = transport_stream_id;
                tn.network_name = NULL;
                parse_descriptors (table_id, buf + 6, descriptors_loop_len, &tn, fe_info.type);

                if ((t = find_transponder(&tn, 1)) != NULL) {
                        /* this transponder is already known.
                         * should we update its informations?
                         * find_transponders gives only matching transponders back,
                         * transponders with same freq + other parameters are understood as *different*,
                         * and will be handled as new transponders.
                         */
                        if (is_different_transponder_deep_scan(t, &tn, 0)) {
                                /* some of the informations is still set to AUTO */
                                if (table_id == TABLE_NIT_ACT) {
                                        /* only nit_actual should update transponders,
                                         * too much garbage in satellite nit_other.
                                         */
                                        print_transponder(buffer, t);
                                        info("\tupdating transponder:\n\t   (%s)\n", buffer);
                                        if (t->network_name != NULL) {
                                                // copy_transponder would overwrite network_name.
                                                tn.network_name = (char *) malloc(strlen(t->network_name));
                                                memcpy(tn.network_name, t->network_name, strlen(t->network_name));
                                                tn.network_name[strlen(t->network_name)] = '\0';
                                                }
                                        else
                                                tn.network_name = NULL;
                                        copy_transponder(t, &tn);
                                        t->updated_by_nit = 1;
                                        print_transponder(buffer, t);
                                        info("\tto (%s)\n", buffer);
                                        }
                                }
                        }
                else
                        /* we could not find the transponder by freq and fe_type.
                         * probably a new one - so adding it to scan list
                         */
                        if (flags.add_frequencies > 0 && (tn.type == fe_info.type)) {
                                t = alloc_transponder(tn.param.frequency);
                                copy_transponder(t, &tn);
                                /* some transponders need explicitly pilot *off* or *on* ,
                                   but NIT doesn't reflect this flag. Setting to 'auto' in
                                   the hope, that the dvb hardware and/or driver will handle
                                   it correctly.
                                 */
                                if (t->type == FE_QPSK)
                                        t->param.u.qpsk.pilot = PILOT_AUTO;
                                print_transponder(buffer, t);
                                info("\tnew transponder:\n\t   (%s)\n", buffer);
                                }
                section_length -= descriptors_loop_len + 6;
                buf += descriptors_loop_len + 6;
        }
}


static void parse_sdt (const unsigned char *buf, int section_length,
                int transport_stream_id)
{
        hexdump(__FUNCTION__, buf, section_length);

        buf += 3;              /*  skip original network id + reserved field */

        while (section_length > 4) {
                int service_id = (buf[0] << 8) | buf[1];
                int descriptors_loop_len = ((buf[3] & 0x0f) << 8) | buf[4];
                struct service *s;

                if (section_length < descriptors_loop_len || !descriptors_loop_len)
                {
                        warning("section too short: service_id == 0x%02x, section_length == %i, "
                             "descriptors_loop_len == %i\n",
                             service_id, section_length,
                             descriptors_loop_len);
                        break;
                }

                s = find_service(current_tp, service_id);
                if (!s)
                        /* maybe PAT has not yet been parsed... */
                        s = alloc_service(current_tp, service_id);

                s->running = (buf[3] >> 5) & 0x7;
                s->scrambled = (buf[3] >> 4) & 1;

                parse_descriptors (TABLE_SDT_ACT, buf + 5, descriptors_loop_len, s, fe_info.type);

                section_length -= descriptors_loop_len + 5;
                buf += descriptors_loop_len + 5;
        }
}





static void parse_psip_descriptors(struct service *s, const unsigned char *buf, int len)
{
        unsigned char *b = (unsigned char *) buf;
        int descriptor_length;
        while (len > 0) {
                descriptor_length = b[1];
                switch (b[0]) {
                        case atsc_service_location_descriptor:
                                parse_atsc_service_location_descriptor(s, b);
                                break;
                        case atsc_extended_channel_name_descriptor:
                                parse_atsc_extended_channel_name_descriptor(s, b);
                                break;
                        default:
                                warning("unhandled psip descriptor: %02x\n",b[0]);
                                break;
                }
                b += 2 + descriptor_length;
                len -= 2 + descriptor_length;
        }
}

static void parse_psip_vct (const unsigned char *buf, int section_length,
                int table_id, int transport_stream_id)
{
        (void)section_length;
        (void)table_id;
        (void)transport_stream_id;

        int num_channels_in_section = buf[1];
        int i;
        int pseudo_id = 0xffff;
        unsigned char *b = (unsigned char *) buf + 2;

        for (i = 0; i < num_channels_in_section; i++) {
                struct service *s;
                struct tvct_channel ch = read_tvct_channel(b);

                switch (ch.service_type) {
                        case atsc_analog_television:
                                verbose("analog channels won't be put info channels.conf\n");
                                break;
                        case atsc_digital_television:   /* ATSC TV */
                        case atsc_radio:                /* ATSC Radio */
                                break;
                        case atsc_data:                 /* ATSC Data */
                        default:
                                continue;
                }

                if (ch.program_number == 0)
                        ch.program_number = --pseudo_id;

                s = find_service(current_tp, ch.program_number);
                if (!s)
                        s = alloc_service(current_tp, ch.program_number);

                if (s->service_name)
                        free(s->service_name);

                s->service_name = malloc(8*sizeof(unsigned char));
                /* TODO find a better solution to convert UTF-16 */
                s->service_name[0] = ch.short_name0;
                s->service_name[1] = ch.short_name1;
                s->service_name[2] = ch.short_name2;
                s->service_name[3] = ch.short_name3;
                s->service_name[4] = ch.short_name4;
                s->service_name[5] = ch.short_name5;
                s->service_name[6] = ch.short_name6;
                s->service_name[7] = '\0';

                parse_psip_descriptors(s,&b[32],ch.descriptors_length);

                s->channel_num = ch.major_channel_number << 10 | ch.minor_channel_number;

                if (ch.hidden) {
                        s->running = rm_not_running;
                        info("service is not running, pseudo program_number.");
                } else {
                        s->running = rm_running;
                        info("service is running.");
                }

                info(" Channel number: %d:%d. Name: '%s'\n",
                        ch.major_channel_number, ch.minor_channel_number,s->service_name);

                b += 32 + ch.descriptors_length;
        }
}

static int get_bit (uint8_t *bitfield, int bit)
{
        return (bitfield[bit/8] >> (bit % 8)) & 1;
}

static void set_bit (uint8_t *bitfield, int bit)
{
        bitfield[bit/8] |= 1 << (bit % 8);
}


/**
 *   returns 0 when more sections are expected
 *           1 when all sections are read on this pid
 *          -1 on invalid table id
 */
static int parse_section (struct section_buf *s)
{
        const unsigned char *buf = s->buf;
        int table_id;
        int section_syntax_indicator;
        int section_length;
        int table_id_ext;
        int section_version_number;
        int current_next_indicator;
        int section_number;
        int last_section_number;
        int pcr_pid;
        int program_info_length;
        int i;

        table_id = buf[0];
        if (s->table_id != table_id)
                return -1;
        section_syntax_indicator = buf[1] & 0x80;
        section_length = (((buf[1] & 0x0f) << 8) | buf[2]) - 11;
        table_id_ext = (buf[3] << 8) | buf[4];                          // p.program_number
        section_version_number = (buf[5] >> 1) & 0x1f;                  // p.version_number = getBits (b, 0, 42, 5); -> 40 + 1 -> 5 bit weit? -> version_number = buf[5] & 0x3e;
        current_next_indicator = buf[5] & 0x01;
        section_number = buf[6];
        last_section_number = buf[7];
        pcr_pid = ((buf[8] & 0x1f) << 8) | buf[9];
        program_info_length = ((buf[10] & 0x0f) << 8) | buf[11];

        if (s->segmented && s->table_id_ext != -1 && s->table_id_ext != table_id_ext) {
                /* find or allocate actual section_buf matching table_id_ext */
                while (s->next_seg) {
                        s = s->next_seg;
                        if (s->table_id_ext == table_id_ext)
                                break;
                }
                if (s->table_id_ext != table_id_ext) {
                        assert(s->next_seg == NULL);
                        s->next_seg = calloc(1, sizeof(struct section_buf));
                        s->next_seg->segmented = s->segmented;
                        s->next_seg->run_once = s->run_once;
                        s->next_seg->timeout = s->timeout;
                        s = s->next_seg;
                        s->table_id = table_id;
                        s->table_id_ext = table_id_ext;
                        s->section_version_number = section_version_number;
                }
        }

        if (s->section_version_number != section_version_number ||
                        s->table_id_ext != table_id_ext) {
                struct section_buf *next_seg = s->next_seg;

                if (s->section_version_number != -1 && s->table_id_ext != -1)
                        debug("section version_number or table_id_ext changed "
                                "%d -> %d / %04x -> %04x\n",
                                s->section_version_number, section_version_number,
                                s->table_id_ext, table_id_ext);
                s->table_id_ext = table_id_ext;
                s->section_version_number = section_version_number;
                s->sectionfilter_done = 0;
                memset (s->section_done, 0, sizeof(s->section_done));
                s->next_seg = next_seg;
        }

        buf += 8;

        if (!get_bit(s->section_done, section_number)) {
                set_bit (s->section_done, section_number);

                debug("pid 0x%02x tid 0x%02x table_id_ext 0x%04x, "
                    "%i/%i (version %i)\n",
                    s->pid, table_id, table_id_ext, section_number,
                    last_section_number, section_version_number);

                switch (table_id) {
                case TABLE_PAT:
                        verbose("PAT\n");
                        parse_pat (buf, section_length, table_id_ext);
                        break;
                case TABLE_PMT:
                        verbose("PMT 0x%04x for service 0x%04x\n", s->pid, table_id_ext);
                        parse_pmt (buf, section_length, table_id_ext);
                        break;
                case TABLE_NIT_OTH:
                case TABLE_NIT_ACT:
                        verbose("NIT (%s TS)\n", table_id == 0x40 ? "actual":"other");
                        parse_nit (buf, section_length, table_id, table_id_ext);
                        break;
                case TABLE_SDT_ACT:
                case TABLE_SDT_OTH:
                        verbose("SDT (%s TS)\n", table_id == 0x42 ? "actual":"other");
                        parse_sdt (buf, section_length, table_id_ext);
                        break;
                case TABLE_VCT_TERR:
                case TABLE_VCT_CABLE:
                        verbose("ATSC VCT\n");
                        parse_psip_vct(buf, section_length, table_id, table_id_ext);
                        break;
                default:
                        ;
                }

                for (i = 0; i <= last_section_number; i++)
                        if (get_bit (s->section_done, i) == 0)
                                break;

                if (i > last_section_number)
                        s->sectionfilter_done = 1;
        }

        if (s->segmented) {
                /* always wait for timeout; this is because we don't now how
                 * many segments there are
                 */
                return 0;
        }
        else if (s->sectionfilter_done)
                return 1;

        return 0;
}


static int read_sections (struct section_buf *s)
{
        int section_length, count;

        if (s->sectionfilter_done && !s->segmented)
                return 1;

        /* the section filter API guarantess that we get one full section
         * per read(), provided that the buffer is large enough (it is)
         */
        if (((count = read (s->fd, s->buf, sizeof(s->buf))) < 0) && errno == EOVERFLOW)
                count = read (s->fd, s->buf, sizeof(s->buf));
        if (count < 0) {
                errorn("read_sections: read error");
                return -1;
        }

        if (count < 4)
                return -1;

        section_length = ((s->buf[1] & 0x0f) << 8) | s->buf[2];

        if (count != section_length + 3)
                return -1;

        if (parse_section(s) == 1)
                return 1;

        return 0;
}


static LIST_HEAD(running_filters);
static LIST_HEAD(waiting_filters);
static int n_running;
// see http://www.linuxtv.org/pipermail/linux-dvb/2005-October/005577.html:
// #define MAX_RUNNING 32
#define MAX_RUNNING 27

static struct pollfd poll_fds[MAX_RUNNING];
static struct section_buf* poll_section_bufs[MAX_RUNNING];


static void setup_filter (struct section_buf* s, const char *dmx_devname,
                          int pid, int table_id, int table_id_ext,
                          int run_once, int segmented) {
        memset (s, 0, sizeof(struct section_buf));

        s->fd = -1;
        s->dmx_devname = dmx_devname;
        s->pid = pid;
        s->table_id = table_id;

        s->run_once = run_once;
        s->segmented = segmented;
        if (flags.filter_timeout > 0)
                s->timeout = 5 * repetition_rate(fe_info.type, table_id);
        else
                s->timeout = repetition_rate(fe_info.type, table_id);

        s->table_id_ext = table_id_ext;
        s->section_version_number = -1;

        INIT_LIST_HEAD (&s->list);
}

static void update_poll_fds(void)
{
        struct list_head *p;
        struct section_buf* s;
        int i;

        memset(poll_section_bufs, 0, sizeof(poll_section_bufs));
        for (i = 0; i < MAX_RUNNING; i++)
                poll_fds[i].fd = -1;
        i = 0;
        list_for_each (p, &running_filters) {
                if (i >= MAX_RUNNING)
                        fatal("too many poll_fds\n");
                s = list_entry (p, struct section_buf, list);
                if (s->fd == -1)
                        fatal("s->fd == -1 on running_filters\n");
                verbosedebug("poll fd %d\n", s->fd);
                poll_fds[i].fd = s->fd;
                poll_fds[i].events = POLLIN;
                poll_fds[i].revents = 0;
                poll_section_bufs[i] = s;
                i++;
        }
        if (i != n_running)
                fatal("n_running is hosed\n");
}

static int start_filter (struct section_buf* s)
{
        struct dmx_sct_filter_params f;

        if (n_running >= MAX_RUNNING)
                goto err0;
        if ((s->fd = open (s->dmx_devname, O_RDWR)) < 0)
                goto err0;

        verbosedebug("start filter pid 0x%04x table_id 0x%02x\n", s->pid, s->table_id);

        memset(&f, 0, sizeof(f));

        f.pid = (uint16_t) s->pid;

        if (s->table_id < 0x100 && s->table_id > 0) {
                f.filter.filter[0] = (uint8_t) s->table_id;
                f.filter.mask[0]   = 0xff;
        }

        f.timeout = 0;
        f.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

        if (ioctl(s->fd, DMX_SET_FILTER, &f) == -1) {
                errorn ("ioctl DMX_SET_FILTER failed");
                goto err1;
        }

        s->sectionfilter_done = 0;
        time(&s->start_time);

        list_del_init (&s->list);  /* might be in waiting filter list */
        list_add (&s->list, &running_filters);

        n_running++;
        update_poll_fds();

        return 0;

err1:
        ioctl (s->fd, DMX_STOP);
        close (s->fd);
err0:
        return -1;
}


static void stop_filter (struct section_buf *s)
{
        verbosedebug("stop filter pid 0x%04x\n", s->pid);
        ioctl (s->fd, DMX_STOP);
        close (s->fd);
        s->fd = -1;
        list_del (&s->list);
        s->running_time += time(NULL) - s->start_time;

        n_running--;
        update_poll_fds();
}


static void add_filter (struct section_buf *s)
{
        verbosedebug("add filter pid 0x%04x\n", s->pid);
        if (start_filter (s))
                list_add_tail (&s->list, &waiting_filters);
}


static void remove_filter (struct section_buf *s)
{
        verbosedebug("remove filter pid 0x%04x\n", s->pid);
        stop_filter (s);

        while (!list_empty(&waiting_filters)) {
                struct list_head *next = waiting_filters.next;
                s = list_entry (next, struct section_buf, list);
                if (start_filter (s))
                        break;
        }
}


static void read_filters (void)
{
        struct section_buf *s;
        int i, n, done;

        n = poll(poll_fds, n_running, 1000);
        if (n == -1)
                errorn("poll");

        for (i = 0; i < n_running; i++) {
                s = poll_section_bufs[i];
                if (!s)
                        fatal("poll_section_bufs[%d] is NULL\n", i);
                if (poll_fds[i].revents)
                        done = read_sections (s) == 1;
                else
                        done = 0; /* timeout */
                if (done || time(NULL) > s->start_time + s->timeout) {
                        if (s->run_once) {
                                if (done)
                                        verbosedebug("filter done pid 0x%04x\n", s->pid);
                                else {
                                        switch (s->table_id) {
                                          case TABLE_PAT:       info("Info: PAT filter timeout\n"); break;
                                          case TABLE_CAT:       info("Info: CAT filter timeout\n"); break;
                                          case TABLE_PMT:       info("Info: PMT filter timeout\n"); break;
                                          case TABLE_TSDT:      info("Info: TSDT filter timeout\n"); break;
                                          case TABLE_NIT_ACT:   info("Info: NIT(actual) filter timeout\n");break;
                                          case TABLE_NIT_OTH:   verbose("Info: NIT(other) filter timeout\n");break; // not always available.
                                          case TABLE_SDT_ACT:   info("Info: SDT(actual) filter timeout\n"); break;
                                          case TABLE_SDT_OTH:   info("Info: SDT(other) filter timeout\n"); break;
                                          case TABLE_BAT:       info("Info: BAT filter timeout\n"); break;
                                          case TABLE_EIT_ACT:   info("Info: EIT(actual) filter timeout\n"); break;
                                          case TABLE_EIT_OTH:   info("Info: EIT(other) filter timeout\n"); break;
                                          case TABLE_TDT:       info("Info: TDT filter timeout\n"); break;
                                          case TABLE_RST:       info("Info: RST filter timeout\n"); break;
                                          case TABLE_TOT:       info("Info: TOT filter timeout\n"); break;
                                          case TABLE_AIT:       info("Info: AIT filter timeout\n"); break;
                                          case TABLE_CST:       info("Info: CST filter timeout\n"); break;
                                          case TABLE_RCT:       info("Info: RCT filter timeout\n"); break;
                                          case TABLE_CIT:       info("Info: CIT filter timeout\n"); break;
                                          case TABLE_VCT_TERR:  info("Info: VCT(terrestrial) filter timeout\n"); break;
                                          case TABLE_VCT_CABLE: info("Info: VCT(cable) filter timeout\n"); break;
                                          default:              info("Info: filter timeout pid 0x%04x\n", s->pid);
                                          }
                                        }
                                remove_filter (s);
                        }
                }
        }
}


static int mem_is_zero (const void *mem, unsigned int size)
{
        const char *p = mem;
        unsigned long i;

        for (i=0; i<size; i++) {
                if (p[i] != 0x0)
                        return 0;
        }

        return 1;
}

const char * frontend_type_to_text (fe_type_t frontend_type) {
        switch(frontend_type) {
                case FE_QAM:  return "DVB-C";
                case FE_QPSK: return "DVB-S";
                case FE_OFDM: return "DVB-T";
                case FE_ATSC: return "ATSC";
                default: return "UNKNOWN";
                }
}

uint32_t bandwidth_Hz (fe_bandwidth_t bandwidth) {
        switch (bandwidth) {
                case BANDWIDTH_8_MHZ: return 8000000;
                case BANDWIDTH_7_MHZ: return 7000000;
                case BANDWIDTH_6_MHZ: return 6000000;
                #ifdef BANDWIDTH_5_MHZ
                case BANDWIDTH_5_MHZ: return 5000000;
                #endif
                default: return 0;
                }
}

fe_delivery_system_t atsc_del_sys(fe_modulation_t modulation) {
        switch (modulation) {
                case VSB_8:
                case VSB_16:
                        return SYS_ATSC;
                default:;
                        return SYS_DVBC_ANNEX_B;
                }
}

static int copy_fe_params(struct extended_dvb_frontend_parameters * dest,
                          struct extended_dvb_frontend_parameters * source) {
        memcpy (dest, source, sizeof(struct extended_dvb_frontend_parameters));
        return 0;
}

/* might be removed later - intermediate solution. */
static int copy_fe_params_new_to_old(struct dvb_frontend_parameters * dest,
                   struct extended_dvb_frontend_parameters * source) {
        dest->frequency                         = source->frequency;
        dest->inversion                         = source->inversion;
        dest->u.qpsk.symbol_rate                = source->u.qpsk.symbol_rate;
        dest->u.qpsk.fec_inner                  = source->u.qpsk.fec_inner;
        dest->u.qam.symbol_rate                 = source->u.qam.symbol_rate;
        dest->u.qam.fec_inner                   = source->u.qam.fec_inner;
        dest->u.qam.modulation                  = source->u.qam.modulation;
        dest->u.vsb.modulation                  = source->u.vsb.modulation;
        dest->u.ofdm.bandwidth                  = source->u.ofdm.bandwidth;
        dest->u.ofdm.code_rate_HP               = source->u.ofdm.code_rate_HP;
        dest->u.ofdm.code_rate_LP               = source->u.ofdm.code_rate_LP;
        dest->u.ofdm.constellation              = source->u.ofdm.constellation;
        dest->u.ofdm.transmission_mode          = source->u.ofdm.transmission_mode;
        dest->u.ofdm.guard_interval             = source->u.ofdm.guard_interval;
        dest->u.ofdm.hierarchy_information      = source->u.ofdm.hierarchy_information;
        return 0;
}

static int set_frontend(int frontend_fd, struct transponder * t) {
        uint8_t switch_to_high_band = 0;
        uint32_t intermediate_freq = 0;
        int sequence_len = 0;
        struct dvb_frontend_parameters p;
        struct dtv_property cmds[12];
        struct dtv_properties cmdseq = {0, cmds};

        info("(time: %.2d:%.2d) ", run_time() / 60, run_time() % 60);

        switch(t->type) {
                case FE_QPSK:

                        if (t->param.u.qpsk.modulation_system == SYS_DVBS2) {
                                if (!(fe_info.caps & FE_CAN_2G_MODULATION) ||
                                     (flags.api_version < 0x0500)) {
                                        info("\t%d: skipped (no driver support)\n", t->param.frequency/1000);
                                        return -2;
                                        }
                                }

                        if (this_lnb.high_val) {
                                if (this_lnb.switch_val) { // voltage controlled switch
                                        switch_to_high_band = 0;

                                        if (t->param.frequency >= this_lnb.switch_val)
                                                switch_to_high_band++;

                                        setup_switch (frontend_fd, committed_switch,
                                                t->param.u.qpsk.polarization == POLARIZATION_VERTICAL ? 0 : 1,
                                                switch_to_high_band, uncommitted_switch);

                                        usleep(50000);

                                        if (switch_to_high_band)
                                                intermediate_freq = abs(t->param.frequency - this_lnb.high_val);
                                        else
                                                intermediate_freq = abs(t->param.frequency - this_lnb.low_val);
                                        }
                                else { // C-Band Multipoint LNB
                                        if (t->param.u.qpsk.polarization == POLARIZATION_VERTICAL)
                                                intermediate_freq = abs(t->param.frequency - this_lnb.low_val);
                                        else
                                                intermediate_freq = abs(t->param.frequency - this_lnb.high_val);
                                        }
                                }
                        else // Monopoint LNB w/o switch
                                intermediate_freq = abs(t->param.frequency - this_lnb.low_val);

                        if ((intermediate_freq < fe_info.frequency_min) || (intermediate_freq > fe_info.frequency_max)) {
                                info ("\tskipped: (freq %u unsupported by driver)\n", intermediate_freq);
                                return -2;
                                }

                        if ((t->param.u.qpsk.symbol_rate < fe_info.symbol_rate_min) || (t->param.u.qpsk.symbol_rate > fe_info.symbol_rate_max)) {
                                info ("\tskipped: (srate %u unsupported by driver)\n", t->param.u.qpsk.symbol_rate);
                                return -2;
                                }

                        if (sat_list[this_channellist].rotor_position > -1) { // rotate DiSEqC rotor to correct orbital position
                                /*
                                if (t->param.u.qpsk.orbital_position)
                                        rotor_pos = rotor_nn(t->param.u.qpsk.orbital_position, t->param.u.qpsk.west_east_flag);
                                 */
                                if (rotate_rotor(frontend_fd, &this_rotor_pos,
                                    sat_list[this_channellist].rotor_position,
                                    t->param.u.qpsk.polarization == POLARIZATION_VERTICAL ? 0 : 1,
                                    switch_to_high_band))
                                        error("Error rotating rotor\n");
                                }
                        break;

                case FE_QAM: // note: fall trough to OFDM && ATSC
                        if ((t->param.u.qam.symbol_rate < fe_info.symbol_rate_min) || (t->param.u.qam.symbol_rate > fe_info.symbol_rate_max)) {
                                info ("\tskipped: (srate %u unsupported by driver)\n", t->param.u.qam.symbol_rate);
                                return -2;
                                }                        
                case FE_OFDM:
                case FE_ATSC:
                        if ((t->param.frequency < fe_info.frequency_min) || (t->param.frequency > fe_info.frequency_max)) {
                                info ("\tskipped: (freq %u unsupported by driver)\n", t->param.frequency);
                                return -2;
                                }
                        break;
                default:;
                }

        if (mem_is_zero (&t->param, sizeof(struct extended_dvb_frontend_parameters)))
                return -1;

        switch (flags.api_version) {
                case 0x0302:
                        verbose("%s: using DVB API 3.2\n", __FUNCTION__);
                        copy_fe_params_new_to_old(&p, &t->param);
                        switch (t->type) {
                                case FE_QPSK:
                                        p.frequency = intermediate_freq;
                                        break;
                                default:;
                                }
                        if (ioctl(frontend_fd, FE_SET_FRONTEND, &p) == -1) {
                                errorn("Setting frontend parameters failed (API v3.2)\n");
                                return -1;
                                }
                        break;
                case 0x0500 ... 0x5FF:
                        verbose("%s: using DVB API %x.%x\n",
                          __FUNCTION__,
                         flags.api_version >> 8,
                         flags.api_version & 0xFF);

                        /* some 'shortcut' here :-)) --wk 20090324 */
                        #define set_cmd_sequence(_cmd, _data)   cmds[sequence_len].cmd = _cmd; \
                                                                cmds[sequence_len].u.data = _data; \
                                                                cmdseq.num = ++sequence_len

                        set_cmd_sequence(DTV_CLEAR, DTV_UNDEFINED);
                        switch (t->type) {
                                case FE_QPSK:
                                        set_cmd_sequence(DTV_DELIVERY_SYSTEM,   t->param.u.qpsk.modulation_system);
                                        set_cmd_sequence(DTV_FREQUENCY,         intermediate_freq);
                                        set_cmd_sequence(DTV_INVERSION,         t->param.inversion);
                                        set_cmd_sequence(DTV_MODULATION,        t->param.u.qpsk.modulation_type);
                                        set_cmd_sequence(DTV_SYMBOL_RATE,       t->param.u.qpsk.symbol_rate);
                                        set_cmd_sequence(DTV_INNER_FEC,         t->param.u.qpsk.fec_inner);
                                        set_cmd_sequence(DTV_PILOT,             t->param.u.qpsk.pilot);
                                        set_cmd_sequence(DTV_ROLLOFF,           t->param.u.qpsk.rolloff);
                                        break;
                                case FE_QAM:
                                        set_cmd_sequence(DTV_DELIVERY_SYSTEM,   SYS_DVBC_ANNEX_AC);
                                        set_cmd_sequence(DTV_FREQUENCY,         t->param.frequency);
                                        set_cmd_sequence(DTV_INVERSION,         t->param.inversion);
                                        set_cmd_sequence(DTV_MODULATION,        t->param.u.qam.modulation);
                                        set_cmd_sequence(DTV_SYMBOL_RATE,       t->param.u.qam.symbol_rate);
                                        set_cmd_sequence(DTV_INNER_FEC,         t->param.u.qam.fec_inner);
                                        break;
                                case FE_OFDM:
                                        set_cmd_sequence(DTV_DELIVERY_SYSTEM,   SYS_DVBT);
                                        set_cmd_sequence(DTV_FREQUENCY,         t->param.frequency);
                                        set_cmd_sequence(DTV_INVERSION,         t->param.inversion);
                                        set_cmd_sequence(DTV_BANDWIDTH_HZ,      bandwidth_Hz(t->param.u.ofdm.bandwidth));
                                        set_cmd_sequence(DTV_CODE_RATE_HP,      t->param.u.ofdm.code_rate_HP);
                                        set_cmd_sequence(DTV_CODE_RATE_LP,      t->param.u.ofdm.code_rate_LP);
                                        set_cmd_sequence(DTV_MODULATION,        t->param.u.ofdm.constellation);
                                        set_cmd_sequence(DTV_TRANSMISSION_MODE, t->param.u.ofdm.transmission_mode);
                                        set_cmd_sequence(DTV_GUARD_INTERVAL,    t->param.u.ofdm.guard_interval);
                                        set_cmd_sequence(DTV_HIERARCHY,         t->param.u.ofdm.hierarchy_information);
                                        break;
                                case FE_ATSC:
                                        set_cmd_sequence(DTV_DELIVERY_SYSTEM,   atsc_del_sys(t->param.u.vsb.modulation));
                                        set_cmd_sequence(DTV_FREQUENCY,         t->param.frequency);
                                        set_cmd_sequence(DTV_INVERSION,         t->param.inversion);
                                        set_cmd_sequence(DTV_MODULATION,        t->param.u.vsb.modulation);
                                        break;
                                default:
                                        fatal("Unhandled type %d\n", t->type);
                                }
                        set_cmd_sequence(DTV_TUNE, DTV_UNDEFINED);
                                                
                        if (ioctl(frontend_fd, FE_SET_PROPERTY, &cmdseq) < 0) {
                                errorn("Setting frontend parameters failed (API v5.x)\n");
                                return -1;
                                }
                        break;
                default:
                        fatal("unsupported DVB API Version %x.%x\n",
                                flags.api_version >> 8,
                                flags.api_version & 0xFF);
                }
        return 0;
}


static int __tune_to_transponder (int frontend_fd, struct transponder *t, int v) {

        fe_status_t s;
        int i, res;

        if (t == NULL)
                return -3;
        current_tp = t;
        if (current_tp->network_name != NULL) {
                free(current_tp->network_name);
                current_tp->network_name = NULL;
                }

        if ((verbosity >= 1) && (v > 0)) {
                char * buf = (char *) malloc(128); // paranoia, max = 52
                print_transponder(buf, t);
                dprintf(1, "tune to: %s %s",
                        buf, t->last_tuning_failed?" (no signal)\n":"\n");
                free(buf);
                }

        res = set_frontend(frontend_fd, t);

        if (res < 0)
                return res;

        for (i = 0; i < 5 * flags.tuning_timeout; i++) {
                usleep (200000);

                if (ioctl(frontend_fd, FE_READ_STATUS, &s) == -1) {
                        errorn("FE_READ_STATUS failed");
                        return -1;
                        }

                if (v > 0)
                        verbose(">>> tuning status == 0x%02x\n", s);

                if (s & FE_HAS_LOCK) {
                        t->last_tuning_failed = 0;
                        return 0;
                        }
                }

        if (v > 0)
                info("----------no signal----------\n");
        else 
                info("\n");

        t->last_tuning_failed = 1;

        return -1;
}

static int tune_to_transponder (int frontend_fd, struct transponder *t) {

        int res;
        /* move TP from "new" to "scanned" list */
        list_del_init(&t->list);
        list_add_tail(&t->list, &scanned_transponders);
        t->scan_done = 1;

        if (t->type != fe_info.type) {
                /* ignore cable descriptors in sat NIT and vice versa */
                t->last_tuning_failed = 1;
                return -1;
        }

        res = __tune_to_transponder (frontend_fd, t, 1);
        switch (res) {
                case 0:         return 0;
                case -1:        return __tune_to_transponder (frontend_fd, t, 1);
                case -2:        return -2;
                default:        return -1;
                }
}


static int tune_to_next_transponder (int frontend_fd)
{
        struct list_head *pos, *tmp;
        struct transponder *t;

        list_for_each_safe(pos, tmp, &new_transponders) {
                t = list_entry (pos, struct transponder, list);
retry:
                if (tune_to_transponder (frontend_fd, t) == 0)
                        return 0;
other_freq:
                if (t->other_frequency_flag &&
                                t->other_f &&
                                t->n_other_f) {
                        t->param.frequency = t->other_f[t->n_other_f - 1];
                        t->n_other_f--;
                        if (NULL == find_transponder(t,1)) {
                                info("retrying with f=%d\n", t->param.frequency);
                                goto retry;
                                }
                        goto other_freq;
                }
        }
        return -1;
}


static int check_frontend (int fd, int verbose) {
        fe_status_t status;
        ioctl(fd, FE_READ_STATUS, &status);
        if (verbose) {
                uint16_t snr, signal;
                uint32_t ber, uncorrected_blocks;

                ioctl(fd, FE_READ_SIGNAL_STRENGTH, &signal);
                ioctl(fd, FE_READ_SNR, &snr);
                ioctl(fd, FE_READ_BER, &ber);
                ioctl(fd, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks);
                info("signal %04x | snr %04x | ber %08x | unc %08x | ", \
                                                        signal, snr, ber, uncorrected_blocks);
                if (status & FE_HAS_LOCK)
                        info("FE_HAS_LOCK");
                info("\n");
                }
        return (status & FE_HAS_LOCK) > 0;
}

static unsigned int chan_to_freq(int channel, int channellist)
{
        moreverbose("channellist=%d, base_offset=%d, channel=%d, step=%d\n",
                channellist, base_offset(channel, channellist),
                channel, freq_step(channel, channellist));
        if (base_offset(channel, channellist) != -1) // -1 == invalid
                return base_offset(channel, channellist) +
                channel * freq_step(channel, channellist);
        return 0;
}


static int dvbc_modulation(int index)
{
        switch(index) {
                case 0:                 return QAM_64;
                case 1:                 return QAM_256;
                case 2:                 return QAM_128;                        
                default:                return QAM_AUTO;
                }
}

static int dvbc_symbolrate(int index)
{
        switch(index) { 
                // 8MHz, Rolloff 0.15 -> 8000000 / 1.15 -> symbolrate <= 6956521,74
                case 0:                 return 6900000;  // 8MHz, 6.900MSymbol/s is mostly used for 8MHz
                case 1:                 return 6875000;  // 8MHz, 6.875MSymbol/s also used quite often for 8MHz
                case 2:                 return 6956500;  // 8MHz
                case 3:                 return 6956000;  // 8MHz
                case 4:                 return 6952000;  // 8MHz
                case 5:                 return 6950000;  // 8MHz
                case 6:                 return 6790000;  // 8MHz
                case 7:                 return 6811000;  // 8MHz
                case 8:                 return 6250000;  // 8MHz
                case 9:                 return 6111000;  // 8MHz

                // 7MHz, Rolloff 0.15 -> 7000000 / 1.15 -> symbolrate <= 6086956,52
                case 10:                return 6086000;  // 8MHz, 7MHz, sort 7MHz descending by probability
                case 11:                return 5900000;  // 8MHz, 7MHz
                case 12:                return 5483000;  // 8MHz, 7MHz

                // 6MHz, Rolloff 0.15 -> 6000000 / 1.15 -> symbolrate <= 5217391,30
                case 13:                return 5217000;  // 6MHz, 7MHz, 8MHz, sort 6MHz descending by probability
                case 14:                return 5156000;  // 6MHz, 7MHz, 8MHz
                case 15:                return 5000000;  // 6MHz, 7MHz, 8MHz
                case 16:                return 4000000;  // 6MHz, 7MHz, 8MHz
                case 17:                return 3450000;  // 6MHz, 7MHz, 8MHz

                default:                return 0;
                }
}

/* called during scan loop. scans an successful tuned new transponder's
 * network information table for update of its transponder data as well as
 * other transponders announced here.
 * TODO: do similar with ATSC. mk, can you add pls?
 */
static void scan_for_other_transponders (void) {
        struct section_buf s0;
        struct section_buf s1;

        setup_filter (&s0, demux_devname, PID_NIT_ST, TABLE_NIT_ACT, -1, 1, 0);
        add_filter (&s0);
        setup_filter (&s1, demux_devname, PID_NIT_ST, TABLE_NIT_OTH, -1, 1, 1);
        add_filter (&s1);
        
        do      {
                read_filters();
                }                
        while (!(list_empty(&running_filters) && list_empty(&waiting_filters)));
}

static int initial_tune (int frontend_fd, int tuning_data)
{
uint32_t f = 0, channel, cnt, ret = 0, mod_parm, sr_parm, this_sr=0, offs;
uint16_t channel_max = 133;
struct transponder *t = NULL, *ptest;
struct transponder test;
char buffer[60];
ptest=&test;
memset(&test, 0, sizeof(test));

if (tuning_data <= 0) {

/* ---- w_scan blindscan loop ----
 *  DVB-T       : changed 20090101 -wk
 *  DVB-C       : changed 20090101 -wk
 *  DVB-S(2)    : changed 20090422 -wk
 * --
 *  ATSC part   : introduced 20080815 by mkrufky
 *                improved and Taiwan support 20081229 by mkrufky
 *                -> strongly adapted version 20090101  by wk
 */


//do last things before starting scan loop
switch (fe_info.type) {
        case FE_ATSC:
                switch(ATSC_type) {
                        case ATSC_VSB:
                                modulation_min=modulation_max=ATSC_VSB;
                                break;
                        case ATSC_QAM:
                                modulation_min=modulation_max=ATSC_QAM;
                                break;
                        default:
                                modulation_min=ATSC_VSB;
                                modulation_max=ATSC_QAM;
                                break;
                        }
                // disable symbolrate loop
                dvbc_symbolrate_min=dvbc_symbolrate_max=0;
                break;
        case FE_OFDM:
                // disable qam loop, disable symbolrate loop
                modulation_min=modulation_max=0;
                dvbc_symbolrate_min=dvbc_symbolrate_max=0;
                break;
        case FE_QAM:
                // if choosen srate is too high for channellist's bandwidth,
                // fall back to scan all srates. scan loop will skip unsupported srates later.
                if (dvbc_symbolrate(dvbc_symbolrate_min) > max_dvbc_srate(freq_step(0, this_channellist))) {
                        dvbc_symbolrate_min=0;
                        dvbc_symbolrate_max=17;
                        }
                break;
        case FE_QPSK:
                // channel means here: transponder,
                // last channel == (item_count - 1) since we're counting from 0
                channel_max = sat_list[this_channellist].item_count - 1;
                // disable qam loop
                modulation_min=modulation_max=0;
                // disable symbolrate loop
                dvbc_symbolrate_min=dvbc_symbolrate_max=0;
                // disable freq offset loop
                freq_offset_min=freq_offset_max=0;
                break;                
        default:warning("unsupported delivery system %d.\n", fe_info.type);
        }

/* ATSC VSB, ATSC QAM, DVB-T, DVB-C, DVB-S(2) here,
 * please change freqs inside country.c for ATSC, DVB-T, DVB-C
 * and inside satellites.c for DVB-S(2)
 */

for (mod_parm = modulation_min; mod_parm <= modulation_max; mod_parm++) {
   for (channel=0; channel <= channel_max; channel++) {
      for (offs = freq_offset_min; offs <= freq_offset_max; offs++)
            for (sr_parm = dvbc_symbolrate_min; sr_parm <= dvbc_symbolrate_max; sr_parm++) {                
                test.type = fe_info.type;
                switch (test.type) {
                        case FE_OFDM:
                                f = chan_to_freq(channel, this_channellist);
                                if (! f) continue; //skip unused channels
                                if (freq_offset(channel, this_channellist, offs) == -1)
                                        continue; //skip this one
                                f += freq_offset(channel, this_channellist, offs);                
                                if (test.param.u.ofdm.bandwidth != (fe_bandwidth_t) bandwidth(channel, this_channellist))
                                        info("Scanning %sMHz frequencies...\n",
                                        vdr_bandwidth_name(bandwidth(channel, this_channellist)));
                                test.param.frequency                    = f;
                                test.param.inversion                    = caps_inversion;
                                test.param.u.ofdm.bandwidth             = bandwidth(channel, this_channellist);
                                test.param.u.ofdm.code_rate_HP          = caps_fec;
                                test.param.u.ofdm.code_rate_LP          = caps_fec;
                                test.param.u.ofdm.constellation         = caps_qam;
                                test.param.u.ofdm.transmission_mode     = caps_transmission_mode;
                                test.param.u.ofdm.guard_interval        = caps_guard_interval;
                                test.param.u.ofdm.hierarchy_information = caps_hierarchy;
                                if (is_known_initial_transponder(&test,0)) {
                                        info("%d: skipped (already known transponder)\n", f/1000);
                                        continue;
                                        }
                                info("%d: ", f/1000);
                                break;
                        case FE_ATSC:
                                switch (mod_parm) {
                                        case ATSC_VSB:
                                                this_atsc = VSB_8;
                                                f = chan_to_freq(channel, ATSC_VSB);
                                                if (! f)
                                                        continue; //skip unused channels
                                                if (freq_offset(channel, ATSC_VSB, offs) == -1)
                                                        continue; //skip this one
                                                f += freq_offset(channel, ATSC_VSB, offs);
                                                break;
                                        case ATSC_QAM:
                                                this_atsc = QAM_256;
                                                f = chan_to_freq(channel, ATSC_QAM);
                                                if (! f)
                                                        continue; //skip unused channels
                                                if (freq_offset(channel, ATSC_QAM, offs) == -1)
                                                        continue; //skip this one
                                                f += freq_offset(channel, ATSC_QAM, offs);
                                                break;
                                        default: fatal("unknown modulation id\n");
                                        }
                                test.param.frequency            = f;
                                test.param.inversion            = caps_inversion;
                                test.param.u.vsb.modulation     = this_atsc;
                                if (is_known_initial_transponder(&test,0)) {
                                        info("%d %s: skipped (already known transponder)\n", f/1000, atsc_mod_to_txt(this_atsc));
                                        continue;
                                        }
                                info("%d: %s", f/1000, atsc_mod_to_txt(this_atsc));
                                break;
                        case FE_QAM:
                                f = chan_to_freq(channel, this_channellist);
                                if (! f)
                                        continue; //skip unused channels
                                if (freq_offset(channel, this_channellist, offs) == -1)
                                        continue; //skip this one
                                f += freq_offset(channel, this_channellist, offs);
                                this_sr = dvbc_symbolrate(sr_parm);
                                if (this_sr > (uint32_t) max_dvbc_srate(freq_step(channel, this_channellist)))
                                        continue; //skip symbol rates higher than theoretical limit given by bw && roll_off
                                this_qam = caps_qam;
                                if (flags.qam_no_auto > 0) {
                                        this_qam = dvbc_modulation(mod_parm);
                                        if (test.param.u.qam.modulation != this_qam)
                                                info ("searching QAM%s...\n", vdr_modulation_name(this_qam));
                                        }
                                test.param.inversion            = caps_inversion;
                                test.param.u.qam.modulation     = this_qam;
                                test.param.u.qam.symbol_rate    = this_sr;
                                test.param.u.qam.fec_inner      = caps_fec;
                                if (f != test.param.frequency) {
                                        test.param.frequency = f;
                                        if (is_known_initial_transponder(&test,0)) {
                                                info("%d: skipped (already known transponder)\n", f/1000);
                                                continue;
                                                }
                                        info("%d: sr%d ",f/1000 , this_sr/1000); 
                                        }
                                else {
                                        if (is_known_initial_transponder(&test,0))
                                                continue;
                                        info("sr%d ", this_sr/1000);
                                        }
                                break;
                        case FE_QPSK:
                                test.param.inversion                    = caps_inversion;
                                test.param.frequency                    = sat_list[this_channellist].items[channel].intermediate_frequency * 1000;
                                test.param.u.qpsk.symbol_rate           = sat_list[this_channellist].items[channel].symbol_rate * 1000;
                                test.param.u.qpsk.fec_inner             = sat_list[this_channellist].items[channel].fec_inner;
                                test.param.u.qpsk.modulation_type       = sat_list[this_channellist].items[channel].modulation_type;
                                test.param.u.qpsk.pilot                 = PILOT_AUTO;
                                test.param.u.qpsk.rolloff               = sat_list[this_channellist].items[channel].rolloff;
                                test.param.u.qpsk.modulation_system     = sat_list[this_channellist].items[channel].modulation_system;
                                test.param.u.qpsk.polarization          = sat_list[this_channellist].items[channel].polarization;
                                test.param.u.qpsk.orbital_position      = sat_list[this_channellist].orbital_position;
                                test.param.u.qpsk.west_east_flag        = sat_list[this_channellist].west_east_flag;
                                if (test.param.u.qpsk.modulation_system == SYS_DVBS2) {
                                        if (!(fe_info.caps & FE_CAN_2G_MODULATION) ||
                                             (flags.api_version < 0x0500)) {
                                                info("%d: skipped (no driver support)\n", test.param.frequency/1000);
                                                continue;
                                                }
                                        } 
                                if (is_known_initial_transponder(&test,0)) {
                                        info("%d: skipped (already known transponder)\n", test.param.frequency/1000);
                                        continue;
                                        }
                        default:;
                        }
                if (set_frontend(frontend_fd, ptest) < 0) {
                        print_transponder(buffer, ptest);
                        dprintf(1,"\n%s:%d: Setting frontend failed %s\n",
                                __FUNCTION__, __LINE__, buffer);
                        continue;
                        }
                usleep (1000000);
                for (cnt=0;cnt<10;cnt++) {
                        ret = check_frontend(frontend_fd,0);
                        if (ret == 1) break;
                        usleep(150000);
                        }
                if (ret == 0) {
                        if (sr_parm == dvbc_symbolrate_max)
                                info("\n");
                        continue;
                        }
                if (__tune_to_transponder (frontend_fd, ptest,0) < 0)
                        continue;
                t = alloc_transponder(f);
                t->type = ptest->type;
                copy_fe_params(&t->param, &ptest->param);
                print_transponder(buffer, t);
                info("signal ok:\n\t%s\n", buffer);
                switch (ptest->type) {
                        case FE_ATSC:
                                //scan_for_other_transponders(); // would this work here? Don't know, need Info!
                                break;
                        default:
                                scan_for_other_transponders(); // speed up scan NITs and later skipping known transponders.
                                break;
                        }
                break;
                }
            }        
        }

}
else {  /* ---- use initial tuning data from dvbscan ---- */
        struct list_head *pos;
        struct transponder *tp;
        info("updating transponder list..\n");
        /* tune to each channel provided and update it from
         * network information table. In parallel scan for
         * other transponders provided by NIT actual and NIT other.
         */
        list_for_each(pos, &new_transponders) {
                tp = list_entry(pos, struct transponder, list);
                print_transponder(buffer, tp);

                switch (flags.fe_type) {
                        case FE_QPSK:
                                if (tp->param.u.qpsk.modulation_system == SYS_DVBS2) {
                                        if (!(fe_info.caps & FE_CAN_2G_MODULATION) ||
                                            (flags.api_version < 0x0500)) {
                                                info("%s: skipped (no driver support)\n", buffer);
                                                continue;
                                                }
                                        }
                                break;
                        // may be later checks for C2, T2 needed.
                        case FE_OFDM:;
                        case FE_QAM:;
                        case FE_ATSC:;
                        default:;
                        }

                info("%s: ", buffer);
                if (set_frontend(frontend_fd, tp) < 0) {
                        print_transponder(buffer, tp);
                        dprintf(1,"\n%s:%d: Setting frontend failed %s\n",
                                __FUNCTION__, __LINE__, buffer);
                        continue;
                        }
                usleep (1500000);
                for (cnt=0; cnt<5; cnt++) {
                        if (check_frontend(frontend_fd, 0) == 1)
                                break;
                        usleep(200000);
                        }
                if (__tune_to_transponder (frontend_fd, tp, 0) >= 0) {
                        info("signal ok\n");
                        scan_for_other_transponders();
                        }
                else
                        info("\n");
                }
        }
/* we should now have here a list of well known transponders. Iterate a second time
 * and scan it's PAT, PMT, SDT for services. In parallel NIT actual and NIT other.
 */
return tune_to_next_transponder(frontend_fd);
}

static void scan_tp_atsc(void)
{
        struct section_buf s0,s1,s2;

        if (no_ATSC_PSIP > 0) {
                setup_filter(&s0, demux_devname, PID_PAT, TABLE_PAT, -1, 1, 0); /* PAT */
                add_filter(&s0);
        } else {
                if (atsc_is_vsb(ATSC_type)) {
                        setup_filter(&s0, demux_devname, PID_VCT, TABLE_VCT_TERR, -1, 1, 0); /* terrestrial VCT */
                        add_filter(&s0);
                }
                if (atsc_is_qam(ATSC_type)) {
                        setup_filter(&s1, demux_devname, PID_VCT, TABLE_VCT_CABLE, -1, 1, 0); /* cable VCT */
                        add_filter(&s1);
                }
                setup_filter(&s2, demux_devname, PID_PAT, TABLE_PAT, -1, 1, 0); /* PAT */
                add_filter(&s2);
        }

        do {
                read_filters ();
        } while (!(list_empty(&running_filters) &&
                   list_empty(&waiting_filters)));
}

static void scan_tp_dvb (void)
{
        struct section_buf s0;
        struct section_buf s1;
        struct section_buf s2;
        struct section_buf s3;

        setup_filter (&s0, demux_devname, PID_PAT, TABLE_PAT, -1, 1, 0);
        setup_filter (&s1, demux_devname, PID_SDT_BAT_ST, TABLE_SDT_ACT, -1, 1, 0);
        setup_filter (&s2, demux_devname, PID_NIT_ST, TABLE_NIT_ACT, -1, 1, 0);

        add_filter (&s0);
        add_filter (&s1);
        add_filter (&s2);

        if (flags.get_other_nits > 0) {
           /* Note: There is more than one NIT-other: one per
            * network, separated by the network_id. */
           setup_filter (&s3, demux_devname, PID_NIT_ST, TABLE_NIT_OTH, -1, 1, 1);
           add_filter (&s3);
        }


        do {
                read_filters ();
        } while (!(list_empty(&running_filters) &&
                   list_empty(&waiting_filters)));
}

static void scan_tp(void)
{
        switch(fe_info.type) {
                case FE_QPSK:
                case FE_QAM:
                case FE_OFDM:
                        scan_tp_dvb();
                        break;
                case FE_ATSC:
                        scan_tp_atsc();
                        break;
                default:
                        warning("unimplemented frontend type %d.\n", fe_info.type);
        }
}

static void network_scan (int frontend_fd, int tuning_data) {
        if (initial_tune (frontend_fd, tuning_data) < 0) {
                error("Sorry - i couldn't get any working frequency/transponder\n Nothing to scan!!\n");
                exit(1);
                }
        do {
                scan_tp();
        } while (tune_to_next_transponder(frontend_fd) == 0);
}

int device_is_preferred(int caps, const char * frontend_name) {
        int preferred = 1; // no preferrence
        /* add other good/bad cards here. */
        if (strncmp("VLSI VES1820", frontend_name, 12) == 0)
                /* bad working FF dvb-c card, known to have qam256 probs. */
                preferred = 0; // not preferred
        if (caps & FE_CAN_2G_MODULATION)
                /* w_scan preferres devices which are DVB-{S,C,T}2 */
                preferred = 2; // preferred
        return preferred;        
}

int get_api_version(int frontend_fd, struct w_scan_flags * flags) {

        struct dtv_property p[] = {{.cmd = DTV_API_VERSION }};
        struct dtv_properties cmdseq = {.num = 1, .props = p};

        /* expected to fail with old drivers,
         * therefore no warning to user. 20090324 -wk
         */
        if (ioctl(frontend_fd, FE_GET_PROPERTY, &cmdseq))
                return -1;

        flags->api_version = p[0].u.data;
        return 0;
}


static void dump_lists (int adapter, int frontend)
{
        struct list_head *p1, *p2;
        struct transponder *t;
        struct service *s;
        int n = 0, i, index = 0;
        char sn[20];

        list_for_each(p1, &scanned_transponders) {
                t = list_entry(p1, struct transponder, list);
                list_for_each(p2, &t->services) {
                        s = list_entry(p2, struct service, list);
                        if (s->video_pid && !(serv_select & 1))
                                continue; /* no TV services */
                        if (!s->video_pid &&  (s->audio_num || s->ac3_num) && !(serv_select & 2))
                                continue; /* no radio services */
                        if (!s->video_pid && !(s->audio_num || s->ac3_num) && !(serv_select & 4))
                                continue; /* no data/other services */
                        if (s->scrambled && (flags.ca_select == 0))
                                continue; /* FTA only */
                        n++;
                }
        }
        info("dumping lists (%d services)\n", n);

        switch (output_format) {
                case OUTPUT_VLC_M3U:
                        vlc_xspf_prolog(stdout, adapter, frontend, &flags, &this_lnb);
                        break;
                default:;
                }

        list_for_each(p1, &scanned_transponders) {
                t = list_entry(p1, struct transponder, list);
                if (output_format == OUTPUT_DVBSCAN_TUNING_DATA) {
                        dvbscan_dump_tuningdata (stdout, &t->param, index++, t->network_name, &flags);
                        continue;
                        }                        
                list_for_each(p2, &t->services) {
                        s = list_entry(p2, struct service, list);

                        if (!s->service_name) { // no service name in SDT                                
                                snprintf(sn, sizeof(sn), "service_id %d", s->service_id);
                                s->service_name = strdup(sn);
                                }
                        /* ':' is field separator in vdr service lists */
                        for (i = 0; s->service_name[i]; i++) {
                                if (s->service_name[i] == ':')
                                        s->service_name[i] = ' ';
                        }
                        for (i = 0; s->provider_name && s->provider_name[i]; i++) {
                                if (s->provider_name[i] == ':')
                                        s->provider_name[i] = ' ';
                        }
                        if (s->video_pid && !(serv_select & 1))                                         // vpid, this is tv
                                continue; /* no TV services */
                        if (!s->video_pid &&  (s->audio_num || s->ac3_num) && !(serv_select & 2))       // no vpid, but apid or ac3pid, this is radio
                                continue; /* no radio services */
                        if (!s->video_pid && !(s->audio_num || s->ac3_num) && !(serv_select & 4))       // no vpid, no apid, no ac3pid, this is service/other
                                continue; /* no data/other services */
                        if (s->scrambled && (flags.ca_select == 0))                                     // caid, this is scrambled tv or radio
                                continue; /* FTA only */
                        switch (output_format) {
                          case OUTPUT_VDR:
                                vdr_dump_service_parameter_set(stdout, s, t, &flags);
                                break;
                          case OUTPUT_KAFFEINE:
                                kaffeine_dump_service_parameter_set(stdout, s, t, &flags);
                                break;
                          case OUTPUT_XINE:
                                xine_dump_service_parameter_set(stdout, s, t, &flags);
                                break;
                          case OUTPUT_MPLAYER:
                                mplayer_dump_service_parameter_set(stdout, s, t, &flags);
                                break;
                          case OUTPUT_VLC_M3U:
                                vlc_dump_service_parameter_set_as_xspf(stdout, s, t, &flags, &this_lnb);
                                break;
                          default:
                                break;
                          }
                }
        }
        switch (output_format) {
                case OUTPUT_VLC_M3U:
                        vlc_xspf_epilog(stdout);
                        break;
                default:;
                }
        info("Done.\n");
}

static void handle_sigint(int sig)
{
        error("interrupted by SIGINT, dumping partial result...\n");
        dump_lists(-1, -1);
        exit(2);
}

static const char *usage = "\n"
        "usage: %s [options...] \n"
        "       -f type frontend type\n"
        "               What programs do you want to search for?\n"
        "               a = atsc (vsb/qam)\n"
        "               c = cable \n"
        "               s = sat \n"
        "               t = terrestrian [default]\n"
        "       -A N    specify ATSC type\n"
        "               1 = Terrestrial [default]\n"
        "               2 = Cable\n"
        "               3 = both, Terrestrial and Cable\n"
        "       -c      choose your country here:\n"
        "                       DE, GB, US, AU, ..\n"
        "                       ? for list\n"
        "               \n"
        "       -s      choose your satellite here:\n"
        "                       S19E2, S13E0, S15W0, ..\n"
        "                       ? for list\n"
        "               ---output switches---\n"
        "       -G      generate channels.conf for dvbsrc plugin\n"
        "       -k      generate channels.dvb for kaffeine\n"
        "       -L      generate VLC xspf playlist (experimental)\n"
        "       -M      mplayer output instead of vdr channels.conf\n"
        "       -X      tzap/czap/xine output instead of vdr channels.conf\n"
        "       -x      generate initial tuning data for (dvb-)scan\n"
        "       -H      view extended help (experts only)\n";


static const char *ext_opts = "%s expert help\n"
        ".................General.................\n"
        "       -C <charset>\n"
        "               convert to charset, i.e. 'UTF-8', 'ISO-8859-15'\n"
        "               use 'iconv --list' for full list of charsets.\n"
        "       -I <file>\n"
        "               scan using dvbscan initial_tuning_data\n"
        "       -v      verbose (repeat for more)\n"
        "       -q      quiet   (repeat for less)\n"
        ".................Services................\n"
        "       -R N    radio channels\n"
        "               0 = don't search radio channels\n"
        "               1 = search radio channels [default]\n"
        "       -T N    TV channels\n"
        "               0 = don't search TV channels\n"
        "               1 = search TV channels[default]\n"
        "       -O N    Other Services\n"
        "               0 = don't search other services [default]\n"
        "               1 = search other services\n"
        "       -E N    Conditional Access (encrypted channels)\n"
        "               N=0 gets only Free TV channels\n"
        "               N=1 search also encrypted channels [default]\n"
        "       -o N    VDR version / channels.conf format\n"
        "               4 = VDR-1.4.x (depreciated)\n"
        "               6 = VDR-1.6.x (default)\n"
        "               7 = VDR-1.7.x\n"
        ".................Device..................\n"
        "       -a N    use device /dev/dvb/adapterN/ [default: auto detect]\n"
        "               (also allowed: -a /dev/dvb/adapterN/frontendM)\n"
        "       -F      use long filter timeout\n"
        "       -t N    tuning timeout\n"
        "               1 = fastest [default]\n"
        "               2 = medium\n"
        "               3 = slowest\n"
        ".................DVB-C...................\n"
        "       -i N    spectral inversion setting for cable TV\n"
        "                       (0: off, 1: on, 2: auto [default])\n"
        "       -Q      set DVB-C modulation, see table:\n"
        "                       0  = QAM64\n"
        "                       1  = QAM256\n"
        "                       2  = QAM128\n"
        "               NOTE: for experienced users only!!\n"
        "       -e      extended scan flags (DVB-C only),\n"
        "               Any combination of these flags:\n"
        "               1 = use extended symbolrate list\n"
        "                       enables scan of symbolrates\n"
        "                       6111, 6250, 6790, 6811, 5900,\n"
        "                       5000, 3450, 4000, 6950, 7000,\n"
        "                       6952, 6956, 6956.5, 5217\n"
        "               2 = extended QAM scan (enable QAM128)\n"
        "                       recommended for Nethterlands and Finland\n"
        "               NOTE: extended scan will be *slow*\n"
        "       -S      set DVB-C symbol rate, see table:\n"
        "                       0  = 6.9000 MSymbol/s\n"
        "                       1  = 6.8750 MSymbol/s\n"
        "                       2  = 6.9565 MSymbol/s\n"
        "                       3  = 6.9560 MSymbol/s\n"
        "                       4  = 6.9520 MSymbol/s\n"
        "                       5  = 6.9500 MSymbol/s\n"
        "                       6  = 6.7900 MSymbol/s\n"
        "                       7  = 6.8110 MSymbol/s\n"
        "                       8  = 6.2500 MSymbol/s\n"
        "                       9  = 6.1110 MSymbol/s\n"
        "                       10 = 6.0860 MSymbol/s\n"
        "                       11 = 5.9000 MSymbol/s\n"
        "                       12 = 5.4830 MSymbol/s\n"
        "                       13 = 5.2170 MSymbol/s\n"
        "                       14 = 5.1560 MSymbol/s\n"
        "                       15 = 5.0000 MSymbol/s\n"
        "                       16 = 4.0000 MSymbol/s\n"
        "                       17 = 3.4500 MSymbol/s\n"
        "               NOTE: for experienced users only!!\n"
        ".................DVB-S/S2................\n"
        "       -l <LNB type>\n"
        "               choose LNB type by name (DVB-S/S2 only)\n"
        "                       ? for list\n"
        "       -D Nc   use DiSEqC committed switch position N\n"
        "       -D Nu   use DiSEqC uncommitted switch position N\n"
        "       -p <file>\n"
        "               use DiSEqC rotor Position file\n"
        "       -r N use Rotor position N (needs -s)\n"
        ".................ATSC....................\n"
        "       -P      do not use ATSC PSIP tables for scanning\n"
        "               (but only PAT and PMT) (applies for ATSC only)\n";


void bad_usage(char *pname)
{
                fprintf (stderr, usage, pname);

}

void ext_help(void)
{
                fprintf (stderr, ext_opts, "w_scan");

}

#define MOD_USE_STANDARD  0x0
#define MOD_OVERRIDE_MIN  0x1
#define MOD_OVERRIDE_MAX  0x2

#define cl(x)  if (x) { free(x); x=NULL; }  

int w_scan_main (int argc, char **argv)
{
        char frontend_devname [80];
        int adapter = 999, frontend = 0, demux = 0;
        int opt;
        unsigned int i = 0, j;
        int frontend_fd = -1;
        int fe_open_mode;
        uint16_t frontend_type = FE_OFDM;
        int Radio_Services = 1;
        int TV_Services = 1;
        int Other_Services = 0; // 20080106: don't search other services by default.
        int ext = 0;
        int retVersion = 0;
        int discover = 0;
        int a=0,c=0,s=0,t=0;
        int device_preferred = -1;
        int valid_initial_data = 0;
        int valid_rotor_data = 0;
        int modulation_flags = MOD_USE_STANDARD;
        char * country = NULL;
        char * codepage = NULL;
        char * satellite = NULL;
        char * initdata = NULL;
        char * positionfile = NULL;
        char sw_type = 0;

        #define cleanup() cl(country); cl(satellite); cl(initdata); cl(positionfile); cl(codepage);

        this_lnb = * lnb_enum(0);
        this_lnb.low_val *= 1000;
        this_lnb.high_val *= 1000;
        this_lnb.switch_val *= 1000;

        flags.version = version;
        start_time = time(NULL);

        while ((opt = getopt(argc, argv, "a:c:e:f:hi:kl:o:p:qr:s:t:vxA:C:D:E:FGHI:LMO:PQ:R:S:T:VX")) != -1) {
                switch (opt) {
                case 'a': //adapter
                        if (sscanf(optarg, "%d", &adapter) < 1)
                                if (sscanf(optarg, "/dev/dvb/adapter%d/frontend%d", &adapter, &frontend) != 2)
                                        adapter = 999, frontend = 0;
                        break;
                case 'c': //country setting
                        if (0 == strcasecmp(optarg, "?")) {
                                print_countries();
                                cleanup();
                                return(0);
                                }
                        cl(country);
                        country=strdup(optarg);
                        break;
                case 'e': //extended scan flags
                        ext = strtoul(optarg, NULL, 0);
                        if (ext & 0x01)
                                dvbc_symbolrate_max = 17;
                        if (ext & 0x02) {
                                modulation_max = 2;
                                modulation_flags |= MOD_OVERRIDE_MAX;
                                }
                        break;
                case 'f': //frontend type
                        if (strcmp(optarg, "t") == 0) frontend_type = FE_OFDM;
                        if (strcmp(optarg, "c") == 0) frontend_type = FE_QAM;
                        if (strcmp(optarg, "a") == 0) frontend_type = FE_ATSC;
                        if (strcmp(optarg, "s") == 0) frontend_type = FE_QPSK;
                        if (strcmp(optarg, "?") == 0) discover++;
                        if (frontend_type == FE_ATSC) {
                                this_channellist = ATSC_VSB;
                                country = strdup("US");
                                }
                        break;
                case 'h': //basic help
                        bad_usage("w_scan");
                        cleanup();
                        return 0;
                        break;
                case 'i': //specify inversion
                        caps_inversion = strtoul(optarg, NULL, 0);
                        break;
                case 'k': //kaffeine output
                        output_format = OUTPUT_KAFFEINE;
                        break;
                case 'l': //satellite lnb type
                        if (strcmp(optarg, "?") == 0) {
                                struct lnb_types_st * p;
                                char ** cp;

                                while((p = lnb_enum(i++))) {
                                        info("%s\n", p->name);
                                        for (cp = p->desc; *cp;)
                                                info("\t%s\n", *cp++);
                                        }
                                cleanup();
                                return 0;
                                }
                        if (lnb_decode(optarg, &this_lnb) < 0) {
                                cleanup();
                                fatal("LNB decoding failed. Use \"-l ?\" for list.\n");
                                }
                        /* MHz -> kHz */
                        this_lnb.low_val        *= 1000;
                        this_lnb.high_val       *= 1000;
                        this_lnb.switch_val     *= 1000;
                        break;
                case 'o': //vdr Version
                        flags.vdr_version = strtoul(optarg, NULL, 0);
                        if (flags.vdr_version > 2) flags.dump_provider = 1;
                        break;
                case 'p': //satellite *p*osition file
                        positionfile=strdup(optarg);
                        break;
                case 'q': //quite
                        if (--verbosity < 0)
                                verbosity = 0;
                        break;
                case 'r': //satellite rotor position
                        flags.rotor_position = strtoul(optarg, NULL, 0);
                        break;
                case 's': //satellite setting
                        if (0 == strcasecmp(optarg, "?")) {
                                print_satellites();
                                cleanup();
                                return(0);
                                }
                        satellite=strdup(optarg);
                        break;
                case 't': //tuning speed
                        flags.tuning_timeout = strtoul(optarg, NULL, 0);
                        if ((flags.tuning_timeout < 1)) bad_usage(argv[0]);
                        if ((flags.tuning_timeout > 3)) bad_usage(argv[0]);
                        break;
                case 'v': //verbose
                        verbosity++;
                        break;
                case 'x': //dvbscan output
                        output_format = OUTPUT_DVBSCAN_TUNING_DATA;
                        break;
                case 'A': //ATSC type
                        ATSC_type = strtoul(optarg,NULL,0);
                        switch (ATSC_type) {
                          case 1: ATSC_type = ATSC_VSB; break;
                          case 2: ATSC_type = ATSC_QAM; break;
                          case 3: ATSC_type = (ATSC_VSB + ATSC_QAM); break;
                          default:
                            cleanup();
                            bad_usage(argv[0]);
                            return -1;
                          }
                        /* if -A is specified, it implies -f a */
                        frontend_type = FE_ATSC;
                        break;
                case 'C': // charset
                        codepage = strdup(optarg);
                        break;
                case 'D': //DiSEqC committed/uncommitted switch
                        sscanf(optarg,"%u%c", &i, &sw_type);
                        switch(sw_type) {
                                case 'u':
                                        uncommitted_switch = i;
                                        if (uncommitted_switch > 15)
                                                fatal("uncommitted switch position needs to be < 16!\n");
                                        flags.sw_pos = (flags.sw_pos & 0xF) | uncommitted_switch;
                                        break;
                                case 'c':
                                        committed_switch = i;
                                        if (committed_switch > 3)
                                                fatal("committed switch position needs to be < 4!\n");
                                        flags.sw_pos = (flags.sw_pos & 0xF0) | committed_switch;
                                        break;
                                default:
                                        cleanup();
                                        fatal("Could not parse Argument \"-D\"\n"
                                              "Should be number followed \"u\" or \"c\"\n");
                                }        
                        break;
                case 'E': //include encrypted channels
                        flags.ca_select = strtoul(optarg, NULL, 0);
                        break;
                case 'F': //filter timeout
                        flags.filter_timeout = 1;
                        break;
                case 'G':
                        output_format = OUTPUT_GSTREAMER;
                        break;
                case 'H': //expert help
                        ext_help();
                        cleanup();
                        return 0;
                        break;
                case 'I': //expert providing initial_tuning_data
                        initdata=strdup(optarg);
                        break;
                case 'L': //vlc output
                        output_format = OUTPUT_VLC_M3U;
                        break;
                case 'M': //mplayer output
                        output_format = OUTPUT_MPLAYER;
                        break;
                case 'O': //other services
                        Other_Services = strtoul(optarg, NULL, 0);
                        if ((Other_Services < 0)) bad_usage(argv[0]);
                        if ((Other_Services > 1)) bad_usage(argv[0]);
                        break;
                case 'P': //ATSC PSIP scan
                        no_ATSC_PSIP = 1;
                        break;
                case 'Q': //specify DVB-C QAM
                        modulation_min=modulation_max=strtoul(optarg, NULL, 0);
                        modulation_flags |= MOD_OVERRIDE_MIN;
                        modulation_flags |= MOD_OVERRIDE_MAX;
                        break;
                case 'R': //include Radio
                        Radio_Services = strtoul(optarg, NULL, 0);
                        if ((Radio_Services < 0)) bad_usage(argv[0]);
                        if ((Radio_Services > 1)) bad_usage(argv[0]);
                        break;
                case 'S': //DVB-C symbolrate index
                        dvbc_symbolrate_min=dvbc_symbolrate_max=strtoul(optarg, NULL, 0);
                        break;
                case 'T': //include TV
                        TV_Services = strtoul(optarg, NULL, 0);
                        if ((TV_Services < 0)) bad_usage(argv[0]);
                        if ((TV_Services > 1)) bad_usage(argv[0]);
                        break;
                case 'V': //Version
                        retVersion++;
                        break;
                case 'X': //xine output
                        output_format = OUTPUT_XINE;
                        break;
                default: //undefined
                        cleanup();
                        bad_usage(argv[0]);
                        return -1;
                }
        }
        if (retVersion) {
                info ("%d", version);
                cleanup();
                return 0;
                }
        if (discover) {
                discover=0;
                fe_open_mode = O_RDWR | O_NONBLOCK;
                for (i=0; i < 8; i++) {
                  snprintf (frontend_devname, sizeof(frontend_devname), "/dev/dvb/adapter%i/frontend0", i);
                  if ((frontend_fd = open (frontend_devname, fe_open_mode)) < 0)
                        continue;
                  if (ioctl(frontend_fd, FE_GET_INFO, &fe_info) == -1) {
                        close (frontend_fd);
                        continue;
                        }                  
                  switch(fe_info.type) {
                        case FE_OFDM: t++; break;
                        case FE_QAM:  c++; break;
                        case FE_QPSK: s++; break;
                        case FE_ATSC: a++; break;
                        default:;
                        }
                  close (frontend_fd);
                  }
                info ("%2d%2d%2d%2d\n", t, c, a, s);
                cleanup();
                return 0;
                }
        info("w_scan version %d (compiled for DVB API %d.%d)\n", version, DVB_API_VERSION, DVB_API_VERSION_MINOR);
        if ((frontend_type == FE_ATSC) && (output_format == OUTPUT_VDR) && (flags.vdr_version < 7)) {
                warning("VDR up to version 1.7.13 doesn't support ATSC.\n"
                     "\tChanging output format to 'vdr-1.7.x'\n");
                flags.vdr_version = 7;
                }
        if (NULL == initdata) {
                if ((NULL == country) && (frontend_type != FE_QPSK)) {
                        country = strdup(country_to_short_name(get_user_country()));
                        info("guessing country '%s', use -c <country> to override\n", country);
                        }
                if ((NULL == satellite) && (frontend_type == FE_QPSK)) {
                        cleanup();
                        fatal("Missing argument \"-s\" (satellite setting)\n");
                        }                
                }
        serv_select = 1 * TV_Services + 2 * Radio_Services + 4 * Other_Services;
        if  (caps_inversion > INVERSION_AUTO) {
                info("Inversion out of range!\n");
                bad_usage(argv[0]);
                cleanup();
                return -1;
                }
        if  (((adapter > 7) && (adapter != 999)) || (adapter < 0)) {
                info("Invalid adapter: out of range (0..7)\n");
                bad_usage(argv[0]);
                cleanup();
                return -1;
                }
        switch(frontend_type) {
                case FE_ATSC:
                case FE_QAM:
                case FE_OFDM:
                        if (country != NULL) {
                                int atsc = ATSC_type;
                                int dvb  = frontend_type;
                                flags.atsc_type = ATSC_type;
                                choose_country(country, &atsc, &dvb, &frontend_type, &this_channellist);
                                //dvbc: setting qam loop
                                if ((modulation_flags & MOD_OVERRIDE_MAX) == MOD_USE_STANDARD)
                                        modulation_max = dvbc_qam_max(2, this_channellist);
                                if ((modulation_flags & MOD_OVERRIDE_MIN) == MOD_USE_STANDARD)
                                        modulation_min = dvbc_qam_min(2, this_channellist);
                                flags.list_id = txt_to_country(country);
                                cl(country);
                                }
                        break;
                case FE_QPSK:
                        if (satellite != NULL) {
                                choose_satellite(satellite, &this_channellist);                                
                                flags.list_id = txt_to_satellite(satellite);
                                cl(satellite);
                                sat_list[this_channellist].rotor_position = flags.rotor_position;
                                }
                        else if (flags.rotor_position > -1) {
                                        cleanup();
                                        fatal("Using rotor position needs option \"-s\"\n");
                                        }
                        if (positionfile != NULL) {
                                valid_rotor_data = dvbscan_parse_rotor_positions(positionfile);
                                cl(positionfile);
                                if (! valid_rotor_data) {
                                        cleanup();
                                        fatal("could not parse rotor position file\n"
                                              "CHECK IDENTIFIERS AND FILE FORMAT.\n");
                                        }
                                }
                        break;
                default:
                        cleanup();
                        fatal("Unknown frontend type %d\n", frontend_type);
                }

        if (initdata != NULL) {
                valid_initial_data = dvbscan_parse_tuningdata(initdata, &flags);
                cl(initdata);
                if (valid_initial_data == 0) {
                        cleanup();
                        fatal("Could not read initial tuning data. EXITING.\n");
                        }
                if (flags.fe_type != frontend_type) {
                        warning("\n"
                                "========================================================================\n"
                                "INITIAL TUNING DATA NEEDS FRONTEND TYPE %s, YOU SELECTED TYPE %s.\n"
                                "I WILL OVERRIDE YOUR DEFAULTS TO %s\n"
                                "========================================================================\n",
                                frontend_type_to_text(flags.fe_type),
                                frontend_type_to_text(frontend_type),
                                frontend_type_to_text(flags.fe_type));
                        frontend_type = flags.fe_type;                        
                        sleep(10); // enshure that user reads warning.
                        }
                }
        info("frontend_type %s, channellist %d\n", frontend_type_to_text(frontend_type), this_channellist);
        switch (output_format) {
                case OUTPUT_VDR:
                        info("output format vdr-1.%d\n", flags.vdr_version);
                        break;
                case OUTPUT_GSTREAMER:
                        // Gstreamer output: As vdr-1.7, but pmt_pid added at end of line.
                        flags.print_pmt = 1;
                        flags.vdr_version = 7;
                        output_format = OUTPUT_VDR;
                        info("output format gstreamer\n");
                        break;
                case OUTPUT_KAFFEINE:
                        info("output format kaffeine channels.dvb\n"); 
                        break;
                case OUTPUT_XINE:
                        info("output format czap/tzap/szap/xine\n");
                        break;
                case OUTPUT_MPLAYER:
                        info("output format mplayer\n");
                        break;
                case OUTPUT_DVBSCAN_TUNING_DATA:
                        info("output format initial tuning data\n");
                        break;
                case OUTPUT_PIDS:
                        info("output format PIDs only\n");
                        break;
                case OUTPUT_VLC_M3U:
                        info("output format vlc xspf playlist\n");
                        break;
                default:
                        cleanup();
                        fatal("unhandled output format %d\n", output_format);
                }
        if (codepage) {
                flags.codepage = get_codepage_index(codepage);
                info("output charset '%s'\n", iconv_codes[flags.codepage]);
                }
        else {
                flags.codepage = get_user_codepage();
                info("output charset '%s', use -C <charset> to override\n", iconv_codes[flags.codepage]);
                }        
        if ( adapter == 999 ) {
                info("Info: using DVB adapter auto detection.\n");
                fe_open_mode = O_RDWR | O_NONBLOCK;
                for (i=0; i < 8; i++) {
                  for (j=0; j < 4; j++) {
                    snprintf (frontend_devname, sizeof(frontend_devname), "/dev/dvb/adapter%i/frontend%i", i, j);
                    if ((frontend_fd = open (frontend_devname, fe_open_mode)) < 0) {
                        continue;
                    /* determine FE type and caps */
                        }
                    if (ioctl(frontend_fd, FE_GET_INFO, &fe_info) == -1) {
                        info("   ERROR: unable to determine frontend type\n");
                        close (frontend_fd);
                        continue;
                        }
                    if (fe_info.type == frontend_type) {
                        info("\t%s -> %s \"%s\": ",
                                frontend_devname, frontend_type_to_text(frontend_type), fe_info.name);
                        if (device_is_preferred(fe_info.caps, fe_info.name) >= device_preferred) {
                                if (device_is_preferred(fe_info.caps, fe_info.name) > device_preferred) {
                                        device_preferred = device_is_preferred(fe_info.caps, fe_info.name);
                                        adapter=i;
                                        frontend=j;
                                        }
                                switch (device_preferred) {
                                        case 0: // device known to have probs. usable anyway..
                                                info("usable :-|\n");
                                                break;
                                        case 1: // device w/o problems
                                                info("good :-)\n");
                                                break;
                                        case 2: // perfect device found. stop scanning
                                                info("very good :-))\n\n");
                                                i=999;
                                                break;
                                        default:;
                                        }
                                }
                        else {
                                info("usable, but not preferred\n");
                                }
                        close (frontend_fd);
                        }
                     else {
                        info("\t%s -> %s \"%s\": specified was %s -> SEARCH NEXT ONE.\n",
                                frontend_devname,
                                frontend_type_to_text(fe_info.type),
                                fe_info.name,
                                frontend_type_to_text(frontend_type));
                        close (frontend_fd);
                        }
                    }
                }
                if (adapter < 999) {
                        snprintf (frontend_devname, sizeof(frontend_devname), "/dev/dvb/adapter%i/frontend%i", adapter, frontend);
                        info("Using %s frontend (adapter %s)\n",
                                frontend_type_to_text(frontend_type), frontend_devname);
                        }
        }
        snprintf (frontend_devname, sizeof(frontend_devname),
                  "/dev/dvb/adapter%i/frontend%i", adapter, frontend);

        snprintf (demux_devname, sizeof(demux_devname),
                  "/dev/dvb/adapter%i/demux%i", adapter, demux);

        for (i = 0; i < MAX_RUNNING; i++)
                poll_fds[i].fd = -1;

        fe_open_mode = O_RDWR;
        if (adapter == 999) {
                cleanup();
                fatal("***** NO USEABLE %s CARD FOUND. *****\n"
                        "Please check wether dvb driver is loaded and\n"
                        "verify that no dvb application (i.e. vdr) is running.\n",
                        frontend_type_to_text(frontend_type));
                }
        
        if ((frontend_fd = open (frontend_devname, fe_open_mode)) < 0) {
                cleanup();
                fatal("failed to open '%s': %d %s\n", frontend_devname, errno, strerror(errno));
                }
        info("-_-_-_-_ Getting frontend capabilities-_-_-_-_ \n");
        /* determine FE type and caps */
        if (ioctl(frontend_fd, FE_GET_INFO, &fe_info) == -1) {
                cleanup();
                fatal("FE_GET_INFO failed: %d %s\n", errno, strerror(errno));
                }
        flags.fe_type = fe_info.type;

        get_api_version(frontend_fd, &flags);
        info("Using DVB API %x.%x\n",
                flags.api_version >> 8,
                flags.api_version & 0xFF);

        info("frontend '%s' supports\n", fe_info.name && *fe_info.name?fe_info.name:"<NULL pointer>");

        switch (fe_info.type) {
           case FE_OFDM:
                if (fe_info.caps & FE_CAN_INVERSION_AUTO) {
                  info("INVERSION_AUTO\n");
                  caps_inversion=INVERSION_AUTO;
                  }
                else {
                  info("INVERSION_AUTO not supported, trying INVERSION_OFF.\n");
                  caps_inversion=INVERSION_OFF;
                  }
                if (fe_info.caps & FE_CAN_QAM_AUTO) {
                  info("QAM_AUTO\n");
                  caps_qam=QAM_AUTO;
                  }
                else {
                  info("QAM_AUTO not supported, trying QAM_64.\n");
                  caps_qam=QAM_64;
                  }
                if (fe_info.caps & FE_CAN_TRANSMISSION_MODE_AUTO) {
                  info("TRANSMISSION_MODE_AUTO\n");
                  caps_transmission_mode=TRANSMISSION_MODE_AUTO;
                  }
                else {
                  caps_transmission_mode=dvbt_transmission_mode(5, this_channellist);
                  info("TRANSMISSION_MODE not supported, trying %s.\n",
                        xine_transmission_mode_name(caps_transmission_mode));
                  }
                if (fe_info.caps & FE_CAN_GUARD_INTERVAL_AUTO) {
                  info("GUARD_INTERVAL_AUTO\n");
                  caps_guard_interval=GUARD_INTERVAL_AUTO;
                  }
                else {
                  info("GUARD_INTERVAL_AUTO not supported, trying GUARD_INTERVAL_1_8.\n");
                  caps_guard_interval=GUARD_INTERVAL_1_8;
                  }
                if (fe_info.caps & FE_CAN_HIERARCHY_AUTO) {
                  info("HIERARCHY_AUTO\n");
                  caps_hierarchy=HIERARCHY_AUTO;
                  }
                else {
                  info("HIERARCHY_AUTO not supported, trying HIERARCHY_NONE.\n");
                  caps_hierarchy=HIERARCHY_NONE;
                  }
                if (fe_info.caps & FE_CAN_FEC_AUTO) {
                  info("FEC_AUTO\n");
                  caps_fec=FEC_AUTO;
                  }
                else {
                  info("FEC_AUTO not supported, trying FEC_NONE.\n");
                  caps_fec=FEC_NONE;
                  }
                if (fe_info.frequency_min == 0 || fe_info.frequency_max == 0) {
                  info("This dvb driver is *buggy*: the frequency limits are undefined - please report to linuxtv.org\n");
                  fe_info.frequency_min = 177500000; fe_info.frequency_min = 858000000;
                  }
                else {
                  info("FREQ (%.2fMHz ... %.2fMHz)\n", fe_info.frequency_min/1e6, fe_info.frequency_max/1e6);
                  }
                break;
           case FE_QAM:
                if (fe_info.caps & FE_CAN_INVERSION_AUTO) {
                  info("INVERSION_AUTO\n");
                  caps_inversion=INVERSION_AUTO;
                  }
                else {
                  info("INVERSION_AUTO not supported, trying INVERSION_OFF.\n");
                  caps_inversion=INVERSION_OFF;
                  }
                if (fe_info.caps & FE_CAN_QAM_AUTO) {
                  info("QAM_AUTO\n");
                  caps_qam=QAM_AUTO;
                  }
                else {
                  info("QAM_AUTO not supported, trying");
                  //print out modulations in the sequence they will be scanned.
                  for (i = modulation_min; i <= modulation_max; i++)
                        info(" %s", xine_modulation_name(dvbc_modulation(i)));
                  info(".\n");
                  caps_qam=QAM_64;
                  flags.qam_no_auto = 1;
                  }
                if (fe_info.caps & FE_CAN_FEC_AUTO) {
                  info("FEC_AUTO\n");
                  caps_fec=FEC_AUTO;
                  }
                else {
                  info("FEC_AUTO not supported, trying FEC_NONE.\n");
                  caps_fec=FEC_NONE;
                  }
                if (fe_info.frequency_min == 0 || fe_info.frequency_max == 0) {
                  info("This dvb driver is *buggy*: the frequency limits are undefined - please report to linuxtv.org\n");
                  fe_info.frequency_min = 177500000; fe_info.frequency_max = 858000000;
                  }
                else {
                  info("FREQ (%.2fMHz ... %.2fMHz)\n", fe_info.frequency_min/1e6, fe_info.frequency_max/1e6);
                  }
                if (fe_info.symbol_rate_min == 0 || fe_info.symbol_rate_max == 0) {
                  info("This dvb driver is *buggy*: the symbol rate limits are undefined - please report to linuxtv.org\n");
                  fe_info.symbol_rate_min = 4000000; fe_info.symbol_rate_max = 7000000;
                  }
                else {
                  info("SRATE (%.3fMBd ... %.3fMBd)\n", fe_info.symbol_rate_min/1e6, fe_info.symbol_rate_max/1e6);
                  }
                break;
           case FE_ATSC:
                if (fe_info.caps & FE_CAN_INVERSION_AUTO) {
                  info("INVERSION_AUTO\n");
                  caps_inversion=INVERSION_AUTO;
                  }
                else {
                  info("INVERSION_AUTO not supported, trying INVERSION_OFF.\n");
                  caps_inversion=INVERSION_OFF;
                  }
                if (fe_info.caps & FE_CAN_8VSB) {
                  info("8VSB\n");
                  }
                if (fe_info.caps & FE_CAN_16VSB) {
                  info("16VSB\n");
                  }
                if (fe_info.caps & FE_CAN_QAM_64) {
                  info("QAM_64\n");
                  }
                if (fe_info.caps & FE_CAN_QAM_256) {
                  info("QAM_256\n");
                  }
                if (fe_info.frequency_min == 0 || fe_info.frequency_max == 0) {
                  info("This dvb driver is *buggy*: the frequency limits are undefined - please report to linuxtv.org\n");
                  fe_info.frequency_min = 177500000; fe_info.frequency_max = 858000000;
                  }
                else {
                  info("FREQ (%.2fMHz ... %.2fMHz)\n", fe_info.frequency_min/1e6, fe_info.frequency_max/1e6);
                  }
                break;
           case FE_QPSK:
                if (fe_info.caps & FE_CAN_INVERSION_AUTO) {
                  info("INVERSION_AUTO\n");
                  caps_inversion=INVERSION_AUTO;
                  }
                if (fe_info.caps & FE_CAN_QPSK) {
                  info("DVB-S\n");
                  caps_inversion=INVERSION_AUTO;
                  }
                if (fe_info.caps & FE_CAN_2G_MODULATION) {
                  info("DVB-S2\n");
                  caps_inversion=INVERSION_AUTO;
                  }
                if (fe_info.frequency_min == 0 || fe_info.frequency_max == 0) {
                  info("This dvb driver is *buggy*: the frequency limits are undefined - please report to linuxtv.org\n");
                  fe_info.frequency_min = 950000; fe_info.frequency_max = 2150000;
                  }
                else {
                  info("FREQ (%.2fGHz ... %.2fGHz)\n", fe_info.frequency_min/1e6, fe_info.frequency_max/1e6);
                  }
                if (fe_info.symbol_rate_min == 0 || fe_info.symbol_rate_max == 0) {
                  info("This dvb driver is *buggy*: the symbol rate limits are undefined - please report to linuxtv.org\n");
                  fe_info.symbol_rate_min = 1000000; fe_info.symbol_rate_max = 45000000;
                  }
                else {
                  info("SRATE (%.3fMBd ... %.3fMBd)\n", fe_info.symbol_rate_min/1e6, fe_info.symbol_rate_max/1e6);
                  }
                info("using LNB \"%s\"\n", this_lnb.name);
                if (committed_switch > 0)
                        info("using DiSEqC committed switch %d\n", committed_switch);
                if (uncommitted_switch > 0)
                        info("using DiSEqC uncommitted switch %d\n", uncommitted_switch);
                /* grrr...
                 * DVB API v5 doesnt allow checking for
                 * S2 capabilities fec3/5, fec9/10, PSK_8,
                 * allowed rolloff..
                 */
                break;
           default:
                cleanup();
                fatal("unsupported frontend type.\n");
           }
        info("-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_ \n");

        if (frontend_type != fe_info.type) {
                cleanup();
                fatal("Frontend '%s'(%s) doesnt support your choosen scan type '%s'\n",
                      fe_info.name, frontend_type_to_text(fe_info.type), frontend_type_to_text(frontend_type));
                }

        signal(SIGINT, handle_sigint);

        network_scan (frontend_fd, valid_initial_data);

        close (frontend_fd);

        dump_lists (adapter, frontend);

        cleanup();

        return 0;
}

