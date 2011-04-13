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


#include "mex-application-category.h"

G_DEFINE_TYPE (MexApplicationCategory, mex_application_category, G_TYPE_OBJECT)

#define CATEGORY_PRIVATE(o)                                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_APPLICATION_CATEGORY,  \
                                MexApplicationCategoryPrivate))

struct _MexApplicationCategoryPrivate
{
  gchar *name;
  GPtrArray *items;
};

enum
{
  PROP_0,

  PROP_NAME,
  PROP_ITEMS
};

/* Frees priv->items */
static void
mex_application_category_free_items (MexApplicationCategory *category)
{
  MexApplicationCategoryPrivate *priv = category->priv;

  if (priv->items)
    {
      guint i;

      for (i = 0; i < priv->items->len; i++)
        g_object_unref (g_ptr_array_index (priv->items, i));

      g_ptr_array_free (priv->items, TRUE);
      priv->items = NULL;
    }
}

/*
 * GObject implementation
 */

static void
mex_application_category_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  MexApplicationCategory *category = MEX_APPLICATION_CATEGORY (object);
  MexApplicationCategoryPrivate *priv = category->priv;

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_ITEMS:
      g_value_set_boxed (value, priv->items);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_application_category_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  MexApplicationCategory *category = MEX_APPLICATION_CATEGORY (object);

  switch (property_id)
    {
    case PROP_NAME:
      mex_application_category_set_name (category, g_value_get_string (value));
      break;
    case PROP_ITEMS:
      mex_application_category_set_items (category,
                                                 g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_application_category_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_application_category_parent_class)->dispose (object);
}

static void
mex_application_category_finalize (GObject *object)
{
  MexApplicationCategory *category = MEX_APPLICATION_CATEGORY (object);
  MexApplicationCategoryPrivate *priv = category->priv;

  mex_application_category_free_items (category);
  g_free (priv->name);
  priv->name = NULL;

  G_OBJECT_CLASS (mex_application_category_parent_class)->finalize (object);
}

static void
mex_application_category_class_init (MexApplicationCategoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexApplicationCategoryPrivate));

  object_class->get_property = mex_application_category_get_property;
  object_class->set_property = mex_application_category_set_property;
  object_class->dispose = mex_application_category_dispose;
  object_class->finalize = mex_application_category_finalize;

  pspec = g_param_spec_string ("name",
			       "Name",
			       "Categoy name",
			       "Unnamed",
			       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_NAME, pspec);

  pspec = g_param_spec_boxed ("items",
                              "Applications",
                              "List of MexApplication/MexApplicationCategory",
                              G_TYPE_PTR_ARRAY,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ITEMS, pspec);
}

static void
mex_application_category_init (MexApplicationCategory *category)
{
  MexApplicationCategoryPrivate *priv;

  category->priv = priv = CATEGORY_PRIVATE (category);

  priv->items = g_ptr_array_new_with_free_func (g_object_unref);
}

MexApplicationCategory *
mex_application_category_new (const gchar *name)
{
  return g_object_new (MEX_TYPE_APPLICATION_CATEGORY,
                       "name", name,
                       NULL);
}

/*
 * Accessors
 */

const gchar *
mex_application_category_get_name (MexApplicationCategory *category)
{
  g_return_val_if_fail (MEX_IS_APPLICATION_CATEGORY (category), NULL);

  return category->priv->name;
}

void
mex_application_category_set_name (MexApplicationCategory *category,
                        const gchar  *name)
{
  MexApplicationCategoryPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION_CATEGORY (category));

  priv = category->priv;

  g_free (priv->name);
  priv->name = g_strdup (name);
  g_object_notify (G_OBJECT (category), "name");
}

/**
 * mex_application_category_get_items
 *
 */
const GPtrArray *
mex_application_category_get_items (MexApplicationCategory *category)
{
  g_return_val_if_fail (MEX_IS_APPLICATION_CATEGORY (category), NULL);

  return category->priv->items;
}

/**
 * mex_application_category_set_items:
 * @category:
 * @items:
 *
 * Takes ownership of @items.
 *
 * Since: 0.2
 */
void
mex_application_category_set_items (MexApplicationCategory *category,
                                    GPtrArray              *items)
{
  MexApplicationCategoryPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION_CATEGORY (category));

  priv = category->priv;

  mex_application_category_free_items (category);
  priv->items = items;
  g_object_notify (G_OBJECT (category), "items");
}

void
mex_application_category_add_category (MexApplicationCategory *category,
                                       MexApplicationCategory *to_add)
{
  MexApplicationCategoryPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION_CATEGORY (category));

  priv = category->priv;
  g_ptr_array_add (priv->items, to_add);
}

void
mex_application_category_add_application (MexApplicationCategory *category,
                                          MexApplication         *application)
{
  MexApplicationCategoryPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION_CATEGORY (category));

  priv = category->priv;
  g_ptr_array_add (priv->items, application);
}
