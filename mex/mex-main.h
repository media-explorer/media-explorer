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


#ifndef __MEX_MAIN_H__
#define __MEX_MAIN_H__

#include <glib.h>
#include <clutter/clutter.h>
#include <mx/mx.h>
#include <mex/mex-mmkeys.h>
#include <mex/mex-model.h>

G_BEGIN_DECLS

/**
 * MEX_KEY_BACK:
 *
 * The keysym of the 'Back' key.
 *
 * Since: 1.0
 */
#define MEX_KEY_BACK(x) ((x == CLUTTER_KEY_Escape) || (x == CLUTTER_KEY_Back))

/**
 * MEX_KEY_HOME:
 *
 * The keysym of the 'Home' key.
 *
 * Since: 1.0
 */
#define MEX_KEY_HOME(x)  ((x == CLUTTER_KEY_Super_L) \
                          || (x == CLUTTER_KEY_Home)\
                          || (x == CLUTTER_KEY_AudioMedia) \
                          || (x == CLUTTER_KEY_HomePage))

/**
 * MEX_KEY_INFO:
 *
 * The keysym of the 'Info' key.
 *
 * Since: 1.0
 */
#define MEX_KEY_INFO(x)  ((x == CLUTTER_KEY_Menu) || (x == 0x1005ff70) \
                          || (x == CLUTTER_KEY_i))

/**
 * MEX_KEY_OK:
 *
 * The keysym of the 'OK' key.
 *
 * Since: 1.0
 */
#define MEX_KEY_OK(x)    (x == CLUTTER_KEY_Return)

/**
 * MEX_DBUS_NAME:
 *
 * The main D-Bus name to access media explorer objects
 *
 * Since: 1.0
 */
#define MEX_DBUS_NAME   "org.media-explorer.MediaExplorer"

gboolean        mex_init                    (int    *argc,
                                             char ***argv);

gboolean        mex_init_with_args          (int            *argc,
                                             char         ***argv,
                                             const char     *parameter_string,
                                             GOptionEntry   *entries,
                                             const char     *translation_domain,
                                             GError        **error);

void            mex_deinit                  (void);

void            mex_set_main_window         (MxWindow *main_window);
MxWindow *      mex_get_main_window         (void);
void            mex_set_fullscreen          (gboolean fullscreen);
gboolean        mex_get_fullscreen          (void);
void            mex_toggle_fullscreen       (void);
ClutterActor *  mex_get_stage               (void);

MexModel*       mex_get_root_model          (void);

G_END_DECLS

#endif /* __MEX_MAIN_H__ */
