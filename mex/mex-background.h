/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#ifndef __MEX_BACKGROUND_H__
#define __MEX_BACKGROUND_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_BACKGROUND (mex_background_get_type ())
#define MEX_BACKGROUND(obj)                                             \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               MEX_TYPE_BACKGROUND,                     \
                               MexBackground))
#define MEX_IS_BACKGROUND(obj)                                          \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                   \
                               MEX_TYPE_BACKGROUND))
#define MEX_BACKGROUND_IFACE(iface)                                     \
  (G_TYPE_CHECK_CLASS_CAST ((iface),                                    \
                            MEX_TYPE_BACKGROUND,                        \
                            MexBackgroundIface))
#define MEX_IS_BACKGROUND_IFACE(iface)                                  \
  (G_TYPE_CHECK_CLASS_TYPE ((iface),                                    \
                            MEX_TYPE_BACKGROUND))
#define MEX_BACKGROUND_GET_IFACE(obj)                                   \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj),                                \
                                  MEX_TYPE_BACKGROUND,                  \
                                  MexBackgroundIface))

typedef struct _MexBackground      MexBackground;
typedef struct _MexBackgroundIface MexBackgroundIface;

struct _MexBackgroundIface
{
  GTypeInterface g_iface;

  void     (*set_active) (MexBackground *background, gboolean active);
};

GType mex_background_get_type (void) G_GNUC_CONST;

void mex_background_set_active (MexBackground *background, gboolean active);

G_END_DECLS

#endif /* __MEX_BACKGROUND_H__ */
