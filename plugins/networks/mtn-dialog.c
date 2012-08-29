/*
 * mex-networks - Connection Manager UI for Media Explorer
 * Copyright © 2010-2011, Intel Corporation.
 * Copyright © 2012, sleep(5) ltd.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#include <string.h>
#include <glib/gi18n.h>
#include <mex/mex.h>

#include "mtn-dialog.h"
#include "mtn-button-box.h"
#include "mtn-connman.h"
#include "mtn-connman-service.h"
#include "mtn-services-view.h"

#define MTN_DIALOG_ENTRY_WIDTH 240.0
#define MTN_DIALOG_COL_SPACE 30.0


static void mtn_dialog_dispose (GObject *object);
static void mtn_dialog_finalize (GObject *object);
static void mtn_dialog_constructed (GObject *object);
static void mtn_dialog_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec);
static void mtn_dialog_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec);
static void mtn_dialog_create_contents (MtnDialog *self);

G_DEFINE_TYPE (MtnDialog, mtn_dialog, MX_TYPE_DIALOG);

#define MTN_DIALOG_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), MTN_TYPE_DIALOG, MtnDialogPrivate))

typedef enum {
    MTN_DIALOG_STATE_DEFAULT,

    MTN_DIALOG_STATE_REQUEST_SSID_FOR_HIDDEN,
    MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN,
    MTN_DIALOG_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN,

    MTN_DIALOG_STATE_REQUEST_PASSPHRASE,

    MTN_DIALOG_STATE_CONNECTING,
    MTN_DIALOG_STATE_CONNECT_FAILED,
    MTN_DIALOG_STATE_CONNECT_OK,
} MtnDialogState;

struct _MtnDialogPrivate
{
  ClutterActor      *transient_for;
  MtnDialogState     state;

  MtnConnman        *connman;
  MtnConnmanService *service;

  const char        *security;

  ClutterActor      *title;
  ClutterActor      *book;
  ClutterActor      *main_page;
  ClutterActor      *ssid_page;
  ClutterActor      *security_page;
  ClutterActor      *passphrase_page;
  ClutterActor      *connecting_page;
  ClutterActor      *connected_page;

  /* in main_page */
  ClutterActor      *services_view;
  ClutterActor      *error_frame;
  ClutterActor      *connect_hidden_button;

  /* in ssid_page */
  ClutterActor      *ssid_entry;

  /* in ssid_page */
  ClutterActor      *none_button;
  ClutterActor      *wep_button;
  ClutterActor      *wpa_button;

  /* in passphrase_page */
  ClutterActor      *service_label;
  ClutterActor      *passphrase_entry;

  /* in connecting_page */
  ClutterActor      *connecting_label;

  /* in connected_page */
  ClutterActor      *connected_icon;
  ClutterActor      *connected_label;

  ClutterActor      *back_button;
  ClutterActor      *forward_button;

  guint disposed : 1;
};

enum
{
  CLOSE,

  N_SIGNALS,
};

enum
{
  PROP_0,

  PROP_TRANSIENT_FOR,
};

static guint signals[N_SIGNALS] = {0};

static void
mtn_dialog_mapped_cb (ClutterActor *dialog,
                      GParamSpec   *pspec,
                      MtnDialog    *self)
{
  ClutterActor *parent;

  parent = clutter_actor_get_parent (dialog);
  clutter_actor_remove_child (parent, dialog);
}

static void
mtn_dialog_close (MtnDialog *self)
{
  /*
   * Hide the dialog, this will trigger the default transition; once it is
   * no longer visible, destroy it.
   *
   * We have to use the mapped property here; MxDialog runs custom animation
   * using a timeline and the 'visible' property is set well before the
   * animation finishes.
   */
  g_signal_connect (self, "notify::mapped",
                    G_CALLBACK (mtn_dialog_mapped_cb), self);
  clutter_actor_hide (CLUTTER_ACTOR (self));
}

static void
mtn_dialog_class_init (MtnDialogClass *klass)
{
  GParamSpec   *pspec;
  GObjectClass *object_class = (GObjectClass *)klass;

  g_type_class_add_private (klass, sizeof (MtnDialogPrivate));

  object_class->dispose      = mtn_dialog_dispose;
  object_class->finalize     = mtn_dialog_finalize;
  object_class->constructed  = mtn_dialog_constructed;
  object_class->get_property = mtn_dialog_get_property;
  object_class->set_property = mtn_dialog_set_property;

  klass->close = mtn_dialog_close;

  pspec = g_param_spec_object ("transient-for",
                               "Transient For",
                               "Transient",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE);
  g_object_class_install_property (object_class,
                                   PROP_TRANSIENT_FOR,
                                   pspec);

  signals[CLOSE] = g_signal_new ("close",
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST,
                                 G_STRUCT_OFFSET (MtnDialogClass, close),
                                 NULL, NULL,
                                 g_cclosure_marshal_VOID__VOID,
                                 G_TYPE_NONE, 0);
}

