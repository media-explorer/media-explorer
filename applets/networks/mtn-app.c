/*
 * mex-networks - Connection Manager UI for Media Explorer 
 * Copyright © 2010-2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St 
 * - Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <string.h>
#include <glib/gi18n.h>
#include <mex/mex.h>

#include "mtn-app.h"

#include "mtn-button-box.h" 
#include "mtn-connman.h" 
#include "mtn-connman-service.h" 
#include "mtn-services-view.h" 

#define MTN_APP_NAME "Media Explorer Networks"
#define MTN_APP_CONTENT_WIDTH 740.0
#define MTN_APP_CONTENT_HEIGHT 650.0
#define MTN_APP_BUTTON_WIDTH 170.0
#define MTN_APP_ENTRY_WIDTH 240.0
#define MTN_APP_COL_SPACE 30.0

G_DEFINE_TYPE (MtnApp, mtn_app, MX_TYPE_APPLICATION)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MTN_TYPE_APP, MtnAppPrivate))

enum
{
    PROP_0,

    PROP_BACK_COMMAND,
    PROP_FIRST_BOOT,
};

typedef enum {
    MTN_APP_STATE_DEFAULT,

    MTN_APP_STATE_REQUEST_SSID_FOR_HIDDEN,
    MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN,
    MTN_APP_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN,

    MTN_APP_STATE_REQUEST_PASSPHRASE,

    MTN_APP_STATE_CONNECTING,
    MTN_APP_STATE_CONNECT_FAILED,
    MTN_APP_STATE_CONNECT_OK,
} MtnAppState;

struct _MtnAppPrivate {
    MtnAppState state;
    char *back_command;
    gboolean first_boot;

    MtnConnman *connman;
    MtnConnmanService *service;

    const char *security;

    MxWindow *win;
    MxFocusManager *fm;

    ClutterActor *title;

    ClutterActor *book;
    ClutterActor *main_page;
    ClutterActor *ssid_page;
    ClutterActor *security_page;
    ClutterActor *passphrase_page;
    ClutterActor *connecting_page;
    ClutterActor *connected_page;

    /* in main_page */
    ClutterActor *services_view;
    ClutterActor *error_frame;
    ClutterActor *connect_hidden_button;

    /* in ssid_page */
    ClutterActor *ssid_entry;

    /* in ssid_page */
    ClutterActor *none_button;
    ClutterActor *wep_button;
    ClutterActor *wpa_button;
  
    /* in passphrase_page */
    ClutterActor *service_label;
    ClutterActor *passphrase_entry;

    /* in connecting_page */
    ClutterActor *connecting_label;

    /* in connected_page */
    ClutterActor *connected_icon;
    ClutterActor *connected_label;

    ClutterActor *back_button;
    ClutterActor *forward_button;
};

static void mtn_app_set_state (MtnApp *app, MtnAppState state);

static gboolean
_need_passphrase (MtnConnmanService *service)
{
    gboolean failed, passphrase_req, immutable, secure, need_passphrase;
    const char **securities, **security;
    const char *str;
    GVariant *var;

    var = mtn_connman_service_get_property (service,
                                            "State");
    str = var ? g_variant_get_string (var, NULL): "failure";
    failed = (g_strcmp0 (str, "failure") == 0);

    var = mtn_connman_service_get_property (service,
                                            "PassphraseRequired");
    passphrase_req = var ? g_variant_get_boolean (var) : FALSE;

    var = mtn_connman_service_get_property (service,
                                            "Immutable");
    immutable = var ? g_variant_get_boolean (var) : FALSE;

    secure = FALSE;
    var = mtn_connman_service_get_property (service,
                                            "Security");
    securities = g_variant_get_strv (var, NULL);
    if (securities) {
      security = securities;
      while (*security) {
        if (g_strcmp0 (*security, "none") != 0)
          secure = TRUE;
        security++;
      }
      g_free (securities);
    }

    /* Need a paswword for non-immutable services that
     * A) ask for a passphrase or
     * B) failed to connect and have a use for the passphrase
     */

    need_passphrase = FALSE;
    if (!immutable) {
        need_passphrase = passphrase_req ||
                          (failed && secure);
    }

    return need_passphrase;
}

static void
_mx_label_set_markup (MxLabel *label, const char *str)
{
    ClutterActor *text;

    text = mx_label_get_clutter_text (label);
    clutter_text_set_markup (CLUTTER_TEXT (text), str);
}

static void
mtn_app_update_connect_hidden_button (MtnApp *app,
                                      GVariant *enabled_techs)
{
    GVariantIter *iter;
    char *tech;
    gboolean have_wifi;

    have_wifi = FALSE;
    if (enabled_techs) {
        g_variant_get (enabled_techs, "as", &iter);
        while (g_variant_iter_next (iter, "&s", &tech))
            if (g_strcmp0 (tech, "wifi") == 0) {
                have_wifi = TRUE;
                break;
            }
        g_variant_iter_free (iter);
    }

    g_object_set (app->priv->connect_hidden_button,
                  "visible", have_wifi,
                  NULL);
}

static void
mtn_app_update_passphrase_page (MtnApp *app)
{
    const char *name, *sec, *pw;
    char *str;
    GVariant *var;

    g_object_set (app->priv->service_label,
                  "visible", app->priv->service != NULL,
                  NULL);

    if (!app->priv->service) {
        /* hidden network */
        mx_entry_set_text (MX_ENTRY (app->priv->passphrase_entry), "");
        return;
    }

    var = mtn_connman_service_get_property (app->priv->service,
                                            "Name");
    name = var ? g_variant_get_string (var, NULL): "Network";
    var = mtn_connman_service_get_property (app->priv->service,
                                            "Security");
    sec = var ? g_variant_get_string (var, NULL): "";

    var = mtn_connman_service_get_property (app->priv->service,
                                            "Password");
    pw = var ? g_variant_get_string (var, NULL) : "";

    if (g_strcmp0 (sec, "rsn") == 0 ||
        g_strcmp0 (sec, "psk") == 0 ||
        g_strcmp0 (sec, "wpa") == 0) {
        /* TRANSLATORS: Info label in the service data input page.
           Placeholder is the name of the wireless network. */
        str = g_strdup_printf (_("<b>%s</b> requires a WPA password"),
                               name);
    } else if (g_strcmp0 (sec, "wep") == 0) {
        /* TRANSLATORS: Info label in the service data input page.
           Placeholder is the name of the wireless network. */
        str = g_strdup_printf (_("<b>%s</b> requires a WEP password"),
                               name);
    } else {
        /* TRANSLATORS: Info label in the service data input page.
           Placeholder is the name of the wireless network. */
        str = g_strdup_printf (_("<b>%s</b> requires a password"),
                               name);
    }

    mx_entry_set_text (MX_ENTRY (app->priv->passphrase_entry), pw);
    _mx_label_set_markup (MX_LABEL (app->priv->service_label), str);
    g_free (str);
}


