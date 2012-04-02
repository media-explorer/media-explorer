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

#include "settings.h"

#include <mex/mex.h>
#include "mex-log.h"

#define MEX_LOG_DOMAIN_DEFAULT  webremote_log_domain
MEX_LOG_DOMAIN(webremote_log_domain);


/* TODO #ifdef HAVE_TRACKER etc split webserver into separate module */

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
      GFile *gfile;
      GFileInfo *info;
      /* TODO: return style dir not datadir where you can request all kinds of
       * things
       */
      gchar *uri;
      char token[sizeof ("/DATADIR")+1];

      if (g_strcmp0 (path, "/") == 0)
        path = "/index.html";

      g_utf8_strncpy (token, path, 8);

      if (g_strcmp0 (token, "/DATADIR") == 0)
          uri = g_strconcat (self->mex_data_dir, "/",
                             (path + sizeof ("DATADIR/")), NULL);
      else
        uri = g_strconcat (self->mex_data_dir, "/webremote/", path, NULL);

      gfile = g_file_new_for_path (uri);
      info = g_file_query_info (gfile,
                                G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                0, NULL, &error);

      mime_type = g_strdup (g_file_info_get_content_type (info));

      g_object_unref (info);
      g_object_unref (gfile);

      MEX_DEBUG ("Requested: %s", uri);

      g_file_get_contents (uri, &processed_data, &data_size, &error);

      /* This is to protect against the corner case where the uri may be
       * NULL if mex_data_dir failed.
       */

      if (uri)
        g_free (uri);

      if (error || !uri)
        {
          MEX_DEBUG ("404: No such file or directory: %s",
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
      mime_type = g_content_type_guess (NULL, (guchar *) processed_data,
                                        data_size, NULL);
    }

  if (error)
    {
      g_error ("ERROR: %s\n", error->message);
      g_clear_error (&error);
    }

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

        dbus_client_input_set_key (self->dbus_client, keyvalue);
      }

  else if (g_str_has_prefix (post_request, "trackersearch"))
    {
      gchar *search_term;
      gchar *sparql_request;
      gchar *result = NULL;

      /* If we were able to connect to tracker backend use it */
      if (self->tracker_interface)
        {
          search_term = g_strdup (post_request + strlen ("trackersearch="));

          MEX_DEBUG ("Got Search request: %s", search_term);

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

          g_free (sparql_request);
        }

      /* We either failed to query or something went wrong with the json
       * generation so return a empty json set (will be freed by libsoup).
       */
      self->data = result;

      MEX_DEBUG ("Search result\n%s", result);

      send_response (server, msg, path, self, CUSTOM);
      return;
    }
  else if (g_str_has_prefix (post_request, "playinginfo"))
    {
      if (self->dbus_client->current_playing_uri &&
          self->current_playing_info[0] &&
          g_strcmp0 (self->dbus_client->current_playing_uri,
                     self->current_playing_info[0]) == 0)
        {
          MEX_DEBUG ("using cache: %s %s",
                     self->dbus_client->current_playing_uri,
                     self->current_playing_info[0]);
          self->data = g_strdup (self->current_playing_info[1]);
        }
      else
        {
          gchar *sparql_request;
          gchar *result;

          if (self->current_playing_info)
            {
              g_free (self->current_playing_info[0]);
              g_free (self->current_playing_info[1]);
            }

          self->current_playing_info[0] =
            dbus_client_player_get (self->dbus_client, "uri");

          sparql_request =
            g_strdup_printf ("SELECT "
                             "?title ?mime ?duration ?filename ?album ?artist "
                             "WHERE { "
                             "?urn nie:url '%s' . "
                             "?urn nfo:fileName ?filename . "
                             "OPTIONAL { ?urn nie:title ?title . } "
                             "?urn nie:mimeType ?mime . "
                             "OPTIONAL { ?urn nfo:duration ?duration . } "
                             "OPTIONAL { ?urn nmm:musicAlbum "
                             " [ nie:title ?album ] . } "
                             "OPTIONAL { ?urn nmm:performer "
                             "[ nmm:artistName ?artist ] . } "
                             " }",
                             self->current_playing_info[0]);

          MEX_DEBUG ("query: %s", sparql_request);

          result = tracker_interface_query (self->tracker_interface,
                                            sparql_request);
          g_free (sparql_request);

          /* We either failed to query or something went wrong with the json
           * generation so return a empty json set (will be freed by libsoup).
           */
          if (!result)
            result = g_strdup ("{}");

          self->current_playing_info[1] = g_strdup (result);

          self->data = result;
        }


      MEX_DEBUG ("Search result\n%s", self->data);

      send_response (server, msg, path, self, CUSTOM);
      return;
    }

  else if (g_str_has_prefix (post_request, "setmedia"))
    {
      gchar *media_uri;

      media_uri = soup_uri_decode (post_request + strlen ("setmedia="));

      MEX_DEBUG ("Resquesting setmedia = %s", media_uri);

      dbus_client_player_set (self->dbus_client, "seturi", media_uri);

      g_free (media_uri);
    }

  else if (g_str_has_prefix (post_request, "mediaplayerget"))
    {
      gchar *get;

      get = g_strdup (post_request + strlen ("mediaplayerget="));

      MEX_DEBUG ("mediaplayerget = %s", get);

      self->data = dbus_client_player_get (self->dbus_client, get);
      g_free (get);

      MEX_DEBUG ("result \"%s\"", self->data);

      if (!self->data || strlen (self->data) == 0)
        self->data = g_strdup ("Unknown media");

      send_response (server, msg, path, self, CUSTOM);
      return;
    }

  else if (g_str_has_prefix (post_request, "playeraction"))
    {
      gchar *action;

      action = g_strdup (post_request + strlen ("playeraction="));

      MEX_DEBUG ("playeraction = %s", action);

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

  MEX_DEBUG ("%s %s HTTP/1.%d", msg->method, path,
             soup_message_get_http_version (msg));

  if (msg->method == SOUP_METHOD_POST)
    http_post (server, msg, path, self);
  else if (msg->method == SOUP_METHOD_GET || msg->method == SOUP_METHOD_HEAD)
    send_response (server, msg, path, self, NORMAL);
  else
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
}

