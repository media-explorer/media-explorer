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

#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>
#include "dbus-client.h"

#include "mex/mex-player-common.h"

static void
dbus_client_player_uri_changed_cb (GDBusConnection *connection,
                                   const gchar     *sender_name,
                                   const gchar     *object_path,
                                   const gchar     *interface_name,
                                   const gchar     *signal_name,
                                   GVariant        *parameters,
                                   DBusClient      *dbus_client)
{
  if (dbus_client->current_playing_uri)
    {
      g_free (dbus_client->current_playing_uri);
      dbus_client->current_playing_uri = NULL;
    }

  g_variant_get (parameters, "(s)", &dbus_client->current_playing_uri);
}

static GDBusProxy *
dbus_player_proxy_new (DBusClient *dbus_client)
{
  GError *error=NULL;
  GDBusProxy *proxy;

  if (dbus_client->mex_player)
    g_object_unref (dbus_client->mex_player);

  proxy = g_dbus_proxy_new_sync (dbus_client->connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 MEX_PLAYER_SERVICE_NAME,
                                 MEX_PLAYER_OBJECT_PATH,
                                 MEX_PLAYER_INTERFACE_NAME,
                                 NULL,
                                 &error);

  /* Connect to the uri changed signal */
  g_dbus_connection_signal_subscribe (dbus_client->connection,
                                      MEX_PLAYER_SERVICE_NAME,
                                      MEX_PLAYER_INTERFACE_NAME,
                                      "UriChanged",
                                      MEX_PLAYER_OBJECT_PATH,
                                      NULL,
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      (GDBusSignalCallback)
                                      dbus_client_player_uri_changed_cb,
                                      dbus_client,
                                      NULL);

  if (error)
    {
      g_warning ("Could not create media player proxy: %s", error->message);
      g_error_free (error);
    }
  return proxy;
}

static GDBusProxy *
dbus_input_proxy_new (DBusClient *dbus_client)
{
  GError *error=NULL;
  GDBusProxy *proxy;

  if (dbus_client->mex_input)
    g_object_unref (dbus_client->mex_input);

  proxy = g_dbus_proxy_new_sync (dbus_client->connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 "org.media-explorer.MediaExplorer",
                                 "/org/MediaExplorer/Input",
                                 "org.MediaExplorer.Input",
                                 NULL,
                                 &error);
  if (error)
    {
      g_warning ("Could not create dbus input proxy: %s", error->message);
      g_error_free (error);
    }
  return proxy;
}

static gboolean
verify_dbus_player_proxy (DBusClient *dbus_client)
{
  if (dbus_client->mex_player)
      return TRUE;
  else
    {
      if ((dbus_client->mex_player = dbus_player_proxy_new (dbus_client)))
        return TRUE;
      else
        return FALSE;
    }
  return FALSE;
}

static gboolean
verify_dbus_input_proxy (DBusClient *dbus_client)
{
  if (dbus_client->mex_player)
      return TRUE;
  else
    {
      if ((dbus_client->mex_player = dbus_input_proxy_new (dbus_client)))
        return TRUE;
      else
        return FALSE;
    }
  return FALSE;
}

void
dbus_client_input_set_key (DBusClient *dbus_client, gint keyval)
{
  GError *error=NULL;

  if (!verify_dbus_input_proxy (dbus_client))
    return;

  g_dbus_proxy_call_sync (dbus_client->mex_input,
                          "ControlKey",
                          g_variant_new ("(u)", keyval),
                          0,
                          -1,
                          NULL,
                          &error);
  if (error)
    {
      g_warning ("Problem calling ControlKey: %s", error->message);
      g_error_free (error);
    }
}

void
dbus_client_input_set_message (DBusClient  *dbus_client,
                               const gchar *message,
                               guint        timeout)
{
  GError *error=NULL;

  if (!verify_dbus_input_proxy (dbus_client))
    return;

  g_dbus_proxy_call_sync (dbus_client->mex_input,
                          "Notification",
                          g_variant_new ("(su)", message, timeout),
                          0,
                          -1,
                          NULL,
                          &error);
  if (error)
    {
      g_warning ("Problem calling Notification: %s", error->message);
      g_error_free (error);
    }
}

void
dbus_client_player_volume_set (DBusClient  *dbus_client,
			const gchar *action,
                        gdouble       *value)

{
  GError *error=NULL;

  if (!verify_dbus_player_proxy (dbus_client))
    return;

  if (g_strcmp0 (action, "setvolume") == 0)
      {
        g_dbus_proxy_call_sync (dbus_client->mex_player,
                                "SetAudioVolume",
                                g_variant_new ("(d)", value),
                                0,
                                -1,
                                NULL,
                                &error);
      }

  if (error)
    {
      g_warning ("Problem calling %s: %s", action, error->message);
      g_error_free (error);
    }
}


