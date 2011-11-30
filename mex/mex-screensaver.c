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

#include "mex-screensaver.h"
#include <gio/gio.h>

G_DEFINE_TYPE (MexScreensaver, mex_screensaver, G_TYPE_OBJECT)

#define SCREENSAVER_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SCREENSAVER, \
                                  MexScreensaverPrivate))

struct _MexScreensaverPrivate
{
  gint gnome_version;

  guint32 cookie;
};

static void
mex_screensaver_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_screensaver_parent_class)->finalize (object);
}

static void
mex_screensaver_class_init (MexScreensaverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexScreensaverPrivate));

  object_class->finalize = mex_screensaver_finalize;
}

static void
mex_screensaver_init (MexScreensaver *self)
{
  self->priv = SCREENSAVER_PRIVATE (self);
}

MexScreensaver *
mex_screensaver_new (void)
{
  return g_object_new (MEX_TYPE_SCREENSAVER, NULL);
}

static GDBusProxy *
connect_gnome_screensaverd (MexScreensaver *self)
{
 MexScreensaverPrivate *priv = MEX_SCREENSAVER (self)->priv;
 GDBusProxy *proxy = NULL;

 /* Try with gnome 2 api first. gnome_version 0 is the default
  * representing the undetermined state
  */
 if (priv->gnome_version == 2 || priv->gnome_version == 0)
   {
     proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL,
                                            "org.gnome.ScreenSaver",
                                            "/org/gnome/ScreenSaver",
                                            "org.gnome.ScreenSaver",
                                            NULL, NULL);
   }

 /* The gnome 2 api didn't work and we've been called again with version 3 */
 if (priv->gnome_version == 3)
   {
     proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL,
                                            "org.gnome.SessionManager",
                                            "/org/gnome/SessionManager",
                                            "org.gnome.SessionManager",
                                            NULL, NULL);
  }

 return proxy;
}

void
mex_screensaver_inhibit (MexScreensaver *self)
{
  MexScreensaverPrivate *priv = MEX_SCREENSAVER (self)->priv;

  GDBusProxy *proxy;
  GError *error = NULL;
  GVariant *variant;

  /* If we're already inhibited don't inhibit again */
  if (priv->cookie > 0)
    return;

  if (priv->gnome_version == -1)
    return;

  proxy = connect_gnome_screensaverd (self);

  /* we always get a proxy unless proxy creating went wrong. even if
   * the target doesn't exist.
   */
  if (!proxy)
    return;

  /* gnome_version will be 0 if the current version has not been determined */
  if (priv->gnome_version == 0 || priv->gnome_version == 2)
    {

      if ((variant = g_dbus_proxy_call_sync (proxy, "Inhibit",
                                            g_variant_new ("(ss)",
                                                           "Media Explorer",
                                                           "Playing media"),
                                            G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                                            &error)))
        {
          priv->gnome_version = 2;
          g_variant_get (variant, "(u)", &priv->cookie);
          g_object_unref (proxy);
          g_variant_unref (variant);
        }
      else
        {
          if (error->domain == G_DBUS_ERROR &&
              (error->code == G_DBUS_ERROR_UNKNOWN_METHOD ||
              error->code == G_DBUS_ERROR_SERVICE_UNKNOWN))
            {
              g_clear_error (&error);
              priv->gnome_version = 3;
              /* the current proxy is useless to us */
              g_object_unref (proxy);
              proxy = NULL;
            }
        }
    }
  /* The code path may originate from the bail out on G_DBUS_ERROR_UNKNOWN_METHOD
   * or if the version has been set by a previous call of the inhibit function
   * which has worked out the version.
   */
  if (priv->gnome_version == 3)
    {
      /* proxy maybe null if the proxy was a gnome 2 proxy */
      if (!proxy)
        proxy = connect_gnome_screensaverd (self);

      /* 8 = GSM_INHIBITOR_FLAG_IDLE */
      if ((variant = g_dbus_proxy_call_sync (proxy, "Inhibit",
                                            g_variant_new ("(susu)",
                                                           "MediaExplorer",
                                                           0, "Playing media",
                                                           8),
                                            G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                                            &error)))
        {
          priv->gnome_version = 3;
          g_variant_get (variant, "(u)", &priv->cookie);
          g_object_unref (proxy);
          g_variant_unref (variant);
        }
      else
        {
          if (error->domain == G_DBUS_ERROR &&
              (error->code == G_DBUS_ERROR_UNKNOWN_METHOD ||
              error->code == G_DBUS_ERROR_SERVICE_UNKNOWN))
            {
              g_clear_error (&error);
              priv->gnome_version = -1;
              g_object_unref (proxy);
              proxy = NULL;
            }
        }
    }

  if (error)
    {
      g_warning ("Problem inhibiting screensaver: %s", error->message);
      g_error_free (error);
    }
}

void
mex_screensaver_uninhibit (MexScreensaver *self)
{
  MexScreensaverPrivate *priv = MEX_SCREENSAVER (self)->priv;

  GDBusProxy *proxy;
  GError *error = NULL;

  /* we're not inhibiting */
  if (priv->cookie == 0)
    return;

  if (priv->cookie)
    {
      proxy = connect_gnome_screensaverd (self);

      if (!proxy)
        return;

      if (priv->gnome_version == 2)
        {
          g_dbus_proxy_call_sync (proxy, "UnInhibit",
                                  g_variant_new ("(u)", priv->cookie),
                                  G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                                  &error);
        }

      if (priv->gnome_version == 3)
        {
          g_dbus_proxy_call_sync (proxy, "Uninhibit",
                                  g_variant_new ("(u)", priv->cookie),
                                  G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                                  &error);
        }

      if (error)
        {
          g_warning ("Problem uninhibiting screensaver: %s",
                     error->message);
          g_error_free (error);
        }
      else
        {
          /* reset the cookie */
          priv->cookie = 0;
        }
      g_object_unref (proxy);
    }
}
