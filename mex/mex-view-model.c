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

#include "mex-view-model.h"

#include "mex-generic-content.h"
#include "mex-model-manager.h"
#include "mex-program.h"
#include "mex-group-item.h"

#include <stdarg.h>
#include <string.h>


static void mex_model_iface_init (MexModelIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexViewModel, mex_view_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL,
                                                mex_model_iface_init))

#define VIEW_MODEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_VIEW_MODEL, MexViewModelPrivate))

#define VIEW_MODEL_LIMIT(priv)                          \
  ((priv)->limit != 0 ? (priv)->limit : G_MAXUINT)

enum
{
  PROP_0,

  PROP_MODEL,
  PROP_OFFSET,
  PROP_LIMIT,
  PROP_TITLE,
  PROP_SORT_FUNC,
  PROP_SORT_DATA,
  PROP_ICON_NAME,
  PROP_LENGTH,
  PROP_CATEGORY,
  PROP_PRIORITY,
  PROP_SORT_FUNCTIONS,
  PROP_ALT_MODEL,
  PROP_ALT_MODEL_STRING,
  PROP_ALT_MODEL_ACTIVE,
  PROP_SKIP_UNGROUPED_ITEMS
};

typedef struct
{
  MexContentMetadata key;
  MexFilterCondition condition;
  gchar *value;
} FilterKeyValue;

struct _MexViewModelPrivate
{
  MexModel *model;

  MexContent *start_content;
  guint       limit;
  guint       offset;

  guint looped  : 1;
  guint skip_ungrouped_items : 1;

  GPtrArray *external_items;
  GPtrArray *internal_items;

  GList *filter_by;

  MexContentMetadata order_by_key;
  gboolean           order_by_descending;

  MexContentMetadata group_by_key;
  GHashTable *group_items;

  GController *controller;

  gchar *title;
};

static void mex_view_model_controller_changed_cb (GController          *controller,
                                                  GControllerAction     action,
                                                  GControllerReference *ref,
                                                  MexViewModel         *self);

static void mex_view_model_refresh_external_items (MexViewModel *model);
static void content_notify_cb (GObject *content, GParamSpec *pspec, MexViewModel *view);

static void
mex_view_model_set_model (MexViewModel *self,
                          MexModel     *model)
{
  MexViewModelPrivate *priv = self->priv;

  if (model == priv->model)
    return;

  if (priv->start_content)
    {
      g_object_unref (priv->start_content);
      priv->start_content = NULL;
    }

  if (model)
    {
      MexContent *content;
      GController *controller;
      gint i = 0;

      priv->model = g_object_ref_sink (model);

      if (priv->model)
        g_object_get (priv->model, "title", &priv->title, NULL);

      controller = mex_model_get_controller (model);
      g_signal_connect (controller, "changed",
                        G_CALLBACK (mex_view_model_controller_changed_cb),
                        self);

      /* copy initial items across */
      g_ptr_array_set_size (priv->internal_items, 0);

      while ((content = mex_model_get_content (priv->model, i++)))
        {
          g_ptr_array_add (priv->internal_items, g_object_ref (content));

          g_signal_connect (content, "notify", G_CALLBACK (content_notify_cb),
                            self);

          /* any MexProgram objects must have all their data resolved to be
           * useful in the view model */
          if (MEX_IS_PROGRAM (content))
            _mex_program_complete (MEX_PROGRAM (content));
        }
    }

  if (priv->group_items)
    g_hash_table_remove_all (priv->group_items);
  mex_view_model_refresh_external_items (self);
}

