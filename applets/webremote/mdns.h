/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

/* Advertise the web remote over mdns */

#ifndef __MDNS_H__
#define __MDNS_H__

#include <avahi-client/publish.h>
#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>

typedef struct _MdnsServiceInfo MdnsServiceInfo;

/* types example: _printer._tcp */
struct _MdnsServiceInfo
{
  const gchar *name;
  const gchar *type;
  gint service_instance;
  gint port;
  AvahiEntryGroup *mdns_entry_group;
  AvahiGLibPoll *mdns_glib_poll;
  AvahiClient *mdns_client;
};

MdnsServiceInfo *mdns_service_info_new (void);

void mdns_service_info_free (MdnsServiceInfo *info);

void mdns_service_start (MdnsServiceInfo *info);

#endif
