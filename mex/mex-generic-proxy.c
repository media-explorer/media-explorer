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


#include "mex-generic-proxy.h"

G_DEFINE_TYPE (MexGenericProxy, mex_generic_proxy, MEX_TYPE_PROXY)

#define GENERIC_PROXY_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GENERIC_PROXY, \
                                MexGenericProxyPrivate))

typedef struct
{
  gchar                        *source_property;
  gchar                        *target_property;
  MexGenericProxyTransformFunc  transform_to;
  gpointer                      user_data;
  GDestroyNotify                notify;
} MexGenericProxyBinding;

struct _MexGenericProxyPrivate
{
  GPtrArray  *bindings;
};

static GQuark mex_generic_proxy_bindings_quark = 0;


static void
mex_generic_proxy_finalize (GObject *object)
{
  MexGenericProxyPrivate *priv = MEX_GENERIC_PROXY (object)->priv;

  if (priv->bindings)
    {
      g_ptr_array_unref (priv->bindings);
      priv->bindings = NULL;
    }

  G_OBJECT_CLASS (mex_generic_proxy_parent_class)->finalize (object);
}

static void
mex_generic_proxy_binding_free (gpointer data)
{
  MexGenericProxyBinding *binding = (MexGenericProxyBinding *)data;

  g_free (binding->source_property);
  g_free (binding->target_property);

  g_slice_free (MexGenericProxyBinding, data);
}

typedef struct
{
  MexContent *content;
  GObject    *object;
  GList      *bindings;
} MexGenericProxyBindData;

static void
mex_generic_proxy_bind_cb (MexGenericProxyBinding  *binding,
                           MexGenericProxyBindData *data)
{
  GBinding *gbinding;

  gbinding = g_object_bind_property_full (data->content,
                                          binding->source_property,
                                          data->object,
                                          binding->target_property,
                                          G_BINDING_SYNC_CREATE,
                                          (GBindingTransformFunc)
                                          binding->transform_to,
                                          NULL,
                                          binding->user_data ?
                                          binding->user_data : data->content,
                                          binding->notify);

  data->bindings = g_list_append (data->bindings, gbinding);
}

static void
mex_generic_proxy_object_created (MexProxy   *proxy,
                                  MexContent *content,
                                  GObject    *object)
{
  MexGenericProxyBindData data;

  MexGenericProxyPrivate *priv = MEX_GENERIC_PROXY (proxy)->priv;

  data.content = content;
  data.object = object;
  data.bindings = NULL;

  g_ptr_array_foreach (priv->bindings, (GFunc)mex_generic_proxy_bind_cb, &data);

  g_object_set_qdata_full (object,
                           mex_generic_proxy_bindings_quark,
                           data.bindings,
                           (GDestroyNotify)g_list_free);
}

static void
mex_generic_proxy_object_removed (MexProxy   *proxy,
                                  MexContent *content,
                                  GObject    *object)
{
  g_object_set_qdata (object, mex_generic_proxy_bindings_quark, NULL);
}

static void
mex_generic_proxy_class_init (MexGenericProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MexProxyClass *proxy_class = MEX_PROXY_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexGenericProxyPrivate));

  object_class->finalize = mex_generic_proxy_finalize;

  proxy_class->object_created = mex_generic_proxy_object_created;
  proxy_class->object_removed = mex_generic_proxy_object_removed;

  mex_generic_proxy_bindings_quark =
    g_quark_from_static_string ("mex-generic-proxy-bindings");
}

static void
mex_generic_proxy_init (MexGenericProxy *self)
{
  MexGenericProxyPrivate *priv = self->priv = GENERIC_PROXY_PRIVATE (self);

  priv->bindings =
    g_ptr_array_new_with_free_func (mex_generic_proxy_binding_free);
}

MexProxy *
mex_generic_proxy_new (MexModel *model,
                       GType     object_type)
{
  return g_object_new (MEX_TYPE_GENERIC_PROXY,
                       "model", model,
                       "object-type", object_type,
                       NULL);
}

void
mex_generic_proxy_bind (MexGenericProxy *proxy,
                        const gchar     *content_property,
                        const gchar     *object_property)
{
  mex_generic_proxy_bind_full (proxy,
                               content_property,
                               object_property,
                               NULL,
                               NULL,
                               NULL);
}

void
mex_generic_proxy_bind_full (MexGenericProxy              *proxy,
                             const gchar                  *content_property,
                             const gchar                  *object_property,
                             MexGenericProxyTransformFunc  transform_to,
                             gpointer                      user_data,
                             GDestroyNotify                notify)
{
  MexGenericProxyBinding *binding;
  MexGenericProxyPrivate *priv = proxy->priv;

  g_return_if_fail (MEX_IS_GENERIC_PROXY (proxy));
  g_return_if_fail (content_property != NULL);
  g_return_if_fail (object_property != NULL);

  binding = g_slice_new0 (MexGenericProxyBinding);
  binding->source_property = g_strdup (content_property);
  binding->target_property = g_strdup (object_property);
  binding->transform_to = transform_to;
  binding->user_data = user_data;
  binding->notify = notify;

  g_ptr_array_add (priv->bindings, binding);
}

