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


#ifndef __MEX_ACTION_MANAGER_H__
#define __MEX_ACTION_MANAGER_H__

#include <glib-object.h>
#include <mx/mx.h>
#include <mex/mex-content.h>

G_BEGIN_DECLS

#define MEX_TYPE_ACTION_MANAGER mex_action_manager_get_type()

#define MEX_ACTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_ACTION_MANAGER, MexActionManager))

#define MEX_ACTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_ACTION_MANAGER, MexActionManagerClass))

#define MEX_IS_ACTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_ACTION_MANAGER))

#define MEX_IS_ACTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_ACTION_MANAGER))

#define MEX_ACTION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_ACTION_MANAGER, MexActionManagerClass))

typedef struct _MexActionManager MexActionManager;
typedef struct _MexActionManagerClass MexActionManagerClass;
typedef struct _MexActionManagerPrivate MexActionManagerPrivate;

typedef struct
{
  MxAction  *action;
  gchar    **mime_types;
  gchar    **exclude_mime_types;
  gint       priority;
} MexActionInfo;

struct _MexActionManager
{
  GObject parent;

  MexActionManagerPrivate *priv;
};

struct _MexActionManagerClass
{
  GObjectClass parent_class;

  void (* action_added)   (MexActionManager *manager,
                           MexActionInfo    *info);
  void (* action_removed) (MexActionManager *manager,
                           const gchar      *name);
};

GType mex_action_manager_get_type (void) G_GNUC_CONST;

MexActionManager *mex_action_manager_get_default (void);

GList *mex_action_manager_get_actions (MexActionManager *manager);

GList *mex_action_manager_get_actions_for_content (MexActionManager *manager,
                                                   MexContent      *content);

void mex_action_manager_add_action    (MexActionManager *manager,
                                       MexActionInfo    *info);

void mex_action_manager_remove_action (MexActionManager *manager,
                                       const gchar      *name);

G_END_DECLS

#endif /* __MEX_ACTION_MANAGER_H__*/
