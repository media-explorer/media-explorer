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

#include <strings.h>
#include <linux/dvb/frontend.h>
#include "scan.h"
#include "dvbscan.h"




#define STRUCT_COUNT(struct_list) (sizeof(struct_list)/sizeof(struct init_item))

/********************************************************************
 * dvbscan.c
 *
 * import / export of initial_tuning_data for dvbscan
 * see doc/README.file_formats
 *
 ********************************************************************/


/********************************************************************
 * DVB-T
 ********************************************************************/

struct init_item ofdm_bw_list[] = {
        {"8MHz", BANDWIDTH_8_MHZ},
        {"7MHz", BANDWIDTH_7_MHZ},
        {"6MHz", BANDWIDTH_6_MHZ},
        #ifdef   BANDWIDTH_5_MHZ
        //currently not supported by Linux DVB API
        {"5MHz", BANDWIDTH_5_MHZ},
        #endif
        {"AUTO", BANDWIDTH_AUTO }
};

struct init_item ofdm_fec_list[] = {
        {"NONE" , FEC_NONE},
        {"1/2"  , FEC_1_2 },
        {"2/3"  , FEC_2_3 },
        {"3/4"  , FEC_3_4 },
        {"4/5"  , FEC_4_5 },
        {"5/6"  , FEC_5_6 },
        {"6/7"  , FEC_6_7 },
        {"7/8"  , FEC_7_8 },
        {"AUTO" , FEC_AUTO}
};

struct init_item ofdm_mod_list[] = {
        {"QPSK" , QPSK    },
        {"QAM16", QAM_16  },
        {"QAM64", QAM_64  },
        {"AUTO" , QAM_AUTO}
};

struct init_item ofdm_transmission_list[] = {
        {"2k"   , TRANSMISSION_MODE_2K  },
        {"8k"   , TRANSMISSION_MODE_8K  },
        #ifdef   TRANSMISSION_MODE_4K
        //currently not supported by Linux DVB API
        {"4k"   , TRANSMISSION_MODE_4K  },
        #endif
        {"AUTO" , TRANSMISSION_MODE_AUTO}
};

struct init_item ofdm_guard_list[] = {
        {"1/32", GUARD_INTERVAL_1_32},
        {"1/16", GUARD_INTERVAL_1_16},
        {"1/8" , GUARD_INTERVAL_1_8 },
        {"1/4" , GUARD_INTERVAL_1_4 },
        {"AUTO", GUARD_INTERVAL_AUTO}
};

struct init_item ofdm_hierarchy_list[] = {
        {"NONE", HIERARCHY_NONE},
        {"1"   , HIERARCHY_1   },
        {"2"   , HIERARCHY_2   },
        {"4"   , HIERARCHY_4   },
        {"AUTO", HIERARCHY_AUTO}
};

/* convert text to identifiers */

int txt_to_ofdm_bw ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_bw_list); i++)
                if (! strcasecmp(txt, ofdm_bw_list[i].name))
                        return ofdm_bw_list[i].id;
        return BANDWIDTH_AUTO; // fallback. should never happen.
}

int txt_to_ofdm_fec ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_fec_list); i++)
                if (! strcasecmp(txt, ofdm_fec_list[i].name))
                        return ofdm_fec_list[i].id;
        return FEC_AUTO; // fallback. should never happen.
}

int txt_to_ofdm_mod ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_mod_list); i++)
                if (! strcasecmp(txt, ofdm_mod_list[i].name))
                        return ofdm_mod_list[i].id;
        return QAM_AUTO; // fallback. should never happen.
}

int txt_to_ofdm_transmission ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_transmission_list); i++)
                if (! strcasecmp(txt, ofdm_transmission_list[i].name))
                        return ofdm_transmission_list[i].id;
        return TRANSMISSION_MODE_AUTO; // fallback. should never happen.
}

int txt_to_ofdm_guard ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_guard_list); i++)
                if (! strcasecmp(txt, ofdm_guard_list[i].name))
                        return ofdm_guard_list[i].id;
        return GUARD_INTERVAL_AUTO; // fallback. should never happen.
}

int txt_to_ofdm_hierarchy ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_hierarchy_list); i++)
                if (! strcasecmp(txt, ofdm_hierarchy_list[i].name))
                        return ofdm_hierarchy_list[i].id;
        return HIERARCHY_AUTO; // fallback. should never happen.
}

/*convert identifier to text */

const char * ofdm_bw_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_bw_list); i++)
                if (id == ofdm_bw_list[i].id)
                        return ofdm_bw_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

