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

struct _MexContactPrivate
{
  TpContact *contact;
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
      mex_contact_set_tp_contact (MEX_CONTACT (object),
                                  TP_CONTACT(g_value_get_object (value)));
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

  if (priv->contact)
    {
      g_object_unref (priv->contact);
      priv->contact = NULL;
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

  pspec = g_param_spec_string ("contact",
                               "Contact",
                               "Contact object",
                               NULL,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CONTACT, pspec);
}

/*
 * Accessors
 */

static void
mex_contact_init (MexContact *self)
{
  self->priv = CONTACT_PRIVATE (self);
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

void
mex_contact_set_tp_contact (MexContact *self,
                            TpContact *contact)
{
  MexContactPrivate *priv;

  g_return_if_fail (MEX_IS_CONTACT (self));

  priv = self->priv;

  g_object_unref (priv->contact);
  priv->contact = contact;
  g_object_ref(priv->contact);
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

  GFile *file;

  switch (key)
  {
    case MEX_CONTENT_METADATA_TITLE:
      return tp_contact_get_alias (priv->contact);
    case MEX_CONTENT_METADATA_SYNOPSIS:
      return tp_contact_get_presence_message (priv->contact);
//     case MEX_CONTENT_METADATA_ID:
//       return tp_contact_get_handle (priv->contact);
    case MEX_CONTENT_METADATA_STILL:
      file = tp_contact_get_avatar_file (priv->contact);
      if (file) {
        return g_file_get_uri (file);
      }
    case MEX_CONTENT_METADATA_MIMETYPE:
      return "x-mex/contact";
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