static void mtn_dialog_set_state (MtnDialog *self, MtnDialogState state);

static gboolean
_need_passphrase (MtnConnmanService *service)
{
  gboolean failed, passphrase_req, immutable, secure, need_passphrase;
  const char **securities, **security;
  const char *str;
  GVariant *var;

  var = mtn_connman_service_get_property (service, "State");
  str = var ? g_variant_get_string (var, NULL): "failure";
  failed = (g_strcmp0 (str, "failure") == 0);

  var = mtn_connman_service_get_property (service, "PassphraseRequired");
  passphrase_req = var ? g_variant_get_boolean (var) : FALSE;

  var = mtn_connman_service_get_property (service, "Immutable");
  immutable = var ? g_variant_get_boolean (var) : FALSE;

  secure = FALSE;
  var = mtn_connman_service_get_property (service, "Security");
  if ((securities = g_variant_get_strv (var, NULL)))
    {
      security = securities;
      while (*security)
        {
          if (g_strcmp0 (*security, "none") != 0)
            secure = TRUE;
          security++;
        }

      g_free (securities);
  }

  /* Need a password for non-immutable services that
   * A) ask for a passphrase or
   * B) failed to connect and have a use for the passphrase
   */

  need_passphrase = FALSE;

  if (!immutable)
    need_passphrase = passphrase_req || (failed && secure);

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
mtn_dialog_update_connect_hidden_button (MtnDialog *self,
                                         GVariant  *enabled_techs)
{
  MtnDialogPrivate *priv = self->priv;
  GVariantIter     *iter;
  char             *tech;
  gboolean          have_wifi;

  have_wifi = FALSE;
  if (enabled_techs)
    {
      g_variant_get (enabled_techs, "as", &iter);
      while (g_variant_iter_next (iter, "&s", &tech))
        if (g_strcmp0 (tech, "wifi") == 0)
          {
            have_wifi = TRUE;
            break;
          }
      g_variant_iter_free (iter);
    }

  g_object_set (priv->connect_hidden_button, "visible", have_wifi, NULL);
}

static void
mtn_dialog_update_passphrase_page (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  const char       *name, *sec, *pw;
  char             *str;
  GVariant         *var;

  g_object_set (priv->service_label,
                "visible", priv->service != NULL,
                NULL);

  if (priv->service)
    {
      /* hidden network */
      mx_entry_set_text (MX_ENTRY (priv->passphrase_entry), "");
      return;
    }

  var = mtn_connman_service_get_property (priv->service, "Name");
  name = var ? g_variant_get_string (var, NULL): "Network";

  var = mtn_connman_service_get_property (priv->service, "Security");
  sec = var ? g_variant_get_string (var, NULL): "";

  var = mtn_connman_service_get_property (priv->service, "Password");
  pw = var ? g_variant_get_string (var, NULL) : "";

  if (g_strcmp0 (sec, "rsn") == 0 ||
      g_strcmp0 (sec, "psk") == 0 ||
      g_strcmp0 (sec, "wpa") == 0)
    {
      /* TRANSLATORS: Info label in the service data input page.
         Placeholder is the name of the wireless network. */
      str = g_strdup_printf (_("<b>%s</b> requires a WPA password"), name);
    }
  else if (g_strcmp0 (sec, "wep") == 0)
    {
      /* TRANSLATORS: Info label in the service data input page.
         Placeholder is the name of the wireless network. */
      str = g_strdup_printf (_("<b>%s</b> requires a WEP password"), name);
    }
  else
    {
      /* TRANSLATORS: Info label in the service data input page.
         Placeholder is the name of the wireless network. */
      str = g_strdup_printf (_("<b>%s</b> requires a password"), name);
    }

  mx_entry_set_text (MX_ENTRY (priv->passphrase_entry), pw);
  _mx_label_set_markup (MX_LABEL (priv->service_label), str);
  g_free (str);
}

static void
mtn_dialog_clear_service (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;

  if (priv->service)
    {
      g_object_unref (priv->service);
      priv->service = NULL;
    }
}

static void
mtn_dialog_update_forward_back_buttons (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  const char       *text, *fwd_do_later;
  gboolean          visible;

  /* TRANSLATORS: Button label, clicking skips this step in the
     first-boot process. */
  fwd_do_later = _("Do later");

  /* TRANSLATORS: Button label. If user is on some other page,
     clicking will return back to first page. If user is in
     the first page, will return the user to previous setup
     app (in first boot mode), or to Settings */
  text = _("Back");
  visible = TRUE;

  switch (priv->state)
    {
    case MTN_DIALOG_STATE_CONNECT_FAILED:
      /* TRANSLATORS: Back-button label when connection failed.
         Will take user back to first page. */
      text = _("Try again...");
      break;
    case MTN_DIALOG_STATE_CONNECT_OK:
      visible = FALSE;
      break;
    default:
      ;
    }

  mx_action_set_display_name (mx_button_get_action (
                                MX_BUTTON (priv->back_button)), text);
  g_object_set (priv->back_button, "visible", visible, NULL);

  /* TRANSLATORS: Button label, clicking will go to the next
     page */
  text = _("Next");
  visible = TRUE;

  switch (priv->state)
    {
    case MTN_DIALOG_STATE_DEFAULT:
      visible = FALSE;
      text = fwd_do_later;
      break;
    case MTN_DIALOG_STATE_CONNECT_FAILED:
      /* TRANSLATORS: Button label, clicking skips this step in the
         first-boot process. */
      text = fwd_do_later;
      break;
    case MTN_DIALOG_STATE_REQUEST_PASSPHRASE:
    case MTN_DIALOG_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN:
      /* TRANSLATORS: Button label, clicking starts the connection
         attempt to the network */
      text = _("Join");
      break;
    case MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN:
    case MTN_DIALOG_STATE_CONNECTING:
      visible = FALSE;
      break;
    default:
      ;
    }

  mx_action_set_display_name (mx_button_get_action (
                                MX_BUTTON (priv->forward_button)), text);
  g_object_set (priv->forward_button, "visible", visible, NULL);
}

static void
mtn_dialog_update_connecting_page (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  const char       *name;
  char             *str;

  if (!priv->service)
    {
      /* hidden network */
      name = mx_entry_get_text (MX_ENTRY (priv->ssid_entry));
    }
  else
    {
      GVariant *var;

      var = mtn_connman_service_get_property (priv->service, "Name");
      name = var ? g_variant_get_string (var, NULL) : "";
    }

  str = g_strdup_printf ("Joining network <b>%s</b>...", name);
  _mx_label_set_markup (MX_LABEL (priv->connecting_label), str);
  g_free (str);
}

static void
mtn_dialog_update_connected_page (MtnDialog *self, gboolean success)
{
  MtnDialogPrivate *priv = self->priv;
  char             *str;
  const char       *name;

  if (!priv->service)
    {
      /* hidden network */
      name = mx_entry_get_text (MX_ENTRY (priv->ssid_entry));
    }
  else
    {
      GVariant *var;

      var = mtn_connman_service_get_property (priv->service, "Name");
      name = var ? g_variant_get_string (var, NULL) : "";
    }

  if (success)
    {
      str = g_strdup_printf ("Succesfully joined <b>%s</b>.", name);
      mx_stylable_set_style_class (MX_STYLABLE (priv->connected_icon),
                                   "mtn-connection-ok");
    }
  else
    {
      str = g_strdup_printf ("Sorry, we've been unable to join <b>%s</b>.",
                             name);
      mx_stylable_set_style_class (MX_STYLABLE (priv->connected_icon),
                                   "mtn-connection-failed");
    }

  _mx_label_set_markup (MX_LABEL (priv->connected_label), str);
  g_free (str);
}


static void
_services_changed (MtnConnman *connman, GVariant *var, MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  static gboolean   prev_have_services = TRUE;
  gboolean          have_services;
  GVariantIter     *it_a, *it_r;

  /*
   * The variant is (a(oa{sv})ao) where:
   *
   * The first array represents services changed/added,
   * The second array represents services removed
   */
  g_variant_get (var, "(a(oa{sv})ao)", &it_a, &it_r);

  have_services = FALSE;
  if (var)
    have_services = (g_variant_iter_n_children (it_a) > 0);

  g_variant_iter_free (it_a);
  g_variant_iter_free (it_r);

  if (have_services == prev_have_services)
    return;

  prev_have_services = have_services;

  g_object_set (priv->error_frame,
                "visible", !have_services,
                NULL);
}

static gboolean
_is_connman_error (GError *error, char *name)
{
  char *remote_error;

  if (!g_dbus_error_is_remote_error (error))
    return FALSE;

  if (!(remote_error = g_dbus_error_get_remote_error (error)))
    return FALSE;

  if (g_str_has_prefix (remote_error, "net.connman.Error."))
    {
      if (!name || g_str_has_suffix (remote_error, name))
        return TRUE;
    }

  return FALSE;
}

static void
_connman_connect_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  MtnDialog        *self = data;
  GVariant         *var;
  MtnDialogState    state = MTN_DIALOG_STATE_CONNECT_OK;
  GError           *error = NULL;

  if ((var = g_dbus_proxy_call_finish (G_DBUS_PROXY (object), res, &error)))
    g_variant_unref (var);

  if (error)
    {
      if (!_is_connman_error (error, "AlreadyConnected"))
        {
          g_warning ("Connecting failed: %s", error->message);
          state = MTN_DIALOG_STATE_CONNECT_FAILED;
        }

      g_error_free (error);
    }

  mtn_dialog_set_state (self, state);
}

