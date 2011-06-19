/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#include "mex-metadatas.h"

G_DEFINE_TYPE (MexMetadatas, mex_metadatas, G_TYPE_OBJECT)

#define _METADATAS_PRIVATE(o)                                   \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_METADATAS,             \
                                MexMetadatasPrivate))

enum
{
  PROP_0,

  PROP_METADATA_LIST
};

struct _MexMetadatasPrivate
{
  GList *metadata_list; /* list(MexContentMetadata) */
};

static void
mex_metadatas_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MexMetadatasPrivate *priv = ((MexMetadatas *)object)->priv;

  switch (property_id)
    {
    case PROP_METADATA_LIST:
      g_value_set_pointer (value, g_list_copy (priv->metadata_list));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_metadatas_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MexMetadatas *metadatas = (MexMetadatas *)object;

  switch (property_id)
    {
    case PROP_METADATA_LIST:
      mex_metadatas_set_metadata_list (metadatas,
                  (GList *) g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_metadatas_dispose (GObject *object)
{
  MexMetadatasPrivate *priv = _METADATAS_PRIVATE (object);

  if (priv->metadata_list)
    {
      g_list_free (priv->metadata_list);
      priv->metadata_list = NULL;
    }

  G_OBJECT_CLASS (mex_metadatas_parent_class)->dispose (object);
}

static void
mex_metadatas_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_metadatas_parent_class)->finalize (object);
}

static void
mex_metadatas_class_init (MexMetadatasClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  g_type_class_add_private (klass, sizeof (MexMetadatasPrivate));

  object_class->get_property = mex_metadatas_get_property;
  object_class->set_property = mex_metadatas_set_property;
  object_class->dispose = mex_metadatas_dispose;
  object_class->finalize = mex_metadatas_finalize;

  pspec = g_param_spec_pointer ("metadata-list", "Metadata list",
                                "List of metadatas",
                                G_PARAM_READWRITE |
                                G_PARAM_STATIC_STRINGS |
                                G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_METADATA_LIST, pspec);
}

static void
mex_metadatas_init (MexMetadatas *self)
{
  self->priv = _METADATAS_PRIVATE (self);
}

MexMetadatas *
mex_metadatas_new (MexContentMetadata meta, ...)
{
  GList *metadata_list = NULL;
  MexContentMetadata next_meta;
  MexMetadatas *obj;
  va_list va_args;

  va_start (va_args, meta);
  next_meta = meta;
  while (next_meta != MEX_CONTENT_METADATA_INVALID)
    {
      metadata_list = g_list_prepend (metadata_list,
                                      GINT_TO_POINTER (next_meta));
      next_meta = va_arg (va_args, MexContentMetadata);
    }
  va_end (va_args);

  obj = g_object_new (MEX_TYPE_METADATAS,
                      "metadata-list", metadata_list,
                      NULL);

  g_list_free (metadata_list);

  return obj;
}

GList *
mex_metadatas_get_metadata_list (MexMetadatas *metadatas)
{
  MexMetadatasPrivate *priv;

  g_return_val_if_fail (MEX_IS_METADATAS (metadatas), NULL);

  priv = metadatas->priv;

  return priv->metadata_list;
}

void
mex_metadatas_set_metadata_list (MexMetadatas *metadatas,
                                 GList        *metadata_list)
{
  MexMetadatasPrivate *priv;

  g_return_if_fail (MEX_IS_METADATAS (metadatas));

  priv = metadatas->priv;

  if (priv->metadata_list)
    {
      g_list_free (priv->metadata_list);
      priv->metadata_list = NULL;
    }

  priv->metadata_list = g_list_copy (metadata_list);
}
