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

#ifndef __MEX_CONTENT_PROXY_H__
#define __MEX_CONTENT_PROXY_H__

#include <clutter/clutter.h>

#include <mex/mex-proxy.h>

G_BEGIN_DECLS

#define MEX_TYPE_CONTENT_PROXY                                          \
   (mex_content_proxy_get_type())
#define MEX_CONTENT_PROXY(obj)                                          \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                MEX_TYPE_CONTENT_PROXY,                 \
                                MexContentProxy))
#define MEX_CONTENT_PROXY_CLASS(klass)                                  \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             MEX_TYPE_CONTENT_PROXY,                    \
                             MexContentProxyClass))
#define MEX_IS_CONTENT_PROXY(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                MEX_TYPE_CONTENT_PROXY))
#define MEX_IS_CONTENT_PROXY_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             MEX_TYPE_CONTENT_PROXY))
#define MEX_CONTENT_PROXY_GET_CLASS(obj)                                \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               MEX_TYPE_CONTENT_PROXY,                  \
                               MexContentProxyClass))

typedef struct _MexContentProxyPrivate MexContentProxyPrivate;
typedef struct _MexContentProxy      MexContentProxy;
typedef struct _MexContentProxyClass MexContentProxyClass;

struct _MexContentProxy
{
    MexProxy parent;

    MexContentProxyPrivate *priv;
};

struct _MexContentProxyClass
{
    MexProxyClass parent_class;
};

GType mex_content_proxy_get_type (void) G_GNUC_CONST;

MexProxy *mex_content_proxy_new (MexModel         *model,
                                 ClutterContainer *view,
                                 GType             object_type);

void mex_content_proxy_set_stage (MexContentProxy *proxy,
                                  ClutterStage    *stage);

G_END_DECLS

#endif /* __MEX_CONTENT_PROXY_H__ */