static void
mtn_dialog_connect_hidden_service (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  GVariantBuilder   builder;
  GVariant         *var;
  const char       *pw, *ssid, *security;

  ssid = mx_entry_get_text (MX_ENTRY (priv->ssid_entry));
  security = priv->security;
  pw = mx_entry_get_text (MX_ENTRY (priv->passphrase_entry));

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

  g_dbus_proxy_call (G_DBUS_PROXY (priv->connman), "ConnectService", var,
                     G_DBUS_CALL_FLAGS_NONE, 120000, NULL,
                     _connman_connect_cb, self);

  g_variant_unref (var);
}

static void
_set_passphrase_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  MtnDialog        *self = data;
  MtnDialogPrivate *priv = self->priv;
  GVariant         *var;
  GError           *error = NULL;

  if ((var = g_dbus_proxy_call_finish (G_DBUS_PROXY (object), res, &error)))
    {
      g_variant_unref (var);

      /* Passphrase is set, now connect */
      g_dbus_proxy_call (G_DBUS_PROXY (priv->service), "Connect", NULL,
                         G_DBUS_CALL_FLAGS_NONE, 120000, NULL,
                         _connman_connect_cb, self);
    }
  else if (error)
    {
      g_warning ("Connman Service.SetProperty() for 'Passphrase' failed: %s",
                 error->message);
      g_error_free (error);

      mtn_dialog_set_state (self, MTN_DIALOG_STATE_CONNECT_FAILED);
    }
}

