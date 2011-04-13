/*
 * Mex - a media explorer
 *
 * Copyright © , 2010, 2011 Intel Corporation.
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

#ifndef _MEX_VOLUME_CONTROL_H
#define _MEX_VOLUME_CONTROL_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_VOLUME_CONTROL mex_volume_control_get_type()

#define MEX_VOLUME_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_VOLUME_CONTROL, MexVolumeControl))

#define MEX_VOLUME_CONTROL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_VOLUME_CONTROL, MexVolumeControlClass))

#define MEX_IS_VOLUME_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_VOLUME_CONTROL))

#define MEX_IS_VOLUME_CONTROL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_VOLUME_CONTROL))

#define MEX_VOLUME_CONTROL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_VOLUME_CONTROL, MexVolumeControlClass))

typedef struct _MexVolumeControl MexVolumeControl;
typedef struct _MexVolumeControlClass MexVolumeControlClass;
typedef struct _MexVolumeControlPrivate MexVolumeControlPrivate;

struct _MexVolumeControl
{
  MxFrame parent;

  MexVolumeControlPrivate *priv;
};

struct _MexVolumeControlClass
{
  MxFrameClass parent_class;
};

GType mex_volume_control_get_type (void) G_GNUC_CONST;

ClutterActor *mex_volume_control_new (void);

void mex_volume_control_volume_up (MexVolumeControl *self);
void mex_volume_control_volume_down (MexVolumeControl *self);
void mex_volume_control_volume_mute (MexVolumeControl *self);

G_END_DECLS

#endif /* _MEX_VOLUME_CONTROL_H */
