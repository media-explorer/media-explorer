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

#ifndef __MEX_INFO_BAR_COMPONENT_H__
#define __MEX_INFO_BAR_COMPONENT_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_INFO_BAR_COMPONENT                                     \
   (mex_info_bar_component_get_type())
#define MEX_INFO_BAR_COMPONENT(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                MEX_TYPE_INFO_BAR_COMPONENT,            \
                                MexInfoBarComponent))
#define MEX_IS_INFO_BAR_COMPONENT(obj)                                  \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                MEX_TYPE_INFO_BAR_COMPONENT))
#define MEX_INFO_BAR_COMPONENT_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MEX_TYPE_INFO_BAR_COMPONENT, MexInfoBarComponentIface))

typedef enum
{
  MEX_INFO_BAR_LOCATION_UNDEFINED = 0,
  MEX_INFO_BAR_LOCATION_BAR,
  MEX_INFO_BAR_LOCATION_SETTINGS
} MexInfoBarLocation;

typedef struct _MexInfoBarComponent        MexInfoBarComponent;
typedef struct _MexInfoBarComponentIface   MexInfoBarComponentIface;

struct _MexInfoBarComponentIface
{
  GObjectClass parent_class;

  MexInfoBarLocation (*get_location)       (MexInfoBarComponent *comp);
  int                (*get_location_index) (MexInfoBarComponent *comp);
  ClutterActor *     (*create_ui)          (MexInfoBarComponent *comp,
                                            ClutterActor *transient_for);
};

GType mex_info_bar_component_get_type (void) G_GNUC_CONST;

MexInfoBarLocation mex_info_bar_component_get_location (MexInfoBarComponent *comp);
int mex_info_bar_component_get_location_index (MexInfoBarComponent *comp);
ClutterActor * mex_info_bar_component_create_ui (MexInfoBarComponent *comp,
                                                 ClutterActor *transient_for);

G_END_DECLS

#endif /* __MEX_INFO_BAR_COMPONENT_H__ */
