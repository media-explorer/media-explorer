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


#ifndef __MEX_OPTICALDISCMANAGER_H__
#define __MEX_OPTICALDISCMANAGER_H__

#include <glib-object.h>
#include <gmodule.h>
#include <mex/mex.h>

G_BEGIN_DECLS

#define MEX_TYPE_OPTICAL_DISC_MANAGER mex_optical_disc_manager_get_type()

#define MEX_OPTICAL_DISC_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_OPTICAL_DISC_MANAGER, MexOpticalDiscManager))

#define MEX_OPTICAL_DISC_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_OPTICAL_DISC_MANAGER, MexOpticalDiscManagerClass))

#define MEX_IS_OPTICAL_DISC_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_OPTICAL_DISC_MANAGER))

#define MEX_IS_OPTICAL_DISC_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_OPTICAL_DISC_MANAGER))

#define MEX_OPTICAL_DISC_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_OPTICAL_DISC_MANAGER, MexOpticalDiscManagerClass))

typedef struct _MexOpticalDiscManager MexOpticalDiscManager;
typedef struct _MexOpticalDiscManagerClass MexOpticalDiscManagerClass;
typedef struct _MexOpticalDiscManagerPrivate MexOpticalDiscManagerPrivate;

struct _MexOpticalDiscManager
{
  GObject parent;

  MexOpticalDiscManagerPrivate *priv;
};

struct _MexOpticalDiscManagerClass
{
  GObjectClass parent_class;
};

GType           mex_optical_disc_manager_get_type         (void) G_GNUC_CONST;
GType           mex_get_plugin_type                       (void);

G_END_DECLS

#endif /* __MEX_OPTICAL_DISC_MANAGER_H__ */
