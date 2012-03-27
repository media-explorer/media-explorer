/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
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
 * Implementes the mrpis media player spec http://mpris.org
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
  ClutterMedia *media;

  GDBusConnection *connection;
  GDBusNodeInfo *introspection_data;
};

static void
mex_mpris_plugin_finalize (GObject *object)
{
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (object)->priv;

  if (priv->connection)
    g_object_unref (priv->connection);

  g_dbus_node_info_unref (priv->introspection_data);

  G_OBJECT_CLASS (mex_mpris_plugin_parent_class)->finalize (object);
}

static void
mex_mpris_plugin_class_init (MexMprisPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexMprisPluginPrivate));

  object_class->finalize = mex_mpris_plugin_finalize;
}

/* clutter gst >= 1.5.4 */
#if 0
static void
_check_if_seeked (ClutterMedia   *media,
                  GParamSpec     *pspec,
                  MexMprisPlugin *self)
{
  MexMprisPluginPrivate *priv = MEX_MPRIS_PLUGIN (self)->priv;

  if (clutter_gst_player_get_in_seek (CLUTTER_GST_PLAYER (priv->media)))
    {
      gdobule progress, duration;
      gint64 newposition;

      duration = clutter_media_get_duration (priv->media);
      progress = clutter_media_get_progress (priv->media);

      newposition = duration * progress * 1000000;

      g_dbus_connection_emit_signal (priv->connection,
                                     NULL, MPRIS_OBJECT_NAME,
                                     MPRIS_PLAYER_INTERFACE, "Seeked",
                                     paramters, NULL);
    }
}
#endif

static GVariant *
_root_property_cb (GDBusConnection  *connection,
                   const gchar      *sender,
                   const gchar      *object_path,
                   const gchar      *interface_name,
                   const gchar      *property_name,
                   GError          **error,
                   gpointer          user_data)
{
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


static GVariant *
_player_property_cb (GDBusConnection  *connection,
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

  media = priv->media;

  if (0 == g_strcmp0 ("PlaybackStatus", property_name))
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

  else if (0 == g_strcmp0 ("LoopStatus", property_name))
    v = g_variant_new_string ("None");

  else if (0 == g_strcmp0 ("Rate", property_name) ||
           0 == g_strcmp0 ("MinimumRate", property_name) ||
           (0 == g_strcmp0 ("MinimumRate", property_name)))
    v = g_variant_new_double (1.0);

  else if (0 == g_strcmp0 ("Shuffle", property_name))
    v = g_variant_new_boolean (FALSE);

  else if (0 == g_strcmp0 ("Volume", property_name)) 
    v = g_variant_new_double (clutter_media_get_audio_volume (media));

  else if (0 == g_strcmp0 ("Position", property_name))
    {
      gdouble duration_s = clutter_media_get_duration (media);
      gdouble progress_rel = clutter_media_get_progress (media);
      gint64 position_ms = duration_s * 1000000 * progress_rel;
      v = g_variant_new_int64 (position_ms);
    }

  else if (0 == g_strcmp0 ("CanGoNext", property_name) ||
           0 == g_strcmp0 ("CanGoPrevious", property_name) ||
           0 == g_strcmp0 ("CanPlay", property_name) ||
           0 == g_strcmp0 ("CanControl", property_name) ||
           0 == g_strcmp0 ("CanPause", property_name))
    v = g_variant_new_boolean (TRUE);

  else if (0 == g_strcmp0 ("CanSeek", property_name))
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
   mex_player_next (priv->player);

 else if (g_strcmp0 (method_name, "OpenUri") == 0)
   {
     const gchar *uri;
     g_variant_get (parameters, "(s)", &uri);
     mex_player_set_uri (priv->player, uri);
   }

 else if (g_strcmp0 (method_name, "Pause") == 0)
   mex_player_pause (priv->player);

 else if (g_strcmp0 (method_name, "Play") == 0)
   mex_player_play (priv->player);

 else if (g_strcmp0 (method_name, "PlayPause") == 0)
   mex_player_pause (priv->player);

 else if (g_strcmp0 (method_name, "Previous") == 0)
   mex_player_previous (priv->player);

 else if (g_strcmp0 (method_name, "Seek") == 0)
   {
     gint64 seek_offset;
     g_variant_get (parameters, "(x)", &seek_offset);
     mex_player_seek_us (priv->player, seek_offset);
   }

 else if (g_strcmp0 (method_name, "Stop") == 0)
   mex_player_quit (priv->player);


  g_dbus_method_invocation_return_value (invocation, NULL);
}

static const GDBusInterfaceVTable player_interface_table =
{
  (GDBusInterfaceMethodCallFunc) _player_method_cb,
  (GDBusInterfaceGetPropertyFunc) _player_property_cb,
  NULL
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
  priv->player = mex_player_get_default ();
  priv->media = mex_player_get_clutter_media (priv->player);

  /* clutter gst >= 1.5.4 */
#if 0
  g_signal_connect (priv->media,
                    "notify::in-seek",
                    G_CALLBACK (_check_if_seeked),
                    self);
#endif

  g_bus_own_name (G_BUS_TYPE_SESSION,
                  MPRIS_BUS_NAME_PREFIX ".mediaexplorer",
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  NULL);

  if (!(priv->introspection_data = g_dbus_node_info_new_for_xml (mpris_introspection_xml, &error)))
      g_warning ("Error %s", error->message);


  g_bus_get (G_BUS_TYPE_SESSION, NULL, on_bus_acquired, self);
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
