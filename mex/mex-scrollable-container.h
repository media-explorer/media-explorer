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


#ifndef __MEX_SCROLLABLE_CONTAINER_H__
#define __MEX_SCROLLABLE_CONTAINER_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_SCROLLABLE_CONTAINER (mex_scrollable_container_get_type ())

#define MEX_SCROLLABLE_CONTAINER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                     \
                               MEX_TYPE_SCROLLABLE_CONTAINER, \
                               MexScrollableContainer))

#define MEX_IS_SCROLLABLE_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_SCROLLABLE_CONTAINER))

#define MEX_SCROLLABLE_CONTAINER_IFACE(iface)                 \
  (G_TYPE_CHECK_CLASS_CAST ((iface),                      \
                            MEX_TYPE_SCROLLABLE_CONTAINER,    \
                            MexScrollableContainerInterface))

#define MEX_IS_SCROLLABLE_CONTAINER_IFACE(iface) \
  (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_SCROLLABLE_CONTAINER))

#define MEX_SCROLLABLE_CONTAINER_GET_IFACE(obj)                     \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj),                        \
                                  MEX_TYPE_SCROLLABLE_CONTAINER,    \
                                  MexScrollableContainerInterface))

typedef struct _MexScrollableContainer          MexScrollableContainer;
typedef struct _MexScrollableContainerInterface MexScrollableContainerInterface;

struct _MexScrollableContainerInterface
{
  GTypeInterface g_iface;

  /* virtual functions */
  void (*get_allocation) (MexScrollableContainer *self,
                          ClutterActor           *child,
                          ClutterActorBox        *box);
};

GType   mex_scrollable_container_get_type       (void) G_GNUC_CONST;

void    mex_scrollable_container_get_allocation (MexScrollableContainer *self,
                                                 ClutterActor           *child,
                                                 ClutterActorBox        *box);

G_END_DECLS

#endif /* __MEX_SCROLLABLE_CONTAINER_H__ */
