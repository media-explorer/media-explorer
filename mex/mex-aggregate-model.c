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


#include "mex-aggregate-model.h"
#include "mex-model-manager.h"

G_DEFINE_TYPE (MexAggregateModel, mex_aggregate_model, MEX_TYPE_GENERIC_MODEL)

#define AGGREGATE_MODEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_AGGREGATE_MODEL, MexAggregateModelPrivate))

enum
{
  MODEL_ADDED,
  MODEL_REMOVED,

  LAST_SIGNAL
};

struct _MexAggregateModelPrivate
{
  GList *models;

  GHashTable *controller_to_model;
  GHashTable *content_to_model;
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
mex_aggregate_model_dispose (GObject *object)
{
  MexAggregateModel *self = MEX_AGGREGATE_MODEL (object);
  MexAggregateModelPrivate *priv = self->priv;

  while (priv->models)
    {
      MexModel *model = priv->models->data;
      mex_aggregate_model_remove_model (self, model);
    }

  if (priv->controller_to_model)
    {
      g_hash_table_unref (priv->controller_to_model);
      priv->controller_to_model = NULL;
    }

  if (priv->content_to_model)
    {
      g_hash_table_unref (priv->content_to_model);
      priv->content_to_model = NULL;
    }

  G_OBJECT_CLASS (mex_aggregate_model_parent_class)->dispose (object);
}

static void
mex_aggregate_model_class_init (MexAggregateModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexAggregateModelPrivate));

  object_class->dispose = mex_aggregate_model_dispose;

  signals[MODEL_ADDED] =
    g_signal_new ("model-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexAggregateModelClass, model_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, G_TYPE_OBJECT);

  signals[MODEL_REMOVED] =
    g_signal_new ("model-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexAggregateModelClass, model_removed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, G_TYPE_OBJECT);
}

static void
mex_aggregate_model_init (MexAggregateModel *self)
{
  MexAggregateModelPrivate *priv = self->priv = AGGREGATE_MODEL_PRIVATE (self);

  priv->controller_to_model = g_hash_table_new (NULL, NULL);
  priv->content_to_model = g_hash_table_new (NULL, NULL);
}

MexModel *
mex_aggregate_model_new (void)
{
  return g_object_new (MEX_TYPE_AGGREGATE_MODEL, NULL);
}

static void
mex_aggregate_model_clear_model (MexAggregateModel *self,
                                 MexModel          *model)
{
  gint i;
  GList *c, *to_remove;
  MexContent *content;

  MexAggregateModelPrivate *priv = self->priv;

  i = 0;
  to_remove = NULL;
  while ((content = mex_model_get_content (MEX_MODEL (self), i++)))
    {
      MexModel *parent = g_hash_table_lookup (priv->content_to_model, content);

      if (parent == model)
        {
          g_hash_table_remove (priv->content_to_model, content);
          to_remove = g_list_prepend (to_remove, content);
        }
    }

  /* Remove the contents after the loop, to avoid modifying what we're
   * iterating over.
   */
  for (c = to_remove; c; c = c->next)
    mex_model_remove_content (MEX_MODEL (self), MEX_CONTENT (c->data));

  g_list_free (to_remove);
}

static void
mex_aggregate_model_controller_changed_cb (GController          *controller,
                                           GControllerAction     action,
                                           GControllerReference *ref,
                                           MexAggregateModel    *self)
{
  gint i;

  gint n_indices = 0;
  MexAggregateModelPrivate *priv = self->priv;
  MexModel *model = g_hash_table_lookup (priv->controller_to_model, controller);

  if (!model)
    {
      g_warning (G_STRLOC ": Signal from unknown controller");
      return;
    }

  if (ref)
    n_indices = g_controller_reference_get_n_indices (ref);

  switch (action)
    {
    case G_CONTROLLER_ADD:
      for (i = 0; i < n_indices; i++)
        {
          MexContent *content;

          gint content_index = g_controller_reference_get_index_uint (ref, i);
          content = mex_model_get_content (model, content_index);
          g_hash_table_insert (priv->content_to_model, content, model);

          mex_model_add_content (MEX_MODEL (self), content);
        }
      break;

    case G_CONTROLLER_REMOVE:
      for (i = 0; i < n_indices; i++)
        {
          MexContent *content;

          gint content_index = g_controller_reference_get_index_uint (ref, i);

          content = mex_model_get_content (model, content_index);
          g_hash_table_remove (priv->content_to_model, content);

          mex_model_remove_content (MEX_MODEL (self), content);
        }
      break;

    case G_CONTROLLER_UPDATE:
      break;

    case G_CONTROLLER_CLEAR:
      mex_aggregate_model_clear_model (self, model);
      break;

    case G_CONTROLLER_REPLACE:
      {
        MexContent *content;

        mex_aggregate_model_clear_model (self, model);
        i = 0;
        while ((content = mex_model_get_content (model, i++)))
          {
            g_hash_table_insert (priv->content_to_model, content, model);
            mex_model_add_content (MEX_MODEL (self), content);
          }
      }
      break;

    case G_CONTROLLER_INVALID_ACTION:
      g_warning (G_STRLOC ": Proxy controller has issued an error");
      break;

    default:
      break;
    }
}

static gint
mex_aggregate_model_sort_func (MexModel *a,
                               MexModel *b)
{
  MexModelManager *manager;
  const MexModelInfo *info_a, *info_b;

  manager = mex_model_manager_get_default ();

  info_a = mex_model_manager_get_model_info (manager, a);
  info_b = mex_model_manager_get_model_info (manager, b);

  if (info_a && info_b)
    return info_a->priority - info_b->priority;
  else
    return 0;
}

void
mex_aggregate_model_add_model (MexAggregateModel *aggregate,
                               MexModel          *model)
{
  gint i;
  MexContent *content;
  GController *controller;
  MexAggregateModelPrivate *priv;

  g_return_if_fail (MEX_IS_AGGREGATE_MODEL (aggregate));
  g_return_if_fail (MEX_IS_MODEL (model));

  priv = aggregate->priv;
  if (g_list_find (priv->models, model))
    return;

  /* Add a link back to the model from the controller */
  controller = mex_model_get_controller (model);
  g_hash_table_insert (priv->controller_to_model, controller,
                       g_object_ref_sink (G_OBJECT (model)));

  /* Add model to list */
  priv->models = g_list_insert_sorted (priv->models, model,
                                       (GCompareFunc) mex_aggregate_model_sort_func);

  /* Add existing items */
  i = 0;
  while ((content = mex_model_get_content (model, i)))
    {
      g_hash_table_insert (priv->content_to_model, content, model);
      mex_model_add_content (MEX_MODEL (aggregate), content);
      i++;
    }

  /* Connect to the controller changed signal */
  g_signal_connect (controller, "changed",
                    G_CALLBACK (mex_aggregate_model_controller_changed_cb),
                    aggregate);

  /* Emit added signal */
  g_signal_emit (aggregate, signals[MODEL_ADDED], 0, model);
}

void
mex_aggregate_model_remove_model (MexAggregateModel *aggregate,
                                  MexModel          *model)
{
  GList *model_link;
  GController *controller;
  MexAggregateModelPrivate *priv;

  g_return_if_fail (MEX_IS_AGGREGATE_MODEL (aggregate));
  g_return_if_fail (MEX_IS_MODEL (model));

  priv = aggregate->priv;
  if (!(model_link = g_list_find (priv->models, model)))
    return;

  controller = mex_model_get_controller (model);

  /* Remove items */
  mex_aggregate_model_clear_model (aggregate, model);

  /* Disconnect from signal */
  g_signal_handlers_disconnect_by_func (controller,
                                      mex_aggregate_model_controller_changed_cb,
                                      aggregate);

  /* Remove model from list and remove custom data */
  g_hash_table_remove (priv->controller_to_model, controller);
  priv->models = g_list_delete_link (priv->models, model_link);

  /* Emit removed signal */
  g_signal_emit (aggregate, signals[MODEL_REMOVED], 0, model);

  /* Unref model */
  g_object_unref (model);
}

void
mex_aggregate_model_clear (MexAggregateModel *aggregate)
{
  MexAggregateModelPrivate *priv;

  g_return_if_fail (MEX_IS_AGGREGATE_MODEL (aggregate));

  priv = aggregate->priv;
  while (priv->models)
    mex_aggregate_model_remove_model (aggregate, priv->models->data);
}

const GList *
mex_aggregate_model_get_models (MexAggregateModel *aggregate)
{
  g_return_val_if_fail (MEX_IS_AGGREGATE_MODEL (aggregate), NULL);
  return aggregate->priv->models;
}

MexModel *
mex_aggregate_model_get_model_for_content (MexAggregateModel *aggregate,
                                           MexContent        *content)
{
  g_return_val_if_fail (MEX_IS_CONTENT (content), NULL);
  return (MexModel *)g_hash_table_lookup (aggregate->priv->content_to_model,
                                          content);

}
