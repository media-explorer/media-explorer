/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright 2009, 2010 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 */


#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>
#include "dbus-interface.h"

void
httpdbus_send_keyvalue (HTTPDBusInterface *dbus_interface, gint keyval)
{
  /* we're using sync at the moment because we don't have a queue for events
   * yet. And besides.. we're calling this all aysnc from the web server.
   */
  GError *error=NULL;

  g_dbus_proxy_call_sync (dbus_interface->dbusinput_proxy,
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
httpdbus_media_player_set_uri (HTTPDBusInterface *dbus_interface,
                               gchar *uri)
{
  GError *error=NULL;

  g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                          "SetUri",
                          g_variant_new ("(s)", uri),
                          0,
                          -1,
                          NULL,
                          &error);

  g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                          "SetPlaying",
                          g_variant_new ("(b)", 1),
                          0,
                          -1,
                          NULL,
                          &error);
  if (error)
    {
      g_warning ("Problem calling SetUri: %s", error->message);
      g_error_free (error);
    }
}


gchar *
httpdbus_media_player_get (HTTPDBusInterface *dbus_interface, gchar *get)
{
  GVariant *result;
  gchar *string;
  GError *error=NULL;

  if (g_strcmp0 (get, "uri") == 0)
    {
      result = g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                                       "GetUri", NULL, 0, -1, NULL, &error);
      if (error)
        {
          g_warning ("problem calling GetUri %s", error->message);
          g_clear_error (&error);
        }
    }
  else if (g_strcmp0 (get, "duration") == 0)
    {
      result = g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                                      "GetDuration", NULL, 0, -1, NULL, &error);
      if (error)
        {
          g_warning ("problem calling GetDuration %s", error->message);
          g_clear_error (&error);
        }
    }
  else if (g_strcmp0 (get, "progress") == 0)
    {
      result = g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                                       "GetProgress", NULL, 0, -1, NULL,
                                       &error);
      if (error)
        {
          g_warning ("problem calling GetProgress %s", error->message);
          g_clear_error (&error);
        }
    }

  if (!result)
    return NULL;

  if (error)
    g_error_free (error);

  g_variant_get (result, "(s)", &string);

  g_variant_unref (result);

  /* soup take memory takes care of the memory here */
  return string;
}

void
httpdbus_media_player_action (HTTPDBusInterface *dbus_interface, gchar *action)
{
  GError *error=NULL;
  gboolean seek_forward;

  if (g_strcmp0 (action, "playpause") == 0)
    {
      GVariant *playing;
      gboolean isplaying;

      playing = g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                                        "GetPlaying", NULL, 0, -1, NULL,
                                        &error);

      if (error)
        {
          g_warning ("problem calling GetPlaying %s", error->message);
          g_clear_error (&error);
        }

      g_variant_get (playing, "(b)", &isplaying);

      g_variant_unref (playing);

      g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                              "SetPlaying",
                              g_variant_new ("(b)", !isplaying),
                              0,
                              -1,
                              NULL,
                              &error);
      if (error)
        {
          g_warning ("problem calling SetPlaying %s", error->message);
          g_clear_error (&error);
        }
    }

  else if ((seek_forward = (g_strcmp0 (action, "seekfwd") == 0)) ||
           g_strcmp0 (action, "seekbk") == 0)
    {
      GVariant *v_progress;
      gdouble progress;

      v_progress = g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                                         "GetProgress", NULL, 0, -1,
                                         NULL, &error);
      if (error)
        {
          g_warning ("problem calling GetProgress %s", error->message);
          g_clear_error (&error);
        }

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

      g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                              "SetProgress",
                              g_variant_new ("(d)", progress),
                              0,
                              -1,
                              NULL,
                              &error);
      if (error)
        {
          g_warning ("problem calling SetProgress %s", error->message);
          g_clear_error (&error);
        }
    }

  /*
    v_duration = g_dbus_proxy_call_sync (dbus_interface->mediaplayer_proxy,
                                         "GetDuration", NULL, 0, -1,
                                         NULL, &error);
      if (error)
        {
          g_warning ("problem calling GetDuration %s", error->message);
          g_clear_error (&error);
        }

      g_variant_get (v_duration, "(d)", &duration);
      */

  if (error)
    g_error_free (error);
}



HTTPDBusInterface *httpdbus_interface_new (void)
{
  HTTPDBusInterface *dbus_interface;

  dbus_interface = g_new0 (HTTPDBusInterface, 1);
  GError *error=NULL;

  dbus_interface->connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                                               NULL, &error);
  if (error)
    {
      g_warning ("Failed to connect to dbus %s\n",
               error->message ? error->message : "Unknown");
      g_error_free (error);
    }
  else
    {
      dbus_interface->dbusinput_proxy =
        g_dbus_proxy_new_sync (dbus_interface->connection,
                               G_DBUS_PROXY_FLAGS_NONE,
                               NULL,
                               "com.meego.mex.inputctrl",
                               "/com/meego/mex/InputControl",
                               "com.meego.mex.Input",
                               NULL,
                               &error);
      if (error)
        {
          g_warning ("Could not create dbus input proxy: %s", error->message);
          g_clear_error (&error);
        }

      dbus_interface->mediaplayer_proxy =
        g_dbus_proxy_new_sync (dbus_interface->connection,
                               G_DBUS_PROXY_FLAGS_NONE,
                               NULL,
                               "com.meego.mex.player",
                               "/com/meego/mex/player",
                               "com.meego.mex.MediaPlayer",
                               NULL,
                               &error);
      if (error)
        {
          g_warning ("Could not create media player proxy: %s", error->message);
          g_error_free (error);
        }
    }
  return dbus_interface;
}


void httpdbus_interface_free (HTTPDBusInterface *dbus_interface)
{
  g_object_unref (dbus_interface->connection);
  g_object_unref (dbus_interface->dbusinput_proxy);
  g_object_unref (dbus_interface->mediaplayer_proxy);
  g_free (dbus_interface);
}
