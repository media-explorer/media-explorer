/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
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

#ifndef __MEX_RESIZING_HBOX_CHILD_H__
#define __MEX_RESIZING_HBOX_CHILD_H__

#include <clutter/clutter.h>
#include "mex-resizing-hbox.h"

G_BEGIN_DECLS

#define MEX_TYPE_RESIZING_HBOX_CHILD mex_resizing_hbox_child_get_type()

#define MEX_RESIZING_HBOX_CHILD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_RESIZING_HBOX_CHILD, MexResizingHBoxChild))

#define MEX_RESIZING_HBOX_CHILD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_RESIZING_HBOX_CHILD, MexResizingHBoxChildClass))

#define MEX_IS_RESIZING_HBOX_CHILD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_RESIZING_HBOX_CHILD))

#define MEX_IS_RESIZING_HBOX_CHILD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_RESIZING_HBOX_CHILD))

#define MEX_RESIZING_HBOX_CHILD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_RESIZING_HBOX_CHILD, MexResizingHBoxChildClass))

typedef struct _MexResizingHBoxChild MexResizingHBoxChild;
typedef struct _MexResizingHBoxChildClass MexResizingHBoxChildClass;
typedef struct _MexResizingHBoxChildPrivate MexResizingHBoxChildPrivate;

/**
 * MexResizingHBoxChild:
 *
 * The contents of this structure are private and should only be accessed
 * through the public API.
 */
struct _MexResizingHBoxChild
{
  /*< private >*/
  ClutterChildMeta parent;

  /* These are private, internal variables */
  guint            dead    : 1;
  guint            stagger : 1;
  ClutterActor    *actor;
  gdouble          initial_width;
  gdouble          target_width;
  gdouble          initial_height;
  gdouble          target_height;
  ClutterTimeline *timeline;
  gfloat           staggered_width;
};

struct _MexResizingHBoxChildClass
{
  ClutterChildMetaClass parent_class;

  /* padding for future expansion */
  void (*_padding_0) (void);
  void (*_padding_1) (void);
  void (*_padding_2) (void);
  void (*_padding_3) (void);
  void (*_padding_4) (void);
};

GType mex_resizing_hbox_child_get_type (void);

G_END_DECLS

#endif /* __MEX_RESIZING_HBOX_CHILD_H__ */
