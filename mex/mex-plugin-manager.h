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


#ifndef _MEX_PLUGIN_MANAGER_H
#define _MEX_PLUGIN_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_PLUGIN_MANAGER mex_plugin_manager_get_type()

#define MEX_PLUGIN_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_PLUGIN_MANAGER, MexPluginManager))

#define MEX_PLUGIN_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_PLUGIN_MANAGER, MexPluginManagerClass))

#define MEX_IS_PLUGIN_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_PLUGIN_MANAGER))

#define MEX_IS_PLUGIN_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_PLUGIN_MANAGER))

#define MEX_PLUGIN_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_PLUGIN_MANAGER, MexPluginManagerClass))

typedef struct _MexPluginManager MexPluginManager;
typedef struct _MexPluginManagerClass MexPluginManagerClass;
typedef struct _MexPluginManagerPrivate MexPluginManagerPrivate;

typedef GType (*MexPluginGetTypeFunc)(void);

struct _MexPluginManager
{
  GObject parent;

  MexPluginManagerPrivate *priv;
};

struct _MexPluginManagerClass
{
  GObjectClass parent_class;

  void (* plugin_loaded) (MexPluginManager *manager,
                          GObject          *plugin);
};

GType mex_plugin_manager_get_type (void) G_GNUC_CONST;

MexPluginManager *mex_plugin_manager_get_default (void);

void mex_plugin_manager_refresh (MexPluginManager *manager);

G_END_DECLS

#endif /* _MEX_PLUGIN_MANAGER_H */