static void
mex_view_model_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MexViewModel *self = MEX_VIEW_MODEL (object);
  MexViewModelPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    case PROP_OFFSET:
      g_value_set_uint (value, priv->offset);
      break;

    case PROP_LIMIT:
      g_value_set_uint (value, priv->limit);
      break;

    case PROP_LENGTH:
      g_value_set_uint (value, priv->external_items->len);
      break;

    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_SKIP_UNGROUPED_ITEMS:
      g_value_set_boolean (value, priv->skip_ungrouped_items);
      break;

    case PROP_SORT_FUNC:
    case PROP_SORT_DATA:
    case PROP_ICON_NAME:
    case PROP_CATEGORY:
    case PROP_PRIORITY:
    case PROP_SORT_FUNCTIONS:
    case PROP_ALT_MODEL:
    case PROP_ALT_MODEL_STRING:
    case PROP_ALT_MODEL_ACTIVE:
      g_object_get_property (G_OBJECT (self->priv->model), pspec->name, value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_view_model_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MexViewModel *self = MEX_VIEW_MODEL (object);

  switch (property_id)
    {
    case PROP_MODEL:
      mex_view_model_set_model (self, g_value_get_object (value));
      break;

    case PROP_LIMIT:
      mex_view_model_set_limit (self, g_value_get_uint (value));
      break;

    case PROP_TITLE:
      g_free (self->priv->title);
      self->priv->title = g_value_dup_string (value);
      break;

    case PROP_SKIP_UNGROUPED_ITEMS:
      self->priv->skip_ungrouped_items = g_value_get_boolean (value);
      mex_view_model_refresh_external_items (self);
      break;

    case PROP_SORT_FUNC:
    case PROP_SORT_DATA:
    case PROP_ICON_NAME:
    case PROP_CATEGORY:
    case PROP_PRIORITY:
    case PROP_SORT_FUNCTIONS:
    case PROP_ALT_MODEL:
    case PROP_ALT_MODEL_STRING:
    case PROP_ALT_MODEL_ACTIVE:
      g_object_set_property (G_OBJECT (self->priv->model), pspec->name, value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_view_model_dispose (GObject *object)
{
  MexViewModelPrivate *priv = VIEW_MODEL_PRIVATE (object);

  if (priv->model)
    {
      g_signal_handlers_disconnect_by_func (mex_model_get_controller (priv->model),
                                            mex_view_model_controller_changed_cb,
                                            object);

      g_object_unref (priv->model);

      priv->model = NULL;
    }

  if (priv->start_content)
    {
      g_object_unref (priv->start_content);
      priv->start_content = NULL;
    }

  if (priv->controller)
    {
      g_object_unref (priv->controller);
      priv->controller = NULL;
    }

  G_OBJECT_CLASS (mex_view_model_parent_class)->dispose (object);
}

static void
mex_view_model_finalize (GObject *object)
{
  MexViewModelPrivate *priv = MEX_VIEW_MODEL (object)->priv;
  gint i;

  if (priv->external_items)
    {
      g_ptr_array_free (priv->external_items, TRUE);
      priv->external_items = NULL;
    }

  if (priv->internal_items)
    {
      for (i = 0; i < priv->internal_items->len; i++)
        {
          GObject *o;

          o = g_ptr_array_index (priv->internal_items, i);

          g_signal_handlers_disconnect_by_func (o,
                                                G_CALLBACK (content_notify_cb),
                                                object);
        }
      g_ptr_array_free (priv->internal_items, TRUE);
      priv->external_items = NULL;

      if (priv->start_content)
        g_object_unref (priv->start_content);
      priv->start_content = NULL;
    }

  if (priv->group_items)
    {
      g_hash_table_destroy (priv->group_items);
      priv->group_items = NULL;
    }

  g_free (priv->title);
  priv->title = NULL;

  /* clear the filter list */
  mex_view_model_set_filter_by (MEX_VIEW_MODEL (object),
                                MEX_CONTENT_METADATA_NONE, 0, NULL);

  G_OBJECT_CLASS (mex_view_model_parent_class)->finalize (object);
}

static MexModel *
mex_view_model_get_model (MexModel *model)
{
  MexViewModelPrivate *priv = VIEW_MODEL_PRIVATE (model);

  return priv->model;
}

static MexContent *
mex_view_model_get_content (MexModel *model, guint idx)
{
  MexViewModelPrivate *priv = MEX_VIEW_MODEL (model)->priv;
  gint start = 0;

  if (idx >= priv->external_items->len)
    return NULL;

  if (idx > priv->limit - 1)
    return NULL;

  if (priv->start_content)
    {
      gboolean found = FALSE;

      for (; start < priv->external_items->len; start++)
        if (g_ptr_array_index (priv->external_items, start) == priv->start_content)
          {
            found = TRUE;
            break;
          }

      /* start at was not found */
      if (!found)
        {
          g_critical (G_STRLOC ": start_at content is invalid in MexModelView");
          return NULL;
        }
    }

  if (start + idx >= priv->external_items->len)
    start = start - priv->external_items->len;

  return g_ptr_array_index (priv->external_items, start + idx);
}

static GController *
mex_view_model_get_controller (MexModel *model)
{
  return MEX_VIEW_MODEL (model)->priv->controller;
}

static guint
mex_view_model_get_length (MexModel *model)
{
  MexViewModelPrivate *priv = MEX_VIEW_MODEL (model)->priv;

  if (priv->limit)
    return MIN (priv->limit, priv->external_items->len);
  else
    return priv->external_items->len;
}

static gint
mex_view_model_index (MexModel *model, MexContent *content)
{
  MexViewModelPrivate *priv = MEX_VIEW_MODEL (model)->priv;
  gint start = 0, idx = 0, position;
  gboolean found;

  if (!content)
    return -1;

  /* find the start content's index */
  if (priv->start_content)
    {
      found = FALSE;

      for (; start < priv->external_items->len; start++)
        if (g_ptr_array_index (priv->external_items, start) == priv->start_content)
          {
            found = TRUE;
            break;
          }

      if (!found)
        {
          g_critical (G_STRLOC ": start_at content is invalid in MexModelView");
          return -1;
        }
    }

  /* find the search item's index */
  found = FALSE;
  idx = position = start;
  while (idx < priv->external_items->len)
    {
      if (g_ptr_array_index (priv->external_items, idx) == content)
        {
          found = TRUE;
          break;
        }

      /* check if search has looped */
      if (priv->looped && idx + 1 == start)
        break;

      /* loop if required */
      if (priv->looped && idx + 1 == priv->external_items->len)
        idx = -1;

      idx++;

      position++;
    }

  if (!found)
    return -1;

  return position;
}

static void
mex_model_iface_init (MexModelIface *iface)
{
  /* view model is read only, so these functions do nothing*/
  iface->add_content = NULL;
  iface->get_model = NULL;
  iface->set_sort_func = NULL;
  iface->clear = NULL;

  iface->get_content = mex_view_model_get_content;
  iface->get_controller = mex_view_model_get_controller;
  iface->get_length = mex_view_model_get_length;
  iface->index = mex_view_model_index;

  /* TODO: this should not be part of the MexModel interface */
  iface->get_model = mex_view_model_get_model;
}

static void
mex_view_model_class_init (MexViewModelClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexViewModelPrivate));

  object_class->get_property = mex_view_model_get_property;
  object_class->set_property = mex_view_model_set_property;
  object_class->dispose = mex_view_model_dispose;
  object_class->finalize = mex_view_model_finalize;

  pspec = g_param_spec_object ("model",
                               "Model",
                               "MexModel the view-model is listening to.",
                               G_TYPE_OBJECT,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MODEL, pspec);

  pspec = g_param_spec_uint ("limit",
                             "Limit",
                             "Limit of the number of contents to add.",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_LIMIT, pspec);

  pspec = g_param_spec_uint ("offset",
                             "Offset",
                             "Offset from which contents are added",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_LIMIT, pspec);

  pspec = g_param_spec_boolean ("skip-ungrouped-items",
                                "Skip Ungrouped items",
                                "Skip items that cannot be grouped",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SKIP_UNGROUPED_ITEMS, pspec);

  g_object_class_override_property (object_class, PROP_TITLE, "title");
  g_object_class_override_property (object_class, PROP_SORT_FUNC, "sort-function");
  g_object_class_override_property (object_class, PROP_SORT_DATA, "sort-data");
  g_object_class_override_property (object_class, PROP_ICON_NAME, "icon-name");
  g_object_class_override_property (object_class, PROP_LENGTH, "length");

  g_object_class_override_property (object_class, PROP_CATEGORY, "category");
  g_object_class_override_property (object_class, PROP_PRIORITY, "priority");
  g_object_class_override_property (object_class, PROP_SORT_FUNCTIONS,
                                    "sort-functions");
  g_object_class_override_property (object_class, PROP_ALT_MODEL, "alt-model");
  g_object_class_override_property (object_class, PROP_ALT_MODEL_STRING,
                                    "alt-model-string");
  g_object_class_override_property (object_class, PROP_ALT_MODEL_ACTIVE,
                                    "alt-model-active");
}

static void
mex_view_model_init (MexViewModel *self)
{
  MexViewModelPrivate *priv;

  priv = VIEW_MODEL_PRIVATE (self);
  self->priv = priv;

  priv->external_items = g_ptr_array_new_with_free_func (g_object_unref);
  priv->internal_items = g_ptr_array_new_with_free_func (g_object_unref);

  priv->controller = g_ptr_array_controller_new (priv->external_items);

  priv->order_by_key = MEX_CONTENT_METADATA_TITLE;
}

MexModel *
mex_view_model_new (MexModel *model)
{
  return g_object_new (MEX_TYPE_VIEW_MODEL, "model", model, NULL);
}

typedef struct
{
  MexContentMetadata key;
  gboolean descending;
} SortFuncInfo;

static gint
order_by_func (gconstpointer a,
               gconstpointer b,
               gpointer      user_data)
{
  MexContent **content_a = (MexContent **) a;
  MexContent **content_b = (MexContent **) b;
  SortFuncInfo *info = user_data;
  const gchar *value_a, *value_b;

  value_a = mex_content_get_metadata (*content_a, info->key);
  value_b = mex_content_get_metadata (*content_b, info->key);

  if (info->descending)
    return g_strcmp0 (value_b, value_a);
  else
    return g_strcmp0 (value_a, value_b);

}

static gint
_g_ptr_array_contains (GPtrArray *haystack, gpointer needle)
{
  gint i;

  for (i = 0; i < haystack->len; i++)
    {
      if (haystack->pdata[i] == needle)
        return i;
    }

  return -1;
}

static gint
_g_ptr_array_add_sorted_with_data (GPtrArray *array,
                                   gpointer item,
                                   GCompareDataFunc compare_func,
                                   gpointer user_data)
{
  gint i;

  /* find the position to insert the item */
  for (i = 0; i < array->len; i++)
    {
      if (compare_func (&item, &array->pdata[i], user_data) < 0)
        break;
    }

  /* increase the size of the array */
  g_ptr_array_set_size (array, array->len + 1);

  /* move all the items after the insertion position if the new position is not
   * at the end of the array */
  if (i < array->len - 1)
    g_memmove (&array->pdata[i + 1], &array->pdata[i],
               (array->len - i - 1) * sizeof (gpointer));

  /* add the new item to the array */
  array->pdata[i] = item;

  return i;
}

static void
mex_view_model_refresh_external_items (MexViewModel *model)
{
  MexViewModelPrivate *priv = model->priv;
  gint i;
  GHashTable *new_items, *external_items;
  GHashTableIter iter;
  GHashTable *groups = NULL;
  GControllerReference *ref;
  gpointer key;
  SortFuncInfo info = { priv->order_by_key, priv->order_by_descending };

  /* allocate the full array to start with */
  new_items = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);

  if (priv->group_by_key)
    {
      groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

      if (!priv->group_items)
        priv->group_items = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                   g_free, NULL);
    }

  /* add the items to the new list */
  for (i = 0; i < priv->internal_items->len; i++)
    {
      MexContent *content, *group_item;

      content = g_ptr_array_index (priv->internal_items, i);

      /* check the item matches the filter */
      if (priv->filter_by)
        {
          GList *list;
          const gchar *v;
          FilterKeyValue *filter;
          gboolean skip;

          for (list = priv->filter_by; list; list = g_list_next (list))
            {
              filter = list->data;

              v = mex_content_get_metadata (content, filter->key);

              /* skip this item if it does not match the filter */

              skip = g_strcmp0 (v, filter->value);

              if (filter->condition == MEX_FILTER_NOT)
                skip = (skip == 0);

              if (skip)
                {
                  break;
                }
            }

          if (skip)
            continue;
        }

      /* create a group item if necessary */
      if (priv->group_by_key)
        {
          const gchar *g;
          gchar *strlower;
          gchar *category = NULL;
          const MexModelCategoryInfo *c_info;

          g_object_get (G_OBJECT (model), "category", &category, NULL);

          c_info = mex_model_manager_get_category_info (mex_model_manager_get_default (),
                                                        category);
          g_free (category);


          g = mex_content_get_metadata (content, priv->group_by_key);

          if (!g && priv->skip_ungrouped_items)
            continue;

          if (g)
            {
              /* build up the group list */
              strlower = g_utf8_strdown (g, -1);
              if (!g_hash_table_lookup (groups, strlower))
                {
                  group_item = g_hash_table_lookup (priv->group_items, strlower);

                  if (!group_item)
                    {
                      const gchar *prop_name;
                      FilterKeyValue *filter2;
                      gint group_key;

                      if (priv->filter_by
                          && ((FilterKeyValue*) priv->filter_by->data)->condition != MEX_FILTER_NOT)
                        filter2 = priv->filter_by->data;
                      else
                        filter2 = NULL;


                      if (c_info->primary_group_by_key == priv->group_by_key)
                        group_key = c_info->secondary_group_by_key;
                      else
                        group_key = 0;

                      group_item =
                        (MexContent*) mex_group_item_new (g,
                                                          priv->model,
                                                          /* filter key, value */
                                                          priv->group_by_key, g,
                                                          /* second filter key, value*/
                                                          (filter2) ? filter2->key : 0,
                                                          (filter2) ? filter2->value : NULL,
                                                          /* group key */
                                                          group_key);

                      prop_name = mex_content_get_property_name (MEX_CONTENT (content),
                                                                 MEX_CONTENT_METADATA_STILL);
                      g_object_bind_property (content, prop_name, group_item, prop_name,
                                              G_BINDING_SYNC_CREATE);

                      prop_name = mex_content_get_property_name (MEX_CONTENT (content),
                                                                 MEX_CONTENT_METADATA_ALBUM);
                      g_object_bind_property (content, prop_name, group_item, prop_name,
                                              G_BINDING_SYNC_CREATE);

                      prop_name = mex_content_get_property_name (MEX_CONTENT (content),
                                                                 MEX_CONTENT_METADATA_ARTIST);
                      g_object_bind_property (content, prop_name, group_item, prop_name,
                                              G_BINDING_SYNC_CREATE);


                      /* keep a list of the groups created during this refresh */
                      g_hash_table_insert (groups, strlower, group_item);

                      /* add this item to the group items cache */
                      g_hash_table_insert (priv->group_items,
                                           g_strdup (strlower), group_item);

                      g_object_ref_sink (group_item);
                    }
                  content = group_item;
                }
              else
                {
                  g_free (strlower);
                  continue;
                }
            }
        }

      /* add the item to the list */
      g_hash_table_insert (new_items, g_object_ref (content), GINT_TO_POINTER (1));
    }

  if (groups)
    {
      g_hash_table_destroy (groups);
      groups = NULL;
    }


  /* compare new_items and external_items */

  /* add external_items to a hash table to improve lookup performance */
  external_items = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);
  for (i = 0; i < priv->external_items->len; i++)
    g_hash_table_insert (external_items, g_object_ref (priv->external_items->pdata[i]), GINT_TO_POINTER (1));

  /* Remove items first, so that the items added later can be added at the
   * correct positions with respect to any limit value */
  /* find items to remove from external_items */
  g_hash_table_iter_init (&iter, external_items);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      if (!g_hash_table_lookup (new_items, key))
        {
          gboolean new_item_visible;

          i = _g_ptr_array_contains (priv->external_items, key);

          /* emit the removed signal */
          if (priv->limit == 0 || i < priv->limit)
            {
              ref = g_controller_create_reference (priv->controller,
                                                   G_CONTROLLER_REMOVE,
                                                   G_TYPE_UINT, 1, i);
              g_controller_emit_changed (priv->controller, ref);
            }


          /* if the item to be removed is below the model's limit and the length
           * of the items is greater than the limit, then a new item should
           * become once the old item is removed */
          if (i < priv->limit && priv->external_items->len > priv->limit)
            new_item_visible = TRUE;
          else
            new_item_visible = FALSE;

          /* remove the item */
          g_ptr_array_remove_index (priv->external_items, i);

          if (new_item_visible)
            {
              /* emit the added signal for the item that is now visible */
              ref = g_controller_create_reference (priv->controller,
                                                   G_CONTROLLER_ADD,
                                                   G_TYPE_UINT, 1,
                                                   priv->limit - 1);
              g_controller_emit_changed (priv->controller, ref);
            }

          i--;
        }
    }



  /* find items to add to external_items */
  g_hash_table_iter_init (&iter, new_items);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      if (!g_hash_table_lookup (external_items, key))
        {
          gint position;

          /* add the item */
          if (priv->order_by_key)
            {
              position = _g_ptr_array_add_sorted_with_data (priv->external_items,
                                                            g_object_ref (key),
                                                            order_by_func, &info);
            }
          else
            {
              g_ptr_array_add (priv->external_items,
                               g_object_ref (key));

              position = priv->external_items->len - 1;
            }

          /* emit the added signal, if there is no limit or the new index is
           * less than the limit */
          if (!priv->limit || position < priv->limit)
            {
              ref = g_controller_create_reference (priv->controller,
                                                   G_CONTROLLER_ADD,
                                                   G_TYPE_UINT, 1,
                                                   position);
              g_controller_emit_changed (priv->controller, ref);

              if (priv->limit && position < priv->limit
                  && priv->external_items->len > priv->limit)
                {
                  /* remove the item that is no longer visible */
                  ref = g_controller_create_reference (priv->controller,
                                                       G_CONTROLLER_REMOVE,
                                                       G_TYPE_UINT, 1,
                                                       priv->limit);
                  g_controller_emit_changed (priv->controller, ref);
                }
            }
        }
    }

  /* destroy the new_items list */
  g_hash_table_destroy (new_items);

  /* destroy the external_items hash table */
  g_hash_table_destroy (external_items);
}

