/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
 * Copyright © 2012 sleep(5) Ltd.
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

/* mex-mpris-plugin.c
 * Implements the mrpis media player spec http://mpris.org
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-mpris-plugin.h"
#include "mpris-spec.h"

#include <string.h>
#include <gio/gio.h>
#include <clutter-gst/clutter-gst.h>
#include <clutter-gst/clutter-gst-player.h>


G_DEFINE_TYPE (MexMprisPlugin, mex_mpris_plugin, G_TYPE_OBJECT)

#define MPRIS_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MPRIS_PLUGIN, MexMprisPluginPrivate))

struct _MexMprisPluginPrivate
{
  MexPlayer *player;
  MexMusicPlayer *music;

  GDBusConnection *connection;
  GDBusNodeInfo *introspection_data;
  gchar **mimes_supported;
};

static void
mex_mpris_plugin_finalize (GObject *object)
{
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (object)->priv;

  if (priv->connection)
    g_object_unref (priv->connection);

  g_dbus_node_info_unref (priv->introspection_data);

  if (priv->mimes_supported)
    g_strfreev (priv->mimes_supported);

  G_OBJECT_CLASS (mex_mpris_plugin_parent_class)->finalize (object);
}

static void
mex_mpris_plugin_class_init (MexMprisPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexMprisPluginPrivate));

  object_class->finalize = mex_mpris_plugin_finalize;
}

static void
_check_if_seeked (ClutterMedia   *media,
                  GParamSpec     *pspec,
                  MexMprisPlugin *self)
{
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (self)->priv;

  if (clutter_gst_player_get_in_seek (CLUTTER_GST_PLAYER (media)))
    {
      gdouble progress, duration;
      gint64 newposition;

      duration = clutter_media_get_duration (media);
      /* progress is in a range of 0.0 1.0 so convert to position in uS */
      progress = clutter_media_get_progress (media);

      newposition = duration * progress * 1000000;

      g_dbus_connection_emit_signal (priv->connection,
                                     NULL, MPRIS_OBJECT_NAME,
                                     MPRIS_PLAYER_INTERFACE, "Seeked",
                                     g_variant_new ("(x)", newposition),
                                     NULL);
    }
}

static GVariant *
_root_property_cb (GDBusConnection  *connection,
                   const gchar      *sender,
                   const gchar      *object_path,
                   const gchar      *interface_name,
                   const gchar      *property_name,
                   GError          **error,
                   MexMprisPlugin   *self)
{
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (self)->priv;

  if (g_strcmp0 (property_name, "SupportedUriSchemes") == 0)
    {
      const char *supported_schemes[] = {
          "file", "http", "cdda", "smb", "sftp", "ftp", NULL
      };
      return g_variant_new_strv (supported_schemes, -1);
    }

  if (g_strcmp0 (property_name, "SupportedMimeTypes") == 0)
    return g_variant_new_strv ((const gchar**)priv->mimes_supported, -1);

  if (g_strcmp0 (property_name, "CanQuit") == 0 ||
      g_strcmp0 (property_name, "CanRaise") == 0 ||
      g_strcmp0 (property_name, "HasTrackList") == 0)
    return g_variant_new_boolean (FALSE);

  if (g_strcmp0 (property_name, "DesktopEntry") == 0)
    return g_variant_new_string ("media-explorer");

  if (g_strcmp0 (property_name, "Identity") == 0)
    return g_variant_new_string ("Media Explorer");

  g_set_error (error,
               G_DBUS_ERROR,
               G_DBUS_ERROR_NOT_SUPPORTED,
               "Property %s.%s not supported",
               interface_name,
               property_name);

  return NULL;
}

static gboolean
_set_player_property_cb (GDBusConnection  *connection,
                         const gchar      *sender,
                         const gchar      *object_path,
                         const gchar      *interface_name,
                         const gchar      *property_name,
                         GVariant         *value,
                         GError           **error,
                         gpointer          user_data)
{
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (user_data)->priv;

  /* Currently unsupported properties */
  if (g_strcmp0 (property_name, "LoopStatus") == 0 ||
      g_strcmp0 (property_name, "Rate") == 0 ||
      g_strcmp0 (property_name, "Shuffle") == 0)
    return FALSE;

  if (g_strcmp0 (property_name, "Volume") == 0)
    {
      ClutterMedia *media;

      if (mex_music_player_is_playing (priv->music))
        media = mex_music_player_get_clutter_media (priv->music);
      else
        media = mex_player_get_clutter_media (priv->player);

      clutter_media_set_audio_volume (media, g_variant_get_double (value));
      return TRUE;
    }

  g_set_error (error,
               G_DBUS_ERROR,
               G_DBUS_ERROR_NOT_SUPPORTED,
               "Property %s.%s not supported",
               interface_name,
               property_name);

  return FALSE;
}


