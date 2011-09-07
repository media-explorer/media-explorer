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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-background.h"
#include "mex-player.h"

static void
mex_background_base_finalize (gpointer g_iface)
{
}

static void
mex_background_base_init (gpointer g_iface)
{
}

GType
mex_background_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0)) {
    GTypeInfo content_info = {
      sizeof (MexBackgroundIface),
      mex_background_base_init,
      mex_background_base_finalize
    };

    our_type = g_type_register_static (G_TYPE_INTERFACE,
                                       "MexBackground",
                                       &content_info, 0);
  }

  return our_type;
}

/**
 * mex_background_set_active:
 * @background: a #MexBackground
 * @active: TRUE to activate the background, FALSE otherwise.
 *
 * Sets the #MexBackground to @active.
 * For example if the background has an animation or a video the
 * implementation would toggle the active state of this.
 */
void
mex_background_set_active (MexBackground *background, gboolean active)
{
  MexBackgroundIface *iface;

  g_return_if_fail (MEX_IS_BACKGROUND (background));

  iface = MEX_BACKGROUND_GET_IFACE (background);

  if (G_LIKELY (iface->set_active)) {
    iface->set_active (background, active);
    return;
  }

  g_warning ("MexBackground of type '%s' does not implement set_active()",
             g_type_name (G_OBJECT_TYPE (background)));
}

/**
 * mex_background_get_name:
 * @background: a #MexBackground
 *
 * Returns: The name of the background
 */
const gchar*
mex_background_get_name (MexBackground *background)
{
  MexBackgroundIface *iface;

  iface = MEX_BACKGROUND_GET_IFACE (background);

  if (G_LIKELY (iface->get_name))
    return iface->get_name (background);

  g_warning ("MexBackground of type '%s' does not implement get_name()",
             g_type_name (G_OBJECT_TYPE (background)));
  return NULL;
}