static void
mtn_dialog_connect (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;

  if (_need_passphrase (priv->service))
    {
      GVariant *var;
      const char *pw;

      /* cant use mtn_connman_service_set_property() because it is
       * fire-and-forget and property-changed won't get emitted for
       * passphrase in some cases */

      pw = mx_entry_get_text (MX_ENTRY (priv->passphrase_entry));
      var = g_variant_new ("(sv)", "Passphrase", g_variant_new_string (pw));

      g_dbus_proxy_call (G_DBUS_PROXY (priv->service), "SetProperty", var,
                         G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                         _set_passphrase_cb, self);
      g_variant_unref (var);
    }
  else
    {
      g_dbus_proxy_call (G_DBUS_PROXY (priv->service), "Connect", NULL,
                         G_DBUS_CALL_FLAGS_NONE, 120000, NULL,
                         _connman_connect_cb, self);
    }
}

static void
mtn_dialog_set_state (MtnDialog *self, MtnDialogState state)
{
  MtnDialogPrivate *priv = self->priv;
  gboolean          error_visible;

  priv->state = state;

  mtn_dialog_update_forward_back_buttons (self);

  switch (state)
    {
    case MTN_DIALOG_STATE_DEFAULT:
      mtn_dialog_clear_service (self);
      mx_notebook_set_current_page (MX_NOTEBOOK (priv->book), priv->main_page);
      g_object_get (priv->error_frame, "visible", &error_visible, NULL);

#if 0
      /*
       * FIXME -- for some reason pushing focus to the services view leaves
       * the dialog button with a focused appearance!
       */
      if (error_visible)
        mex_push_focus (MX_FOCUSABLE (priv->back_button));
      else
        mex_push_focus (MX_FOCUSABLE (priv->services_view));
#else
      mex_push_focus (MX_FOCUSABLE (priv->back_button));
#endif
      break;

    case MTN_DIALOG_STATE_REQUEST_SSID_FOR_HIDDEN:
      mx_entry_set_text (MX_ENTRY (priv->ssid_entry), "");
      mx_notebook_set_current_page (MX_NOTEBOOK (priv->book), priv->ssid_page);
      mex_push_focus (MX_FOCUSABLE (priv->ssid_entry));
      break;

    case MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN:
      mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                    priv->security_page);
      mex_push_focus (MX_FOCUSABLE (priv->none_button));
      break;

    case MTN_DIALOG_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN:
    case MTN_DIALOG_STATE_REQUEST_PASSPHRASE:
      mtn_dialog_update_passphrase_page (self);
      mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                    priv->passphrase_page);
      mex_push_focus (MX_FOCUSABLE (priv->passphrase_entry));
      break;

    case MTN_DIALOG_STATE_CONNECTING:
      mtn_dialog_update_connecting_page (self);
      mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                    priv->connecting_page);
      mex_push_focus (MX_FOCUSABLE (priv->back_button));

      if (priv->service)
        mtn_dialog_connect (self);
      else
        mtn_dialog_connect_hidden_service (self);

      break;

    case MTN_DIALOG_STATE_CONNECT_FAILED:
      mtn_dialog_update_connected_page (self, FALSE);
      mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                    priv->connected_page);

      mex_push_focus (MX_FOCUSABLE (priv->back_button));
      break;

    case MTN_DIALOG_STATE_CONNECT_OK:
      mtn_dialog_update_connected_page (self, TRUE);
      mx_notebook_set_current_page (MX_NOTEBOOK (priv->book),
                                    priv->connected_page);

      mex_push_focus (MX_FOCUSABLE (priv->forward_button));
      break;
    }
}


