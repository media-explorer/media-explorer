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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <mex/mex-plugin.h>

#include "mex-dvb-plugin.h"

G_DEFINE_TYPE (MexDvbPlugin, mex_dvb_plugin, G_TYPE_OBJECT)

#define DVB_PLUGIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_DVB_PLUGIN, MexDvbPluginPrivate))

struct _MexDvbPluginPrivate
{
  gint dummy;
};


static void
mex_dvb_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_dvb_plugin_parent_class)->finalize (object);
}

static void
mex_dvb_plugin_class_init (MexDvbPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexDvbPluginPrivate));

  object_class->finalize = mex_dvb_plugin_finalize;
}

static void
mex_dvb_plugin_init (MexDvbPlugin *self)
{
  self->priv = DVB_PLUGIN_PRIVATE (self);
}

static GType
mex_dvb_get_type (void)
{
  return MEX_TYPE_DVB_PLUGIN;
}

MEX_DEFINE_PLUGIN ("DVB",
		   "linux-dvb integration",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Damien Lespiau <damien.lespiau@intel.com",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_dvb_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