static void
mtn_app_clear_service (MtnApp *app)
{
    if (app->priv->service) {
        g_object_unref (app->priv->service);
        app->priv->service = NULL;
    }
}

static void
mtn_app_update_forward_back_buttons (MtnApp *app)
{
    const char *text, *fwd_do_later;
    gboolean visible;

    /* TRANSLATORS: Button label, clicking skips this step in the 
       first-boot process. */
    fwd_do_later = _("Do later");


    /* TRANSLATORS: Button label. If user is on some other page, 
        clicking will return back to first page. If user is in
        the first page, will return the user to previous setup
        app (in first boot mode), or to Settings */
    text = _("Back");
    visible = TRUE;

    switch (app->priv->state) {
    case MTN_APP_STATE_CONNECT_FAILED:
        /* TRANSLATORS: Back-button label when connection failed. 
           Will take user back to first page. */
        text = _("Try again...");
        break;
    case MTN_APP_STATE_CONNECT_OK:
        visible = FALSE;
        break;
    default:
        ;
    }
    mx_button_set_label (MX_BUTTON (app->priv->back_button), text);
    g_object_set (app->priv->back_button, "visible", visible, NULL);


    /* TRANSLATORS: Button label, clicking will go to the next
       page */
    text = _("Next");
    visible = TRUE;

    switch (app->priv->state) {
    case MTN_APP_STATE_DEFAULT:
        visible = app->priv->first_boot;
        text = fwd_do_later;
        break;
    case MTN_APP_STATE_CONNECT_FAILED:
        /* TRANSLATORS: Button label, clicking skips this step in the 
           first-boot process. */
        text = fwd_do_later;
        break;
    case MTN_APP_STATE_REQUEST_PASSPHRASE:
    case MTN_APP_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN:
        /* TRANSLATORS: Button label, clicking starts the connection 
           attempt to the network */
        text = _("Join");
        break;
    case MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN:
    case MTN_APP_STATE_CONNECTING:
        visible = FALSE;
        break;
    default:
        ;
    }

    mx_button_set_label (MX_BUTTON (app->priv->forward_button), text);
    g_object_set (app->priv->forward_button, "visible", visible, NULL);
}

static void
mtn_app_update_connecting_page (MtnApp *app)
{
    const char *name;
    char *str;

    if (!app->priv->service) {
        /* hidden network */
        name = mx_entry_get_text (MX_ENTRY (app->priv->ssid_entry));
    } else {
        GVariant *var;

        var = mtn_connman_service_get_property (app->priv->service,
                                                "Name");
        name = var ? g_variant_get_string (var, NULL) : "";
    }

    str = g_strdup_printf ("Joining network <b>%s</b>...", name);
    _mx_label_set_markup (MX_LABEL (app->priv->connecting_label), str);
    g_free (str);
}

static void
mtn_app_update_connected_page (MtnApp *app, gboolean success)
{
    char *str;
    const char *name;

    if (!app->priv->service) {
        /* hidden network */
        name = mx_entry_get_text (MX_ENTRY (app->priv->ssid_entry));
    } else {
        GVariant *var;

        var = mtn_connman_service_get_property (app->priv->service,
                                                "Name");
        name = var ? g_variant_get_string (var, NULL) : "";
    }

    if (success) {
        str = g_strdup_printf ("Succesfully joined <b>%s</b>.", name);
        mx_stylable_set_style_class (MX_STYLABLE (app->priv->connected_icon),
                                     "mtn-connection-ok");
    } else {
        str = g_strdup_printf ("Sorry, we've been unable to join <b>%s</b>.",
                               name);
        mx_stylable_set_style_class (MX_STYLABLE (app->priv->connected_icon),
                                     "mtn-connection-failed");
    }

    _mx_label_set_markup (MX_LABEL (app->priv->connected_label), str);
    g_free (str);
}

static void
mtn_app_set_first_boot (MtnApp *app, gboolean first_boot)
{
    const char *str;

    app->priv->first_boot = first_boot;
    mtn_app_set_state (app, app->priv->state);

    if (first_boot) {
        /* TRANSLATORS: Title at the top of the UI.
           This version will appear when running in "first-boot" mode,
           please make sure the string matches the format used in the
           other "first-boot" applications. */
        str = _("Step 2: Join a network");
    } else {
        /* TRANSLATORS: Title at the top of the UI.
           This version will appear when started from Settings. */
        str = _("Join a network");
    }
    mx_label_set_text (MX_LABEL (app->priv->title), str);
}

static void
mtn_app_set_back_command (MtnApp *app, const char *command)
{
    if (app->priv->back_command)
        g_free (app->priv->back_command);

    if (!command || strlen (command) == 0)
        app->priv->back_command = NULL;
    else
        app->priv->back_command = g_strdup (command);
}

static void
mtn_app_execute_back_command (MtnApp *app)
{
    GError *error;

    error = NULL;

    if (app->priv->back_command &&
        !g_spawn_command_line_async (app->priv->back_command, &error))
    {
        g_warning ("Failed to execute back action: %s\n",
                   error->message);
        g_error_free (error);
    }

    mx_application_quit (MX_APPLICATION (app));
}

