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

#include "mex-group-item.h"
#include "mex-view-model.h"

G_DEFINE_TYPE (MexGroupItem, mex_group_item, MEX_TYPE_GENERIC_CONTENT)

#define GROUP_ITEM_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GROUP_ITEM, MexGroupItemPrivate))

struct _MexGroupItemPrivate
{
  MexModel *model;

  MexModel *source_model;

  gint filter_key;
  gchar *filter_value;

  gint second_filter_key;
  gchar *second_filter_value;

  gint group_key;
};

enum
{
  PROP_0,

  PROP_SOURCE_MODEL,
  PROP_MODEL,

  PROP_FILTER_KEY,
  PROP_FILTER_VALUE,

  PROP_GROUP_KEY,

  PROP_SECOND_FILTER_KEY,
  PROP_SECOND_FILTER_VALUE,
};

static void
mex_group_item_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MexGroupItemPrivate *priv = MEX_GROUP_ITEM (object)->priv;

  switch (property_id)
    {
    case (PROP_SOURCE_MODEL):
      g_value_set_object (value, priv->source_model);
      break;

    case (PROP_MODEL):
      g_value_set_object (value,
                          mex_group_item_get_model (MEX_GROUP_ITEM (object)));
      break;

    case (PROP_FILTER_KEY):
      g_value_set_int (value, priv->filter_key);
      break;

    case (PROP_FILTER_VALUE):
      g_value_set_string (value, priv->filter_value);
      break;

    case (PROP_GROUP_KEY):
      g_value_set_int (value, priv->group_key);
      break;

    case (PROP_SECOND_FILTER_KEY):
      g_value_set_int (value, priv->second_filter_key);
      break;

    case (PROP_SECOND_FILTER_VALUE):
      g_value_set_string (value, priv->second_filter_value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_group_item_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MexGroupItemPrivate *priv = MEX_GROUP_ITEM (object)->priv;

  switch (property_id)
    {
    case (PROP_SOURCE_MODEL):
      priv->source_model = g_value_dup_object (value);
      break;

    case (PROP_FILTER_KEY):
      priv->filter_key = g_value_get_int (value);
      break;

    case (PROP_FILTER_VALUE):
      priv->filter_value = g_value_dup_string (value);
      break;

    case (PROP_GROUP_KEY):
      priv->group_key = g_value_get_int (value);
      break;

    case (PROP_SECOND_FILTER_KEY):
      priv->second_filter_key = g_value_get_int (value);
      break;

    case (PROP_SECOND_FILTER_VALUE):
      priv->second_filter_value = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_group_item_dispose (GObject *object)
{
  MexGroupItemPrivate *priv = MEX_GROUP_ITEM (object)->priv;

  if (priv->source_model)
    {
      g_object_unref (priv->source_model);
      priv->source_model = NULL;
    }

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  G_OBJECT_CLASS (mex_group_item_parent_class)->dispose (object);
}

static void
mex_group_item_finalize (GObject *object)
{
  MexGroupItemPrivate *priv = MEX_GROUP_ITEM (object)->priv;

  g_free (priv->filter_value);
  priv->filter_value = NULL;

  g_free (priv->second_filter_value);
  priv->second_filter_value = NULL;

  G_OBJECT_CLASS (mex_group_item_parent_class)->finalize (object);
}

static void
mex_group_item_class_init (MexGroupItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexGroupItemPrivate));

  object_class->get_property = mex_group_item_get_property;
  object_class->set_property = mex_group_item_set_property;
  object_class->dispose = mex_group_item_dispose;
  object_class->finalize = mex_group_item_finalize;

  pspec = g_param_spec_object ("source-model", "Source Model",
                               "The model the group originates from",
                               G_TYPE_OBJECT,
                               G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY
                               | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SOURCE_MODEL, pspec);

  pspec = g_param_spec_object ("model", "Model",
                               "The model containing the items in the group",
                               G_TYPE_OBJECT,
                               G_PARAM_STATIC_STRINGS | G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_MODEL, pspec);

  pspec = g_param_spec_int ("filter-key", "Filter Key",
                            "The filter key used to filter the group",
                            0, G_MAXINT, 0,
                            G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY
                            | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FILTER_KEY, pspec);

  pspec = g_param_spec_string ("filter-value", "Filter Value",
                               "The filter value used to filter the group",
                               "",
                               G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY
                               | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FILTER_VALUE, pspec);

  pspec = g_param_spec_int ("group-key", "Group Key",
                            "The grouping key used to form the group",
                            0, G_MAXINT, 0,
                            G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY
                            | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_GROUP_KEY, pspec);

  pspec = g_param_spec_int ("second-filter-key", "Second Filter Key",
                            "The second filter key used to filter the group",
                            0, G_MAXINT, 0,
                            G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY
                            | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SECOND_FILTER_KEY, pspec);

  pspec = g_param_spec_string ("second-filter-value", "Second Filter Value",
                               "The second filter value used to filter the"
                               " group",
                               "",
                               G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY
                               | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SECOND_FILTER_VALUE,
                                   pspec);
}

static void
mex_group_item_init (MexGroupItem *self)
{
  self->priv = GROUP_ITEM_PRIVATE (self);

  mex_content_set_metadata (MEX_CONTENT (self), MEX_CONTENT_METADATA_MIMETYPE,
                            "x-mex/group");
}

MexGroupItem *
mex_group_item_new (const gchar *title,
                    MexModel    *source_model,
                    gint         filter_key,
                    const gchar *filter_value,
                    gint         second_filter_key,
                    const gchar *second_filter_value,
                    gint         group_key)
{
  return g_object_new (MEX_TYPE_GROUP_ITEM,
                       "title", title,
                       "source-model", source_model,
                       "filter-key", filter_key,
                       "filter-value", filter_value,
                       "second-filter-key", second_filter_key,
                       "second-filter-value", second_filter_value,
                       "group-key", group_key,
                       NULL);
}


MexModel*
mex_group_item_get_model (MexGroupItem *item)
{
  MexGroupItemPrivate *priv = item->priv;

  if (!priv->model)
    {
      priv->model = mex_view_model_new (priv->source_model);

      mex_view_model_set_group_by (MEX_VIEW_MODEL (priv->model),
                                   priv->group_key);


      mex_view_model_set_filter_by (MEX_VIEW_MODEL (priv->model),
                                    priv->filter_key, priv->filter_value,
                                    priv->second_filter_key,
                                    priv->second_filter_value,
                                    MEX_CONTENT_METADATA_NONE);

      if (priv->second_filter_key)
        {
          gchar *title;

          title = g_strdup_printf ("%s - %s",
                                   priv->second_filter_value, priv->filter_value);
          g_object_set (priv->model, "title", title, NULL);
          g_free (title);
        }
      else
        g_object_set (priv->model, "title", priv->filter_value, NULL);
    }

  return priv->model;
}

