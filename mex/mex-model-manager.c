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


#include "mex-model-manager.h"
#include "mex-marshal.h"

G_DEFINE_TYPE (MexModelManager, mex_model_manager, G_TYPE_OBJECT)

#define MODEL_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MODEL_MANAGER, MexModelManagerPrivate))

enum
{
  MODEL_ADDED,
  MODEL_REMOVED,
  CATEGORIES_CHANGED,

  LAST_SIGNAL
};

struct _MexModelManagerPrivate
{
  GList      *models;
  GHashTable *categories;
};

static guint signals[LAST_SIGNAL] = { 0, };

void
mex_model_sort_func_info_free (MexModelSortFuncInfo *sort_info)
{
  g_free (sort_info->name);
  g_free (sort_info->display_name);
  g_slice_free (MexModelSortFuncInfo, sort_info);
}

MexModelSortFuncInfo *
mex_model_sort_func_info_new (const gchar      *name,
                              const gchar      *display_name,
                              MexModelSortFunc  func,
                              gpointer          user_data)
{
  MexModelSortFuncInfo *info;

  info = g_slice_new (MexModelSortFuncInfo);

  info->name = g_strdup (name);
  info->display_name = g_strdup (display_name);
  info->sort_func = func;
  info->userdata = user_data;

  return info;
}

static void
mex_model_manager_free_category (MexModelCategoryInfo *info)
{
  g_free (info->name);
  g_free (info->display_name);
  g_slice_free (MexModelCategoryInfo, info);
}

static void
mex_model_manager_dispose (GObject *object)
{
  MexModelManagerPrivate *priv = MEX_MODEL_MANAGER (object)->priv;

  while (priv->models)
    {
      g_object_unref (priv->models->data);
      priv->models = g_list_delete_link (priv->models, priv->models);
    }

  if (priv->categories)
    {
      g_hash_table_unref (priv->categories);
      priv->categories = NULL;
    }

  G_OBJECT_CLASS (mex_model_manager_parent_class)->dispose (object);
}

static void
mex_model_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_model_manager_parent_class)->finalize (object);
}