static void
_services_changed (MtnConnman      *connman,
                   GVariant        *var,
                   MtnApp          *app)
{
    static gboolean prev_have_services = TRUE;
    gboolean have_services;

    have_services = FALSE;
    if (var)
        have_services = (g_variant_n_children (var) > 0);

    if (have_services == prev_have_services)
        return;
    prev_have_services = have_services;

    g_object_set (app->priv->error_frame,
                  "visible", !have_services,
                  NULL);
}

static gboolean
_is_connman_error (GError *error, char *name)
{
    char *remote_error;

    if (!g_dbus_error_is_remote_error (error))
        return FALSE;

    remote_error = g_dbus_error_get_remote_error (error);
    if (!remote_error)
        return FALSE;

    if (g_str_has_prefix (remote_error, "net.connman.Error.")) {
        if (!name || g_str_has_suffix (remote_error, name))
            return TRUE;
    }

    return FALSE;
} 

static void
_connman_connect_cb (GObject      *object,
                     GAsyncResult *res,
                     gpointer      user_data)
{
    MtnApp *app;
    GVariant *var;
    MtnAppState state = MTN_APP_STATE_CONNECT_OK;
    GError *error;

    app = MTN_APP (user_data);
    error = NULL;

    var = g_dbus_proxy_call_finish (G_DBUS_PROXY (object),
                                    res,
                                    &error);
    if (var)
      g_variant_unref (var);

    if (error) {
        if (!_is_connman_error (error, "AlreadyConnected")) {
            g_warning ("Connecting failed: %s", error->message);
            state = MTN_APP_STATE_CONNECT_FAILED;
        }
        g_error_free (error);
    }

    mtn_app_set_state (app, state);
}

static void
mtn_app_connect_hidden_service (MtnApp *app)
{
    GVariantBuilder builder;
    GVariant *var;
    const char *pw, *ssid, *security;

    ssid = mx_entry_get_text (MX_ENTRY (app->priv->ssid_entry));
    security = app->priv->security;
    pw = mx_entry_get_text (MX_ENTRY (app->priv->passphrase_entry));

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("(a{sv})"));
    g_variant_builder_open (&builder , G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_add (&builder , "{sv}",
                           "Type", g_variant_new_string ("wifi"));
    g_variant_builder_add (&builder , "{sv}",
                           "Mode", g_variant_new_string ("managed"));
    g_variant_builder_add (&builder , "{sv}",
                           "SSID", g_variant_new_string (ssid));
    g_variant_builder_add (&builder , "{sv}",
                           "Security", g_variant_new_string (security));
    if (g_strcmp0 (security, "none") != 0)
        g_variant_builder_add (&builder , "{sv}",
                               "Passphrase", g_variant_new_string (pw));
    g_variant_builder_close (&builder);
    var = g_variant_builder_end (&builder);

    g_dbus_proxy_call (G_DBUS_PROXY (app->priv->connman),
                       "ConnectService",
                       var,
                       G_DBUS_CALL_FLAGS_NONE,
                       120000,
                       NULL,
                       _connman_connect_cb,
                       app);

    g_variant_unref (var);
}

static void
_set_passphrase_cb (GObject *object,
                    GAsyncResult *res,
                    gpointer user_data)
{
    MtnApp *app;
    GVariant *var;
    GError *error;

    app = MTN_APP (user_data);
    error = NULL;

    var = g_dbus_proxy_call_finish (G_DBUS_PROXY (object),
                                    res,
                                    &error);
    if (var) {
        g_variant_unref (var);

        /* Passphrase is set, now connect */
        g_dbus_proxy_call (G_DBUS_PROXY (app->priv->service),
                           "Connect",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           120000,
                           NULL,
                           _connman_connect_cb,
                           user_data);
    } else if (error) {
        g_warning ("Connman Service.SetProperty() for 'Passphrase' failed: %s",
                   error->message);
        g_error_free (error);

        mtn_app_set_state (app, MTN_APP_STATE_CONNECT_FAILED);
    }
}

static void
mtn_app_connect (MtnApp *app)
{
    if (_need_passphrase (app->priv->service)) {
        GVariant *var;
        const char *pw;

        /* cant use mtn_connman_service_set_property() because it is 
         * fire-and-forget and property-changed won't get emitted for 
         * passphrase in some cases */

        pw = mx_entry_get_text (MX_ENTRY (app->priv->passphrase_entry));
        var = g_variant_new ("(sv)",
                             "Passphrase",
                             g_variant_new_string (pw));
    
        g_dbus_proxy_call (G_DBUS_PROXY (app->priv->service),
                           "SetProperty",
                           var,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           _set_passphrase_cb,
                           app);
        g_variant_unref (var);
    } else {
        g_dbus_proxy_call (G_DBUS_PROXY (app->priv->service),
                           "Connect",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           120000,
                           NULL,
                           _connman_connect_cb,
                           app);
    }
}