static void
remind_user_pass (MexWebRemote *webremote)
{
  gchar **userpass;
  gchar *message;

  if (!webremote->userpass)
    return;

  userpass = g_strsplit (webremote->userpass, ":", 2);

  message = g_strdup_printf ("Webremote Username: %s Password: %s",
                             userpass[0], userpass[1]);

  dbus_client_input_set_message (webremote->dbus_client, message, 20);

  g_free (message);
  g_strfreev (userpass);
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

  if (g_strcmp0 (temp, self->userpass) == 0)
    {
      MEX_DEBUG ("Authentication success");
      g_free (temp);
      return TRUE;
    }
  else
    {
      MEX_DEBUG ("Authentication failure");

      remind_user_pass (self);

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

static void
new_connection_cb (SoupSocket *sock, SoupSocket *new, MexWebRemote *webremote)
{
  SoupAddress *client_address;
  guint ipaddresshash;
  const gchar *client_address_str;

  /* create glist of connected clients to see if they have connected
   * before if they haven't then notify the username and password.
   */
  client_address = soup_socket_get_remote_address (new);
  if (client_address)
    client_address_str = soup_address_get_physical (client_address);
  else
    return;

  ipaddresshash = g_str_hash (client_address_str);

  if (g_list_find (webremote->clients, GUINT_TO_POINTER (ipaddresshash)))
    return;

  webremote->clients =
    g_list_prepend (webremote->clients, GUINT_TO_POINTER (ipaddresshash));


  remind_user_pass (webremote);

  if (webremote->opt_debug)
    MEX_DEBUG ("New connection %s", client_address_str);
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
  MexWebRemote webremote = { 0, };

  SoupServer *server;
  SoupAuthDomain *domain;

  SoupAddress *address;
  SoupAddress *interface_address;

  gint dbus_service_id;

  GOptionContext *context;
  GRand *randomiser;

  SoupSocket *server_socket;

  GError *error=NULL;

  GOptionEntry entries[] =
    {
        { "bind", 'b', 0, G_OPTION_ARG_STRING, &webremote.opt_interface,
          "Bind to a particular ip address", NULL },
        { "port", 'p', 0, G_OPTION_ARG_INT, &webremote.opt_port,
          "Port to listen to http requests on", NULL }, 
        { "auth", 'a', 0, G_OPTION_ARG_STRING, &webremote.opt_auth,
          "username:password", NULL },
        { "noauth", 'n', 0, G_OPTION_ARG_NONE, &webremote.opt_noauth,
          "Disable authentication", NULL },
        { NULL }
    };

  /* Use the bus backend for tracker - the default one is flakey.. */
  g_setenv ("TRACKER_SPARQL_BACKEND", "bus", TRUE);

  g_type_init ();

  MEX_LOG_DOMAIN_INIT (webremote_log_domain, "webremote");

  context = g_option_context_new ("- Media explorer web remote");

  g_option_context_add_main_entries (context, entries, NULL);

  settings_load (&webremote);

  /* Command line arguments override those specified in a config file */

  if (!g_option_context_parse (context, &argc, &argv, &error))
    g_warning ("Failed to parse options: %s\n", error->message);

  webremote.clients = NULL;

  if (!webremote.mex_data_dir)
    {
      g_warning ("Could not find program data please verify your installation");
      goto clean_up;
    }

  /* We want to talk to dbus, tracker and avahi/mdns */
  webremote.dbus_client = dbus_client_new ();
  webremote.tracker_interface = tracker_interface_new ();
  webremote.mdns_service = mdns_service_info_new ();

  /* Allocate our playing info cache:
   * 0 - uri
   * 1 - json info on uri
   */
  webremote.current_playing_info = g_malloc0 (2 * sizeof (gchar *));

  /* Start the our own dbus service for the Quit method and auto activation */
  dbus_service_id = dbus_service_start ();

  if (webremote.opt_interface)
    {
      gint resolve_status;

      interface_address = soup_address_new (webremote.opt_interface,
                                            webremote.opt_port);

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
                   webremote.opt_interface);
          goto clean_up;
        }
    }
  else
    {
      server = soup_server_new (SOUP_SERVER_PORT, webremote.opt_port,
                                SOUP_SERVER_SERVER_HEADER, "mex-remote-control",
                                NULL);
    }

  if (!server)
    {
      g_error ("Sorry, unable to start server on port %d", webremote.opt_port);
      goto clean_up;
    }

  soup_server_run_async (server);

  /* Avahi/mdns advertise server */
  webremote.mdns_service->port = webremote.opt_port;
  webremote.mdns_service->type = "_http._tcp";
  webremote.mdns_service->name = "Mex Webremote";

  /* Edit /etc/avahi/avahi-daemon.conf for further configuration */
  mdns_service_start (webremote.mdns_service);

  g_message ("WebServer started");

  dbus_client_input_set_message (webremote.dbus_client, "Webremote started", 5);

  g_object_get (server, "interface", &address, NULL);

  /* Probably don't need to run the resolve but it's more reliable to do so. */
  soup_address_resolve_async (address, NULL, NULL, address_resolved_cb, NULL);

  /* use password */
  if (!webremote.opt_noauth)
    {
      if (webremote.opt_auth)
        {
          /* Use the argument if set */
          webremote.userpass = g_strdup (webremote.opt_auth);
        }
      else
        {
          /* Generate a password */
          randomiser = g_rand_new ();

          webremote.userpass =
            g_strdup_printf ("mex:%d%d%d%d",
                             g_rand_int_range (randomiser, 0, 9),
                             g_rand_int_range (randomiser, 0, 9),
                             g_rand_int_range (randomiser, 0, 9),
                             g_rand_int_range (randomiser, 0, 9));

          g_rand_free (randomiser);

          g_message ("User Password %s", webremote.userpass);
        }

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

  server_socket = soup_server_get_listener (server);

  g_signal_connect (server_socket, "new-connection",
                    (GCallback) new_connection_cb,
                    &webremote);



  signal (SIGINT, sig_webremote_quit);
  signal (SIGTERM, sig_webremote_quit);

  main_loop = g_main_loop_new (NULL, TRUE);

  g_main_loop_run (main_loop);

clean_up:

  if (dbus_service_id > 0)
    dbus_service_stop (dbus_service_id);

  if (error)
    g_error_free (error);

  if (webremote.userpass)
    g_free (webremote.userpass);

  if (webremote.clients)
    g_list_free (webremote.clients);

  if (webremote.dbus_client)
    dbus_client_free (webremote.dbus_client);

  if (webremote.tracker_interface)
    tracker_interface_free (webremote.tracker_interface);

  if (webremote.mdns_service)
    mdns_service_info_free (webremote.mdns_service);

  if (webremote.current_playing_info)
    g_strfreev (webremote.current_playing_info);

  if (context)
    g_option_context_free (context);

  return EXIT_SUCCESS;
}
