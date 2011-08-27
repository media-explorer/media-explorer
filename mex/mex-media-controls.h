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


#ifndef _MEX_MEDIA_CONTROLS_H
#define _MEX_MEDIA_CONTROLS_H

#include <glib-object.h>
#include <mx/mx.h>
#include "mex-content.h"
#include "mex-model.h"
#include "mex-utils.h"

G_BEGIN_DECLS

#define MEX_TYPE_MEDIA_CONTROLS mex_media_controls_get_type()

#define MEX_MEDIA_CONTROLS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_MEDIA_CONTROLS, MexMediaControls))

#define MEX_MEDIA_CONTROLS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_MEDIA_CONTROLS, MexMediaControlsClass))

#define MEX_IS_MEDIA_CONTROLS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_MEDIA_CONTROLS))

#define MEX_IS_MEDIA_CONTROLS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_MEDIA_CONTROLS))

#define MEX_MEDIA_CONTROLS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_MEDIA_CONTROLS, MexMediaControlsClass))

typedef struct _MexMediaControls MexMediaControls;
typedef struct _MexMediaControlsClass MexMediaControlsClass;
typedef struct _MexMediaControlsPrivate MexMediaControlsPrivate;

struct _MexMediaControls
{
  MxWidget parent;

  MexMediaControlsPrivate *priv;
};

struct _MexMediaControlsClass
{
  MxWidgetClass parent_class;
};

GType mex_media_controls_get_type (void) G_GNUC_CONST;

ClutterActor *mex_media_controls_new (void);

void mex_media_controls_set_media (MexMediaControls *controls,
                                   ClutterMedia     *media);
ClutterMedia *mex_media_controls_get_media (MexMediaControls *controls);

void mex_media_controls_set_content (MexMediaControls *self,
                                     MexContent       *content,
                                     MexModel         *context);

MexContent *mex_media_controls_get_content (MexMediaControls *self);

void mex_media_controls_focus_content (MexMediaControls *self,
                                       MexContent       *content);

MexContent *mex_media_controls_get_enqueued (MexMediaControls *controls,
                                             MexContent *current_content);

gboolean mex_media_controls_get_playing_queue (MexMediaControls *self);

void mex_media_controls_set_disabled (MexMediaControls *self,
                                      gboolean disabled);

G_END_DECLS

#endif /* _MEX_MEDIA_CONTROLS_H */
