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

#ifndef __REBINDER_EVDEV_MANAGER_H__
#define __REBINDER_EVDEV_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define REBINDER_TYPE_EVDEV_MANAGER rebinder_evdev_manager_get_type()

#define REBINDER_EVDEV_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  REBINDER_TYPE_EVDEV_MANAGER, RebinderEvdevManager))

#define REBINDER_EVDEV_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  REBINDER_TYPE_EVDEV_MANAGER, RebinderEvdevManagerClass))

#define REBINDER_IS_EVDEV_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  REBINDER_TYPE_EVDEV_MANAGER))

#define REBINDER_IS_EVDEV_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  REBINDER_TYPE_EVDEV_MANAGER))

#define REBINDER_EVDEV_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  REBINDER_TYPE_EVDEV_MANAGER, RebinderEvdevManagerClass))

typedef struct _RebinderEvdevManager RebinderEvdevManager;
typedef struct _RebinderEvdevManagerClass RebinderEvdevManagerClass;
typedef struct _RebinderEvdevManagerPrivate RebinderEvdevManagerPrivate;

typedef void (*KeyNotifier) (guint32  time_,
                             guint32  key,
                             guint32  state,
                             gpointer data);

struct _RebinderEvdevManager
{
  GObject parent;

  RebinderEvdevManagerPrivate *priv;
};

struct _RebinderEvdevManagerClass
{
  GObjectClass parent_class;
};

GType rebinder_evdev_manager_get_type (void) G_GNUC_CONST;

RebinderEvdevManager *rebinder_evdev_manager_get_default (void);

void
rebinder_evdev_manager_set_key_notifier (RebinderEvdevManager *manager,
                                         KeyNotifier           notifier,
                                         gpointer              data);

G_END_DECLS

#endif /* __REBINDER_EVDEV_MANAGER_H__ */
