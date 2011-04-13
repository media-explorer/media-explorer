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


#ifndef _MEX_PROXY_H
#define _MEX_PROXY_H

#include <glib-object.h>
#include <mex/mex-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_PROXY mex_proxy_get_type()

#define MEX_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_PROXY, MexProxy))

#define MEX_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_PROXY, MexProxyClass))

#define MEX_IS_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_PROXY))

#define MEX_IS_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_PROXY))

#define MEX_PROXY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_PROXY, MexProxyClass))

typedef struct _MexProxy MexProxy;
typedef struct _MexProxyClass MexProxyClass;
typedef struct _MexProxyPrivate MexProxyPrivate;

struct _MexProxy
{
  GObject parent;

  MexProxyPrivate *priv;
};

struct _MexProxyClass
{
  GObjectClass parent_class;

  void (*object_created) (MexProxy   *proxy,
                          MexContent *content,
                          GObject    *object);
  void (*object_removed) (MexProxy   *proxy,
                          MexContent *content,
                          GObject    *object);
};

GType mex_proxy_get_type (void) G_GNUC_CONST;

MexModel *mex_proxy_get_model (MexProxy *proxy);

GType     mex_proxy_get_object_type (MexProxy *proxy);

void  mex_proxy_set_limit (MexProxy *proxy,
                           guint     limit);
guint mex_proxy_get_limit (MexProxy *proxy);

void mex_proxy_start (MexProxy *proxy);
void mex_proxy_start_at (MexProxy   *proxy,
                         MexContent *start_at_content,
                         gboolean    loop);

void mex_proxy_stop (MexProxy *proxy);

G_END_DECLS

#endif /* _MEX_PROXY_H */
