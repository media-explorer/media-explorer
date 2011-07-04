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


#ifndef _MEX_APPLET_MANAGER_H
#define _MEX_APPLET_MANAGER_H

#include <glib-object.h>
#include <mex/mex-applet.h>

G_BEGIN_DECLS

#define MEX_TYPE_APPLET_MANAGER mex_applet_manager_get_type()

#define MEX_APPLET_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_APPLET_MANAGER, MexAppletManager))

#define MEX_APPLET_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_APPLET_MANAGER, MexAppletManagerClass))

#define MEX_IS_APPLET_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_APPLET_MANAGER))

#define MEX_IS_APPLET_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_APPLET_MANAGER))

#define MEX_APPLET_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_APPLET_MANAGER, MexAppletManagerClass))

typedef struct _MexAppletManager MexAppletManager;
typedef struct _MexAppletManagerClass MexAppletManagerClass;
typedef struct _MexAppletManagerPrivate MexAppletManagerPrivate;

struct _MexAppletManager
{
  GObject parent;

  MexAppletManagerPrivate *priv;
};

struct _MexAppletManagerClass
{
  GObjectClass parent_class;

  void (* applet_added) (MexAppletManager *manager,
                         MexApplet        *applet);
  void (* applet_removed) (MexAppletManager *manager,
                           const gchar      *id);
};

GType mex_applet_manager_get_type (void) G_GNUC_CONST;

MexAppletManager *mex_applet_manager_get_default (void);

GList *mex_applet_manager_get_applets (MexAppletManager *manager);

void mex_applet_manager_add_applet (MexAppletManager *manager,
                                    MexApplet        *applet);
void mex_applet_manager_remove_applet (MexAppletManager *manager,
                                       const gchar      *id);

G_END_DECLS

#endif /* _MEX_APPLET_MANAGER_H */
