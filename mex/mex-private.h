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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib-object.h>

#ifndef __MEX_PRIVATE_H__
#define __MEX_PRIVATE_H__

G_BEGIN_DECLS

#define I_(str)         (g_intern_static_string ((str)))

#define MEX_PARAM_READABLE     \
        (G_PARAM_READABLE |     \
         G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB)

#define MEX_PARAM_WRITABLE     \
        (G_PARAM_WRITABLE |     \
         G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB)

#define MEX_PARAM_READWRITE    \
        (G_PARAM_READABLE | G_PARAM_WRITABLE | \
         G_PARAM_STATIC_NICK | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB)

/* Automatically determine which file extension to use for thumbnails */
#ifdef WITH_THUMBNAILER_INTERNAL
#  if defined (MEX_THUMBNAIL_FORMAT_JPEG)
#    define MEX_THUMBNAIL_EXTENSION   ".jpg"
#  elif defined (MEX_THUMBNAIL_FORMAT_PVR_PVRTC)
#    define MEX_THUMBNAIL_EXTENSION   ".pvr"
#  endif
#else /* tumbler */
#  define MEX_THUMBNAIL_EXTENSION   ".png"
#endif

gboolean mex_content_title_fallback_cb (GBinding     *binding,
                                        const GValue *source_value,
                                        GValue       *target_value,
                                        gpointer      user_data);
void _mex_print_date (GDateTime *date);

G_END_DECLS

#endif /* __MEX_PRIVATE_H__ */
