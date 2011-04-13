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


#ifndef __MEX_APPLET_PROVIDER_H__
#define __MEX_APPLET_PROVIDER_H__

#include <glib-object.h>
#include <mex/mex-applet.h>

G_BEGIN_DECLS

#define MEX_TYPE_APPLET_PROVIDER (mex_applet_provider_get_type ())

#define MEX_APPLET_PROVIDER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                     \
                               MEX_TYPE_APPLET_PROVIDER, \
                               MexAppletProvider))

#define MEX_IS_APPLET_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_APPLET_PROVIDER))

#define MEX_APPLET_PROVIDER_IFACE(iface)                 \
  (G_TYPE_CHECK_CLASS_CAST ((iface),                      \
                            MEX_TYPE_APPLET_PROVIDER,    \
                            MexAppletProviderInterface))

#define MEX_IS_APPLET_PROVIDER_IFACE(iface) \
  (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_APPLET_PROVIDER))

#define MEX_APPLET_PROVIDER_GET_IFACE(obj)                     \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj),                        \
                                  MEX_TYPE_APPLET_PROVIDER,    \
                                  MexAppletProviderInterface))

typedef struct _MexAppletProvider          MexAppletProvider;
typedef struct _MexAppletProviderInterface MexAppletProviderInterface;

struct _MexAppletProviderInterface
{
  GTypeInterface g_iface;

  /* virtual functions */
  const GList * (*get_applets)   (MexAppletProvider *provider);
};

GType   mex_applet_provider_get_type       (void) G_GNUC_CONST;

const GList * mex_applet_provider_get_applets (MexAppletProvider *provider);

G_END_DECLS

#endif /* __MEX_APPLET_PROVIDER_H__ */
