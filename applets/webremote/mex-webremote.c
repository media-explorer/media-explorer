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

#include "dbus-client.h"
#include "tracker-client.h"
#include "mdns-client.h"

#include "dbus-service.h"

#include <mex/mex.h>

/* TODO #ifdef HAVE_TRACKER etc split webserver into separate module */
/* TODO web-service.h */

typedef struct
{
  DBusClient *dbus_client;
  TrackerInterface *tracker_interface;
  MdnsServiceInfo *mdns_service;

  gboolean opt_debug;
  guint userpass;
  const gchar *mex_data_dir;

  gchar *data;
} MexWebRemote;

static GMainLoop *main_loop;

typedef enum
{
  NORMAL,

  CUSTOM,

  LAST
} MexWebRemoteResponseType;


/* Send response after having done an HTTP post/get */
static void
send_response (SoupServer   *server,
               SoupMessage  *msg,
               const gchar  *path,
               MexWebRemote *self,
               MexWebRemoteResponseType response_type)
{
  SoupBuffer *buffer;
  GError *error=NULL;

  gchar *processed_data;
  gsize data_size;
  gchar *mime_type;

  /* Normal request or Unspecified */
  if (response_type == NORMAL || !response_type)
    {
      gchar *uri;
      char token[sizeof ("/DATADIR")+1];

      if (g_strcmp0 (path, "/") == 0)
        path = "/index.html";

      g_utf8_strncpy (token, path, 8);

      if (g_strcmp0 (token, "/DATADIR") == 0)
          uri = g_strconcat (self->mex_data_dir, "/common/",
                             (path + sizeof ("DATADIR/")), NULL);
      else
        uri = g_strconcat (self->mex_data_dir, "/webremote/", path, NULL);

      mime_type = g_content_type_guess (uri, NULL, 0, NULL);

      if (self->opt_debug)
        g_debug ("Requested: %s", uri);

      g_file_get_contents (uri, &processed_data, &data_size, &error);

      /* This is to protect against the corner case where the uri may be
       * NULL if mex_data_dir failed.
       */

      if (uri)
        g_free (uri);

      if (error || !uri)
        {
          if (self->opt_debug)
            g_debug ("404: No such file or directory: %s",
                     error ? error->message : path);

          if (error)
            g_error_free (error);

          soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
          return;
        }
    }
  else if (response_type == CUSTOM)
    {
      processed_data = self->data;
      data_size = strlen (processed_data);
      mime_type = g_content_type_guess (NULL, processed_data, data_size, NULL);
    }

  if (error)
    g_error ("ERROR: %s\n", error->message);

  buffer = soup_buffer_new (SOUP_MEMORY_TAKE, processed_data, data_size);

  soup_message_body_truncate (msg->response_body);
  soup_message_body_append_buffer (msg->response_body, buffer);

  soup_message_headers_set_content_type (msg->response_headers,
                                         mime_type,
                                         NULL);
  g_free (mime_type);

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

  post_request = msg->request_body->data;

  /* prefix is "="
   * so we get send something like cats=cool where the prefix would be cats
   * Currently we can't handle mutiple values like ?this=that&then=cat
   */
  if (g_str_has_prefix (post_request, "keyvalue"))
      {
        gint keyvalue;
        keyvalue = atoi ((post_request + strlen ("keyvalue=")));

        dbus_client_input_set (self->dbus_client, keyvalue);
      }

  else if (g_str_has_prefix (post_request, "trackersearch"))
    {
      gchar *search_term;
      gchar *sparql_request;
      gchar *result;

      search_term = g_strdup (post_request + strlen ("trackersearch="));

      if (self->opt_debug)
        g_debug ("Got Search request: %s", search_term);

      sparql_request =
        g_strdup_printf ("SELECT ?title ?url {"
                         "?urn a nfo:Media ."
                         "?urn tracker:available true ."
                         "?urn fts:match '*%s*'."
                         "?urn nie:title ?title ."
                         "?urn nie:url ?url }",
                         search_term);

      g_free (search_term);

      result = tracker_interface_query (self->tracker_interface,
                                        sparql_request);

      /* we shouldn't ever get a null result as we normally return empty json
       * but for safety here is an empty string that can be freed by soup.
       */
      if (!result)
        result = g_strdup ("{}");

      self->data = result;

      if (self->opt_debug)
        g_debug ("Search result\n%s", result);

      g_free (sparql_request);
      send_response (server, msg, path, self, CUSTOM);
      return;
    }

  else if (g_str_has_prefix (post_request, "setmedia"))
    {
      gchar *media_uri;

      media_uri = soup_uri_decode (post_request + strlen ("setmedia="));

      if (self->opt_debug)
        g_debug ("Resquesting setmedia = %s", media_uri);

      dbus_client_player_set (self->dbus_client, "seturi", media_uri);

      g_free (media_uri);
    }

  else if (g_str_has_prefix (post_request, "mediaplayerget"))
    {
      gchar *get;

      get = g_strdup (post_request + strlen ("mediaplayerget="));

      if (self->opt_debug)
        g_debug ("mediaplayerget = %s", get);

      self->data = dbus_client_player_get (self->dbus_client, get);
      g_free (get);

      if (self->opt_debug)
        g_debug ("result \"%s\"", self->data);

      if (!self->data || strlen (self->data) == 0)
        self->data = g_strdup ("Unknown media");

      send_response (server, msg, path, self, CUSTOM);
      return;
    }

  else if (g_str_has_prefix (post_request, "playeraction"))
    {
      gchar *action;

      action = g_strdup (post_request + strlen ("playeraction="));

      if (self->opt_debug)
        g_debug ("playeraction = %s", action);

      dbus_client_player_action (self->dbus_client, action);

      g_free (action);
    }

  send_response (server, msg, path, self, NORMAL);
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
    send_response (server, msg, path, self, NORMAL);
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

void
mex_webremote_quit ()
{
  g_main_loop_quit (main_loop);
}

static void
sig_webremote_quit (int sig)
{
  mex_webremote_quit ();
}

int main (int argc, char **argv)
{
  g_type_init ();

  MexWebRemote webremote = { 0, };
  SoupServer *server;
  SoupAuthDomain *domain;

  SoupAddress *address;
  SoupAddress *interface_address;

  gint dbus_service_id;

  GOptionContext *context;

  guint opt_port = 9090;
  gchar *opt_interface = NULL;
  const gchar *opt_auth = NULL;

  GError *error=NULL;

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
    g_warning ("Failed to parse options: %s\n", error->message);

  webremote.mex_data_dir = mex_get_data_dir ();

  /* Fallback method */
  if (!webremote.mex_data_dir)
    webremote.mex_data_dir = MEXPKGDATADIR;

  if (!webremote.mex_data_dir)
    {
      g_warning ("Could not find program data please verify your installation");
      goto clean_up;
    }

  /* We want to talk to dbus, tracker and avahi/mdns */
  webremote.dbus_client = dbus_client_new ();
  webremote.tracker_interface = tracker_interface_new ();
  webremote.mdns_service = mdns_service_info_new ();

  /* Start the our own dbus service for the Quit method and auto activation */
  dbus_service_id = dbus_service_start ();

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
          g_error ("Sorry, could not spawn server with %s address",
                   opt_interface);
          goto clean_up;
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
      goto clean_up;
    }

  soup_server_run_async (server);

  /* Avahi/mdns advertise server */
  webremote.mdns_service->port = opt_port;
  webremote.mdns_service->type = "_http._tcp";
  webremote.mdns_service->name = "Mex Webremote";

  /* Edit /etc/avahi/avahi-daemon.conf for further configuration */
  mdns_service_start (webremote.mdns_service);

  g_message ("WebServer started");

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

  signal (SIGINT, sig_webremote_quit);
  signal (SIGTERM, sig_webremote_quit);

  main_loop = g_main_loop_new (NULL, TRUE);

  g_main_loop_run (main_loop);

clean_up:

  if (dbus_service_id > 0)
    dbus_service_stop (dbus_service_id);

  if (error)
    g_error_free (error);

  if (webremote.dbus_client)
    dbus_client_free (webremote.dbus_client);

  if (webremote.tracker_interface)
    tracker_interface_free (webremote.tracker_interface);

  if (webremote.mdns_service)
    mdns_service_info_free (webremote.mdns_service);

  if (context)
    g_option_context_free (context);

  return EXIT_SUCCESS;
}
