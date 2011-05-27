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


#ifndef _MEX_RESIZING_HBOX_H
#define _MEX_RESIZING_HBOX_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_RESIZING_HBOX mex_resizing_hbox_get_type()

#define MEX_RESIZING_HBOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_RESIZING_HBOX, MexResizingHBox))

#define MEX_RESIZING_HBOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_RESIZING_HBOX, MexResizingHBoxClass))

#define MEX_IS_RESIZING_HBOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_RESIZING_HBOX))

#define MEX_IS_RESIZING_HBOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_RESIZING_HBOX))

#define MEX_RESIZING_HBOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_RESIZING_HBOX, MexResizingHBoxClass))

typedef struct _MexResizingHBox MexResizingHBox;
typedef struct _MexResizingHBoxClass MexResizingHBoxClass;
typedef struct _MexResizingHBoxPrivate MexResizingHBoxPrivate;

struct _MexResizingHBox
{
  MxWidget parent;

  MexResizingHBoxPrivate *priv;
};

struct _MexResizingHBoxClass
{
  MxWidgetClass parent_class;
};

GType mex_resizing_hbox_get_type (void) G_GNUC_CONST;

ClutterActor * mex_resizing_hbox_new (void);

void mex_resizing_hbox_set_resizing_enabled (MexResizingHBox *hbox,
                                             gboolean         enabled);
gboolean mex_resizing_hbox_get_resizing_enabled (MexResizingHBox *hbox);

void mex_resizing_hbox_set_horizontal_depth_scale (MexResizingHBox *hbox,
                                             gfloat           multiplier);
gfloat mex_resizing_hbox_get_horizontal_depth_scale (MexResizingHBox *hbox);

void mex_resizing_hbox_set_vertical_depth_scale (MexResizingHBox *hbox,
                                             gfloat           multiplier);
gfloat mex_resizing_hbox_get_vertical_depth_scale (MexResizingHBox *hbox);

void mex_resizing_hbox_set_depth_index (MexResizingHBox *hbox,
                                        gint             index);
gint mex_resizing_hbox_get_depth_index (MexResizingHBox *hbox);

void mex_resizing_hbox_set_max_depth (MexResizingHBox *hbox,
                                      gint             depth);
gint mex_resizing_hbox_get_max_depth (MexResizingHBox *hbox);

void mex_resizing_hbox_set_depth_fade (MexResizingHBox *hbox,
                                       gboolean         fade);
gboolean mex_resizing_hbox_get_depth_fade (MexResizingHBox *hbox);

G_END_DECLS

#endif /* _MEX_RESIZING_HBOX_H */
