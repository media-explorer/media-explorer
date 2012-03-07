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


#ifndef _MEX_MENU_H
#define _MEX_MENU_H

#include <glib-object.h>
#include <mx/mx.h>
#include <mex/mex-resizing-hbox.h>

G_BEGIN_DECLS

#define MEX_TYPE_MENU mex_menu_get_type()

#define MEX_MENU(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_MENU, MexMenu))

#define MEX_MENU_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_MENU, MexMenuClass))

#define MEX_IS_MENU(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_MENU))

#define MEX_IS_MENU_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_MENU))

#define MEX_MENU_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_MENU, MexMenuClass))

typedef struct _MexMenu MexMenu;
typedef struct _MexMenuClass MexMenuClass;
typedef struct _MexMenuPrivate MexMenuPrivate;

typedef enum
{
  MEX_MENU_NONE,
  MEX_MENU_LEFT,
  MEX_MENU_RIGHT,
  MEX_MENU_TOGGLE
} MexMenuActionType;

/**
 * MexMenu:
 *
 * The content of this structure is private and should only be accessed using
 * the provided API.
 */
struct _MexMenu
{
  MexResizingHBox parent;

  MexMenuPrivate *priv;
};

struct _MexMenuClass
{
  MexResizingHBoxClass parent_class;
};

GType mex_menu_get_type (void) G_GNUC_CONST;

ClutterActor *mex_menu_new (void);

void mex_menu_add_action          (MexMenu           *menu,
                                   MxAction          *action,
                                   MexMenuActionType  type);

void mex_menu_action_set_detail   (MexMenu     *menu,
                                   const gchar *action,
                                   const gchar *detail);

const gchar *mex_menu_action_get_detail (MexMenu     *menu,
                                         const gchar *action_name);

void mex_menu_action_set_toggled  (MexMenu     *menu,
                                   const gchar *action_name,
                                   gboolean     toggled);

gboolean mex_menu_action_get_toggled (MexMenu     *menu,
                                      const gchar *action_name);

void mex_menu_remove_action       (MexMenu     *menu,
                                   const gchar *action);

GList *mex_menu_get_actions       (MexMenu *menu,
                                   gint     depth);

gint  mex_menu_push               (MexMenu *menu);
gint  mex_menu_pop                (MexMenu *menu);

void mex_menu_clear_all           (MexMenu *menu);

void   mex_menu_set_min_width (MexMenu *menu,
                               gfloat   min_width);
gfloat mex_menu_get_min_width (MexMenu *menu);

MxBoxLayout *mex_menu_get_layout  (MexMenu *menu);

void mex_menu_add_section_header (MexMenu     *menu,
                                  const gchar *title);
G_END_DECLS

#endif /* _MEX_MENU_H */
