/*
 * mex-tileout-effect.h: A simple tile based effect
 *
 * Copyright © 2011 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Written by: Damien Lespiau <damien.lespiau@intel.com>
 */

#ifndef __MEX_TILEOUT_EFFECT_H__
#define __MEX_TILEOUT_EFFECT_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_TILEOUT_EFFECT mex_tileout_effect_get_type()

#define MEX_TILEOUT_EFFECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_TILEOUT_EFFECT, MexTileoutEffect))

#define MEX_TILEOUT_EFFECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_TILEOUT_EFFECT, MexTileoutEffectClass))

#define MEX_IS_TILEOUT_EFFECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_TILEOUT_EFFECT))

#define MEX_IS_TILEOUT_EFFECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_TILEOUT_EFFECT))

#define MEX_TILEOUT_EFFECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_TILEOUT_EFFECT, MexTileoutEffectClass))

typedef struct _MexTileoutEffect MexTileoutEffect;
typedef struct _MexTileoutEffectClass MexTileoutEffectClass;
typedef struct _MexTileoutEffectPrivate MexTileoutEffectPrivate;

struct _MexTileoutEffect
{
  ClutterOffscreenEffect parent;

  MexTileoutEffectPrivate *priv;
};

struct _MexTileoutEffectClass
{
  ClutterOffscreenEffectClass parent_class;
};

GType mex_tileout_effect_get_type (void) G_GNUC_CONST;

ClutterEffect * mex_tileout_effect_new            (void);
void            mex_tileout_effect_set_n_columns  (MexTileoutEffect *effect,
                                                   gint              n_columns);
gint            mex_tileout_effect_get_n_columns  (MexTileoutEffect *effect);
void            mex_tileout_effect_set_n_rows     (MexTileoutEffect *effect,
                                                   gint              n_rows);
gint            mex_tileout_effect_get_n_rows     (MexTileoutEffect *effect);

G_END_DECLS

#endif /* __MEX_TILEOUT_EFFECT_H__ */