static void
content_notify_cb (GObject      *content,
                   GParamSpec   *pspec,
                   MexViewModel *view)
{
  MexViewModelPrivate *priv = view->priv;
  const gchar *group_key;
  const gchar *order_by_key;
  GList *list;

  group_key = mex_content_metadata_key_to_string (priv->group_by_key);
  order_by_key = mex_content_metadata_key_to_string (priv->order_by_key);

  /* refresh the external list when one of the keys has changed */
  if (g_str_equal (pspec->name, group_key)
      || g_str_equal (pspec->name, order_by_key))
    {
      /* TODO: update only the items which have changed */
      mex_view_model_refresh_external_items (view);

      return;
    }

  /* check for one of the filter keys */
  for (list = priv->filter_by; list; list = g_list_next (list))
    {
      FilterKeyValue *filter = list->data;

      if (g_str_equal (pspec->name,
                       mex_content_metadata_key_to_string (filter->key)))
        {
          mex_view_model_refresh_external_items (view);

          return;
        }
    }
}

static void
mex_view_model_controller_changed_cb (GController          *controller,
                                      GControllerAction     action,
                                      GControllerReference *ref,
                                      MexViewModel         *self)
{
  gint n_indices;

  MexViewModelPrivate *priv = self->priv;

  n_indices = g_controller_reference_get_n_indices (ref);

  switch (action)
    {
    case G_CONTROLLER_ADD:
      {
        /* add the new items */
        while (n_indices-- > 0)
          {
            MexContent *content;
            guint idx;

            idx = g_controller_reference_get_index_uint (ref, n_indices);

            content = mex_model_get_content (priv->model, idx);

            g_signal_connect (content, "notify", G_CALLBACK (content_notify_cb),
                              self);

            g_ptr_array_add (priv->internal_items, g_object_ref (content));
          }
      }
      break;

    case G_CONTROLLER_REMOVE:
      {
        while (n_indices-- > 0)
          {
            MexContent *content;
            gint idx;

            idx = g_controller_reference_get_index_uint (ref, n_indices);

            content = mex_model_get_content (priv->model, idx);

            g_signal_handlers_disconnect_by_func (content,
                                                  G_CALLBACK (content_notify_cb),
                                                  self);

            g_ptr_array_remove_fast (priv->internal_items, content);

            if (priv->start_content == content)
              {
                g_object_unref (priv->start_content);
                priv->start_content = NULL;
              }
          }
      }
      break;

    case G_CONTROLLER_UPDATE:
      /* Should be no need for this, GBinding sorts it out for us :) */
      break;

    case G_CONTROLLER_CLEAR:
      while (priv->internal_items->len > 0)
        {
          GObject *o;

          o = g_ptr_array_index (priv->internal_items, 0);

          g_signal_handlers_disconnect_by_func (o,
                                                G_CALLBACK (content_notify_cb),
                                                self);
          g_ptr_array_remove_index_fast (priv->internal_items, 0);
        }
      if (priv->start_content)
        g_object_unref (priv->start_content);
      priv->start_content = NULL;
      break;

    case G_CONTROLLER_REPLACE:
      g_warning (G_STRLOC ": G_CONTROLLER_REPLACE not implemented by MexViewModel");
      break;

    case G_CONTROLLER_INVALID_ACTION:
      g_warning (G_STRLOC ": View-model controller has issued an error");
      break;

    default:
      g_warning (G_STRLOC ": Unhandled action");
      break;
    }

  mex_view_model_refresh_external_items (self);
}