static void
mtn_dialog_constructed (GObject *object)
{
  MtnDialog        *self = (MtnDialog*) object;
  MtnDialogPrivate *priv = self->priv;

  g_assert (priv->transient_for);

  if (G_OBJECT_CLASS (mtn_dialog_parent_class)->constructed)
    G_OBJECT_CLASS (mtn_dialog_parent_class)->constructed (object);

  mx_dialog_set_transient_parent (MX_DIALOG (self), priv->transient_for);

  mtn_dialog_create_contents (self);
}

static void
mtn_dialog_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_dialog_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  MtnDialog        *self = (MtnDialog*) object;
  MtnDialogPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_TRANSIENT_FOR:
      priv->transient_for = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_dialog_init (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;

  priv = self->priv = MTN_DIALOG_GET_PRIVATE (self);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/mex-networks.css",
                           NULL);

  priv->security = "none";

  clutter_actor_hide (CLUTTER_ACTOR (self));
}

static void
mtn_dialog_dispose (GObject *object)
{
  MtnDialog        *self = (MtnDialog*) object;
  MtnDialogPrivate *priv = self->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  mtn_dialog_clear_service (self);

  if (priv->connman)
    {
      g_object_unref (priv->connman);
      priv->connman = NULL;
    }

  G_OBJECT_CLASS (mtn_dialog_parent_class)->dispose (object);
}

static void
mtn_dialog_finalize (GObject *object)
{
  G_OBJECT_CLASS (mtn_dialog_parent_class)->finalize (object);
}

static void
mtn_dialog_forward (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;

  switch (priv->state)
    {
    case MTN_DIALOG_STATE_REQUEST_SSID_FOR_HIDDEN:
      mtn_dialog_set_state (self,
                            MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN);
      break;

    case MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN:
      if (g_strcmp0 (priv->security, "none") == 0)
        mtn_dialog_set_state (self,
                              MTN_DIALOG_STATE_CONNECTING);
      else
        mtn_dialog_set_state (self,
                              MTN_DIALOG_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN);
      break;

    case MTN_DIALOG_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN:
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_CONNECTING);
      break;

    case MTN_DIALOG_STATE_REQUEST_PASSPHRASE:
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_CONNECTING);
      break;

    case MTN_DIALOG_STATE_CONNECTING:
      g_warn_if_reached ();
      break;

    case MTN_DIALOG_STATE_DEFAULT:
    case MTN_DIALOG_STATE_CONNECT_FAILED:
    case MTN_DIALOG_STATE_CONNECT_OK:
      g_signal_emit (self, signals[CLOSE], 0);
      break;
    }
}

static void
mtn_dialog_back (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;

  switch (priv->state)
    {
    case MTN_DIALOG_STATE_DEFAULT:
      g_signal_emit (self, signals[CLOSE], 0);
      break;

    case MTN_DIALOG_STATE_REQUEST_PASSPHRASE:
    case MTN_DIALOG_STATE_REQUEST_SSID_FOR_HIDDEN:
    case MTN_DIALOG_STATE_CONNECT_FAILED:
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_DEFAULT);
      break;

    case MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN:
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_REQUEST_SSID_FOR_HIDDEN);
      break;

    case MTN_DIALOG_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN:
      mtn_dialog_set_state (self,
                            MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN);
      break;

    case MTN_DIALOG_STATE_CONNECTING:
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_DEFAULT);
      break;

    case MTN_DIALOG_STATE_CONNECT_OK:
      g_warn_if_reached ();
      break;
    }
}

static gboolean
_entry_key_release (ClutterActor    *actor,
                    ClutterKeyEvent *event,
                    MtnDialog       *self)
{
  if (MEX_KEY_OK (event->keyval))
    {
      mtn_dialog_forward (self);
      return TRUE;
    }

  return FALSE;
}

static void
mtn_dialog_forward_key_release (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  gboolean          go_forward;

  go_forward = FALSE;
  if (priv->state == MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN)
    {
      GObject *o;
      ClutterActor *focus;
      ClutterStage *stage;

      /* special case: forward button is not visible,
       * but if a security button is focused, we can proceed */

      o = NULL;
      stage = CLUTTER_STAGE (clutter_actor_get_stage ((ClutterActor*)self));
      focus = clutter_stage_get_key_focus (stage);
      if (focus == priv->none_button)
        o = G_OBJECT (priv->none_button);
      else if (focus == priv->wep_button)
        o = G_OBJECT (priv->wep_button);
      else if (focus == priv->wpa_button)
        o = G_OBJECT (priv->wpa_button);
      if (o)
        {
          priv->security = g_object_get_data (o, "security");
          go_forward = TRUE;
        }
    }
  else
    {
      gboolean visible, disabled;

      g_object_get (priv->forward_button,
                    "visible", &visible,
                    "disabled", &disabled,
                    NULL);
      if (visible && !disabled)
        go_forward = TRUE;
    }

  if (go_forward)
    mtn_dialog_forward (self);
}

static void
mtn_dialog_back_key_release (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  gboolean          visible, disabled;

  g_object_get (priv->back_button,
                "visible", &visible,
                "disabled", &disabled,
                NULL);

  mtn_dialog_back (self);
}

