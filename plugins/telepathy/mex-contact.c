/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Collabora Ltd.
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


#include "mex-contact.h"

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/_gen/telepathy-interfaces.h>

static void mex_content_iface_init (MexContentIface *iface);
G_DEFINE_TYPE_WITH_CODE (MexContact,
                         mex_contact,
                         MEX_TYPE_GENERIC_CONTENT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                mex_content_iface_init))

#define CONTACT_PRIVATE(o)                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                    \
                                MEX_TYPE_CONTACT,   \
                                MexContactPrivate))

enum {
  SHOULD_ADD_TO_MODEL_CHANGED,
  LAST_SIGNAL
};

static guint
mex_contact_signals[LAST_SIGNAL] = { 0 };

struct _MexContactPrivate
{
  TpContact *contact;
  gchar *avatar_path;
  gchar *mimetype;
};

enum
{
  PROP_0,

  PROP_CONTACT,
};

/*
 * GObject implementation
 */

static void
mex_contact_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  MexContact *contact = MEX_CONTACT (object);
  MexContactPrivate *priv = contact->priv;

  switch (property_id)
    {
    case PROP_CONTACT:
      g_value_set_object (value, priv->contact);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_contact_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_CONTACT:
      if (value != NULL) {
        mex_contact_set_tp_contact (MEX_CONTACT (object),
                                    TP_CONTACT(g_value_get_pointer (value)));
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_contact_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_contact_parent_class)->dispose (object);
}

static void
mex_contact_finalize (GObject *object)
{
  MexContact *contact = MEX_CONTACT (object);
  MexContactPrivate *priv = contact->priv;

  if (priv->contact) {
    g_object_unref (priv->contact);
    priv->contact = NULL;
  }
  if (priv->avatar_path) {
    g_free(priv->avatar_path);
    priv->avatar_path = NULL;
  }

  G_OBJECT_CLASS (mex_contact_parent_class)->finalize (object);
}

static void
mex_contact_class_init (MexContactClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexContactPrivate));

  object_class->get_property = mex_contact_get_property;
  object_class->set_property = mex_contact_set_property;
  object_class->dispose = mex_contact_dispose;
  object_class->finalize = mex_contact_finalize;

  // Signals
  mex_contact_signals[SHOULD_ADD_TO_MODEL_CHANGED] =
    g_signal_new("should-add-to-model-changed",
                 MEX_TYPE_CONTACT,
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL,
                 NULL,
                 g_cclosure_marshal_VOID__BOOLEAN,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_BOOLEAN);

  // Properties
  pspec = g_param_spec_pointer ("contact",
                                "Contact",
                                "Contact object",
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CONTACT, pspec);
}

gboolean mex_contact_should_add_mimetype_to_model(gchar *mimetype)
{
  if (!tp_strdiff(mimetype, "x-mex-av-contact")) {
    return TRUE;
  }
  if (!tp_strdiff(mimetype, "x-mex-audio-contact")) {
    return TRUE;
  }
  if (!tp_strdiff(mimetype, "x-mex-pending-contact")) {
    return TRUE;
  }

  return FALSE;
}

gboolean mex_contact_should_add_to_model(MexContact* self)
{
  return mex_contact_should_add_mimetype_to_model(self->priv->mimetype);
}

