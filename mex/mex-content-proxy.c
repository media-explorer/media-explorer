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

/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#include "mex-content-proxy.h"
#include "mex-content-view.h"
#include <mx/mx.h>

/**
 * SECTION:mex-content-proxy
 * @short_description: A bridge between a model and user interface elements,
 * which creates widgets from model content items.
 *
 * An #MexContentProxy is associated with an #MexModel, a #ClutterContainer, and
 * an #MxWidget subclass which implements #MexContentView.
 *
 * As objects from the model are added to the proxy, they are added to
 * the container as widgets of the specified class.
 *
 * The implementation of #MexContentView determines how metadata or other
 * properties of a content item are translated to properties on
 * the widget.
 */

enum {
  PROP_0,
  PROP_VIEW,
};

enum {
  LAST_SIGNAL,
};

struct _MexContentProxyPrivate {
  ClutterActor *view;
  ClutterStage *stage;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MEX_TYPE_CONTENT_PROXY, MexContentProxyPrivate))
G_DEFINE_TYPE (MexContentProxy, mex_content_proxy, MEX_TYPE_PROXY);

static void
mex_content_proxy_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_content_proxy_parent_class)->finalize (object);
}

static void
mex_content_proxy_dispose (GObject *object)
{
  MexContentProxy *self = (MexContentProxy *) object;
  MexContentProxyPrivate *priv = self->priv;

  if (priv->stage)
    {
      g_object_remove_weak_pointer (G_OBJECT (priv->stage),
                                    (gpointer *)&priv->stage);
      priv->stage = NULL;
    }

  if (priv->view)
    {
      g_object_remove_weak_pointer (G_OBJECT (priv->view),
                                    (gpointer *)&priv->view);
      priv->view = NULL;
    }

  G_OBJECT_CLASS (mex_content_proxy_parent_class)->dispose (object);
}

static void
mex_content_proxy_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MexContentProxy *self = (MexContentProxy *) object;
  MexContentProxyPrivate *priv = self->priv;

  switch (prop_id)
    {
    case PROP_VIEW:
      if (priv->view)
        g_object_remove_weak_pointer (G_OBJECT (priv->view),
                                      (gpointer *)&priv->view);

      priv->view = g_value_get_object (value);

      if (priv->view)
        g_object_add_weak_pointer (G_OBJECT (priv->view),
                                   (gpointer *)&priv->view);
      break;

    default:
      break;
    }
}

static void
mex_content_proxy_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MexContentProxy *self = (MexContentProxy *) object;
  MexContentProxyPrivate *priv = self->priv;

  switch (prop_id) {
  case PROP_VIEW:
    g_value_set_object (value, priv->view);
    break;

  default:
    break;
  }
}

static void
mex_content_proxy_class_init (MexContentProxyClass *klass)
{
  GObjectClass *o_class = (GObjectClass *) klass;
  GParamSpec *pspec;

  o_class->dispose = mex_content_proxy_dispose;
  o_class->finalize = mex_content_proxy_finalize;
  o_class->set_property = mex_content_proxy_set_property;
  o_class->get_property = mex_content_proxy_get_property;

  g_type_class_add_private (klass, sizeof (MexContentProxyPrivate));

  pspec = g_param_spec_object ("view", "View",
                               "The view that will display the objects",
                               CLUTTER_TYPE_CONTAINER,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (o_class, PROP_VIEW, pspec);
}

static void
mex_content_proxy_object_created_cb (MexProxy       *proxy,
                                     MexContent     *content,
                                     MexContentView *content_view,
                                     gpointer        userdata)
{
  ClutterStage *stage;

  MexContentProxy *content_proxy = (MexContentProxy *) proxy;
  MexContentProxyPrivate *priv = content_proxy->priv;

  mex_content_view_set_content (content_view, content);
  mex_content_view_set_context (content_view, mex_proxy_get_model (proxy));

  if (!priv->view)
    return;

  stage = priv->stage ?
    priv->stage : (ClutterStage *)clutter_actor_get_stage (priv->view);

  if (stage)
    {
      MxActorManager *manager =
        mx_actor_manager_get_for_stage (stage);
      mx_actor_manager_add_actor (manager,
                                  (ClutterContainer *)priv->view,
                                  (ClutterActor *)content_view);
    }
  else
    clutter_container_add_actor ((ClutterContainer *) priv->view,
                                 (ClutterActor *) content_view);
}

static void
mex_content_proxy_object_removed_cb (MexProxy       *proxy,
                                     MexContent     *content,
                                     MexContentView *content_view,
                                     gpointer        userdata)
{
  ClutterStage *stage;

  MexContentProxy *content_proxy = (MexContentProxy *) proxy;
  MexContentProxyPrivate *priv = content_proxy->priv;

  if (!priv->view)
    return;

  stage = priv->stage ?
    priv->stage : (ClutterStage *)clutter_actor_get_stage (priv->view);

  if (stage)
    {
      MxActorManager *manager =
        mx_actor_manager_get_for_stage (stage);
      mx_actor_manager_remove_actor (manager,
                                     (ClutterContainer *) priv->view,
                                     (ClutterActor *) content_view);
    }
  else
    clutter_container_remove_actor ((ClutterContainer *) priv->view,
                                    (ClutterActor *) content_view);
}

static void
mex_content_proxy_init (MexContentProxy *self)
{
  MexContentProxyPrivate *priv = GET_PRIVATE (self);

  self->priv = priv;

  g_signal_connect_after (self, "object-created",
                          G_CALLBACK (mex_content_proxy_object_created_cb),
                          NULL);
  g_signal_connect_after (self, "object-removed",
                          G_CALLBACK (mex_content_proxy_object_removed_cb),
                          NULL);
}

MexProxy *
mex_content_proxy_new (MexModel         *model,
                       ClutterContainer *view,
                       GType             object_type)
{
  return g_object_new (MEX_TYPE_CONTENT_PROXY,
                       "model", model,
                       "view", view,
                       "object-type", object_type,
                       NULL);
}

void
mex_content_proxy_set_stage (MexContentProxy *proxy,
                             ClutterStage    *stage)
{
  MexContentProxyPrivate *priv;

  g_return_if_fail (MEX_IS_CONTENT_PROXY (proxy));
  g_return_if_fail (!stage || CLUTTER_IS_STAGE (stage));

  priv = proxy->priv;

  if (priv->stage == stage)
    return;

  if (priv->stage)
    g_object_remove_weak_pointer (G_OBJECT (priv->stage),
                                  (gpointer *)&priv->stage);

  priv->stage = stage;

  if (stage)
    g_object_add_weak_pointer (G_OBJECT (stage), (gpointer *)&priv->stage);
}
