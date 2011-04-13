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
mex_model_info_free (MexModelInfo *info)
{
  while (info->sort_infos)
    {
      MexModelSortFuncInfo *sort_info = info->sort_infos->data;

      g_free (sort_info->name);
      g_free (sort_info->display_name);
      g_slice_free (MexModelSortFuncInfo, sort_info);

      info->sort_infos = g_list_delete_link (info->sort_infos,
                                             info->sort_infos);
    }

  if (info->model)
    g_object_unref (info->model);

  if (info->alt_model)
    g_object_unref (info->alt_model);

  g_free (info->category);

  g_slice_free (MexModelInfo, info);
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
      mex_model_info_free (priv->models->data);
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
mex_model_manager_get_default ()
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
  const MexModelInfo *info_a = a;
  const MexModelInfo *info_b = b;

  return info_b->priority - info_a->priority;
}

static gint
mex_model_manager_sort_cb (gconstpointer a,
                           gconstpointer b,
                           gpointer      userdata)
{
  const MexModelInfo *info_a = a;
  const MexModelInfo *info_b = b;
  MexModelManager *manager = userdata;
  MexModelManagerPrivate *priv = manager->priv;
  MexModelCategoryInfo *category_a, *category_b;

  if (info_a->category)
    category_a = g_hash_table_lookup (priv->categories, info_a->category);
  else
    category_a = NULL;

  if (info_b->category)
    category_b = g_hash_table_lookup (priv->categories, info_b->category);
  else
    category_b = NULL;

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

  return info_b->priority - info_a->priority;
}

static gint
mex_model_manager_find_cb (gconstpointer a,
                           gconstpointer b)
{
  const MexModelInfo *info_a = a;
  const MexModel *model = b;

  if (info_a->model == model)
    return 0;
  else
    return -1;
}

static gint
mex_model_manager_find_alt_cb (gconstpointer a,
                               gconstpointer b)
{
  const MexModelInfo *info_a = a;
  const MexModel *model = b;

  if (info_a->alt_model == model)
    return 0;
  else
    return -1;
}

GList *
mex_model_manager_get_models (MexModelManager *manager)
{
  GList *i, *models;
  MexModelManagerPrivate *priv;

  g_return_val_if_fail (MEX_IS_MODEL_MANAGER (manager), NULL);

  priv = manager->priv;

  models = g_list_copy (priv->models);
  for (i = models; i; i = i->next)
    i->data = ((MexModelInfo *)i->data)->model;

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
      MexModelInfo *info = i->data;

      if (!category && info->category)
        continue;

      if ((category == info->category) ||
          (g_strcmp0 (category, info->category) == 0))
        models = g_list_prepend (models, info);
    }

  models = g_list_sort (models, mex_model_manager_simple_sort_cb);
  for (i = models; i; i = i->next)
    i->data = ((MexModelInfo *)i->data)->model;

  return models;
}

void
mex_model_manager_add_model (MexModelManager    *manager,
                             const MexModelInfo *info)
{
  MexModelInfo *info_copy;
  MexModelManagerPrivate *priv;

  g_return_if_fail (MEX_IS_MODEL_MANAGER (manager));

  priv = manager->priv;

  info_copy = mex_model_info_copy (info);

  priv->models = g_list_insert_sorted_with_data (priv->models,
                                                 info_copy,
                                                 mex_model_manager_sort_cb,
                                                 manager);

  g_signal_emit (manager, signals[MODEL_ADDED], 0, info_copy);
}

void
mex_model_manager_remove_model (MexModelManager *manager,
                                MexModel        *model)
{
  GList *i;
  MexModelInfo *info;
  MexModelManagerPrivate *priv;

  g_return_if_fail (MEX_IS_MODEL_MANAGER (manager));

  priv = manager->priv;

  i = g_list_find_custom (priv->models, model, mex_model_manager_find_cb);
  if (!i)
    {
      g_warning (G_STRLOC ": Model is unrecognised");
      return;
    }

  info = i->data;
  priv->models = g_list_delete_link (priv->models, i);

  g_signal_emit (manager, signals[MODEL_REMOVED], 0, model, info->category);

  mex_model_info_free (info);
}

const MexModelInfo *
mex_model_manager_get_model_info (MexModelManager *manager,
                                  MexModel        *model)
{
  GList *link;
  MexModelManagerPrivate *priv;

  g_return_val_if_fail (MEX_IS_MODEL_MANAGER (manager), NULL);
  g_return_val_if_fail (MEX_IS_MODEL (model), NULL);

  priv = manager->priv;

  link = g_list_find_custom (priv->models, model, mex_model_manager_find_cb);
  if (!link)
    link = g_list_find_custom (priv->models, model,
                               mex_model_manager_find_alt_cb);

  return link ? link->data : NULL;
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

MexModelInfo *
mex_model_info_new (MexModel         *model,
                    const gchar      *category,
                    gint              priority,
                    const gchar      *first_sort_func_name,
                    ...)
{
  va_list args;
  GList *sort_infos;
  MexModelInfo *info;

  g_return_val_if_fail (MEX_IS_MODEL (model), NULL);

  info = g_slice_new0 (MexModelInfo);
  info->model = g_object_ref_sink (model);
  info->category = g_strdup (category);
  info->priority = priority;

  sort_infos = NULL;
  if (first_sort_func_name)
    {
      const gchar *sort_name = first_sort_func_name;

      va_start (args, first_sort_func_name);
      while (sort_name)
        {
          MexModelSortFuncInfo *sort_info = g_slice_new0 (MexModelSortFuncInfo);
          sort_info->name = g_strdup (sort_name);
          sort_info->display_name = g_strdup (va_arg (args, gchar *));
          sort_info->sort_func = va_arg (args, MexModelSortFunc);
          sort_info->userdata = va_arg (args, gpointer);

          sort_infos = g_list_prepend (sort_infos, sort_info);

          sort_name = va_arg (args, gchar *);
        }
      va_end (args);

      sort_infos = g_list_reverse (sort_infos);
    }

  info->sort_infos = sort_infos;

  return info;
}

MexModelInfo *
mex_model_info_copy (const MexModelInfo *info)
{
  GList *i;
  MexModelInfo *info_copy;

  g_return_val_if_fail (info != NULL, NULL);

  info_copy = g_slice_dup (MexModelInfo, info);
  info_copy->model = g_object_ref_sink (info->model);
  if (info->alt_model)
    info_copy->alt_model = g_object_ref_sink (info->alt_model);
  info_copy->category = g_strdup (info->category);
  info_copy->sort_infos = g_list_copy (info->sort_infos);

  for (i = info_copy->sort_infos; i; i = i->next)
    {
      MexModelSortFuncInfo *sort_info;

      i->data = g_slice_dup (MexModelSortFuncInfo, i->data);
      sort_info = i->data;
      sort_info->name = g_strdup (sort_info->name);
      sort_info->display_name = g_strdup (sort_info->display_name);
    }

  return info_copy;
}