void
mex_view_model_set_limit (MexViewModel *self, guint limit)
{
  MexViewModelPrivate *priv;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));

  priv = self->priv;

  if (limit == priv->limit)
    return;

  priv->limit = limit;

  mex_view_model_refresh_external_items (self);
}

void
mex_view_model_set_start_content (MexViewModel *self, MexContent *content)
{
  MexViewModelPrivate *priv;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));
  g_return_if_fail (!content || MEX_IS_CONTENT (content));

  priv = self->priv;

  if (priv->start_content)
    {
      g_object_unref (priv->start_content);
      priv->start_content = NULL;
    }

  if (content)
    priv->start_content = g_object_ref (content);
  else
    priv->start_content = NULL;

  mex_view_model_refresh_external_items (self);
}

void
mex_view_model_set_loop (MexViewModel *self,
                         gboolean      loop)
{
  g_return_if_fail (MEX_IS_VIEW_MODEL (self));

  if (self->priv->looped != loop)
    {
      self->priv->looped = loop;

      mex_view_model_refresh_external_items (self);
    }
}

void
mex_view_model_set_filter_by (MexViewModel       *model,
                              MexContentMetadata  metadata_key,
                              MexFilterCondition  filter_condition,
                              const gchar        *value,
                              ...)
{
  va_list args;

  MexViewModelPrivate *priv = MEX_VIEW_MODEL (model)->priv;
  FilterKeyValue *filter;
  GList *list;

  g_return_if_fail (MEX_IS_VIEW_MODEL (model));

  /* clear the old filter by list */
  for (list = priv->filter_by; list; list = g_list_next (list))
    {
      filter = list->data;

      g_free (filter->value);

      g_slice_free (FilterKeyValue, filter);
    }
  g_list_free (priv->filter_by);
  priv->filter_by = NULL;

  /* start the new filter by list */

  if (metadata_key == MEX_CONTENT_METADATA_NONE)
    return;

  filter = g_slice_new (FilterKeyValue);
  filter->key = metadata_key;
  filter->condition = filter_condition;
  filter->value = g_strdup (value);

  priv->filter_by = g_list_prepend (priv->filter_by, filter);

  /* add any extra filters to the list */

  va_start (args, value);

  while (TRUE)
    {
      filter = g_slice_new (FilterKeyValue);

      filter->key = va_arg (args, int);
      if (filter->key == MEX_CONTENT_METADATA_NONE)
        {
          g_slice_free (FilterKeyValue, filter);
          filter = NULL;
          break;
        }

      filter->condition = va_arg (args, MexFilterCondition);

      filter->value = g_strdup (va_arg (args, char*));

      priv->filter_by = g_list_prepend (priv->filter_by, filter);
    }

  va_end (args);


  /* refresh the external items */
  if (priv->group_items)
    g_hash_table_remove_all (priv->group_items);
  mex_view_model_refresh_external_items (model);
}

