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

#ifndef __REBINDER_H__
#define __REBINDER_H__

#include <mx/mx.h>

#include "fakekey.h"
#include "rebinder-evdev-manager.h"

#define MEX_REBINDER_DBUS_INTERFACE	      "com.meego.mex.Rebinder"
#define MEX_REBINDER_DBUS_PATH		      "/com/meego/mex/Rebinder"
#define MEX_REBINDER_CONFIGURE_DBUS_INTERFACE "com.meego.mex.RebinderConfigure"

typedef struct _Configure Configure;
typedef struct _Rebinder Rebinder;

typedef struct
{
  const gchar *name;  /* User friendly name */
  KeySym keysym;
  guint keycode;
} Binding;

/* rebinder-configure.c */
Configure * rebinder_configure	    (Rebinder *rebinder,
				     MxWindow *window,
				     gboolean  evdev,
				     gboolean  fullscreen);
void        rebinder_configure_free (Configure *config);

/* rebinder-main.c */
struct _Rebinder
{
  Display *dpy;
  FakeKey *fake;
  Configure *config;
  GMainLoop *mainloop;
  GArray *original_bindings;
  GPtrArray *evdev_bindings;
  RebinderEvdevManager *evdev_manager;
  GFileMonitor *monitor;
};

/* revinder-util.c */
const gchar *rebinder_get_config_dir (void);

/* rebinder-evdev-keymap.c */
Binding * find_evdev_binding_by_keycode (guint32 key);

#endif /* __REBINDER_H__ */