static void
mtn_app_set_state (MtnApp *app, MtnAppState state)
{
    MtnAppPrivate *priv;
    gboolean error_visible;

    priv = app->priv;
    priv->state = state;

    mtn_app_update_forward_back_buttons (app);

    switch (state) {
    case MTN_APP_STATE_DEFAULT:
        mtn_app_clear_service (app);
        mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                      priv->main_page);

        g_object_get (app->priv->error_frame,
                      "visible", &error_visible,
                      NULL);
        if (error_visible)
          mx_focus_manager_push_focus (priv->fm,
                                       MX_FOCUSABLE (priv->back_button));
        else
          mx_focus_manager_push_focus (priv->fm,
                                       MX_FOCUSABLE (priv->services_view));
        break;

    case MTN_APP_STATE_REQUEST_SSID_FOR_HIDDEN:
        mx_entry_set_text (MX_ENTRY (priv->ssid_entry), "");
        mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                      priv->ssid_page);
        mx_focus_manager_push_focus (priv->fm,
                                     MX_FOCUSABLE (priv->ssid_entry));
        break;

    case MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN:
        mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                      priv->security_page);
        mx_focus_manager_push_focus (priv->fm,
                                     MX_FOCUSABLE (priv->none_button));
        break;

    case MTN_APP_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN:
    case MTN_APP_STATE_REQUEST_PASSPHRASE:
        mtn_app_update_passphrase_page (app);
        mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                      priv->passphrase_page);
        mx_focus_manager_push_focus (priv->fm,
                                     MX_FOCUSABLE (priv->passphrase_entry));
        break;

    case MTN_APP_STATE_CONNECTING:
        mtn_app_update_connecting_page (app);
        mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                      priv->connecting_page);
        mx_focus_manager_push_focus (priv->fm,
                                     MX_FOCUSABLE (priv->back_button));

        if (priv->service)
            mtn_app_connect (app);
        else
            mtn_app_connect_hidden_service (app);

        break;

    case MTN_APP_STATE_CONNECT_FAILED:
        mtn_app_update_connected_page (app, FALSE);
        mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                      priv->connected_page);
        mx_focus_manager_push_focus (priv->fm,
                                     MX_FOCUSABLE (priv->back_button));
        break;

    case MTN_APP_STATE_CONNECT_OK:
        mtn_app_update_connected_page (app,TRUE);
        mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                      priv->connected_page);
        mx_focus_manager_push_focus (priv->fm,
                                     MX_FOCUSABLE (priv->forward_button));
        break;    
    }
}

