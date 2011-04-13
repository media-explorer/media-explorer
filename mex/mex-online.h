/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
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


#ifndef _MEX_ONLINE
#define _MEX_ONLINE

#include <glib.h>

G_BEGIN_DECLS

gboolean mex_is_online (void);

typedef void (*MexOnlineNotify) (gboolean online, gpointer user_data);

void mex_online_add_notify (MexOnlineNotify callback,
                            gpointer       user_data);

void mex_online_remove_notify (MexOnlineNotify callback,
                               gpointer       user_data);

G_END_DECLS

#endif /* _MEX_ONLINE */
