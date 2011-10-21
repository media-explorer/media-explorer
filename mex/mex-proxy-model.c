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


#include "mex-proxy-model.h"

static void mex_model_iface_init (MexModelIface *iface);

static MexModelIface *mex_proxy_model_parent_iface = NULL;

G_DEFINE_TYPE_WITH_CODE (MexProxyModel, mex_proxy_model,
                         MEX_TYPE_GENERIC_MODEL,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL,
                                                mex_model_iface_init))

#define PROXY_MODEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_PROXY_MODEL, MexProxyModelPrivate))

enum
{
  PROP_0,

  PROP_MODEL
};

struct _MexProxyModelPrivate
{
  MexModel *model;
  GList    *bindings;
  gboolean  ignore_controller;
};

static void
mex_proxy_model_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MexProxyModel *self = MEX_PROXY_MODEL (object);

  switch (property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, mex_proxy_model_get_model (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_proxy_model_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MexProxyModel *self = MEX_PROXY_MODEL (object);

  switch (property_id)
    {
    case PROP_MODEL:
      mex_proxy_model_set_model (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_proxy_model_dispose (GObject *object)
{
  MexProxyModel *self = MEX_PROXY_MODEL (object);
  MexProxyModelPrivate *priv = self->priv;

  if (priv->model)
    mex_proxy_model_set_model (self, NULL);

  G_OBJECT_CLASS (mex_proxy_model_parent_class)->dispose (object);
}

static void
mex_proxy_model_set_sort_func (MexModel         *model,
                               MexModelSortFunc  sort_func,
                               gpointer          userdata)
{
  MexProxyModelPrivate *priv = MEX_PROXY_MODEL (model)->priv;

  /* Ignore the 'replace' signal that will come as a result of the sort
   * function changing.
   */
  priv->ignore_controller = TRUE;

  mex_proxy_model_parent_iface->set_sort_func (model, sort_func, userdata);

  priv->ignore_controller = FALSE;
}

static void
mex_model_iface_init (MexModelIface *iface)
{
  /* Store a pointer to the parent implementation so we can chain up */
  mex_proxy_model_parent_iface = g_type_interface_peek_parent (iface);

  iface->set_sort_func = mex_proxy_model_set_sort_func;
}

static void
mex_proxy_model_class_init (MexProxyModelClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexProxyModelPrivate));

  object_class->get_property = mex_proxy_model_get_property;
  object_class->set_property = mex_proxy_model_set_property;
  object_class->dispose = mex_proxy_model_dispose;

  pspec = g_param_spec_object ("model",
                               "Model",
                               "The MexModel currently being proxied.",
                               G_TYPE_OBJECT,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MODEL, pspec);
}

static void
mex_proxy_model_init (MexProxyModel *self)
{
  self->priv = PROXY_MODEL_PRIVATE (self);
}

MexModel *
mex_proxy_model_new (void)
{
  return g_object_new (MEX_TYPE_PROXY_MODEL, NULL);
}

static void
mex_proxy_model_controller_changed_cb (GController          *controller,
                                       GControllerAction     action,
                                       GControllerReference *ref,
                                       MexProxyModel        *self)
{
  gint i;

  gint n_indices = 0;
  MexProxyModelPrivate *priv = self->priv;

  /* We've been told to ignore this signal (due to sorting) */
  if (priv->ignore_controller)
    return;

  if (ref)
    n_indices = g_controller_reference_get_n_indices (ref);

  switch (action)
    {
    case G_CONTROLLER_ADD:
      for (i = 0; i < n_indices; i++)
        {
          MexContent *content;

          gint content_index = g_controller_reference_get_index_uint (ref, i);
          content = mex_model_get_content (priv->model, content_index);

          mex_model_add_content (MEX_MODEL (self), content);
        }
      break;

    case G_CONTROLLER_REMOVE:
      for (i = 0; i < n_indices; i++)
        {
          MexContent *content;

          gint content_index = g_controller_reference_get_index_uint (ref, i);

          content = mex_model_get_content (priv->model, content_index);

          mex_model_remove_content (MEX_MODEL (self), content);
        }
      break;

    case G_CONTROLLER_UPDATE:
      break;

    case G_CONTROLLER_CLEAR:
      mex_model_clear (MEX_MODEL (self));
      break;

    case G_CONTROLLER_REPLACE:
      mex_model_clear (MEX_MODEL (self));
      for (i = 0; i < mex_model_get_length (priv->model); i++)
        mex_model_add_content (MEX_MODEL (self),
                               mex_model_get_content (priv->model, i));
      break;

    case G_CONTROLLER_INVALID_ACTION:
      g_warning (G_STRLOC ": Proxy controller has issued an error");
      break;

    default:
      break;
    }
}

void
mex_proxy_model_set_model (MexProxyModel *proxy,
                           MexModel      *model)
{
  GController *controller;
  MexProxyModelPrivate *priv;

  g_return_if_fail (MEX_IS_PROXY_MODEL (proxy));
  g_return_if_fail (!model || MEX_IS_MODEL (model));

  priv = proxy->priv;

  if (priv->model == model)
    return;

  if (priv->model)
    {
      while (priv->bindings)
        {
          g_object_unref (priv->bindings->data);
          priv->bindings = g_list_delete_link (priv->bindings, priv->bindings);
        }

      controller = mex_model_get_controller (priv->model);
      g_signal_handlers_disconnect_by_func (controller,
                                          mex_proxy_model_controller_changed_cb,
                                          proxy);
      g_object_unref (priv->model);

      mex_model_clear (MEX_MODEL (proxy));
    }

  priv->model = model;

  if (priv->model)
    {
      g_object_ref (priv->model);
      controller = mex_model_get_controller (priv->model);
      g_signal_connect (controller, "changed",
                        G_CALLBACK (mex_proxy_model_controller_changed_cb),
                        proxy);

      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "title",
                                proxy, "title",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "placeholder-text",
                                proxy, "placeholder-text",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "icon-name",
                                proxy, "icon-name",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "display-item-count",
                                proxy, "display-item-count",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "always-visible",
                                proxy, "always-visible",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "category",
                                proxy, "category",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "priority",
                                proxy, "priority",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "alt-model",
                                proxy, "alt-model",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "alt-model-string",
                                proxy, "alt-model-string",
                                G_BINDING_SYNC_CREATE));
      priv->bindings = g_list_prepend (priv->bindings,
        g_object_bind_property (priv->model, "alt-model-active",
                                proxy, "alt-model-active",
                                G_BINDING_SYNC_CREATE));

     /* XXX: sort functions are not synced */

      /* Synthesise a 'replace' signal to populate existing items */
      mex_proxy_model_controller_changed_cb (controller,
                                             G_CONTROLLER_REPLACE,
                                             NULL,
                                             proxy);
    }

  g_object_notify (G_OBJECT (proxy), "model");
}

MexModel *
mex_proxy_model_get_model (MexProxyModel *proxy)
{
  g_return_val_if_fail (MEX_IS_PROXY_MODEL (proxy), NULL);
  return proxy->priv->model;
}