static void
mtn_app_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_app_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    case PROP_BACK_COMMAND:
        mtn_app_set_back_command (MTN_APP (object),
                                  g_value_get_string (value));
        break;
    case PROP_FIRST_BOOT:
        mtn_app_set_first_boot (MTN_APP (object),
                                g_value_get_boolean (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_app_dispose (GObject *object)
{
    MtnApp *app;

    app = MTN_APP (object);

    mtn_app_clear_service (app);

    if (app->priv->back_command) {
        g_free (app->priv->back_command);
        app->priv->back_command = NULL;
    }

    if (app->priv->connman) {
        g_object_unref (app->priv->connman);
        app->priv->connman = NULL;
    }
 
  G_OBJECT_CLASS (mtn_app_parent_class)->dispose (object);
}

static void
mtn_app_forward (MtnApp *app)
{
    switch (app->priv->state) {
    case MTN_APP_STATE_REQUEST_SSID_FOR_HIDDEN:
        mtn_app_set_state (app,
                           MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN);
        break;

    case MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN:
        if (g_strcmp0 (app->priv->security, "none") == 0)
            mtn_app_set_state (app,
                               MTN_APP_STATE_CONNECTING);
        else
            mtn_app_set_state (app,
                               MTN_APP_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN);
        break;

    case MTN_APP_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN:
        mtn_app_set_state (app, MTN_APP_STATE_CONNECTING);
        break;

    case MTN_APP_STATE_REQUEST_PASSPHRASE:
        mtn_app_set_state (app, MTN_APP_STATE_CONNECTING);
        break;

    case MTN_APP_STATE_CONNECTING:
        g_warn_if_reached ();
        break;

    case MTN_APP_STATE_DEFAULT:
    case MTN_APP_STATE_CONNECT_FAILED:
    case MTN_APP_STATE_CONNECT_OK:
        mx_application_quit (MX_APPLICATION (app));
        break;
    }
}

static void
mtn_app_back (MtnApp *app)
{
    switch (app->priv->state) {
    case MTN_APP_STATE_DEFAULT:
        mtn_app_execute_back_command (app);
        break;

    case MTN_APP_STATE_REQUEST_PASSPHRASE:
    case MTN_APP_STATE_REQUEST_SSID_FOR_HIDDEN:
    case MTN_APP_STATE_CONNECT_FAILED:
        mtn_app_set_state (app, MTN_APP_STATE_DEFAULT);
        break;

    case MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN:
        mtn_app_set_state (app, MTN_APP_STATE_REQUEST_SSID_FOR_HIDDEN);
        break;

    case MTN_APP_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN:
        mtn_app_set_state (app,
                           MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN);
        break;

    case MTN_APP_STATE_CONNECTING:
        mtn_app_set_state (app, MTN_APP_STATE_DEFAULT);
        break;

    case MTN_APP_STATE_CONNECT_OK:
        g_warn_if_reached ();
        break;
    }
}

static gboolean
_entry_key_release (ClutterActor    *actor,
                    ClutterKeyEvent *event,
                    MtnApp          *app)
{
    if (event->keyval == MEX_KEY_OK) {
        mtn_app_forward (app);
        return TRUE;
    }
    return FALSE;
}

static void
mtn_app_forward_key_release (MtnApp *app)
{
    gboolean go_forward;

    go_forward = FALSE;
    if (app->priv->state == MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN) {
        GObject *o;
        ClutterActor *focus;
        ClutterStage *stage;

        /* special case: forward button is not visible,
         * but if a security button is focused, we can proceed */

        o = NULL;
        stage = mx_window_get_clutter_stage (app->priv->win);
        focus = clutter_stage_get_key_focus (stage);
        if (focus == app->priv->none_button)
            o = G_OBJECT (app->priv->none_button);
        else if (focus == app->priv->wep_button)
            o = G_OBJECT (app->priv->wep_button);
        else if (focus == app->priv->wpa_button)
            o = G_OBJECT (app->priv->wpa_button);
        if (o) {
            app->priv->security = g_object_get_data (o, "security");
            go_forward = TRUE;
        }
    } else {
        gboolean visible, disabled;

        g_object_get (app->priv->forward_button,
                      "visible", &visible,
                      "disabled", &disabled,
                      NULL);
        if (visible && !disabled)
            go_forward = TRUE;
    }

    if (go_forward)
        mtn_app_forward (app);
}

static void
mtn_app_back_key_release (MtnApp *app)
{
    gboolean visible, disabled;

    g_object_get (app->priv->back_button,
                  "visible", &visible,
                  "disabled", &disabled,
                  NULL);
    if (visible && !disabled)
        mtn_app_back (app);
}

static gboolean
_stage_key_release (ClutterActor    *actor,
                    ClutterKeyEvent *event,
                    MtnApp          *app)
{
    switch (event->keyval) {
    case CLUTTER_Forward:
        mtn_app_forward_key_release (app);
        return TRUE;
    case MEX_KEY_BACK:
    case CLUTTER_Back:
        mtn_app_back_key_release (app);
        return TRUE;

#ifdef DEBUG
    case CLUTTER_1:
        mtn_app_set_state (app, MTN_APP_STATE_DEFAULT);
        return TRUE;
    case CLUTTER_2:
        mtn_app_set_state (app, MTN_APP_STATE_REQUEST_PASSPHRASE);
        return TRUE;
    case CLUTTER_3:
        mtn_app_set_state (app, MTN_APP_STATE_REQUEST_SSID_FOR_HIDDEN);
        return TRUE;
    case CLUTTER_4:
        mx_entry_set_text (MX_ENTRY (app->priv->ssid_entry), "Dummy");
        mtn_app_set_state (app, MTN_APP_STATE_REQUEST_SECURITY_FOR_HIDDEN);
        return TRUE;
    case CLUTTER_5:
        mx_entry_set_text (MX_ENTRY (app->priv->ssid_entry), "Dummy");
        app->priv->security = "wpa";
        mtn_app_set_state (app, MTN_APP_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN);
        return TRUE;
    case CLUTTER_6:
        mx_entry_set_text (MX_ENTRY (app->priv->ssid_entry), "Dummy");
        mtn_app_set_state (app, MTN_APP_STATE_CONNECTING);
        return TRUE;
    case CLUTTER_7:
        mx_entry_set_text (MX_ENTRY (app->priv->ssid_entry), "Dummy");
        mtn_app_set_state (app, MTN_APP_STATE_CONNECT_FAILED);
        return TRUE;
    case CLUTTER_8:
        mx_entry_set_text (MX_ENTRY (app->priv->ssid_entry), "Dummy");
        mtn_app_set_state (app, MTN_APP_STATE_CONNECT_OK);
        return TRUE;
#endif
    default:
        return FALSE;
    }
}

static void
_forward_clicked (MxButton *btn, MtnApp *app)
{
    mtn_app_forward (app);
}

static void
_back_clicked (MxButton *btn, MtnApp *app)
{
    mtn_app_back (app);
}

static void
_connect_security_clicked (MxButton *btn, MtnApp *app)
{
    app->priv->security = g_object_get_data (G_OBJECT (btn),
                                             "security");
    mtn_app_forward (app);
}

static void
_connman_new_cb (GObject *object,
                 GAsyncResult *res,
                 gpointer user_data)
{
    GError *error;
    MtnAppPrivate *priv;
    GVariant *var;
    const char *state;
    ClutterActor *stage;

    priv = MTN_APP (user_data)->priv;

    error = NULL;
    priv->connman = mtn_connman_new_finish (res, &error);
    if (!priv->connman) {
        g_warning ("Failed to create Connman proxy: %s", error->message);
        g_error_free (error);
        return;
    }

    /* on first boot, bail out if we already have a connection */
    var = mtn_connman_get_property (priv->connman, "State");
    state = var ? g_variant_get_string (var, NULL): "offline";

    if (priv->first_boot && g_strcmp0 (state, "online") == 0) {
        g_debug ("Exiting on first boot: already connected");
        mx_application_quit (MX_APPLICATION (user_data));
        return;
    }

    /* keep an eye on Services for the "no networks" info */
    g_signal_connect (priv->connman, "property-changed::Services",
                      G_CALLBACK (_services_changed), user_data);
    var = mtn_connman_get_property (priv->connman, "Services");
    _services_changed (priv->connman, var, user_data);

    /* hide "hidden network" UI if we do not have wifi */
    var = mtn_connman_get_property (priv->connman,
                                    "EnabledTechnologies");
    mtn_app_update_connect_hidden_button (MTN_APP (user_data), var);

    /* ServicesView shows all the services in a list */
    mtn_services_view_set_connman (MTN_SERVICES_VIEW (priv->services_view),
                                   priv->connman);

    mtn_app_set_state (MTN_APP (user_data), MTN_APP_STATE_DEFAULT);

    stage = CLUTTER_ACTOR (mx_window_get_clutter_stage (priv->win));
    clutter_actor_show (stage);
    clutter_stage_set_fullscreen (CLUTTER_STAGE (stage), TRUE);
}

static void
_connman_service_new (GObject *object,
                      GAsyncResult *res,
                      gpointer user_data)
{
    GError *error;
    MtnAppPrivate *priv;

    priv = MTN_APP (user_data)->priv;

    error = NULL;
    priv->service = mtn_connman_service_new_finish (res, &error);
    if (!priv->service) {
        g_warning ("Failed to create Connman service proxy: %s",
                   error->message);
        g_error_free (error);
 
        /* show error */
 
        return;
    }

    if (_need_passphrase (priv->service)) {
        mtn_app_set_state (MTN_APP (user_data),
                           MTN_APP_STATE_REQUEST_PASSPHRASE);
    } else {
        mtn_app_set_state (MTN_APP (user_data),
                           MTN_APP_STATE_CONNECTING);
    }
}

static void
_view_connection_requested (MtnServicesView *view,
                            const char *object_path,
                            MtnApp *app)
{
    if (!object_path) {
        g_warning ("No object path in connection request");
        return;
    }
    if (app->priv->service) {
        g_warning ("Connection requested, but still working on something else");
        mtn_app_clear_service (app);
    }
    mtn_connman_service_new (object_path, NULL,
                             _connman_service_new, app);
}

static void
_connect_hidden_clicked (MxButton *btn, MtnApp *app)
{
    mtn_app_set_state (app, MTN_APP_STATE_REQUEST_SSID_FOR_HIDDEN);
}

static void
mtn_app_class_init (MtnAppClass *klass)
{
    GParamSpec *pspec;
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MtnAppPrivate));

    object_class->get_property = mtn_app_get_property;
    object_class->set_property = mtn_app_set_property;
    object_class->dispose = mtn_app_dispose;

    pspec = g_param_spec_string ("back-command",
                                 "Back command",
                                 "Command to execute as the 'back' action",
                                 NULL,
                                 G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE);
    g_object_class_install_property (object_class,
                                     PROP_BACK_COMMAND,
                                     pspec);

    pspec = g_param_spec_boolean ("first-boot",
                                  "First-boot mode",
                                  "Enable first-boot mode",
                                  FALSE,
                                  G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE);
    g_object_class_install_property (object_class,
                                     PROP_FIRST_BOOT,
                                     pspec);
}

