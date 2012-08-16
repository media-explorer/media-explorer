/*
 * Mex - a media explorer
 *
 * Copyright © 2012, sleep(5) ltd.
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

#include "mex-info-bar-component.h"

static void
mex_info_bar_component_init (gpointer g_iface)
{
}

GType
mex_info_bar_component_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo info =
      {
        sizeof (MexInfoBarComponentIface),
        mex_info_bar_component_init,
        NULL,
      };
      type = g_type_register_static (G_TYPE_INTERFACE, "MexInfoBarComponent",
                                     &info, 0);
    }

  return type;
}

MexInfoBarLocation
mex_info_bar_component_get_location (MexInfoBarComponent *comp)
{
  MexInfoBarComponentIface *iface;

  g_return_val_if_fail (MEX_IS_INFO_BAR_COMPONENT (comp),
                        MEX_INFO_BAR_LOCATION_UNDEFINED);

  iface = MEX_INFO_BAR_COMPONENT_GET_INTERFACE (comp);

  if (iface->get_location)
    return iface->get_location (comp);
  else
    return MEX_INFO_BAR_LOCATION_UNDEFINED;
}

int
mex_info_bar_component_get_location_index (MexInfoBarComponent *comp)
{
  MexInfoBarComponentIface *iface;

  g_return_val_if_fail (MEX_IS_INFO_BAR_COMPONENT (comp), -1);

  iface = MEX_INFO_BAR_COMPONENT_GET_INTERFACE (comp);

  if (iface->get_location_index)
    return iface->get_location_index (comp);
  else
    return -1;
}

ClutterActor *
mex_info_bar_component_create_ui (MexInfoBarComponent *comp,
                                  ClutterActor        *transient_for)
{
  MexInfoBarComponentIface *iface;

  g_return_val_if_fail (MEX_IS_INFO_BAR_COMPONENT (comp), NULL);

  iface = MEX_INFO_BAR_COMPONENT_GET_INTERFACE (comp);

  if (iface->create_ui)
    return iface->create_ui (comp, transient_for);
  else
    return NULL;
}

