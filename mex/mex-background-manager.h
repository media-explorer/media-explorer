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

#ifndef _MEX_BACKGROUND_MANAGER_H
#define _MEX_BACKGROUND_MANAGER_H

#include <glib-object.h>

#include <mex/mex-background.h>

G_BEGIN_DECLS

#define MEX_TYPE_BACKGROUND_MANAGER mex_background_manager_get_type()

#define MEX_BACKGROUND_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_BACKGROUND_MANAGER, MexBackgroundManager))

#define MEX_BACKGROUND_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_BACKGROUND_MANAGER, MexBackgroundManagerClass))

#define MEX_IS_BACKGROUND_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_BACKGROUND_MANAGER))

#define MEX_IS_BACKGROUND_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_BACKGROUND_MANAGER))

#define MEX_BACKGROUND_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_BACKGROUND_MANAGER, MexBackgroundManagerClass))

typedef struct _MexBackgroundManager MexBackgroundManager;
typedef struct _MexBackgroundManagerClass MexBackgroundManagerClass;
typedef struct _MexBackgroundManagerPrivate MexBackgroundManagerPrivate;

struct _MexBackgroundManager
{
  GObject parent;

  MexBackgroundManagerPrivate *priv;
};

struct _MexBackgroundManagerClass
{
  GObjectClass parent_class;
};

GType mex_background_manager_get_type (void) G_GNUC_CONST;

MexBackgroundManager *mex_background_manager_get_default (void);

void mex_background_manager_register (MexBackgroundManager *manager,
                                      MexBackground        *background);
void mex_background_manager_unregister (MexBackgroundManager *manager,
                                        MexBackground        *background);

void mex_background_manager_set_active (MexBackgroundManager *manager,
                                        gboolean              active);
gboolean mex_background_manager_get_active (MexBackgroundManager *manager);

G_END_DECLS

#endif /* _MEX_BACKGROUND_MANAGER_H */
