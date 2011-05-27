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


/**
 * SECTION:mex-scrollable-container
 * @short_description: Interface for containers that are scrollable
 *
 * Implementing #MexScrollable allows a scrollable container to provide further
 * information about its children, for example, their bounds within said
 * scrollable container.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-scrollable-container.h"

G_DEFINE_INTERFACE (MexScrollableContainer, mex_scrollable_container, G_TYPE_INVALID);

static void
mex_scrollable_container_default_init (MexScrollableContainerInterface *klass)
{
}

/**
 * mex_scrollable_container_get_allocation:
 * @container: a #MexScrollableContainer
 * @box: (out): A #ClutterActorBox
 *
 * Retrieves an child's prospective allocation. This should be the allocation
 * that the child will be given after all animations are complete. If the
 * container implements #MxFocusable, this should be the allocation when the
 * child is focused and all animations are complete.
 */
void
mex_scrollable_container_get_allocation (MexScrollableContainer *self,
                                         ClutterActor           *child,
                                         ClutterActorBox        *box)
{
  MexScrollableContainerInterface *iface;

  g_return_if_fail (MEX_IS_SCROLLABLE_CONTAINER (self));

  iface = MEX_SCROLLABLE_CONTAINER_GET_IFACE (self);

  if (G_LIKELY (iface->get_allocation))
    iface->get_allocation (self, child, box);
  else
    g_warning (G_STRLOC ": Object does not implement get_allocation");
}