static ClutterActor*
mtn_app_get_connected_page (MtnApp *self)
{
    MtnAppPrivate *priv;
    ClutterActor *page, *box, *txt;

    priv = self->priv;

    page = mx_box_layout_new ();

    box = mx_box_layout_new ();
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (page),
                                             box,
                                             -1,
                                             "expand", TRUE,
                                             "y-fill", FALSE,
                                             NULL);

    priv->connected_icon = mx_icon_new ();
    mx_icon_set_icon_size (MX_ICON (priv->connected_icon), 26);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (box),
                                             self->priv->connected_icon,
                                             -1,
                                             "expand", FALSE,
                                             NULL);

    priv->connected_label = mx_label_new ();
    txt = mx_label_get_clutter_text (MX_LABEL (priv->connected_label));
    clutter_text_set_ellipsize (CLUTTER_TEXT (txt),
                                PANGO_ELLIPSIZE_NONE);
    clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (box),
                                             priv->connected_label,
                                             -1,
                                             "expand", TRUE,
                                             "y-fill", FALSE,
                                             "x-fill", FALSE,
                                             "x-align", MX_ALIGN_START,
                                             "y-align", MX_ALIGN_MIDDLE,
                                             NULL);

    return page;
}

static ClutterActor*
mtn_app_get_connecting_page (MtnApp *self)
{
    MtnAppPrivate *priv;
    ClutterActor *page, *box, *spinner, *txt;

    priv = self->priv;

    page = mx_box_layout_new ();

    box = mx_box_layout_new ();
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (page),
                                             box,
                                             -1,
                                             "expand", TRUE,
                                             "y-fill", FALSE,
                                             NULL);

    spinner = mx_spinner_new ();
    clutter_actor_set_name (spinner, "mtn-connecting-spinner");
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (box), spinner, -1,
                                             "expand", TRUE,
                                             "x-fill", FALSE,
                                             "x-align", MX_ALIGN_END,
                                             NULL);

    priv->connecting_label = mx_label_new ();
    txt = mx_label_get_clutter_text (MX_LABEL (priv->connecting_label));
    clutter_text_set_ellipsize (CLUTTER_TEXT (txt),
                                PANGO_ELLIPSIZE_NONE);
    clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (box),
                                             priv->connecting_label,
                                             -1,
                                             "expand", TRUE,
                                             "y-fill", FALSE,
                                             "x-align", MX_ALIGN_START,
                                             "y-align", MX_ALIGN_MIDDLE,
                                             NULL);

    return page;
}

static ClutterActor*
mtn_app_get_passphrase_page (MtnApp *self)
{
    MtnAppPrivate *priv;
    ClutterActor *page, *table, *label, *txt;

    priv = self->priv;

    page = mx_box_layout_new ();

    table = mx_table_new ();
    mx_table_set_column_spacing (MX_TABLE (table), MTN_APP_COL_SPACE);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (page),
                                             table,
                                             -1,
                                             "expand", TRUE,
                                             "y-fill", FALSE,
                                             "x-fill", TRUE,
                                             NULL);

    priv->service_label = mx_label_new ();
    txt = mx_label_get_clutter_text (MX_LABEL (priv->service_label));
    clutter_text_set_ellipsize (CLUTTER_TEXT (txt),
                                PANGO_ELLIPSIZE_NONE);
    clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
    mx_table_add_actor_with_properties (MX_TABLE (table),
                                        priv->service_label,
                                        0, 0,
                                        "x-align", MX_ALIGN_START,
                                        "x-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-expand", FALSE,
                                        "y-fill", FALSE,
                                        "column-span", 2,
                                        NULL);


    /* TRANSLATORS: label to the left of a password entry. Max length 
       is around 28 characters */
    label = mx_label_new_with_text (_("Password:"));
    mx_table_add_actor_with_properties (MX_TABLE (table),
                                        label,
                                        1, 0,
                                        "y-expand", FALSE,
                                        "y-fill", FALSE,
                                        "y-align", MX_ALIGN_MIDDLE,
                                        "x-expand", FALSE,
                                        NULL);

    priv->passphrase_entry = mx_entry_new ();
    clutter_actor_set_size (priv->passphrase_entry, 
                            MTN_APP_ENTRY_WIDTH,
                            -1);
    g_signal_connect (priv->passphrase_entry, "key-release-event",
                      G_CALLBACK (_entry_key_release), self);
    mx_table_add_actor_with_properties (MX_TABLE (table),
                                        priv->passphrase_entry,
                                        1, 1,
                                        "x-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-expand", FALSE,
                                        "y-fill", FALSE,
                                        NULL);

    return page;
}

