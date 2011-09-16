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

/* mex-twitter_applet-plugin.h */

#ifndef _MEX_TWITTER_APPLET_PLUGIN_H
#define _MEX_TWITTER_APPLET_PLUGIN_H

#include <glib-object.h>
#include <mex/mex.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define MEX_TYPE_TWITTER_APPLET_PLUGIN mex_twitter_applet_plugin_get_type()

#define MEX_TWITTER_APPLET_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_TWITTER_APPLET_PLUGIN, MexTwitterAppletPlugin))

#define MEX_TWITTER_APPLET_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_TWITTER_APPLET_PLUGIN, MexTwitterAppletPluginClass))

#define MEX_IS_TWITTER_APPLET_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_TWITTER_APPLET_PLUGIN))

#define MEX_IS_TWITTER_APPLET_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_TWITTER_APPLET_PLUGIN))

#define MEX_TWITTER_APPLET_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_TWITTER_APPLET_PLUGIN, MexTwitterAppletPluginClass))

typedef struct _MexTwitterAppletPlugin MexTwitterAppletPlugin;
typedef struct _MexTwitterAppletPluginClass MexTwitterAppletPluginClass;
typedef struct _MexTwitterAppletPluginPrivate MexTwitterAppletPluginPrivate;

struct _MexTwitterAppletPlugin
{
  GObject parent;

  MexTwitterAppletPluginPrivate *priv;
};

struct _MexTwitterAppletPluginClass
{
  GObjectClass parent_class;
};

GType mex_twitter_applet_plugin_get_type (void) G_GNUC_CONST;

G_MODULE_EXPORT const GType mex_get_plugin_type (void);

G_END_DECLS

#endif /* _MEX_TWITTER_APPLET_PLUGIN_H */
