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

#ifndef __MEX_GENERIC_CONTENT_H__
#define __MEX_GENERIC_CONTENT_H__

#include <glib-object.h>

#include <mex/mex-content.h>

G_BEGIN_DECLS

#define MEX_TYPE_GENERIC_CONTENT                \
  (mex_generic_content_get_type())
#define MEX_GENERIC_CONTENT(obj)                                \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               MEX_TYPE_GENERIC_CONTENT,        \
                               MexGenericContent))
#define MEX_GENERIC_CONTENT_CLASS(klass)                \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                    \
                            MEX_TYPE_GENERIC_CONTENT,   \
                            MexGenericContentClass))
#define IS_MEX_GENERIC_CONTENT(obj)                             \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               MEX_TYPE_GENERIC_CONTENT))
#define IS_MEX_GENERIC_CONTENT_CLASS(klass)             \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                    \
                            MEX_TYPE_GENERIC_CONTENT))
#define MEX_GENERIC_CONTENT_GET_CLASS(obj)              \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              MEX_TYPE_GENERIC_CONTENT, \
                              MexGenericContentClass))

typedef struct _MexGenericContentPrivate MexGenericContentPrivate;
typedef struct _MexGenericContent      MexGenericContent;
typedef struct _MexGenericContentClass MexGenericContentClass;

struct _MexGenericContent
{
  GInitiallyUnowned parent;

  MexGenericContentPrivate *priv;
};

struct _MexGenericContentClass
{
  GInitiallyUnownedClass parent_class;
};

GType mex_generic_content_get_type (void) G_GNUC_CONST;

gboolean mex_generic_content_get_last_position_start (MexGenericContent *self);

G_END_DECLS

#endif /* __MEX_GENERIC_CONTENT_H__ */