static gboolean
mtn_dialog_key_press (ClutterActor    *actor,
                      ClutterKeyEvent *event,
                      MtnDialog       *self)
{
  if (MEX_KEY_BACK (event->keyval))
    {
      mtn_dialog_back_key_release (self);
      return TRUE;
    }

  switch (event->keyval)
    {
    case CLUTTER_Forward:
      mtn_dialog_forward_key_release (self);
      return TRUE;

#ifdef DEBUG
    case CLUTTER_1:
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_DEFAULT);
      return TRUE;
    case CLUTTER_2:
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_REQUEST_PASSPHRASE);
      return TRUE;
    case CLUTTER_3:
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_REQUEST_SSID_FOR_HIDDEN);
      return TRUE;
    case CLUTTER_4:
      mx_entry_set_text (MX_ENTRY (priv->ssid_entry), "Dummy");
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_REQUEST_SECURITY_FOR_HIDDEN);
      return TRUE;
    case CLUTTER_5:
      mx_entry_set_text (MX_ENTRY (priv->ssid_entry), "Dummy");
      priv->security = "wpa";
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_REQUEST_PASSPHRASE_FOR_HIDDEN);
      return TRUE;
    case CLUTTER_6:
      mx_entry_set_text (MX_ENTRY (priv->ssid_entry), "Dummy");
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_CONNECTING);
      return TRUE;
    case CLUTTER_7:
      mx_entry_set_text (MX_ENTRY (priv->ssid_entry), "Dummy");
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_CONNECT_FAILED);
      return TRUE;
    case CLUTTER_8:
      mx_entry_set_text (MX_ENTRY (priv->ssid_entry), "Dummy");
      mtn_dialog_set_state (self, MTN_DIALOG_STATE_CONNECT_OK);
      return TRUE;
#endif
    default:
      return FALSE;
    }
}

static void
_forward_activated (MxAction *action, MtnDialog *self)
{
  mtn_dialog_forward (self);
}

static void
_back_activated (MxAction *action, MtnDialog *self)
{
  mtn_dialog_back (self);
}

static void
_connect_security_clicked (MxButton *btn, MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;

  priv->security = g_object_get_data (G_OBJECT (btn), "security");
  mtn_dialog_forward (self);
}

static void
_connman_new_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  MtnDialog        *self = data;
  MtnDialogPrivate *priv = self->priv;
  GError           *error = NULL;
  GVariant         *var;

  priv->connman = mtn_connman_new_finish (res, &error);
  if (!priv->connman)
    {
      g_warning ("Failed to create Connman proxy: %s", error->message);
      g_error_free (error);
      return;
    }

  if ((var = mtn_connman_get_services (priv->connman)))
    {
      GVariantIter *iter;
      gboolean      have_services;

      g_variant_get (var, "(a(oa{sv}))", &iter);
      have_services = (g_variant_iter_n_children (iter) > 0);
      g_variant_iter_free (iter);

      g_object_set (priv->error_frame, "visible", !have_services, NULL);
    }

  /* keep an eye on Services for the "no networks" info */
  g_signal_connect (priv->connman, "services-changed",
                    G_CALLBACK (_services_changed), self);

  /* hide "hidden network" UI if we do not have wifi */
  var = mtn_connman_get_property (priv->connman, "EnabledTechnologies");
  mtn_dialog_update_connect_hidden_button (self, var);

  /* ServicesView shows all the services in a list */
  mtn_services_view_set_connman (MTN_SERVICES_VIEW (priv->services_view),
                                 priv->connman);
  mtn_dialog_set_state (self, MTN_DIALOG_STATE_DEFAULT);
}

static void
_connman_service_new (GObject *object, GAsyncResult *res, gpointer data)
{
  MtnDialog        *self = data;
  MtnDialogPrivate *priv = self->priv;
  GError *error = NULL;

  if (!(priv->service = mtn_connman_service_new_finish (res, &error)))
    {
      g_warning ("Failed to create Connman service proxy: %s", error->message);
      g_error_free (error);

      /* TODO show error */
      return;
    }

  if (_need_passphrase (priv->service))
    mtn_dialog_set_state (self, MTN_DIALOG_STATE_REQUEST_PASSPHRASE);
  else
    mtn_dialog_set_state (self, MTN_DIALOG_STATE_CONNECTING);
}

static void
_view_connection_requested (MtnServicesView *view,
                            const char      *object_path,
                            MtnDialog       *self)
{
  MtnDialogPrivate *priv = self->priv;

  if (!object_path)
    {
      g_warning ("No object path in connection request");
      return;
    }

  if (priv->service)
    {
      g_warning ("Connection requested, but still working on something else");
      mtn_dialog_clear_service (self);
    }

  mtn_connman_service_new (object_path, NULL, _connman_service_new, self);
}

static void
_connect_hidden_clicked (MxButton *btn, MtnDialog *self)
{
  mtn_dialog_set_state (self, MTN_DIALOG_STATE_REQUEST_SSID_FOR_HIDDEN);
}

