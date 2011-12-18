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
 *
 * Authors:
 *   Damien Lespiau <damien.lespiau@intel.com>
 */

#ifndef __MEX_DVB_PLUGIN_H__
#define __MEX_DVB_PLUGIN_H__

#include <glib-object.h>
#include <mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_DVB_PLUGIN mex_dvb_plugin_get_type()

#define MEX_DVB_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_DVB_PLUGIN, MexDvbPlugin))

#define MEX_DVB_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_DVB_PLUGIN, MexDvbPluginClass))

#define MEX_IS_DVB_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_DVB_PLUGIN))

#define MEX_IS_DVB_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_DVB_PLUGIN))

#define MEX_DVB_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_DVB_PLUGIN, MexDvbPluginClass))

typedef struct _MexDvbPlugin MexDvbPlugin;
typedef struct _MexDvbPluginClass MexDvbPluginClass;
typedef struct _MexDvbPluginPrivate MexDvbPluginPrivate;

struct _MexDvbPlugin
{
  MexPlugin parent;

  MexDvbPluginPrivate *priv;
};

struct _MexDvbPluginClass
{
  MexPluginClass parent_class;
};

GType mex_dvb_plugin_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __MEX_DVB_PLUGIN_H__ */
