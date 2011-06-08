/*
 * Mex - a media explorer
 *
 * Copyright © , 2010, 2011 Intel Corporation.
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


#include <stdlib.h>
#include <libgssdp/gssdp-resource-group.h>
#include "ssdp.h"

void
ssdp_advertise_server (void)
{
  GSSDPClient *client;
  GSSDPResourceGroup *resource_group;
  GError *error=NULL;
  GMainLoop *loop;

  client = gssdp_client_new (NULL, NULL, &error);
  if (error)
  {
      g_error ("Error creating GSSDP client: %s\n", error->message);
      g_error_free (error);
      return;
  }

  resource_group = gssdp_resource_group_new (client);

  /* TODO get ip address of currently active iface */
  gssdp_resource_group_add_resource_simple (resource_group,
                                            "upnp:rootdevice",
                                            "uuid:48ccbaffbb0675f2a0d9e6ef8875a03c:upnp:rootdevice",
                                            "http://127.0.0.1:9090/");

  gssdp_resource_group_set_available (resource_group, TRUE);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (resource_group);
  g_object_unref (client);
  
  return;
}