static ClutterActor *
mtn_dialog_get_connected_page (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  ClutterActor     *page, *box, *txt;

  page = mx_box_layout_new ();
  box = mx_box_layout_new ();
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (page),
                                              box,
                                              -1,
                                              "expand", TRUE,
                                              "y-fill", FALSE,
                                              NULL);

  priv->connected_icon = mx_icon_new ();
  mx_icon_set_icon_size (MX_ICON (priv->connected_icon), 26);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (box),
                                              self->priv->connected_icon,
                                              -1,
                                              "expand", FALSE,
                                              NULL);

  priv->connected_label = mx_label_new ();
  txt = mx_label_get_clutter_text (MX_LABEL (priv->connected_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (txt),
                              PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (box),
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
mtn_dialog_get_connecting_page (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  ClutterActor     *page, *box, *spinner, *txt;

  page = mx_box_layout_new ();
  box = mx_box_layout_new ();
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (page),
                                              box,
                                              -1,
                                              "expand", TRUE,
                                              "y-fill", FALSE,
                                              NULL);

  spinner = mx_spinner_new ();
  clutter_actor_set_name (spinner, "mtn-connecting-spinner");
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (box), spinner, -1,
                                              "expand", TRUE,
                                              "x-fill", FALSE,
                                              "x-align", MX_ALIGN_END,
                                              NULL);

  priv->connecting_label = mx_label_new ();
  txt = mx_label_get_clutter_text (MX_LABEL (priv->connecting_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (txt),
                              PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (box),
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
mtn_dialog_get_passphrase_page (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  ClutterActor     *page, *table, *label, *txt;

  page = mx_box_layout_new ();

  table = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (table), MTN_DIALOG_COL_SPACE);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (page),
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
  mx_table_insert_actor_with_properties (MX_TABLE (table),
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
  mx_table_insert_actor_with_properties (MX_TABLE (table),
                                         label,
                                         1, 0,
                                         "y-expand", FALSE,
                                         "y-fill", FALSE,
                                         "y-align", MX_ALIGN_MIDDLE,
                                         "x-expand", FALSE,
                                         NULL);

  priv->passphrase_entry = mx_entry_new ();
  clutter_actor_set_size (priv->passphrase_entry,
                          MTN_DIALOG_ENTRY_WIDTH,
                          -1);
  g_signal_connect (priv->passphrase_entry, "key-release-event",
                    G_CALLBACK (_entry_key_release), self);
  mx_table_insert_actor_with_properties (MX_TABLE (table),
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
mtn_dialog_get_security_page (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  ClutterActor     *page, *box, *label, *txt;

  page = mx_box_layout_new ();

  box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box),
                                 MX_ORIENTATION_VERTICAL);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (page),
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
  clutter_actor_insert_child_at_index (box, label, -1);

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
  clutter_actor_insert_child_at_index (box, priv->none_button, -1);

  /* TRANSLATORS: Button for selecting the security method
     for hidden network. 'WEP' probably needs no translation. */
  priv->wep_button = mx_button_new_with_label (_("WEP"));
  g_object_set_data (G_OBJECT (priv->wep_button), "security", "wep");
  mx_bin_set_alignment (MX_BIN (priv->wep_button),
                        MX_ALIGN_START,
                        MX_ALIGN_MIDDLE);
  g_signal_connect (priv->wep_button, "clicked",
                    G_CALLBACK (_connect_security_clicked), self);
  clutter_actor_insert_child_at_index (box, priv->wep_button, -1);

  /* TRANSLATORS: Button for selecting the security method
     for hidden network. 'WPA' probably needs no translation. */
  priv->wpa_button = mx_button_new_with_label (_("WPA"));
  g_object_set_data (G_OBJECT (priv->wpa_button), "security", "wpa");
  mx_bin_set_alignment (MX_BIN (priv->wpa_button),
                        MX_ALIGN_START,
                        MX_ALIGN_MIDDLE);
  g_signal_connect (priv->wpa_button, "clicked",
                    G_CALLBACK (_connect_security_clicked), self);
  clutter_actor_insert_child_at_index (box, priv->wpa_button, -1);

  return page;
}

static ClutterActor*
mtn_dialog_get_ssid_page (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  ClutterActor     *page, *table, *label;

  page = mx_box_layout_new ();

  table = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (table), MTN_DIALOG_COL_SPACE);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (page),
                                              table,
                                              -1,
                                              "expand", TRUE,
                                              "y-fill", FALSE,
                                              "x-fill", TRUE,
                                              NULL);

  /* TRANSLATORS: label to the left of a SSID entry. Max length is
     around 27 characters */
  label = mx_label_new_with_text (_("Enter network name:"));
  mx_table_insert_actor_with_properties (MX_TABLE (table),
                                         label,
                                         0, 0,
                                         "y-expand", TRUE,
                                         "y-fill", FALSE,
                                         "y-align", MX_ALIGN_MIDDLE,
                                         "x-expand", FALSE,
                                         NULL);

  priv->ssid_entry = mx_entry_new ();
  clutter_actor_set_size (priv->ssid_entry, MTN_DIALOG_ENTRY_WIDTH, -1);
  g_signal_connect (priv->ssid_entry, "key-release-event",
                    G_CALLBACK (_entry_key_release), self);
  mx_table_insert_actor_with_properties (MX_TABLE (table),
                                         priv->ssid_entry,
                                         0, 1,
                                         "x-fill", FALSE,
                                         "x-align", MX_ALIGN_START,
                                         NULL);

  return page;
}

static ClutterActor*
mtn_dialog_get_main_page (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  ClutterActor     *page, *scroll, *box, *stack, *icon,
                   *error_box, *label, *txt;
  const char       *str;

  page = mx_box_layout_new ();

  box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box),
                                 MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (box), 15);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (page),
                                              box,
                                              -1,
                                              "expand", TRUE,
                                              NULL);

  stack = mx_stack_new ();
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (box),
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
  mx_bin_set_child (MX_BIN (scroll), priv->services_view);

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
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (error_box),
                                              icon,
                                              -1,
                                              "expand", FALSE,
                                              NULL);

  label = mx_label_new_with_text (_("Sorry, we can't find any networks."));
  txt = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (txt),
                              PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (error_box),
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
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (box),
                                              priv->connect_hidden_button,
                                              -1,
                                              "expand", FALSE,
                                              "x-fill", FALSE,
                                              "x-align", MX_ALIGN_START,
                                              NULL);

  return page;
}

