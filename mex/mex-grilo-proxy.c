/* mex-grilo-proxy.c */

#include "mex-grilo-proxy.h"

G_DEFINE_TYPE (MexGriloProxy, mex_grilo_proxy, G_TYPE_OBJECT)

#define GRILO_PROXY_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_GRILO_PROXY, MexGriloProxyPrivate))

struct _MexGriloProxyPrivate
{

};

static void
mex_grilo_proxy_get_property (GObject    *object,
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
mex_grilo_proxy_set_property (GObject      *object,
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
mex_grilo_proxy_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_grilo_proxy_parent_class)->dispose (object);
}

static void
mex_grilo_proxy_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_grilo_proxy_parent_class)->finalize (object);
}

static void
mex_grilo_proxy_class_init (MexGriloProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexGriloProxyPrivate));

  object_class->get_property = mex_grilo_proxy_get_property;
  object_class->set_property = mex_grilo_proxy_set_property;
  object_class->dispose = mex_grilo_proxy_dispose;
  object_class->finalize = mex_grilo_proxy_finalize;

  operation_list = g_async_queue_new ();
}

static void
mex_grilo_proxy_init (MexGriloProxy *self)
{
  self->priv = GRILO_PROXY_PRIVATE (self);
}

MexGriloProxy *
mex_grilo_proxy_new (void)
{
  return g_object_new (MEX_TYPE_GRILO_PROXY, NULL);
}

void
mex_grilo_proxy_search (MexGriloProxy *proxy, const gchar *text)
{
  g_return_if_fail (MEX_IS_GRILO_PROXY (proxy));
  g_return_if_fail (text != NULL);


}

void
mex_grilo_proxy_metadata (MexGriloProxy *proxy, MexContent *content)
{
  g_return_if_fail (MEX_IS_GRILO_PROXY (proxy));
  g_return_if_fail (MEX_IS_CONTENT (content));

}

void
mex_grilo_proxy_stop (MexGriloProxy *proxy)
{
  MexGriloProxyPrivate *priv;

  g_return_if_fail (MEX_IS_GRILO_PROXY (proxy));

  priv = proxy->priv;

  if (priv->request_id)
    {
      grl_metadata_source_cancel (priv->
    }
}