static GVariant *
_get_player_property_cb (GDBusConnection  *connection,
                         const gchar      *sender,
                         const gchar      *object_path,
                         const gchar      *interface_name,
                         const gchar      *property_name,
                         GError          **error,
                         gpointer          user_data)
{
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (user_data)->priv;
  ClutterMedia *media;
  GVariant *v = NULL;

  if (mex_music_player_is_playing (priv->music))
    media = mex_music_player_get_clutter_media (priv->music);
  else
    media = mex_player_get_clutter_media (priv->player);

  if (g_strcmp0 ("PlaybackStatus", property_name) == 0)
    {
      /* Doesn't map to ClutterMedia straight away so try to emulate.
       * Playback could theoretically be paused at progress 0.0 but well ...*/
      gdouble progress = clutter_media_get_progress (media);
      gboolean playing = clutter_media_get_playing (media);

      if (playing)
          v = g_variant_new_string ("Playing");
      else if (progress != 0)
          v = g_variant_new_string ("Paused");
       else
          v = g_variant_new_string ("Stopped");
    }

  else if (g_strcmp0 ("LoopStatus", property_name) == 0)
    v = g_variant_new_string ("None");

  else if (g_strcmp0 ("Rate", property_name) == 0 ||
           g_strcmp0 ("MinimumRate", property_name)  == 0 ||
           g_strcmp0 ("MinimumRate", property_name) == 0 )
    v = g_variant_new_double (1.0);

  else if (g_strcmp0 ("Shuffle", property_name) == 0)
    v = g_variant_new_boolean (FALSE);

  else if (g_strcmp0 ("Volume", property_name) == 0)
    v = g_variant_new_double (clutter_media_get_audio_volume (media));

  else if (g_strcmp0 ("Position", property_name) == 0)
    {
      gdouble duration_s = clutter_media_get_duration (media);
      gdouble progress_rel = clutter_media_get_progress (media);
      gint64 position_ms = duration_s * 1000000 * progress_rel;
      v = g_variant_new_int64 (position_ms);
    }

  else if (g_strcmp0 ("CanGoNext", property_name) == 0 ||
           g_strcmp0 ("CanGoPrevious", property_name) == 0 ||
           g_strcmp0 ("CanPlay", property_name) == 0 ||
           g_strcmp0 ("CanControl", property_name) == 0 ||
           g_strcmp0 ("CanPause", property_name) == 0)
    v = g_variant_new_boolean (TRUE);

  else if (g_strcmp0 ("CanSeek", property_name) == 0)
    v = g_variant_new_boolean (clutter_media_get_can_seek (media));

  if (v)
    return v;

  g_set_error (error,
               G_DBUS_ERROR,
               G_DBUS_ERROR_NOT_SUPPORTED,
               "Property %s.%s not supported",
               interface_name,
               property_name);
  return NULL;
}

static char *
_content_mime_prefix (const gchar *uri)
{
  char *pfx = NULL;

  g_return_val_if_fail (uri, NULL);

  if (g_str_has_prefix (uri, "dvd") ||
      g_str_has_prefix (uri, "vcd"))
    {
      pfx = g_strdup ("video");
      return pfx;
    }
  else
    {
      gchar *guess;

      if ((guess = g_content_type_guess (uri, NULL, 0, NULL)))
        {
          char *p;

          if ((p = strchr (guess, '/')))
            *p = 0;

          pfx = guess;
        }
    }

  return pfx;
}

