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


#ifndef _MEX_SHELL_H
#define _MEX_SHELL_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_SHELL mex_shell_get_type()

#define MEX_SHELL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_SHELL, MexShell))

#define MEX_SHELL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_SHELL, MexShellClass))

#define MEX_IS_SHELL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_SHELL))

#define MEX_IS_SHELL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_SHELL))

#define MEX_SHELL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_SHELL, MexShellClass))

typedef struct _MexShell MexShell;
typedef struct _MexShellClass MexShellClass;
typedef struct _MexShellPrivate MexShellPrivate;

typedef enum
{
  MEX_SHELL_DIRECTION_NONE,
  MEX_SHELL_DIRECTION_TOP,
  MEX_SHELL_DIRECTION_RIGHT,
  MEX_SHELL_DIRECTION_BOTTOM,
  MEX_SHELL_DIRECTION_LEFT
} MexShellDirection;

struct _MexShell
{
  MxWidget parent;

  MexShellPrivate *priv;
};

struct _MexShellClass
{
  MxWidgetClass parent_class;

  void (*transition_started)   (MexShell *shell);
  void (*transition_completed) (MexShell *shell);
};

GType mex_shell_get_type (void) G_GNUC_CONST;

ClutterActor *mex_shell_new (void);

ClutterActor *mex_shell_get_presented (MexShell *shell);

void mex_shell_present (MexShell          *shell,
                        ClutterActor      *actor,
                        MexShellDirection  in);

G_END_DECLS

#endif /* _MEX_SHELL_H */
