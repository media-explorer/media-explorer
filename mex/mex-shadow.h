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


#ifndef _MEX_SHADOW_H
#define _MEX_SHADOW_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <mex/mex-utils.h>

G_BEGIN_DECLS

#define MEX_TYPE_SHADOW mex_shadow_get_type()

#define MEX_SHADOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_SHADOW, MexShadow))

#define MEX_SHADOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_SHADOW, MexShadowClass))

#define MEX_IS_SHADOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_SHADOW))

#define MEX_IS_SHADOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_SHADOW))

#define MEX_SHADOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_SHADOW, MexShadowClass))

typedef struct _MexShadow MexShadow;
typedef struct _MexShadowClass MexShadowClass;
typedef struct _MexShadowPrivate MexShadowPrivate;

struct _MexShadow
{
  GObject parent;

  MexShadowPrivate *priv;
};

struct _MexShadowClass
{
  GObjectClass parent_class;
};

GType mex_shadow_get_type (void) G_GNUC_CONST;

MexShadow *mex_shadow_new (ClutterActor *actor);

ClutterActor *mex_shadow_get_actor (MexShadow *shadow);

void mex_shadow_set_color (MexShadow *shadow, const ClutterColor *color);
const ClutterColor *mex_shadow_get_color (MexShadow *shadow);

void mex_shadow_set_radius_x (MexShadow *shadow, gint radius);
gint mex_shadow_get_radius_x (MexShadow *shadow);

void mex_shadow_set_radius_y (MexShadow *shadow, gint radius);
gint mex_shadow_get_radius_y (MexShadow *shadow);

void mex_shadow_set_offset_x (MexShadow *shadow, gint offset);
gint mex_shadow_get_offset_x (MexShadow *shadow);

void mex_shadow_set_offset_y (MexShadow *shadow, gint offset);
gint mex_shadow_get_offset_y (MexShadow *shadow);

void mex_shadow_set_paint_flags (MexShadow                 *shadow,
                                 MexPaintTextureFrameFlags  flags);
MexPaintTextureFrameFlags mex_shadow_get_paint_flags (MexShadow *shadow);

G_END_DECLS

#endif /* _MEX_SHADOW_H */