void
dbus_client_player_set (DBusClient  *dbus_client,
                        const gchar *action,
                        gchar       *value)

{
  GError *error=NULL;

  if (!verify_dbus_player_proxy (dbus_client))
    return;

  if (g_strcmp0 (action, "seturi") == 0)
      {
        g_dbus_proxy_call_sync (dbus_client->mex_player,
                                "SetUri",
                                g_variant_new ("(s)", value),
                                0,
                                -1,
                                NULL,
                                &error);
      }

  if (error)
    {
      g_warning ("Problem calling %s: %s", action, error->message);
      g_error_free (error);
    }
}


gchar *
dbus_client_player_get (DBusClient *dbus_client, const gchar *get)
{
  GVariant *result;
  gchar *string = NULL;
  GError *error=NULL;

  if (!verify_dbus_player_proxy (dbus_client))
    return NULL;

  if (g_strcmp0 (get, "uri") == 0)
    {
      /* we expect our returned string to be freed */
      if (dbus_client->current_playing_uri)
        {
          string = g_strdup (dbus_client->current_playing_uri);
          return string;
        }
      else
        {
          result = g_dbus_proxy_call_sync (dbus_client->mex_player,
                                         "GetUri", NULL, 0, -1, NULL,
                                         &error);

          g_variant_get (result, "(s)", &dbus_client->current_playing_uri);
        }
    }
  else if (g_strcmp0 (get, "duration") == 0)
    {
      result = g_dbus_proxy_call_sync (dbus_client->mex_player,
                                      "GetDuration", NULL, 0, -1, NULL,
                                      &error);
    }
  else if (g_strcmp0 (get, "progress") == 0)
    {
      result = g_dbus_proxy_call_sync (dbus_client->mex_player,
                                       "GetProgress", NULL, 0, -1, NULL,
                                       &error);
    }


  if (error)
    {
      g_warning ("problem calling %s: %s", get, error->message);
      g_error_free (error);
      return NULL;
    }


  if (result)
    {
      g_variant_get (result, "(s)", &string);
      g_variant_unref (result);
    }

  return string;
}

void
dbus_client_player_action (DBusClient *dbus_client, const gchar *action)
{
  GError *error=NULL;
  gboolean seek_forward;

  if (!verify_dbus_player_proxy (dbus_client))
    return;

  if (g_strcmp0 (action, "playpause") == 0)
    {
      GVariant *playing;
      gboolean isplaying;

      playing = g_dbus_proxy_call_sync (dbus_client->mex_player,
                                        "GetPlaying", NULL, 0, -1, NULL,
                                        &error);
      if (error)
        goto fail_out;

      g_variant_get (playing, "(b)", &isplaying);

      g_variant_unref (playing);

      g_dbus_proxy_call_sync (dbus_client->mex_player,
                              "SetPlaying",
                              g_variant_new ("(b)", !isplaying),
                              0,
                              -1,
                              NULL,
                              &error);

    }
  else if ((seek_forward = (g_strcmp0 (action, "seekfwd") == 0)) ||
           g_strcmp0 (action, "seekbk") == 0)
    {
      GVariant *v_progress;
      gdouble progress;

      v_progress = g_dbus_proxy_call_sync (dbus_client->mex_player,
                                           "GetProgress", NULL, 0, -1,
                                           NULL, &error);
      if (error)
        goto fail_out;

      g_variant_get (v_progress, "(d)", &progress);

      if (seek_forward)
        {
          /* move on by 1% */
          progress = progress + 0.01;
        }
      else
        {
          progress = progress - 0.01;
        }

      g_dbus_proxy_call_sync (dbus_client->mex_player,
                              "SetProgress",
                              g_variant_new ("(d)", progress),
                              0,
                              -1,
                              NULL,
                              &error);

    }

fail_out:

  if (error)
    {
      g_warning ("problem calling %s: %s", action, error->message);
      g_error_free (error);
    }
}


DBusClient *dbus_client_new (void)
{
  DBusClient *dbus_client;
  GError *error = NULL;

  dbus_client = g_new0 (DBusClient, 1);

  dbus_client->mex_input = NULL;
  dbus_client->mex_player = NULL;

  dbus_client->connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                                               NULL, &error);
  if (error)
    {
      g_warning ("Failed to connect to dbus %s\n",
               error->message ? error->message : "Unknown");
      g_error_free (error);
    }
  else
    {
      dbus_client->mex_input = dbus_input_proxy_new (dbus_client);
      dbus_client->mex_player = dbus_player_proxy_new (dbus_client);
     }
  return dbus_client;
}


void dbus_client_free (DBusClient *dbus_client)
{
  g_object_unref (dbus_client->connection);
  g_object_unref (dbus_client->mex_input);
  g_object_unref (dbus_client->mex_player);
  g_free (dbus_client->current_playing_uri);
  g_free (dbus_client);
}