const char * ofdm_fec_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_fec_list); i++)
                if (id == ofdm_fec_list[i].id)
                        return ofdm_fec_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

const char * ofdm_mod_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_mod_list); i++)
                if (id == ofdm_mod_list[i].id)
                        return ofdm_mod_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

const char * ofdm_transmission_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_transmission_list); i++)
                if (id == ofdm_transmission_list[i].id)
                        return ofdm_transmission_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

const char * ofdm_guard_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_guard_list); i++)
                if (id == ofdm_guard_list[i].id)
                        return ofdm_guard_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

const char * ofdm_hierarchy_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(ofdm_hierarchy_list); i++)
                if (id == ofdm_hierarchy_list[i].id)
                        return ofdm_hierarchy_list[i].name;
        return "AUTO"; // fallback. should never happen.
}


/********************************************************************
 * DVB-C
 ********************************************************************/

struct init_item qam_fec_list[] = {
        {"NONE" , FEC_NONE},
        {"1/2"  , FEC_1_2 },
        {"2/3"  , FEC_2_3 },
        {"3/4"  , FEC_3_4 },
        {"4/5"  , FEC_4_5 },
        {"5/6"  , FEC_5_6 },
        {"6/7"  , FEC_6_7 },
        {"7/8"  , FEC_7_8 },
        {"8/9"  , FEC_8_9 },
        {"3/5"  , FEC_3_5 },
        {"9/10" , FEC_9_10},
        {"AUTO" , FEC_AUTO}
};

struct init_item qam_mod_list[] = {
        {"QAM16"  , QAM_16  },
        {"QAM32"  , QAM_32  },
        {"QAM64"  , QAM_64  },
        {"QAM128" , QAM_128 },
        {"QAM256" , QAM_256 },
        #ifdef   QAM_512             
        //currently not supported by Linux DVB API
        {"QAM512" , QAM_512 },
        #endif
        #ifdef   QAM_1024             
        //currently not supported by Linux DVB API
        {"QAM1024", QAM_1024},
        #endif
        #ifdef   QAM_2048             
        //currently not supported by Linux DVB API
        {"QAM2048", QAM_2048},
        #endif
        #ifdef   QAM_4096             
        //currently not supported by Linux DVB API
        {"QAM4096", QAM_4096},
        #endif
};

/* convert text to identifiers */

int txt_to_qam_fec ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qam_fec_list); i++)
                if (! strcasecmp(txt, qam_fec_list[i].name))
                        return qam_fec_list[i].id;
        return FEC_AUTO; // fallback. should never happen.
}

int txt_to_qam_mod ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qam_mod_list); i++)
                if (! strcasecmp(txt, qam_mod_list[i].name))
                        return qam_mod_list[i].id;
        return QAM_AUTO; // fallback. should never happen.
}

/*convert identifier to text */

const char * qam_fec_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qam_fec_list); i++)
                if (id == qam_fec_list[i].id)
                        return qam_fec_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

const char * qam_mod_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qam_mod_list); i++)
                if (id == qam_mod_list[i].id)
                        return qam_mod_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

/********************************************************************
 * ATSC
 ********************************************************************/


struct init_item atsc_mod_list[] = {
        {"QAM64" , QAM_64 },
        {"QAM256", QAM_256},
        {"8VSB"  , VSB_8  },
        {"16VSB" , VSB_16 },
};

/* convert text to identifiers */

int txt_to_atsc_mod ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(atsc_mod_list); i++)
                if (! strcasecmp(txt, atsc_mod_list[i].name))
                        return atsc_mod_list[i].id;
        return QAM_AUTO; // fallback. should never happen.
}

/*convert identifier to text */

const char * atsc_mod_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(atsc_mod_list); i++)
                if (id == atsc_mod_list[i].id)
                        return atsc_mod_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

/********************************************************************
 * DVB-S
 ********************************************************************/

struct init_item qpsk_delivery_system_list[] = {
        {"S" , SYS_DVBS  },
        {"S1", SYS_DVBS  },
        {"S2", SYS_DVBS2 },
};

struct init_item qpsk_pol_list[] = {
        {"H", POLARIZATION_HORIZONTAL    },
        {"V", POLARIZATION_VERTICAL      },
        {"R", POLARIZATION_CIRCULAR_RIGHT},
        {"L", POLARIZATION_CIRCULAR_LEFT },
}; // NOTE: no AUTO used here.