static void
_player_method_cb (GDBusConnection       *connection,
                   const gchar           *sender,
                   const gchar           *object_path,
                   const gchar           *interface_name,
                   const gchar           *method_name,
                   GVariant              *parameters,
                   GDBusMethodInvocation *invocation,
                   MexMprisPlugin        *self)
{
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (self)->priv;

 if (g_strcmp0 (method_name, "Next") == 0)
   {
     if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->music)) ||
         mex_music_player_is_playing (priv->music))
       mex_music_player_next (priv->music);
     else
       mex_player_next (priv->player);
   }
 else if (g_strcmp0 (method_name, "OpenUri") == 0)
   {
     const gchar *uri;
     char *pfx;

     g_variant_get (parameters, "(s)", &uri);

     if ((pfx = _content_mime_prefix (uri)))
       {
         if (!strcmp (pfx, "audio"))
           mex_music_player_set_uri (priv->music, uri);
         else if (!strcmp (pfx, "image"))
           g_warning ("Mpris image playback is not implemented");
         else
           mex_player_set_uri (priv->player, uri);
       }
     else
       mex_player_set_uri (priv->player, uri);
   }
 else if (g_strcmp0 (method_name, "Pause") == 0)
   {
     if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->music)) ||
         mex_music_player_is_playing (priv->music))
       mex_music_player_stop (priv->music);
     else
       mex_player_pause (priv->player);
   }
 else if (g_strcmp0 (method_name, "Play") == 0)
   {
     if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->music)) ||
         mex_music_player_is_playing (priv->music))
       mex_music_player_play (priv->music);
     else
       mex_player_play (priv->player);
   }
 else if (g_strcmp0 (method_name, "PlayPause") == 0)
   {
     if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->music)) ||
         mex_music_player_is_playing (priv->music))
       mex_music_player_play_toggle (priv->music);
     else
       mex_player_playpause (priv->player);
   }
 else if (g_strcmp0 (method_name, "Previous") == 0)
   {
     if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->music)) ||
         mex_music_player_is_playing (priv->music))
       mex_music_player_previous (priv->music);
     else
       mex_player_previous (priv->player);
   }
 else if (g_strcmp0 (method_name, "Seek") == 0)
   {
     gint64 seek_offset;
     g_variant_get (parameters, "(x)", &seek_offset);

     if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->music)) ||
         mex_music_player_is_playing (priv->music))
       mex_music_player_seek_us (priv->music, seek_offset);
     else
       mex_player_seek_us (priv->player, seek_offset);
   }
 else if (g_strcmp0 (method_name, "Stop") == 0)
   {
     if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->music)) ||
         mex_music_player_is_playing (priv->music))
       mex_music_player_quit (priv->music);
     else
       mex_player_quit (priv->player);
   }
 else
     g_message ("Unhandled MPRIS Player method %s", method_name);

  g_dbus_method_invocation_return_value (invocation, NULL);
}

static const GDBusInterfaceVTable player_interface_table =
{
  (GDBusInterfaceMethodCallFunc) _player_method_cb,
  (GDBusInterfaceGetPropertyFunc) _get_player_property_cb,
  (GDBusInterfaceSetPropertyFunc) _set_player_property_cb
};

static const GDBusInterfaceVTable root_interface_table =
{
  /* Currently we can do any of the methods so just ignore them */
  NULL,
  (GDBusInterfaceGetPropertyFunc) _root_property_cb,
  NULL
};

static void
on_bus_acquired (GObject      *source_object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  MexMprisPlugin *self = user_data;
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (self)->priv;
  GError *error = NULL;

  priv->connection = g_bus_get_finish (result, &error);
  if (error)
    {
      g_warning ("Could not acquire bus connection: %s", error->message);
      g_error_free (error);
      return;
    }

  /* Note: Dbus object and name subject to change */
  g_dbus_connection_register_object (priv->connection,
                                     MPRIS_OBJECT_NAME,
                                     priv->introspection_data->interfaces[0],
                                     &root_interface_table,
                                     self,
                                     NULL,
                                     &error);

  g_dbus_connection_register_object (priv->connection,
                                     MPRIS_OBJECT_NAME,
                                     priv->introspection_data->interfaces[1],
                                     &player_interface_table,
                                     self,
                                     NULL,
                                     &error);

  if (error)
    {
      g_warning ("Problem registering object: %s", error->message);
      g_error_free (error);
    }
}

static void
mex_mpris_plugin_init (MexMprisPlugin *self)
{
  MexMprisPluginPrivate *priv = self->priv =
    MPRIS_PLUGIN_PRIVATE (self);

  GError *error=NULL;
  ClutterMedia *media;

  priv->player = mex_player_get_default ();
  media = mex_player_get_clutter_media (priv->player);

  g_signal_connect (media,
                    "notify::in-seek",
                    G_CALLBACK (_check_if_seeked),
                    self);

  priv->music = mex_music_player_get_default ();
  media = mex_music_player_get_clutter_media (priv->music);

  g_signal_connect (media,
                    "notify::in-seek",
                    G_CALLBACK (_check_if_seeked),
                    self);

  g_bus_own_name (G_BUS_TYPE_SESSION,
                  MPRIS_BUS_NAME_PREFIX ".media-explorer",
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  NULL);

  if (!(priv->introspection_data = g_dbus_node_info_new_for_xml (mpris_introspection_xml, &error)))
      g_warning ("Error %s", error->message);


  g_bus_get (G_BUS_TYPE_SESSION, NULL, on_bus_acquired, self);

  priv->mimes_supported = g_strsplit (MEX_MIME_TYPES, ";", -1);
}

static GType
mex_mpris_get_type (void)
{
  return MEX_TYPE_MPRIS_PLUGIN;
}

MEX_DEFINE_PLUGIN ("mpris",
                   "Mpris.org plugin",
                   PACKAGE_VERSION,
                   "LGPLv2.1+",
                   "Michael Wood <michael.g.wood@linux.intel.com>",
                   MEX_API_MAJOR, MEX_API_MINOR,
                   mex_mpris_get_type,
                   MEX_PLUGIN_PRIORITY_NORMAL)
