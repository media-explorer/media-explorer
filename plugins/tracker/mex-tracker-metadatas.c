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

#include "mex-tracker-metadatas.h"

G_DEFINE_TYPE (MexTrackerMetadatas, mex_tracker_metadatas, G_TYPE_OBJECT)

#define TRACKER_METADATAS_PRIVATE(o)                            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TRACKER_METADATAS,     \
                                MexTrackerMetadatasPrivate))

enum
{
  PROP_0,

  PROP_METADATA_LIST
};

struct _MexTrackerMetadatasPrivate
{
  GList *metadata_list; /* list(MexContentMetadata) */
};

static void
mex_tracker_metadatas_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  MexTrackerMetadatasPrivate *priv = ((MexTrackerMetadatas *)object)->priv;

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
mex_tracker_metadatas_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  MexTrackerMetadatas *metadatas = (MexTrackerMetadatas *)object;

  switch (property_id)
    {
    case PROP_METADATA_LIST:
      mex_tracker_metadatas_set_metadata_list (metadatas,
                  (GList *) g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tracker_metadatas_dispose (GObject *object)
{
  MexTrackerMetadatasPrivate *priv = TRACKER_METADATAS_PRIVATE (object);

  if (priv->metadata_list)
    {
      g_list_free (priv->metadata_list);
      priv->metadata_list = NULL;
    }

  G_OBJECT_CLASS (mex_tracker_metadatas_parent_class)->dispose (object);
}

static void
mex_tracker_metadatas_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_metadatas_parent_class)->finalize (object);
}

static void
mex_tracker_metadatas_class_init (MexTrackerMetadatasClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  g_type_class_add_private (klass, sizeof (MexTrackerMetadatasPrivate));

  object_class->get_property = mex_tracker_metadatas_get_property;
  object_class->set_property = mex_tracker_metadatas_set_property;
  object_class->dispose = mex_tracker_metadatas_dispose;
  object_class->finalize = mex_tracker_metadatas_finalize;

  pspec = g_param_spec_pointer ("metadata-list", "Metadata list",
                                "List of metadatas",
                                G_PARAM_READWRITE |
                                G_PARAM_STATIC_STRINGS |
                                G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_METADATA_LIST, pspec);
}

static void
mex_tracker_metadatas_init (MexTrackerMetadatas *self)
{
  self->priv = TRACKER_METADATAS_PRIVATE (self);
}

MexTrackerMetadatas *
mex_tracker_metadatas_new (MexContentMetadata meta, ...)
{
  GList *metadata_list = NULL;
  MexContentMetadata next_meta;
  va_list va_list;

  va_start (va_list, meta);
  next_meta = meta;
  while (next_meta != MEX_CONTENT_METADATA_INVALID)
    {
      metadata_list = g_list_prepend (metadata_list,
                                      GINT_TO_POINTER (next_meta));
      next_meta = va_arg (va_list, MexContentMetadata);
    }
  va_end (va_list);

  return g_object_new (MEX_TYPE_TRACKER_METADATAS,
                       "metadata-list", metadata_list,
                       NULL);
}

GList *
mex_tracker_metadatas_get_metadata_list (MexTrackerMetadatas *metadatas)
{
  MexTrackerMetadatasPrivate *priv;

  g_return_if_fail (MEX_IS_TRACKER_METADATAS (metadatas));

  priv = metadatas->priv;

  return priv->metadata_list;
}

void
mex_tracker_metadatas_set_metadata_list (MexTrackerMetadatas *metadatas,
                                         GList               *metadata_list)
{
  MexTrackerMetadatasPrivate *priv;

  g_return_if_fail (MEX_IS_TRACKER_METADATAS (metadatas));

  priv = metadatas->priv;

  if (priv->metadata_list)
    {
      g_list_free (priv->metadata_list);
      priv->metadata_list = NULL;
    }

  priv->metadata_list = g_list_copy (metadata_list);
}
