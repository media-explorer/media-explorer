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

#ifndef _MEX_INFO_PANEL_H
#define _MEX_INFO_PANEL_H

#include <glib-object.h>
#include <mx/mx.h>
#include <mex/mex-content-view.h>

G_BEGIN_DECLS

#define MEX_TYPE_INFO_PANEL mex_info_panel_get_type()

#define MEX_INFO_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_INFO_PANEL, MexInfoPanel))

#define MEX_INFO_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_INFO_PANEL, MexInfoPanelClass))

#define MEX_IS_INFO_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_INFO_PANEL))

#define MEX_IS_INFO_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_INFO_PANEL))

#define MEX_INFO_PANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_INFO_PANEL, MexInfoPanelClass))

typedef struct _MexInfoPanel MexInfoPanel;
typedef struct _MexInfoPanelClass MexInfoPanelClass;
typedef struct _MexInfoPanelPrivate MexInfoPanelPrivate;

typedef enum
{
  MEX_INFO_PANEL_MODE_FULL,
  MEX_INFO_PANEL_MODE_SIMPLE
} MexInfoPanelMode;

struct _MexInfoPanel
{
  MxFrame parent;

  MexInfoPanelPrivate *priv;
};

struct _MexInfoPanelClass
{
  MxFrameClass parent_class;
};

GType mex_info_panel_get_type (void) G_GNUC_CONST;

ClutterActor *mex_info_panel_new (MexInfoPanelMode mode);

G_END_DECLS

#endif /* _MEX_INFO_PANEL_H */