struct init_item qpsk_fec_list[] = {
        {"NONE" , FEC_NONE},
        {"1/2"  , FEC_1_2 },
        {"2/3"  , FEC_2_3 },
        {"3/4"  , FEC_3_4 },
        {"4/5"  , FEC_4_5 },
        {"5/6"  , FEC_5_6 },
        {"6/7"  , FEC_6_7 },
        {"7/8"  , FEC_7_8 },
        {"8/9"  , FEC_8_9 },
        {"3/5"  , FEC_3_5 }, //S2
        {"9/10" , FEC_9_10}, //S2
        {"AUTO" , FEC_AUTO}
};

struct init_item qpsk_rolloff_list[] = {
        {"35"  , ROLLOFF_35},
        {"25"  , ROLLOFF_25},
        {"20"  , ROLLOFF_20},
        {"AUTO", ROLLOFF_35},
}; // NOTE: "AUTO" == 0,35 in w_scan !

struct init_item qpsk_mod_list[] = {
        {"QPSK"  , QPSK   },
        {"8PSK"  , PSK_8  },
        {"16APSK", APSK_16},
        {"32APSK", APSK_32},
        {"AUTO"  , QPSK   },
}; // NOTE: "AUTO" == QPSK in w_scan !

/* convert text to identifiers */

int txt_to_qpsk_delivery_system ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_delivery_system_list); i++)
                if (! strcasecmp(txt, qpsk_delivery_system_list[i].name))
                        return qpsk_delivery_system_list[i].id;
        return SYS_DVBS; // fallback. should never happen.
}

int txt_to_qpsk_pol ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_pol_list); i++)
                if (! strcasecmp(txt, qpsk_pol_list[i].name))
                        return qpsk_pol_list[i].id;
        return POLARIZATION_HORIZONTAL; // fallback. should never happen.
}

int txt_to_qpsk_fec ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_fec_list); i++)
                if (! strcasecmp(txt, qpsk_fec_list[i].name))
                        return qpsk_fec_list[i].id;
        return FEC_AUTO; // fallback. should never happen.
}

int txt_to_qpsk_rolloff ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_rolloff_list); i++)
                if (! strcasecmp(txt, qpsk_rolloff_list[i].name))
                        return qpsk_rolloff_list[i].id;
        return ROLLOFF_35; // fallback. should never happen.
}

int txt_to_qpsk_mod ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_mod_list); i++)
                if (! strcasecmp(txt, qpsk_mod_list[i].name))
                        return qpsk_mod_list[i].id;
        return QPSK; // fallback. should never happen.
}

/*convert identifier to text */

const char * qpsk_delivery_system_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_delivery_system_list); i++)
                if (id == qpsk_delivery_system_list[i].id)
                        return qpsk_delivery_system_list[i].name;
        return "S"; // fallback. should never happen.
}

const char * qpsk_pol_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_pol_list); i++)
                if (id == qpsk_pol_list[i].id)
                        return qpsk_pol_list[i].name;
        return "H"; // fallback. should never happen.
}

const char * qpsk_fec_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_fec_list); i++)
                if (id == qpsk_fec_list[i].id)
                        return qpsk_fec_list[i].name;
        return "AUTO"; // fallback. should never happen.
}

const char * qpsk_rolloff_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_rolloff_list); i++)
                if (id == qpsk_rolloff_list[i].id)
                        return qpsk_rolloff_list[i].name;
        return "35"; // fallback. should never happen.
}

const char * qpsk_mod_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(qpsk_mod_list); i++)
                if (id == qpsk_mod_list[i].id)
                        return qpsk_mod_list[i].name;
        return "QPSK"; // fallback. should never happen.
}

/********************************************************************
 * non-frontend specific part
 *
 ********************************************************************/

struct init_item fe_type_list[] = {
        {"ATSC", FE_ATSC},
        {"QAM" , FE_QAM },
        {"OFDM", FE_OFDM},
        {"QPSK", FE_QPSK},
}; // NOTE: "AUTO" == FE_OFDM in w_scan !

/* convert text to identifiers */

int txt_to_fe_type ( const char * txt ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(fe_type_list); i++)
                if (! strcasecmp(txt, fe_type_list[i].name))
                        return fe_type_list[i].id;
        return FE_OFDM; // fallback. should never happen.
}

/*convert identifier to text */

const char * fe_type_to_txt ( int id ) {
        unsigned int i;

        for (i = 0; i < STRUCT_COUNT(fe_type_list); i++)
                if (id == fe_type_list[i].id)
                        return fe_type_list[i].name;
        return "OFDM"; // fallback. should never happen.
}
