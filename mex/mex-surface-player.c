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

#include "mex-surface-player.h"
#include "mex-log.h"

#include <gst/gst.h>
#include <clutter/clutter.h>
#include <clutter-gst/clutter-gst.h>
#include <clutter-gst/clutter-gst-player.h>

#define MEX_LOG_DOMAIN_DEFAULT  surface_player_log_domain
MEX_LOG_DOMAIN(surface_player_log_domain);

static void clutter_media_iface_init (ClutterMediaIface *iface);
static void clutter_gst_player_iface_init (ClutterGstPlayerIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexSurfacePlayer, mex_surface_player, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_MEDIA,
                                                clutter_media_iface_init)
                         G_IMPLEMENT_INTERFACE (CLUTTER_GST_TYPE_PLAYER,
                                                clutter_gst_player_iface_init))

#define SURFACE_PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_SURFACE_PLAYER, MexSurfacePlayerPrivate))

struct _MexSurfacePlayerPrivate
{
  guint dummy;
};

static void
clutter_media_iface_init (ClutterMediaIface *iface)
{
}

static void
clutter_gst_player_iface_init (ClutterGstPlayerIface *iface)
{
}

static void
mex_surface_player_class_init (MexSurfacePlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  clutter_gst_player_class_init (object_class);

  g_type_class_add_private (klass, sizeof (MexSurfacePlayerPrivate));
}

static void
mex_surface_player_init (MexSurfacePlayer *self)
{
  MEX_DEBUG ("init");

  self->priv = SURFACE_PLAYER_PRIVATE (self);

  clutter_gst_player_init (CLUTTER_GST_PLAYER (self));
}

MexSurfacePlayer *
mex_surface_player_new (void)
{
  return g_object_new (MEX_TYPE_SURFACE_PLAYER, NULL);
}
