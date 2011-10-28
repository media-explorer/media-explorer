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

#ifndef __MEX_WEBREMOTE_H__
#define __MEX_WEBREMOTE_H__

#include <glib.h>
#include "dbus-client.h"
#include "tracker-client.h"
#include "mdns-client.h"

#include "dbus-service.h"

G_BEGIN_DECLS

typedef struct _MexWebRemote MexWebRemote;

struct _MexWebRemote
{
  DBusClient *dbus_client;
  TrackerInterface *tracker_interface;
  MdnsServiceInfo *mdns_service;

  gboolean opt_debug;
  guint opt_port;
  gchar *opt_interface;
  const gchar *opt_auth;
  gboolean opt_noauth;

  gchar *userpass;
  const gchar *mex_data_dir;
  gboolean successful_auth;
  GList *clients;

  gchar **current_playing_info;

  gchar *data;
};


void mex_webremote_quit (void);

G_END_DECLS

#endif
