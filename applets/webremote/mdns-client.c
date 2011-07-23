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

#include <stdlib.h>
#include <glib.h>

#include "mdns-client.h"
#include <avahi-common/error.h>

static void
mdns_service_add_service (AvahiClient *client, MdnsServiceInfo *info);

static void
mdns_service_group_state_changed (AvahiEntryGroup      *entry_group,
                                  AvahiEntryGroupState  state,
                                  MdnsServiceInfo      *info)
{
  switch (state)
    {
      case AVAHI_ENTRY_GROUP_ESTABLISHED:
      g_message ("Avahi service established");
      break;

      case AVAHI_ENTRY_GROUP_COLLISION:
      info->service_instance++;
      mdns_service_add_service (info->mdns_client, info);
      break;

      case AVAHI_ENTRY_GROUP_FAILURE:
      case AVAHI_ENTRY_GROUP_UNCOMMITED:
      case AVAHI_ENTRY_GROUP_REGISTERING:
      break;
    }
}

static void
mdns_service_add_service (AvahiClient *client, MdnsServiceInfo *info)
{
  gint ret;
  gchar *service_name;

  if (!info->mdns_entry_group)
    {
      info->mdns_entry_group =
        avahi_entry_group_new (client,
                               (AvahiEntryGroupCallback) mdns_service_group_state_changed,
                               info);
    }

  g_debug ("%d", info->service_instance);

  if (info->service_instance > 0)
    service_name = g_strdup_printf ("%s on %s %d",
                                    info->name,
                                    avahi_client_get_host_name (client),
                                    info->service_instance);
  else
    service_name = g_strdup_printf ("%s on %s",
                                    info->name,
                                    avahi_client_get_host_name (client));

  ret = avahi_entry_group_add_service (info->mdns_entry_group,
                                       AVAHI_IF_UNSPEC,
                                       AVAHI_PROTO_UNSPEC,
                                       0,
                                       service_name,
                                       info->type,
                                       NULL,
                                       NULL,
                                       info->port,
                                       NULL);
  g_free (service_name);

  if (ret < 0)
    {
      if (ret == AVAHI_ERR_COLLISION)
        {
          gchar *new_name;
          gint retry;
          gint i; /* safety belt */

          retry = ret;
          i = 0;

          while (retry == AVAHI_ERR_COLLISION && i <= 100)
            {
              service_name =
                g_strdup_printf ("%s on %s %d",
                                 info->name,
                                 avahi_client_get_host_name (client),
                                 i);

              retry = avahi_entry_group_add_service (info->mdns_entry_group,
                                                     AVAHI_IF_UNSPEC,
                                                     AVAHI_PROTO_UNSPEC,
                                                     0,
                                                     service_name,
                                                     info->type,
                                                     NULL,
                                                     NULL,
                                                     info->port,
                                                     NULL);
              g_free (service_name);
              i++;
            }

          if (retry == AVAHI_ERR_COLLISION)
            g_warning ("Tried to rename service to avoid collision but"
                      " failed to find an alternative name in a timely manner");
          else
            avahi_entry_group_commit (info->mdns_entry_group);
        }
      else
        {
          g_warning ("Avahi: Failed to add service: \"%s\" because of a %s",
                     info->name,
                     avahi_strerror (ret));
        }
    }
  else
    {
      avahi_entry_group_commit (info->mdns_entry_group);
    }
}


static void
mdns_service_client_state_changed_cb (AvahiClient *client,
                                      AvahiClientState state,
                                      MdnsServiceInfo *info)
{
  switch (state)
    {
      case AVAHI_CLIENT_S_RUNNING:
        mdns_service_add_service (client, info);
        break;

      case AVAHI_CLIENT_FAILURE:
        g_warning ("Avahi: client failure: %s",
                   avahi_strerror (avahi_client_errno (client)));
        break;

      case AVAHI_CLIENT_S_COLLISION:
        if (info->mdns_entry_group)
          avahi_entry_group_reset (info->mdns_entry_group);
        break;
    }
}

void
mdns_service_start (MdnsServiceInfo *info)
{
  gint ret=0;

  if (info->mdns_glib_poll || info->mdns_client)
    {
      g_warning ("Avahi: service already started");
      return;
    }

  avahi_set_allocator (avahi_glib_allocator ());

  info->mdns_glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);

  /* AVAHI_CLIENT_NO_FAIL = don't block waiting to connect to avahi-daemon */
  info->mdns_client =
    avahi_client_new (avahi_glib_poll_get (info->mdns_glib_poll),
                      AVAHI_CLIENT_NO_FAIL,
                      (AvahiClientCallback)
                      mdns_service_client_state_changed_cb,
                      info,
                      &ret);

  if (!info->mdns_client)
    g_warning ("Failed to create client: %s", avahi_strerror (ret));
}

MdnsServiceInfo *mdns_service_info_new (void)
{
  MdnsServiceInfo *info;

  info = g_new0 (MdnsServiceInfo, 1);

  return info;
}

void
mdns_service_info_free (MdnsServiceInfo *info)
{

  if (info->mdns_entry_group)
    {
      avahi_entry_group_free (info->mdns_entry_group);
      info->mdns_entry_group = NULL;
    }

  if (info->mdns_client)
    {
      avahi_client_free (info->mdns_client);
      info->mdns_client = NULL;
    }

  if (info->mdns_glib_poll)
    {
      avahi_glib_poll_free (info->mdns_glib_poll);
      info->mdns_glib_poll = NULL;
    }
}