static void
mex_model_manager_get_property (GObject    *object,
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
mex_model_manager_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_model_manager_class_init (MexModelManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexModelManagerPrivate));

  object_class->dispose = mex_model_manager_dispose;
  object_class->finalize = mex_model_manager_finalize;
  object_class->get_property = mex_model_manager_get_property;
  object_class->set_property = mex_model_manager_set_property;

  signals[MODEL_ADDED] =
    g_signal_new ("model-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexModelManagerClass, model_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  signals[MODEL_REMOVED] =
    g_signal_new ("model-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexModelManagerClass, model_removed),
                  NULL, NULL,
                  mex_marshal_VOID__OBJECT_STRING,
                  G_TYPE_NONE, 2,
                  G_TYPE_OBJECT, G_TYPE_STRING);

  signals[CATEGORIES_CHANGED] =
    g_signal_new ("categories-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexModelManagerClass, categories_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mex_model_manager_init (MexModelManager *self)
{
  MexModelManagerPrivate *priv = self->priv = MODEL_MANAGER_PRIVATE (self);

  priv->categories = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            NULL,
                                            (GDestroyNotify)
                                            mex_model_manager_free_category);
}

MexModelManager *
mex_model_manager_get_default (void)
{
  static MexModelManager *manager = NULL;

  if (!manager)
    manager = g_object_new (MEX_TYPE_MODEL_MANAGER, NULL);

  return manager;
}

static gint
mex_model_manager_sort_categories_cb (gconstpointer a,
                                      gconstpointer b)
{
  const MexModelCategoryInfo *cat_a = a;
  const MexModelCategoryInfo *cat_b = b;

  return cat_a->priority - cat_b->priority;
}

static gint
mex_model_manager_simple_sort_cb (gconstpointer a,
                                  gconstpointer b)
{
  gint priority_a, priority_b;

  g_object_get (G_OBJECT (a), "priority", &priority_a, NULL);
  g_object_get (G_OBJECT (b), "priority", &priority_b, NULL);

  return priority_b - priority_a;
}

static gint
mex_model_manager_sort_cb (gconstpointer a,
                           gconstpointer b,
                           gpointer      userdata)
{
  MexModel *model_a = MEX_MODEL (a);
  MexModel *model_b = MEX_MODEL (b);
  MexModelManager *manager = userdata;
  MexModelManagerPrivate *priv = manager->priv;
  MexModelCategoryInfo *category_a = NULL, *category_b = NULL;
  gchar *category;
  gint priority_a, priority_b;

  g_object_get (model_a, "category", &category, "priority", &priority_a, NULL);
  if (category)
    category_a = g_hash_table_lookup (priv->categories, category);
  g_free (category);

  g_object_get (model_b, "category", &category, "priority", &priority_b, NULL);
  if (category)
    category_b = g_hash_table_lookup (priv->categories, category);
  g_free (category);

  if (category_a)
    {
      if (!category_b)
        return -1;
      else
        {
          gint order = category_b->priority - category_a->priority;
          if (order != 0)
            return order;
        }
    }
  else if (category_b)
    return 1;

  return priority_b - priority_a;
}

GList *
mex_model_manager_get_models (MexModelManager *manager)
{
  GList *models;
  MexModelManagerPrivate *priv;

  g_return_val_if_fail (MEX_IS_MODEL_MANAGER (manager), NULL);

  priv = manager->priv;

  models = g_list_copy (priv->models);

  return models;
}

GList *
mex_model_manager_get_models_for_category (MexModelManager *manager,
                                           const gchar     *category)
{
  GList *i, *models;
  MexModelManagerPrivate *priv;

  g_return_val_if_fail (MEX_IS_MODEL_MANAGER (manager), NULL);

  models = NULL;
  priv = manager->priv;

  for (i = priv->models; i; i = i->next)
    {
      MexModel *model = i->data;
      gchar *c;

      g_object_get (model, "category", &c, NULL);

      if (g_strcmp0 (category, c) == 0)
        models = g_list_prepend (models, model);
    }

  models = g_list_sort (models, mex_model_manager_simple_sort_cb);

  return models;
}

void
mex_model_manager_add_model (MexModelManager *manager,
                             MexModel        *model)
{
  MexModelManagerPrivate *priv;

  g_return_if_fail (MEX_IS_MODEL_MANAGER (manager));

  priv = manager->priv;

  priv->models = g_list_insert_sorted_with_data (priv->models,
                                                 g_object_ref (model),
                                                 mex_model_manager_sort_cb,
                                                 manager);

  g_signal_emit (manager, signals[MODEL_ADDED], 0, model);
}

void
mex_model_manager_remove_model (MexModelManager *manager,
                                MexModel        *model)
{
  gchar *category;
  MexModelManagerPrivate *priv;

  g_return_if_fail (MEX_IS_MODEL_MANAGER (manager));

  priv = manager->priv;

  priv->models = g_list_remove (priv->models, model);

  g_object_get (model, "category", &category, NULL);

  g_signal_emit (manager, signals[MODEL_REMOVED], 0, model, category);

  g_object_unref (model);
  g_free (category);
}

void
mex_model_manager_add_category (MexModelManager            *manager,
                                const MexModelCategoryInfo *info)
{
  MexModelCategoryInfo *info_copy;
  MexModelManagerPrivate *priv;

  g_return_if_fail (MEX_IS_MODEL_MANAGER (manager));

  priv = manager->priv;
  if (g_hash_table_lookup (priv->categories, info->name))
    {
      g_warning (G_STRLOC ": Category '%s' already exists", info->name);
      return;
    }

  info_copy = g_slice_dup (MexModelCategoryInfo, info);
  info_copy->name = g_strdup (info->name);
  info_copy->display_name = g_strdup (info->display_name);
  info_copy->icon_name = g_strdup (info->icon_name);

  g_hash_table_insert (priv->categories, info_copy->name, info_copy);

  priv->models = g_list_sort_with_data (priv->models,
                                        mex_model_manager_sort_cb, manager);

  g_signal_emit (manager, signals[CATEGORIES_CHANGED], 0);
}

void
mex_model_manager_remove_category (MexModelManager *manager,
                                   const gchar     *name)
{
  MexModelManagerPrivate *priv;

  g_return_if_fail (MEX_IS_MODEL_MANAGER (manager));

  priv = manager->priv;
  if (!g_hash_table_remove (priv->categories, name))
    {
      g_warning (G_STRLOC ": Category '%s' doesn't exist", name);
      return;
    }

  priv->models = g_list_sort_with_data (priv->models,
                                        mex_model_manager_sort_cb, manager);

  g_signal_emit (manager, signals[CATEGORIES_CHANGED], 0);
}

GList *
mex_model_manager_get_categories (MexModelManager *manager)
{
  GList *categories;

  g_return_val_if_fail (MEX_IS_MODEL_MANAGER (manager), NULL);

  categories = g_list_sort (g_hash_table_get_values (manager->priv->categories),
                            mex_model_manager_sort_categories_cb);

  return categories;
}

const MexModelCategoryInfo *
mex_model_manager_get_category_info (MexModelManager *manager,
                                     const gchar     *name)
{
  MexModelManagerPrivate *priv = manager->priv;

  g_return_val_if_fail (MEX_IS_MODEL_MANAGER (manager), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  priv = manager->priv;
  return g_hash_table_lookup (priv->categories, name);
}
