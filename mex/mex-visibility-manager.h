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

#ifndef _MEX_VISIBILITY_MANAGER_H
#define _MEX_VISIBILITY_MANAGER_H

#include <glib-object.h>

#include <mex-content-view.h>

G_BEGIN_DECLS

#define MEX_TYPE_VISIBILITY_MANAGER mex_visibility_manager_get_type()

#define MEX_VISIBILITY_MANAGER(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               MEX_TYPE_VISIBILITY_MANAGER,             \
                               MexVisibilityManager))

#define MEX_VISIBILITY_MANAGER_CLASS(klass)                             \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            MEX_TYPE_VISIBILITY_MANAGER,                \
                            MexVisibilityManagerClass))

#define MEX_IS_VISIBILITY_MANAGER(obj)                          \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               MEX_TYPE_VISIBILITY_MANAGER))

#define MEX_IS_VISIBILITY_MANAGER_CLASS(klass)                  \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                            \
                            MEX_TYPE_VISIBILITY_MANAGER))

#define MEX_VISIBILITY_MANAGER_GET_CLASS(obj)                           \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              MEX_TYPE_VISIBILITY_MANAGER,              \
                              MexVisibilityManagerClass))

typedef struct _MexVisibilityManager MexVisibilityManager;
typedef struct _MexVisibilityManagerClass MexVisibilityManagerClass;
typedef struct _MexVisibilityManagerPrivate MexVisibilityManagerPrivate;

struct _MexVisibilityManager
{
  GObject parent;

  MexVisibilityManagerPrivate *priv;
};

struct _MexVisibilityManagerClass
{
  GObjectClass parent_class;
};

GType mex_visibility_manager_get_type (void) G_GNUC_CONST;

MexVisibilityManager *mex_visibility_manager_new (void);

void mex_visibility_manager_push (MexVisibilityManager *manager,
                                  MexContentView       *view);

void mex_visibility_manager_pop (MexVisibilityManager *manager,
                                 MexContentView       *view);

void mex_visibility_manager_set_max_visible_items (MexVisibilityManager *manager,
                                                   guint                 max);

guint mex_visibility_manager_get_max_visible_items (MexVisibilityManager *manager);

void mex_visibility_manager_set_time_slice (MexVisibilityManager *manager,
                                            guint                 msec);

guint mex_visibility_manager_get_time_slice (MexVisibilityManager *manager);

G_END_DECLS

#endif /* _MEX_VISIBILITY_MANAGER_H */
