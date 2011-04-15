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

#ifndef __MEX_REBINDER_H__
#define __MEX_REBINDER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_REBINDER mex_rebinder_get_type()

#define MEX_REBINDER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_REBINDER, MexRebinder))

#define MEX_REBINDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_REBINDER, MexRebinderClass))

#define MEX_IS_REBINDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_REBINDER))

#define MEX_IS_REBINDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_REBINDER))

#define MEX_REBINDER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_REBINDER, MexRebinderClass))

typedef struct _MexRebinder MexRebinder;
typedef struct _MexRebinderClass MexRebinderClass;
typedef struct _MexRebinderPrivate MexRebinderPrivate;

struct _MexRebinder
{
  GObject parent;

  MexRebinderPrivate *priv;
};

struct _MexRebinderClass
{
  GObjectClass parent_class;
};

GType mex_rebinder_get_type (void) G_GNUC_CONST;

MexRebinder *mex_rebinder_new (void);

G_END_DECLS

#endif /* __MEX_REBINDER_H__ */