gboolean
mex_contact_validate_presence(TpContact* contact)
{
  TpConnectionPresenceType presence = tp_contact_get_presence_type(contact);

  TpConnectionPresenceType valid_presence = TP_CONNECTION_PRESENCE_TYPE_AVAILABLE |
                                            TP_CONNECTION_PRESENCE_TYPE_AWAY |
                                            TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY |
                                            TP_CONNECTION_PRESENCE_TYPE_HIDDEN |
                                            TP_CONNECTION_PRESENCE_TYPE_BUSY;

  if (presence & valid_presence != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}


/*
 * Accessors
 */

static void
mex_contact_init (MexContact *self)
{
  self->priv = CONTACT_PRIVATE (self);
  self->priv->contact = NULL;
  self->priv->avatar_path = NULL;
}

MexContact *
mex_contact_new (void)
{
  return g_object_new (MEX_TYPE_CONTACT, NULL);
}

TpContact *
mex_contact_get_tp_contact (MexContact* self)
{
  g_return_val_if_fail (MEX_IS_CONTACT (self), NULL);

  return self->priv->contact;
}

static void
mex_contact_compute_mimetype (MexContact *self)
{
  MexContactPrivate *priv = self->priv;

  // Capabilities
  guint i;

  TpCapabilities *capabilities;
  capabilities = tp_contact_get_capabilities(priv->contact);
  GPtrArray *classes;
  classes = tp_capabilities_get_channel_classes(capabilities);

  priv->mimetype = "x-mex-contact";

  for (i = 0; i < classes->len; i++) {
    GValueArray *arr = g_ptr_array_index (classes, i);
    GHashTable *fixed;
    const gchar *chan_type;
    TpHandleType handle_type;
    gboolean valid;

    fixed =  g_value_get_boxed (g_value_array_get_nth (arr, 0));

    if (g_hash_table_size (fixed) != 2)
      continue;

    chan_type = tp_asv_get_string (fixed, TP_PROP_CHANNEL_CHANNEL_TYPE);
    handle_type = tp_asv_get_uint32 (fixed,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, &valid);

    if (!valid) {
      continue;
    }

    if (!tp_strdiff (chan_type, "org.freedesktop.Telepathy.Channel.Type.Call.DRAFT")) {
      priv->mimetype = "x-mex-av-contact";
    }
  }
}

static void
mex_contact_on_subscription_states_changed (TpContact *contact,
                                            guint      subscribe,
                                            guint      publish,
                                            gchar     *publish_request,
                                            gpointer   user_data)
{
  MexContact *self = MEX_CONTACT (user_data);

  mex_contact_compute_mimetype(self);
}

static void
mex_contact_on_presence_changed (TpContact *contact,
                                 guint      type,
                                 gchar     *status,
                                 gchar     *message,
                                 gpointer   user_data)
{
  MexContact *self = MEX_CONTACT (user_data);

  mex_contact_compute_mimetype(self);
}

void
mex_contact_set_tp_contact (MexContact *self,
                            TpContact *contact)
{
  if (contact == NULL) {
    return;
  }

  MexContactPrivate *priv;

  g_return_if_fail (MEX_IS_CONTACT (self));

  priv = self->priv;

  if (priv->contact) {
    g_object_unref (priv->contact);
  }

  g_object_ref(contact);
  priv->contact = contact;

  if (priv->avatar_path) {
    g_free(priv->avatar_path);
  }

  GFile *file = tp_contact_get_avatar_file (priv->contact);
  if (file) {
    priv->avatar_path = g_file_get_uri (file);
  } else {
    priv->avatar_path = NULL;
  }

  if (tp_contact_get_publish_state(contact) == TP_SUBSCRIPTION_STATE_ASK) {
    g_signal_connect(contact,
                     "subscription-states-changed",
                     G_CALLBACK(mex_contact_on_subscription_states_changed),
                     self);
  }
  mex_contact_compute_mimetype(self);

  g_signal_connect(contact,
                   "presence-changed",
                   G_CALLBACK(mex_contact_on_presence_changed),
                   self);

  g_object_notify (G_OBJECT (self), "contact");
}

static GParamSpec *
content_get_property (MexContent         *content,
                      MexContentMetadata  key)
{
  /* TODO */
  return NULL;
}

static const char *
content_get_metadata (MexContent         *content,
                      MexContentMetadata  key)
{
  MexContact *contact = MEX_CONTACT (content);
  MexContactPrivate *priv = contact->priv;

  if (priv->contact == NULL) {
    return NULL;
  }

  switch (key)
  {
    case MEX_CONTENT_METADATA_TITLE:
      return tp_contact_get_alias (priv->contact);
    case MEX_CONTENT_METADATA_SYNOPSIS:
      return tp_contact_get_presence_message (priv->contact);
//     case MEX_CONTENT_METADATA_ID:
//       return tp_contact_get_handle (priv->contact);
    case MEX_CONTENT_METADATA_STILL:
      return priv->avatar_path;
    case MEX_CONTENT_METADATA_MIMETYPE:
      return priv->mimetype;
    default:
      return NULL;
  }

  return NULL;
}

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->get_property = content_get_property;
  iface->get_metadata = content_get_metadata;
}