static void
mtn_dialog_create_contents (MtnDialog *self)
{
  MtnDialogPrivate *priv = self->priv;
  ClutterActor     *box;
  MxAction         *action;
  ClutterActor     *button_box = NULL;
  GList            *l, *kids;

  box = mx_box_layout_new ();
  mx_bin_set_child (MX_BIN (self), box);
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box),
                                 MX_ORIENTATION_VERTICAL);

  priv->title = mx_label_new ();
  clutter_actor_set_name (priv->title, "mtn-app-title");
  clutter_actor_insert_child_at_index (box, priv->title, -1);

  priv->book = mx_notebook_new ();
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (box),
                                              priv->book,
                                              -1,
                                              "expand", TRUE,
                                              NULL);

  priv->main_page = mtn_dialog_get_main_page (self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                               priv->main_page);
  priv->ssid_page = mtn_dialog_get_ssid_page (self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                               priv->ssid_page);
  priv->security_page = mtn_dialog_get_security_page (self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                               priv->security_page);
  priv->passphrase_page = mtn_dialog_get_passphrase_page (self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                               priv->passphrase_page);
  priv->connecting_page = mtn_dialog_get_connecting_page (self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                               priv->connecting_page);
  priv->connected_page = mtn_dialog_get_connected_page (self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->book),
                               priv->connected_page);

  /*
   * Add buttons to dialog
   */
  action = mx_action_new_full ("back", _("Back"),
                               G_CALLBACK (_back_activated),
                               self);
  mx_dialog_add_action (MX_DIALOG (self), action);

  action = mx_action_new_full ("forward", _("Next"),
                               G_CALLBACK (_forward_activated),
                               self);
  mx_dialog_add_action (MX_DIALOG (self), action);

  /*
   * Now get the buttons from the dialog (Mx does not provide API for this,
   * so do it the hard way.
   *
   * (We need access to the buttons to manipulate focus and visibility.)
   */
  kids = clutter_actor_get_children ((ClutterActor*)self);
  for (l = kids; l; l = l->next)
    {
      MxStylable *s = MX_STYLABLE (l->data);
      const char *klass = mx_stylable_get_style_class (s);

      if (!g_strcmp0 (klass, "MxDialogButtonBox"))
        {
          button_box = l->data;
          break;
        }
    }

  g_assert (button_box);
  g_list_free (kids);

  kids = clutter_actor_get_children (button_box);
  for (l = kids; l; l = l->next)
    {
      MxButton *b = MX_BUTTON (l->data);
      MxAction *a = mx_button_get_action (b);
      const char *name = mx_action_get_name (a);

      if (!g_strcmp0 (name , "back"))
        priv->back_button = (ClutterActor *)b;
      else if (!g_strcmp0 (name , "forward"))
        priv->forward_button = (ClutterActor *)b;
    }

  g_assert (priv->back_button && priv->forward_button);
  g_list_free (kids);

  /*
   * NB: MUST handle key press, not release, since we will never get a
   *     release for the BACK key event unless we handle the press.
   */
  g_signal_connect (self, "key-press-event",
                    G_CALLBACK (mtn_dialog_key_press), self);

  mex_push_focus (MX_FOCUSABLE (self));

  mtn_connman_new (NULL, _connman_new_cb, self);
}

ClutterActor *
mtn_dialog_new (ClutterActor *transient_for)
{
  return g_object_new (MTN_TYPE_DIALOG, "transient-for", transient_for, NULL);
}

