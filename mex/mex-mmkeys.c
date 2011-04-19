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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <string.h>
#include <gio/gio.h>

#include "mex-mmkeys.h"

G_DEFINE_TYPE (MexMMkeys, mex_mmkeys, G_TYPE_OBJECT)

#define MMKEYS_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MMKEYS, MexMMkeysPrivate))

struct _MexMMkeysPrivate
{
  GDBusProxy *proxy;
  ClutterActor *stage;
  gint key_grab_active : 1;
};

static void
mex_mmkeys_finalize (GObject *object)
{
  MexMMkeys *mmkeys = MEX_MMKEYS (object);
  MexMMkeysPrivate *priv = MEX_MMKEYS (mmkeys)->priv;

  if (priv->proxy)
    {
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  G_OBJECT_CLASS (mex_mmkeys_parent_class)->finalize (object);
}

static void
mex_mmkeys_class_init (MexMMkeysClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexMMkeysPrivate));

  object_class->finalize = mex_mmkeys_finalize;
}

static void
mex_mmkeys_init (MexMMkeys *self)
{
  self->priv = MMKEYS_PRIVATE (self);
}

MexMMkeys *
mex_mmkeys_new (void)
{
  return g_object_new (MEX_TYPE_MMKEYS, NULL);
}

void
mex_mmkeys_set_stage (MexMMkeys *self, ClutterActor *stage)
{
  MexMMkeysPrivate *priv = MEX_MMKEYS (self)->priv;

  priv->stage = stage;
}

static void
mex_mmkeys_control (MexMMkeys *self, const gchar *key)
{
  MexMMkeysPrivate *priv = MEX_MMKEYS (self)->priv;

  ClutterEvent *event;
  ClutterKeyEvent *kevent;

  event = clutter_event_new (CLUTTER_KEY_PRESS);
  kevent = (ClutterKeyEvent *)event;

  /* These don't change */
  kevent->flags = 0;
  kevent->source = priv->stage;
  kevent->stage = CLUTTER_STAGE (priv->stage);

  if (g_strcmp0 (key, "Play") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioPlay;
      goto end;
    }

  if (g_strcmp0 (key, "Pause") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioPause;
      goto end;
    }

  if (g_strcmp0 (key, "Stop") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioStop;
      goto end;
    }

  if (g_strcmp0 (key, "FastForward") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioForward;
      goto end;
    }

  if (g_strcmp0 (key, "Rewind") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioRewind;
      goto end;
    }

  if (g_strcmp0 (key, "Next") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioNext;
      goto end;
    }

  if (g_strcmp0 (key, "Previous") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioPrev;
      goto end;
    }

  /* One day we might be able to get volume keys from gnome setting daemon:
   * https://bugzilla.gnome.org/show_bug.cgi?id=647166
   * In the meantime you can get these keys by unbinding them
   */

  if (g_strcmp0 (key, "VolumeUp") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioRaiseVolume;
      goto end;
    }

  if (g_strcmp0 (key, "VolumeDown") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioLowerVolume;
      goto end;
    }

  if (g_strcmp0 (key, "VolumeMute") == 0)
    {
      kevent->keyval = CLUTTER_KEY_AudioMute;
      goto end;
    }

end:
  kevent->time = time (NULL);
  clutter_event_put (event);
  clutter_event_free (event);
}

static void
mm_keys_pressed (GDBusProxy *proxy,
                  const char *sender,
                  const char *signalin,
                  GVariant *parameters,
                  MexMMkeys *self)
{
  gchar *key;
  gchar *application;

  g_variant_get (parameters, "(ss)", &application, &key);

  /* check that we're the intended application */
  if (g_strcmp0 (application, "media-explorer") > 0)
    {
      g_free (application);
      g_free (key);
      return;
    }

  mex_mmkeys_control (self, key);

  g_free (application);
  g_free (key);
}

static void
keys_ungrab_complete_cb (GObject *proxy,
                         GAsyncResult *result,
                         MexMMkeys *self)
{
  MexMMkeysPrivate *priv = MEX_MMKEYS (self)->priv;
  GError *error = NULL;

  g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), result, &error);

  if (error)
    {
      g_warning ("media player keys not released: %s", error->message);
      g_clear_error (&error);
    }
  else
    {
      if (priv->proxy)
        g_object_unref (priv->proxy);

      priv->key_grab_active = FALSE;
    }
}

static void
keys_grab_complete_cb (GObject *proxy,
                       GAsyncResult *result,
                       MexMMkeys *self)
{
  MexMMkeysPrivate *priv = MEX_MMKEYS (self)->priv;
  GError *error = NULL;

  g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), result, &error);

  if (error)
    {
      g_warning ("media player keys not available: %s", error->message);
      g_clear_error (&error);
    }
  else
    {
      priv->key_grab_active = TRUE;

      g_signal_connect_object (priv->proxy,
                               "g-signal",
                               G_CALLBACK (mm_keys_pressed),
                               self, 0);

    }
}

void
mex_mmkeys_grab_keys (MexMMkeys *self)
{
  MexMMkeysPrivate *priv = MEX_MMKEYS (self)->priv;

  GDBusConnection *connection;
  GError *error = NULL;

  if (priv->key_grab_active)
    return;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (error)
    {
      g_warning ("Could not connect to dbus %s\n", error->message);
      g_clear_error (&error);
    }

    priv->proxy = g_dbus_proxy_new_sync (connection,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.gnome.SettingsDaemon",
                                         "/org/gnome/SettingsDaemon/MediaKeys",
                                         "org.gnome.SettingsDaemon.MediaKeys",
                                         NULL,
                                         &error);
  if (error)
    {
      g_warning ("Could not grab media player keys: %s\n", error->message);
      g_clear_error (&error);
    }
  else
    {
      g_dbus_proxy_call (priv->proxy,
                         "GrabMediaPlayerKeys",
                         g_variant_new ("(su)", "media-explorer", 0),
                         G_DBUS_CALL_FLAGS_NONE,
                         -1,
                         NULL,
                         (GAsyncReadyCallback)keys_grab_complete_cb,
                         self);
    }
}

void
mex_mmkeys_ungrab_keys (MexMMkeys *self)
{
  MexMMkeysPrivate *priv = MEX_MMKEYS (self)->priv;

  if (!priv->key_grab_active)
    return;

  if (priv->proxy)
    {
      g_dbus_proxy_call (priv->proxy,
                         "ReleaseMediaPlayerKeys",
                         g_variant_new ("(s)", "media-explorer"),
                         G_DBUS_PROXY_FLAGS_NONE,
                         -1,
                         NULL,
                         (GAsyncReadyCallback) keys_ungrab_complete_cb,
                         self);

      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }
}
