/*
 * gobject-list: a LD_PRELOAD library for tracking the lifetime of GObjects
 *
 * Copyright (C) 2011  Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 *
 * Authors:
 *     Damien Lespiau <damien.lespiau@intel.com>
 */

#ifndef __GOBJECT_LIST__
#define __GOBJECT_LIST__

typedef struct
{
  gchar *str;
  gint value;
} GObjectListTuple;

void    gobject_list_toggle_verbose (void);
GList * gobject_list_get_summary    (void);
void    gobject_list_free_summary   (GList *tuples);

#endif /* __GOBJECT_LIST__ */
