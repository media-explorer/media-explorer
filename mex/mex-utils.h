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


#include <glib.h>
#include <mx/mx.h>
#include <mex/mex-content.h>
#include <mex/mex-model.h>
#include <mex/mex-model-manager.h>

#ifndef _MEX_UTILS_H
#define _MEX_UTILS_H

typedef enum
{
  MEX_TEXTURE_FRAME_EMPTY        = 0,
  MEX_TEXTURE_FRAME_TOP_LEFT     = 1 << 0,
  MEX_TEXTURE_FRAME_TOP          = 1 << 1,
  MEX_TEXTURE_FRAME_TOP_RIGHT    = 1 << 2,
  MEX_TEXTURE_FRAME_LEFT         = 1 << 3,
  MEX_TEXTURE_FRAME_MIDDLE       = 1 << 4,
  MEX_TEXTURE_FRAME_RIGHT        = 1 << 5,
  MEX_TEXTURE_FRAME_BOTTOM_LEFT  = 1 << 6,
  MEX_TEXTURE_FRAME_BOTTOM       = 1 << 7,
  MEX_TEXTURE_FRAME_BOTTOM_RIGHT = 1 << 8
} MexPaintTextureFrameFlags;

#define MEX_PAINT_TEXTURE_FRAME_ALL (MEX_TEXTURE_FRAME_TOP_LEFT | \
                                     MEX_TEXTURE_FRAME_TOP | \
                                     MEX_TEXTURE_FRAME_TOP_RIGHT | \
                                     MEX_TEXTURE_FRAME_LEFT | \
                                     MEX_TEXTURE_FRAME_MIDDLE | \
                                     MEX_TEXTURE_FRAME_RIGHT | \
                                     MEX_TEXTURE_FRAME_BOTTOM_LEFT | \
                                     MEX_TEXTURE_FRAME_BOTTOM | \
                                     MEX_TEXTURE_FRAME_BOTTOM_RIGHT)
#define MEX_PAINT_TEXTURE_FRAME_NOC (MEX_TEXTURE_FRAME_TOP_LEFT | \
                                     MEX_TEXTURE_FRAME_TOP | \
                                     MEX_TEXTURE_FRAME_TOP_RIGHT | \
                                     MEX_TEXTURE_FRAME_LEFT | \
                                     MEX_TEXTURE_FRAME_RIGHT | \
                                     MEX_TEXTURE_FRAME_BOTTOM_LEFT | \
                                     MEX_TEXTURE_FRAME_BOTTOM | \
                                     MEX_TEXTURE_FRAME_BOTTOM_RIGHT)

typedef gint (*MexActorSortFunc) (ClutterActor *a,
                                  ClutterActor *b,
                                  gpointer      userdata);

void mex_style_load_default (void);
void mex_paint_texture_frame (gfloat                    x,
                              gfloat                    y,
                              gfloat                    width,
                              gfloat                    height,
                              gfloat                    tex_width,
                              gfloat                    tex_height,
                              gfloat                    top,
                              gfloat                    right,
                              gfloat                    bottom,
                              gfloat                    left,
                              MexPaintTextureFrameFlags flags);

void mex_push_focus (MxFocusable *actor);

void        mex_action_set_content (MxAction   *action,
                                    MexContent *content);
MexContent* mex_action_get_content (MxAction   *action);
void        mex_action_set_context (MxAction   *action,
                                    MexModel   *model);
MexModel*   mex_action_get_context (MxAction   *action);

gint mex_model_sort_alpha_cb (MexContent *a,
                              MexContent *b,
                              gpointer    bool_reverse);

gint mex_model_sort_time_cb (MexContent *a,
                             MexContent *b,
                             gpointer    bool_reverse);

gint mex_model_sort_smart_cb (MexContent *a,
                              MexContent *b,
                              gpointer    bool_reverse);

void    mex_print_date      (GDateTime   *date,
                             const gchar *prefix);
gchar * mex_date_to_string  (GDateTime *date);

extern const gchar *mex_shader_box_blur;

GQuark mex_tile_shadow_quark (void);

void mex_replace_border_image (CoglHandle     *texture_p,
                               MxBorderImage  *image,
                               MxBorderImage **image_p,
                               CoglHandle     *material_p);

const char* mex_get_data_dir (void);

gboolean mex_actor_has_focus (MxFocusManager *manager,
                              ClutterActor   *actor);

MexContent *mex_content_from_uri (const gchar *uri);

GKeyFile *mex_get_settings_key_file (void);

#endif /* _MEX_UTILS_H */