static ClutterActor*
mtn_app_get_security_page (MtnApp *self)
{
    MtnAppPrivate *priv;
    ClutterActor *page, *box, *label, *txt;

    priv = self->priv;

    page = mx_box_layout_new ();

    box = mx_box_layout_new ();
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (box),
                                   MX_ORIENTATION_VERTICAL);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (page),
                                             box,
                                             -1,
                                             "expand", TRUE,
                                             "y-fill", FALSE,
                                             "x-fill", TRUE,
                                             NULL);

    /* TRANSLATORS: label above the buttons for selecting security 
       method ('None', 'WEP', 'WPA'). */
    label = mx_label_new_with_text (_("Choose a security option:"));
    txt = mx_label_get_clutter_text (MX_LABEL (label));
    clutter_text_set_ellipsize (CLUTTER_TEXT (txt),
                                PANGO_ELLIPSIZE_NONE);
    clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
    mx_box_layout_add_actor (MX_BOX_LAYOUT (box),
                             label,
                             -1);

    /* TRANSLATORS: Button for selecting the security method
       for hidden network. 'None' means no security. */
    priv->none_button = mx_button_new_with_label (_("None"));
    g_object_set_data (G_OBJECT (priv->none_button),
                       "security", "none");
    mx_bin_set_alignment (MX_BIN (priv->none_button),
                          MX_ALIGN_START,
                          MX_ALIGN_MIDDLE);
    g_signal_connect (priv->none_button, "clicked",
                      G_CALLBACK (_connect_security_clicked), self);
    mx_box_layout_add_actor (MX_BOX_LAYOUT (box),
                             priv->none_button,
                             -1);

    /* TRANSLATORS: Button for selecting the security method
       for hidden network. 'WEP' probably needs no translation. */
    priv->wep_button = mx_button_new_with_label (_("WEP"));
    g_object_set_data (G_OBJECT (priv->wep_button), "security", "wep");
    mx_bin_set_alignment (MX_BIN (priv->wep_button),
                          MX_ALIGN_START,
                          MX_ALIGN_MIDDLE);
    g_signal_connect (priv->wep_button, "clicked",
                      G_CALLBACK (_connect_security_clicked), self);
    mx_box_layout_add_actor (MX_BOX_LAYOUT (box),
                             priv->wep_button,
                             -1);

    /* TRANSLATORS: Button for selecting the security method
       for hidden network. 'WPA' probably needs no translation. */
    priv->wpa_button = mx_button_new_with_label (_("WPA"));
    g_object_set_data (G_OBJECT (priv->wpa_button), "security", "wpa");
    mx_bin_set_alignment (MX_BIN (priv->wpa_button),
                          MX_ALIGN_START,
                          MX_ALIGN_MIDDLE);
    g_signal_connect (priv->wpa_button, "clicked",
                      G_CALLBACK (_connect_security_clicked), self);
    mx_box_layout_add_actor (MX_BOX_LAYOUT (box),
                             priv->wpa_button,
                             -1);

    return page;
}

static ClutterActor*
mtn_app_get_ssid_page (MtnApp *self)
{
    MtnAppPrivate *priv;
    ClutterActor *page, *table, *label; 

    priv = self->priv;

    page = mx_box_layout_new ();

    table = mx_table_new ();
    mx_table_set_column_spacing (MX_TABLE (table), MTN_APP_COL_SPACE);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (page),
                                             table,
                                             -1,
                                             "expand", TRUE,
                                             "y-fill", FALSE,
                                             "x-fill", TRUE,
                                             NULL);

    /* TRANSLATORS: label to the left of a SSID entry. Max length is
       around 27 characters */
    label = mx_label_new_with_text (_("Enter network name:"));
    mx_table_add_actor_with_properties (MX_TABLE (table),
                                        label,
                                        0, 0,
                                        "y-expand", TRUE,
                                        "y-fill", FALSE,
                                        "y-align", MX_ALIGN_MIDDLE,
                                        "x-expand", FALSE,
                                        NULL);

    priv->ssid_entry = mx_entry_new ();
    clutter_actor_set_size (priv->ssid_entry, MTN_APP_ENTRY_WIDTH, -1);
    g_signal_connect (priv->ssid_entry, "key-release-event",
                      G_CALLBACK (_entry_key_release), self);
    mx_table_add_actor_with_properties (MX_TABLE (table),
                                        priv->ssid_entry,
                                        0, 1,
                                        "x-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        NULL);

    return page;
}

static ClutterActor*
mtn_app_get_main_page (MtnApp *self)
{
    MtnAppPrivate *priv;
    ClutterActor *page, *scroll, *box, *stack, *icon,
                 *error_box, *label, *txt;
    const char *str;

    priv = self->priv;

    page = mx_box_layout_new ();

    box = mx_box_layout_new ();
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (box),
                                   MX_ORIENTATION_VERTICAL);
    mx_box_layout_set_spacing (MX_BOX_LAYOUT (box), 15);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (page),
                                             box,
                                             -1,
                                             "expand", TRUE,
                                             NULL);

    stack = mx_stack_new ();
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (box),
                                             stack,
                                             -1,
                                             "expand", TRUE,
                                             NULL);

    scroll = mex_scroll_view_new ();
    /* FIXME: need this size setting because of clutter bug 2461 */
    clutter_actor_set_size (scroll, -1, 230);
    clutter_actor_set_name (scroll, "mtn-scroll-view");
    clutter_actor_set_clip_to_allocation (scroll, TRUE);
    mex_scroll_view_set_indicators_hidden (MEX_SCROLL_VIEW (scroll),
                                           FALSE);
    mx_kinetic_scroll_view_set_scroll_policy (MX_KINETIC_SCROLL_VIEW (scroll),
                                              MX_SCROLL_POLICY_VERTICAL);
    clutter_container_add_actor (CLUTTER_CONTAINER (stack),
                                 scroll);

    priv->services_view = mtn_services_view_new ();
    g_signal_connect (priv->services_view, "connection-requested",
                      G_CALLBACK (_view_connection_requested), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (scroll),
                                 priv->services_view);

    priv->error_frame = mx_frame_new ();
    clutter_actor_hide (priv->error_frame);
    clutter_actor_set_name (priv->error_frame, "mtn-no-services-frame");
    clutter_container_add_actor (CLUTTER_CONTAINER (stack),
                                 priv->error_frame);
    clutter_container_child_set (CLUTTER_CONTAINER (stack),
                                 priv->error_frame,
                                 "x-align", MX_ALIGN_MIDDLE,
                                 "y-align", MX_ALIGN_MIDDLE,
                                 "y-fill", FALSE,
                                 NULL);
    clutter_container_raise_child (CLUTTER_CONTAINER (stack),
                                   priv->error_frame,
                                   scroll);

    error_box = mx_box_layout_new ();
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->error_frame),
                                 error_box);

    icon = mx_icon_new ();
    mx_stylable_set_style_class (MX_STYLABLE (icon), "mtn-info");
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (error_box),
                                             icon,
                                             -1,
                                             "expand", FALSE,
                                             NULL);

    label = mx_label_new_with_text (_("Sorry, we can't find any networks."));
    txt = mx_label_get_clutter_text (MX_LABEL (label));
    clutter_text_set_ellipsize (CLUTTER_TEXT (txt),
                                PANGO_ELLIPSIZE_NONE);
    clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (error_box),
                                             label,
                                             -1,
                                             "expand", TRUE,
                                             "x-fill", FALSE,
                                             "y-fill", FALSE,
                                             "x-align", MX_ALIGN_START,
                                             "y-align", MX_ALIGN_MIDDLE,
                                             NULL);

   /* TRANSLATORS: Button on main page. Opens the input page for 
      hidden connection details input.*/
    str = _("Join other network...");
    priv->connect_hidden_button = mx_button_new_with_label (str);
    g_signal_connect (priv->connect_hidden_button, "clicked",
                      G_CALLBACK (_connect_hidden_clicked), self);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (box), 
                                             priv->connect_hidden_button,
                                             -1,
                                             "expand", FALSE,
                                             "x-fill", FALSE,
                                             "x-align", MX_ALIGN_START,
                                             NULL);

    return page;
}

