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
 * The project's page is http://wirbel.htpc-forum.de/w_scan/index2.html
 */

#ifndef __DUMP_XINE_H__
#define __DUMP_XINE_H__

/* 20110702 --wk */

#include <stdint.h>
#include "extended_frontend.h"
#include "scan.h"

const char * xine_inversion_name(int inversion);
const char * xine_fec_name(int fec);
const char * xine_modulation_name(int modulation);
const char * xine_bandwidth_name (int bandwidth);
const char * xine_transmission_mode_name (int transmission_mode);
const char * xine_guard_name (int guard_interval);
const char * xine_hierarchy_name (int hierarchy);

void xine_dump_dvb_parameters (FILE *f,
                                struct extended_dvb_frontend_parameters *p,
                                struct w_scan_flags * flags);

void xine_dump_service_parameter_set (FILE * f,
                                struct service * s,
                                struct transponder * t,
                                struct w_scan_flags * flags);

#endif

