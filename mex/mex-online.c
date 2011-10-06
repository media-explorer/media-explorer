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

#include <glib.h>
#include <gio/gio.h>

#include "mex-online.h"
#include "mex-marshal.h"

/* This is the common infrastructure */
typedef struct {
  MexOnlineNotify callback;
  gpointer user_data;
} ListenerData;

static GList *listeners = NULL;

static gboolean online_init (void);

/**
 * mex_online_add_notify: (skip)
 * @callback: the callback to call we come online
 * @user_data: the data given to the callback when called
 *
 */
void
mex_online_add_notify (MexOnlineNotify callback,
                       gpointer        user_data)
{
  ListenerData *data;

  if (!online_init ())
    return;

  data = g_slice_new (ListenerData);
  data->callback = callback;
  data->user_data = user_data;

  listeners = g_list_prepend (listeners, data);
}

/**
 * mex_online_remove_notify: (skip)
 * @callback: the callback to remove
 * @user_data: the data given when the callback was added
 *
 */
void
mex_online_remove_notify (MexOnlineNotify callback,
                          gpointer        user_data)
{
  GList *l = listeners;

  while (l) {
    ListenerData *data = l->data;
    if (data->callback == callback && data->user_data == user_data) {
      GList *next = l->next;
      listeners = g_list_delete_link (listeners, l);
      l = next;
    } else {
      l = l->next;
    }
  }
}

static void
emit_notify (gboolean online)
{
  GList *l;

  for (l = listeners; l; l = l->next) {
    ListenerData *data = l->data;
    data->callback (online, data->user_data);
  }
}

#ifdef WITH_ONLINE_ALWAYS

/*
 * A bit nasty but this case never uses emit_notify we get a compile warning
 * otherwise.
 */
static const gpointer dummy = &emit_notify;

static gboolean
online_init (void)
{
  return FALSE;
}

gboolean
mex_is_online (void)
{
  return TRUE;
}

#endif

#ifdef WITH_ONLINE_NM
#include <libnm-glib/nm-client.h>

/*
 * Use NMClient since it correctly handles the NetworkManager service
 * appearing and disappearing, as can happen at boot time, or during
 * a network subsystem restart.
 */
static NMClient *client = NULL;

static gboolean
we_are_online (gpointer user_data)
{
  emit_notify (mex_is_online ());
  return FALSE;
}

static void
state_changed (NMClient         *client_in,
               const GParamSpec *pspec,
               gpointer          data)
{
  if (mex_is_online()) {
    /* NM is notifying us too early - workaround that */
    g_timeout_add (1500, (GSourceFunc)we_are_online, NULL);
  } else {
    emit_notify (FALSE); /* mex_is_online ()); */
  }
}

static gboolean
online_init (void)
{
  if (!client) {
    client = nm_client_new();
    g_signal_connect (client,
                      "notify::" NM_CLIENT_STATE,
                      G_CALLBACK (state_changed),
                      NULL);
  }
  return TRUE;
}

gboolean
mex_is_online (void)
{
  NMState state = NM_STATE_UNKNOWN;

  if (!online_init ())
    return TRUE;

  g_object_get (G_OBJECT (client), NM_CLIENT_STATE, &state, NULL);

  switch (state) {
  case NM_STATE_CONNECTED:
    return TRUE;
  case NM_STATE_CONNECTING:
  case NM_STATE_ASLEEP:
  case NM_STATE_DISCONNECTED:
  case NM_STATE_UNKNOWN:
  default:
    return FALSE;
  }
}

#endif


#ifdef WITH_ONLINE_CONNMAN
#include <string.h>

static GDBusProxy *proxy = NULL;
static gboolean current_state;

static void
connman_manager_signal (GDBusProxy *aproxy,
                        gchar      *sender_name,
                        gchar      *signal_name,
                        GVariant   *parameters,
                        gpointer    user_data)
{

  g_debug ("Signal : %s", signal_name);
  if (!g_strcmp0 (signal_name, "StateChanged"))
    {
      gchar *new_state;

      g_variant_get (parameters, "(s)", &new_state);

      current_state = (g_strcmp0 (new_state, "online") == 0);

      emit_notify (current_state);

      g_free (new_state);
    }
}

static void
got_state_cb (GDBusProxy   *aproxy,
              GAsyncResult *res,
              void         *user_data)
{
  char *state = NULL;
  GError *error = NULL;
  GVariant *variant;

  variant = g_dbus_proxy_call_finish (aproxy, res, &error);

  if (!variant) {
    g_printerr ("Cannot get current online state: %s", error->message);
    g_error_free (error);
    return;
  }

  g_variant_get (variant, "(s)", &state);

  current_state = (g_strcmp0 (state, "online") == 0);
  emit_notify (current_state);
  g_free (state);

  g_variant_unref (variant);
}

static gboolean
online_init (void)
{
  GError *error = NULL;

  if (proxy)
    return TRUE;

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "net.connman",
                                         "/",
                                         "net.connman.Manager",
                                         NULL, &error);

  if (!proxy)
    {
      g_printerr ("Could not connect to Connection Manager: %s",
                  error->message);
      g_error_free (error);
      return FALSE;
    }


  g_signal_connect (proxy, "g-signal", G_CALLBACK (connman_manager_signal),
                    NULL);

  current_state = FALSE;

  /* Get the current state */
  g_dbus_proxy_call (proxy, "GetState", NULL, G_DBUS_CALL_FLAGS_NONE, -1,
                     NULL, (GAsyncReadyCallback) got_state_cb, NULL);

  return TRUE;
}


gboolean
mex_is_online (void)
{
  if (!online_init ())
    return TRUE;

  return current_state;
}
#endif
