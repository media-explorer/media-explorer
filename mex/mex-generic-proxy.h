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


#ifndef _MEX_GENERIC_PROXY_H
#define _MEX_GENERIC_PROXY_H

#include <glib-object.h>
#include <mex/mex-proxy.h>

G_BEGIN_DECLS

#define MEX_TYPE_GENERIC_PROXY mex_generic_proxy_get_type()

#define MEX_GENERIC_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_GENERIC_PROXY, MexGenericProxy))

#define MEX_GENERIC_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_GENERIC_PROXY, MexGenericProxyClass))

#define MEX_IS_GENERIC_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_GENERIC_PROXY))

#define MEX_IS_GENERIC_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_GENERIC_PROXY))

#define MEX_GENERIC_PROXY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_GENERIC_PROXY, MexGenericProxyClass))

typedef struct _MexGenericProxy MexGenericProxy;
typedef struct _MexGenericProxyClass MexGenericProxyClass;
typedef struct _MexGenericProxyPrivate MexGenericProxyPrivate;

typedef gboolean (* MexGenericProxyTransformFunc) (gpointer      binding,
                                                   const GValue *source_value,
                                                   GValue       *target_value,
                                                   gpointer      user_data);

struct _MexGenericProxy
{
  MexProxy parent;

  MexGenericProxyPrivate *priv;
};

struct _MexGenericProxyClass
{
  MexProxyClass parent_class;

  void (*object_created) (MexGenericProxy *generic_proxy,
                          MexContent      *content,
                          GObject         *object);
  void (*object_removed) (MexGenericProxy *generic_proxy,
                          MexContent      *content,
                          GObject         *object);
};

GType mex_generic_proxy_get_type (void) G_GNUC_CONST;

MexProxy *mex_generic_proxy_new (MexModel *list,
                                 GType     object_type);

void mex_generic_proxy_bind (MexGenericProxy    *generic_proxy,
                             const gchar        *program_property,
                             const gchar        *object_property);

void mex_generic_proxy_bind_full (MexGenericProxy              *generic_proxy,
                                  const gchar                  *program_property,
                                  const gchar                  *object_property,
                                  MexGenericProxyTransformFunc  transform_to,
                                  gpointer                      user_data,
                                  GDestroyNotify                notify);

G_END_DECLS

#endif /* _MEX_GENERIC_PROXY_H */
