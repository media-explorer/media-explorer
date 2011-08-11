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
 * along with this program; if not, see <://www.gnu.org/licenses>
 */

#ifndef __DBUS_CLIENT_H__
#define __DBUS_CLIENT_H__

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _DBusClient DBusClient;

struct _DBusClient
{
  GDBusConnection *connection;
  GDBusProxy *mex_input;
  GDBusProxy *mex_player;
};

DBusClient *dbus_client_new (void);

void dbus_client_free (DBusClient *dbus_client);

void dbus_client_input_set_key (DBusClient *dbus_client, gint keyval);
void dbus_client_input_set_message (DBusClient  *dbus_client,
                                    const gchar *message,
                                    guint        timeout);

gchar *dbus_client_player_get (DBusClient *dbus_client, const gchar *get);
void dbus_client_player_action (DBusClient *dbus_client, const gchar *action);
void dbus_client_player_set (DBusClient  *dbus_client,
                             const gchar *action,
                             gchar       *uri);
G_END_DECLS

#endif
