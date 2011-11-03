/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifndef __TRACKER_INTERFACE_H__
#define __TRACKER_INTERFACE_H__

#include <glib.h>
#include <libtracker-sparql/tracker-sparql.h>

G_BEGIN_DECLS

typedef struct _TrackerInterface TrackerInterface;

struct _TrackerInterface
{
  TrackerSparqlConnection *connection;
};

TrackerInterface *tracker_interface_new (void);
void tracker_interface_free (TrackerInterface *tracker_interface);

gchar *
tracker_interface_query (TrackerInterface *tracker_interface,
                         const gchar      *query);


G_END_DECLS

#endif