void
mex_view_model_set_group_by (MexViewModel       *model,
                             MexContentMetadata  metadata_key)
{
  MexViewModelPrivate *priv;

  g_return_if_fail (MEX_IS_VIEW_MODEL (model));

  priv = MEX_VIEW_MODEL (model)->priv;



  if (priv->group_by_key == metadata_key)
    return;

  priv->group_by_key = metadata_key;

  if (priv->group_items)
    g_hash_table_remove_all (priv->group_items);
  mex_view_model_refresh_external_items (model);
}

void
mex_view_model_set_order_by (MexViewModel       *model,
                             MexContentMetadata  metadata_key,
                             gboolean            descending)
{
  MexViewModelPrivate *priv;

  g_return_if_fail (MEX_IS_VIEW_MODEL (model));

  priv = MEX_VIEW_MODEL (model)->priv;

  if (priv->order_by_key == metadata_key
      && priv->order_by_descending == descending)
    return;

  priv->order_by_key = metadata_key;
  priv->order_by_descending = descending;

  /* remove the existing external items, so that it is fully re-sorted */
  while (priv->external_items->len > 0)
    {
      GControllerReference *ref;

      ref = g_controller_create_reference (priv->controller,
                                           G_CONTROLLER_REMOVE,
                                           G_TYPE_UINT, 1, 0);
      g_controller_emit_changed (priv->controller, ref);
      g_ptr_array_remove_index (priv->external_items, 0);
    }

  mex_view_model_refresh_external_items (model);
}

/**
 * mex_view_model_get_is_filtered:
 * @model: A #MexViewModel
 *
 * Determines whether the model has any filters set.
 *
 * Returns: #TRUE if any filters are set
 */
gboolean
mex_view_model_get_is_filtered (MexViewModel *model)
{
  return (model->priv->filter_by != NULL);
}
