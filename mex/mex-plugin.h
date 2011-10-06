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

#ifndef __MEX_PLUGIN_H__
#define __MEX_PLUGIN_H__

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

typedef enum _MexPluginPriority MexPluginPriority;
typedef struct _MexPluginDescription MexPluginDescription;

/**
 * MexPluginPriority:
 * @MEX_PLUGIN_PRIORITY_DEBUG: first to load, the debug plugin
 * @MEX_PLUGIN_PRIORITY_CORE: core plugins, part of base UI
 * @MEX_PLUGIN_PRIORITY_NORMAL: other plugins with no particular need
 */
enum _MexPluginPriority
{
  MEX_PLUGIN_PRIORITY_DEBUG     = 0,
  MEX_PLUGIN_PRIORITY_CORE      = 100,
  MEX_PLUGIN_PRIORITY_NORMAL    = 200
};

/**
 * MexPluginDescription:
 * @name: the name of the plugin
 * @description: a short description
 * @license: string with the license name
 * @authors: comma separated list of "$name <$email>"
 * @version: the full version string of the plugin
 * @mex_api_major: the major number of the mex API this plugin was compiled for
 * @mex_api_major: the minor number of the mex API this plugin was compiled for
 * @get_type: a function that returns the GObject type to instanciate
 * @priority: the priority that determines the loading order see
 *            #MexPluginPriority
 */
struct _MexPluginDescription
{
  const gchar *name;
  const gchar *description;
  const gchar *version;
  const gchar *license;
  const gchar *authors;
  gint mex_api_major, mex_api_minor;
  GType (*get_type) (void);
  MexPluginPriority priority;
  gpointer padding[8];
};

/**
 * MEX_PLUGIN_DEFINE:
 * @name: the name of the plugin
 * @description: a short description
 * @license: string with the license name
 * @authors: comma separated list of "$name <$email>"
 * @version: the full version string of the plugin
 * @mex_api_major: the major number of the mex API this plugin was compiled for
 * @mex_api_major: the minor number of the mex API this plugin was compiled for
 * @get_type: a function that returns the GObject type to instanciate
 * @priority: the priority that determines the loading order see
 *            #MexPluginPriority
 */
#define MEX_DEFINE_PLUGIN(name, description, version,     \
                          license, authors,               \
                          mex_api_major, mex_api_minor,   \
                          get_type, priority)             \
G_MODULE_EXPORT MexPluginDescription mex_plugin_info =    \
{                                                         \
    name, description, version, license, authors,         \
    mex_api_major, mex_api_minor,                         \
    get_type, priority,                                   \
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }    \
};

G_END_DECLS

#endif /* __MEX_PLUGIN_H__ */
