/*
 * Mex - a media explorer
 *
 * Copyright © 2010 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifndef __MEX_LOG_PRIVATE_H__
#define __MEX_LOG_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#include "mex-log.h"

G_BEGIN_DECLS

#ifdef MEX_ENABLE_DEBUG

MEX_LOG_DOMAIN_EXTERN(log_log_domain);
MEX_LOG_DOMAIN_EXTERN(epg_log_domain);
MEX_LOG_DOMAIN_EXTERN(applet_manager_log_domain);
MEX_LOG_DOMAIN_EXTERN(channel_log_domain);
MEX_LOG_DOMAIN_EXTERN(download_queue_log_domain);
MEX_LOG_DOMAIN_EXTERN(surface_player_log_domain);
MEX_LOG_DOMAIN_EXTERN(player_log_domain);

void _mex_log_init_core_domains (void);
void _mex_log_free_core_domains (void);

#endif

G_END_DECLS

#endif /* __MEX_LOG_PRIVATE_H__ */
