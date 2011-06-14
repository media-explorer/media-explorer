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

/* This is a small webserver using libsoup to pass up control keys to dbus */

#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <libsoup/soup.h>
#include "dbus-interface.h"
#include <mex/mex.h>

typedef struct
{
  HTTPDBusInterface *dbus_interface;
  gboolean opt_debug;
  guint userpass;
} MexWebRemote;

/* Send response after having done an HTTP post/get */
static void
send_response (SoupServer   *server,
               SoupMessage  *msg,
               const gchar  *path,
               MexWebRemote *self)
{
  gchar *file_contents, *uri;
  gsize file_size;
  GError *error=NULL;

  SoupBuffer *buffer;

  if (g_strcmp0 (path, "/") == 0)
    path = "/index.html";

  uri = g_strconcat (mex_get_data_dir(), "/webremote/", path, NULL);

  if (self->opt_debug)
    g_debug ("Requested: %s", uri);

  g_file_get_contents (uri, &file_contents, &file_size, &error);
  g_free (uri);

  if (error)
    {
      if (self->opt_debug)
        g_debug ("404: No such file or directory: %s", error->message);

      g_error_free (error);

      if (file_contents)
        g_free (file_contents);

      soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
      return;
    }

  if (error)
    g_error ("ERROR: %s\n", error->message);

  buffer = soup_buffer_new (SOUP_MEMORY_TAKE, file_contents,
                            strlen (file_contents));

  soup_message_body_append_buffer (msg->response_body, buffer);

  soup_buffer_free (buffer);
  soup_message_set_status (msg, SOUP_STATUS_OK);
  return;
}

static void
http_post (SoupServer   *server,
           SoupMessage  *msg,
           const gchar  *path,
           MexWebRemote *self)
{
  const gchar *post_request;

  if (self->dbus_interface == NULL)
  {
    g_error ("failed to initiate dbus interface");
    return;
  }

  post_request = msg->request_body->data;

  if (g_str_has_prefix (post_request, "keyvalue"))
      {
        gint keyvalue;
        keyvalue = atoi ((post_request + strlen ("keyvalue=")));

        httpdbus_send_keyvalue (self->dbus_interface, keyvalue);
      }

  send_response (server, msg, path, self);
  return;
}

static void
server_cb (SoupServer        *server,
           SoupMessage       *msg,
           const char        *path,
           GHashTable        *query,
           SoupClientContext *client,
           gpointer          *user_data)
{

  MexWebRemote *self = (MexWebRemote *)user_data;

  if (self->opt_debug)
    g_debug ("%s %s HTTP/1.%d", msg->method, path,
             soup_message_get_http_version (msg));

  if (msg->method == SOUP_METHOD_POST)
    http_post (server, msg, path, self);
  else if (msg->method == SOUP_METHOD_GET || msg->method == SOUP_METHOD_HEAD)
    send_response (server, msg, path, self);
  else
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
}

static gboolean
auth_cb (SoupAuthDomain *domain,
         SoupMessage    *msg,
         const gchar    *username,
         const gchar    *password,
         MexWebRemote   *self)
{
  gchar *temp;
  temp = g_strdup_printf ("%s:%s", username, password);

  if (g_str_hash (temp) == self->userpass)
    {
      if (self->opt_debug)
        g_debug ("Authentication success");
      g_free (temp);
      return TRUE;
    }
  else
    {
      if (self->opt_debug)
        g_debug ("Authentication failure");
      g_free (temp);
      return FALSE;
    }
}

static void
address_resolved_cb (SoupAddress *address,
                     guint        status,
                     gpointer     data)
{
 const gchar *name, *physical;

 name = soup_address_get_name (address);
 physical = soup_address_get_physical (address);

 if (g_strcmp0 (physical, "0.0.0.0") == 0 ||
     g_strcmp0 (physical, "::0") == 0)
   physical = "All";

 g_message ("Listening on Address: %s %s Port: %d",
            name ? name : "",
            physical ? physical : "",
            soup_address_get_port (address));

}

int main (int argc, char **argv)
{
  g_type_init ();

  MexWebRemote webremote = { 0, };
  SoupServer *server;
  GMainLoop *loop;
  SoupAuthDomain *domain;

  SoupAddress *address;
  SoupAddress *interface_address;

  GOptionContext *context;

  guint opt_port = 9090;
  gchar *opt_interface = NULL;
  const gchar *opt_auth = NULL;

  GError *error=NULL;

  webremote.dbus_interface = httpdbus_interface_new ();

  webremote.tracker_interface = tracker_interface_new ();

  GOptionEntry entries[] =
    {
        { "bind", 'b', 0, G_OPTION_ARG_STRING, &opt_interface,
          "Bind to a particular ip address", NULL },
        { "port", 'p', 0, G_OPTION_ARG_INT, &opt_port,
          "Port to listen to http requests on", NULL },
        { "debug", 'd', 0, G_OPTION_ARG_NONE, &webremote.opt_debug,
          "Dispay debugging info", NULL },
        { "auth", 'a', 0, G_OPTION_ARG_STRING, &opt_auth,
          "username:password", NULL },
        { NULL }
    };

  context = g_option_context_new ("- Media explorer web remote");

  g_option_context_add_main_entries (context, entries, "mex");
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("Failed to parse options: %s\n", error->message);
      g_error_free (error);
      exit (1);
    }

  if (opt_interface)
    {
      gint resolve_status;

      interface_address = soup_address_new (opt_interface, opt_port);

      resolve_status = soup_address_resolve_sync (interface_address, NULL);

      /* You can pass anything in as an opt_interface so we have to be careful
       * that the address was valid/resolvable.
       */
      if (resolve_status == SOUP_STATUS_OK)
        {
          server = soup_server_new (SOUP_SERVER_SERVER_HEADER,
                                    "mex-remote-control",
                                    SOUP_SERVER_INTERFACE,
                                    interface_address,
                                    NULL);
        }
      else
        {
          g_error ("Sorry, could not swan server.. exiting");
          return EXIT_FAILURE;
        }
    }
  else
    {
      server = soup_server_new (SOUP_SERVER_PORT, opt_port,
                                SOUP_SERVER_SERVER_HEADER, "mex-remote-control",
                                NULL);
    }

  if (!server)
    {
      g_error ("Sorry, unable to start server on port %d", opt_port);
      return EXIT_FAILURE;
    }

  soup_server_run_async (server);

  g_message ("Server started..\nResolving our address");

  g_object_get (server, "interface", &address, NULL);

  /* Probably don't need to run the resolve but it's more reliable to do so. */
  soup_address_resolve_async (address, NULL, NULL, address_resolved_cb, NULL);

  /* use password */
  if (opt_auth)
    {
     webremote.userpass = g_str_hash (opt_auth);

      domain = soup_auth_domain_basic_new (SOUP_AUTH_DOMAIN_REALM,
                                           "Authenticate",
                                           SOUP_AUTH_DOMAIN_BASIC_AUTH_CALLBACK,
                                           auth_cb,
                                           SOUP_AUTH_DOMAIN_BASIC_AUTH_DATA,
                                           (gpointer)&webremote,
                                           SOUP_AUTH_DOMAIN_ADD_PATH, "/",
                                           NULL);

      soup_server_add_auth_domain (server, domain);
      g_object_unref (domain);
    }

  soup_server_add_handler (server,
                           NULL,
                           (SoupServerCallback)server_cb,
                           (gpointer)&webremote,
                           NULL);

  loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (loop);

  httpdbus_interface_free (webremote.dbus_interface);
  g_option_context_free (context);

  return EXIT_SUCCESS;
}
