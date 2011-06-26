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

#include "mex-generic-content.h"
#include "mex-content.h"
#include "mex-enum-types.h"

enum {
  PROP_0 = MEX_CONTENT_METADATA_LAST_ID,
  PROP_LAST_POSITION_START,
  PROP_SAVE_LAST_POSITION,
};

enum {
  LAST_SIGNAL,
};

struct _MexGenericContentPrivate {
  int dummy;

  GHashTable *metadata;

  gboolean last_position_start;
  gboolean save_last_position;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MEX_TYPE_GENERIC_CONTENT, MexGenericContentPrivate))
static void mex_content_iface_init (MexContentIface *iface);
G_DEFINE_TYPE_WITH_CODE (MexGenericContent, mex_generic_content, G_TYPE_INITIALLY_UNOWNED,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                mex_content_iface_init));

/*
 * MexContent implementation
 */

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
  MexGenericContent *gc = (MexGenericContent *) content;
  MexGenericContentPrivate *priv = gc->priv;

  return (const gchar *) g_hash_table_lookup (priv->metadata,
                                              GUINT_TO_POINTER (key));
}

static void
content_set_metadata (MexContent         *content,
                      MexContentMetadata  key,
                      const gchar        *value)
{
  const char *property;
  MexGenericContent *gc = (MexGenericContent *) content;
  MexGenericContentPrivate *priv = gc->priv;

  if (value)
    g_hash_table_insert (priv->metadata, GUINT_TO_POINTER (key),
                         g_strdup (value));
  else
    g_hash_table_remove (priv->metadata, GUINT_TO_POINTER (key));

  property = mex_content_get_property_name (content, key);
  g_object_notify (G_OBJECT (content), property);
}

static char *
content_get_metadata_fallback (MexContent         *content,
                               MexContentMetadata  key)
{
  return NULL;
}

static const char *
content_get_property_name (MexContent         *content,
                           MexContentMetadata  key)
{
  return mex_enum_to_string (MEX_TYPE_CONTENT_METADATA, key);
}

static void
content_save_metadata (MexContent *content)
{
  /* No implementation possible here */
}

struct MetadataCbStuff {
  MexContentMetadataCb callback;
  gpointer             data;
};

static void
content_foreach_metadata_cb (gpointer key,
                             gpointer value,
                             gpointer data)
{
  struct MetadataCbStuff *stuff = (struct MetadataCbStuff *) data;

  stuff->callback ((MexContentMetadata) key,
                   (const gchar *) value,
                   stuff->data);
}

static void
content_foreach_metadata (MexContent           *content,
                          MexContentMetadataCb  callback,
                          gpointer              data)
{
  MexGenericContent *gc = (MexGenericContent *) content;
  MexGenericContentPrivate *priv = gc->priv;
  struct MetadataCbStuff stuff;

  stuff.callback = callback;
  stuff.data     = data;

  g_hash_table_foreach (priv->metadata,
                        content_foreach_metadata_cb,
                        &stuff);
}

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->get_property          = content_get_property;
  iface->get_metadata          = content_get_metadata;
  iface->set_metadata          = content_set_metadata;
  iface->get_metadata_fallback = content_get_metadata_fallback;
  iface->get_property_name     = content_get_property_name;
  iface->save_metadata         = content_save_metadata;
  iface->foreach_metadata      = content_foreach_metadata;
}

/*
 * GObject implementation
 */

static void
mex_generic_content_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_generic_content_parent_class)->finalize (object);
}

static void
mex_generic_content_dispose (GObject *object)
{
  MexGenericContent *content = (MexGenericContent *) object;
  MexGenericContentPrivate *priv = content->priv;

  if (priv->metadata)
    {
      g_hash_table_unref (priv->metadata);
      priv->metadata = NULL;
    }

  G_OBJECT_CLASS (mex_generic_content_parent_class)->dispose (object);
}

static void
mex_generic_content_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MexGenericContentPrivate *priv = GET_PRIVATE (object);

  /* For all dynamic properties, just pass them to the subclass */
  if (prop_id < MEX_CONTENT_METADATA_LAST_ID) {
    mex_content_set_metadata (MEX_CONTENT (object), prop_id,
                              g_value_get_string (value));
    return;
  }

  switch (prop_id) {
  case PROP_LAST_POSITION_START:
    priv->last_position_start = g_value_get_boolean (value);
    break;

  case PROP_SAVE_LAST_POSITION:
    priv->save_last_position = g_value_get_boolean (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
mex_generic_content_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MexGenericContent *self = (MexGenericContent *) object;
  MexGenericContentPrivate *priv = GET_PRIVATE (object);
  MexContent *content = MEX_CONTENT (self);

  /* For all dynamic properties, just pass them to the subclass */
  if (prop_id < MEX_CONTENT_METADATA_LAST_ID) {
    g_value_set_string (value, mex_content_get_metadata (content, prop_id));
    return;
  }

  switch (prop_id) {
  case PROP_LAST_POSITION_START:
    g_value_set_boolean (value, priv->last_position_start);
    break;

  case PROP_SAVE_LAST_POSITION:
    g_value_set_boolean (value, priv->save_last_position);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
mex_generic_content_class_init (MexGenericContentClass *klass)
{
  GObjectClass *o_class = (GObjectClass *) klass;
  int i;

  o_class->dispose = mex_generic_content_dispose;
  o_class->finalize = mex_generic_content_finalize;
  o_class->set_property = mex_generic_content_set_property;
  o_class->get_property = mex_generic_content_get_property;

  g_type_class_add_private (klass, sizeof (MexGenericContentPrivate));

  /* Install a property for each metadata type */
  for (i = MEX_CONTENT_METADATA_NONE;
       i < MEX_CONTENT_METADATA_LAST_ID;
       i++) {
    GParamSpec *pspec;
    const char *name;

    if (!i) continue;

    name = mex_content_metadata_key_to_string (i);

    pspec = g_param_spec_string (name, name, "A dynamic metadata property",
                                 NULL, G_PARAM_READWRITE |
                                 G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (o_class, i, pspec);
  }

  g_object_class_override_property (o_class,
                                    PROP_LAST_POSITION_START,
                                    "last-position-start");

  g_object_class_override_property (o_class,
                                    PROP_SAVE_LAST_POSITION,
                                    "save-last-position");
}

static void
mex_generic_content_init (MexGenericContent *self)
{
  MexGenericContentPrivate *priv = GET_PRIVATE (self);

  self->priv = priv;

  priv->metadata = g_hash_table_new_full (NULL, NULL, NULL, g_free);

  priv->last_position_start = TRUE;
  priv->save_last_position = TRUE;
}

gboolean
mex_generic_content_get_last_position_start (MexGenericContent *self)
{
  MexGenericContentPrivate *priv = self->priv;

  g_return_val_if_fail (IS_MEX_GENERIC_CONTENT (self), FALSE);

  return priv->last_position_start;
}

gboolean
mex_generic_content_get_save_last_position (MexGenericContent *self)
{
  MexGenericContentPrivate *priv = self->priv;

  g_return_val_if_fail (IS_MEX_GENERIC_CONTENT (self), FALSE);

  return priv->save_last_position;
}
