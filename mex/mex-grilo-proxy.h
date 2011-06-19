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

#ifndef _MEX_GRILO_PROXY_H
#define _MEX_GRILO_PROXY_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_GRILO_PROXY mex_grilo_proxy_get_type()

#define MEX_GRILO_PROXY(obj)                                            \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               MEX_TYPE_GRILO_PROXY,                    \
                               MexGriloProxy))

#define MEX_GRILO_PROXY_CLASS(klass)                                    \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            MEX_TYPE_GRILO_PROXY,                       \
                            MexGriloProxyClass))

#define MEX_IS_GRILO_PROXY(obj)                         \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MEX_TYPE_GRILO_PROXY))

#define MEX_IS_GRILO_PROXY_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                    \
                            MEX_TYPE_GRILO_PROXY))

#define MEX_GRILO_PROXY_GET_CLASS(obj)                                  \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              MEX_TYPE_GRILO_PROXY,                     \
                              MexGriloProxyClass))

typedef struct _MexGriloProxy MexGriloProxy;
typedef struct _MexGriloProxyClass MexGriloProxyClass;
typedef struct _MexGriloProxyPrivate MexGriloProxyPrivate;

struct _MexGriloProxy
{
  GObject parent;

  MexGriloProxyPrivate *priv;
};

struct _MexGriloProxyClass
{
  GObjectClass parent_class;
};

GType mex_grilo_proxy_get_type (void) G_GNUC_CONST;

MexGriloProxy *mex_grilo_proxy_new (void);

void mex_grilo_proxy_search (MexGriloProxy *proxy, const gchar *text);

void mex_grilo_proxy_metadata (MexGriloProxy *proxy, MexContent *content);

G_END_DECLS

#endif /* _MEX_GRILO_PROXY_H */