static void
mtn_app_init (MtnApp *self)
{
    ClutterActor *stage, *stage_box, *box, *book_box, *hbox;
    MtnAppPrivate *priv;

    priv = self->priv = GET_PRIVATE (self);

    mtn_connman_new (NULL, _connman_new_cb, self);

    priv->security = "none";

    priv->win = mx_application_create_window (MX_APPLICATION (self));
    mx_window_set_has_toolbar (priv->win, FALSE);

    stage = CLUTTER_ACTOR (mx_window_get_clutter_stage (priv->win));
    clutter_stage_hide_cursor (CLUTTER_STAGE (stage));
    clutter_stage_set_title (CLUTTER_STAGE (stage), MTN_APP_NAME);
    g_signal_connect (stage, "key-release-event",
                      G_CALLBACK (_stage_key_release), self);

    priv->fm = mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));

    mx_style_load_from_file (mx_style_get_default (),
                             THEMEDIR "/mex-networks.css",
                             NULL);

    stage_box = mx_box_layout_new ();
    clutter_actor_set_name (CLUTTER_ACTOR (stage_box), "mtn-background");
    mx_window_set_child (priv->win, stage_box);

    box = mx_box_layout_new ();
    clutter_actor_set_name (CLUTTER_ACTOR (box), "mtn-content-box");
    clutter_actor_set_size (CLUTTER_ACTOR (box),
                            MTN_APP_CONTENT_WIDTH,
                            MTN_APP_CONTENT_HEIGHT);
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (box),
                                   MX_ORIENTATION_VERTICAL);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (stage_box),
                                             box,
                                             -1,
                                             "expand", TRUE,
                                             "x-fill", FALSE,
                                             "y-fill", FALSE,
                                             NULL);

    priv->title = mx_label_new ();
    clutter_actor_set_name (priv->title, "mtn-app-title");
    mx_box_layout_add_actor (MX_BOX_LAYOUT (box), priv->title, -1);

    book_box = mx_box_layout_new ();
    clutter_actor_set_name (book_box, "mtn-notebook-box");
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (box),
                                             book_box,
                                             -1,
                                             "expand", TRUE,
                                             NULL);

    priv->book = mx_notebook_new ();
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (book_box),
                                             priv->book,
                                             -1,
                                             "expand", TRUE,
                                             NULL);

    priv->main_page = mtn_app_get_main_page (self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                                 priv->main_page);
    priv->ssid_page = mtn_app_get_ssid_page (self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                                 priv->ssid_page);
    priv->security_page = mtn_app_get_security_page (self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                                 priv->security_page);
    priv->passphrase_page = mtn_app_get_passphrase_page (self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                                 priv->passphrase_page);
    priv->connecting_page = mtn_app_get_connecting_page (self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                                 priv->connecting_page);
    priv->connected_page = mtn_app_get_connected_page (self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                                 priv->connected_page);

    /* button area at the bottom */
    hbox = mtn_button_box_new ();
    mx_box_layout_add_actor (MX_BOX_LAYOUT (box), hbox, -1);

    self->priv->back_button = mx_button_new ();
    clutter_actor_set_size (self->priv->back_button,
                            MTN_APP_BUTTON_WIDTH,
                            -1);
    g_signal_connect (self->priv->back_button, "clicked",
                      G_CALLBACK (_back_clicked), self);
    mx_box_layout_add_actor (MX_BOX_LAYOUT (hbox), 
                             self->priv->back_button,
                             -1);

    priv->forward_button = mx_button_new ();
    clutter_actor_set_size (priv->forward_button,
                            MTN_APP_BUTTON_WIDTH,
                            -1);
    g_signal_connect (priv->forward_button, "clicked",
                      G_CALLBACK (_forward_clicked), self);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (hbox),
                                             priv->forward_button,
                                             -1,
                                             "x-align", MX_ALIGN_END,
                                             "expand", TRUE,
                                             "x-fill", FALSE,
                                             NULL);
}

MtnApp*
mtn_app_new (gint         *argc,
             gchar      ***argv,
             const char   *back_command,
             gboolean      first_boot)
{
    MtnApp *app;
    GError *error = NULL;

    if (clutter_init_with_args (argc, argv,
                               MTN_APP_NAME, NULL, NULL,
                               &error) != CLUTTER_INIT_SUCCESS) {
        g_warning ("Falied to init clutter: %s", error->message);
        g_error_free (error);
        return NULL;
    }

    mx_set_locale ();
    mex_style_load_default ();

    app = g_object_new (MTN_TYPE_APP,
                        "flags", MX_APPLICATION_SINGLE_INSTANCE,
                        "application-name", MTN_APP_NAME,
                        "back-command", back_command,
                        "first-boot", first_boot,
                        NULL);

    return app;
}
