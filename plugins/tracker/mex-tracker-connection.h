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

#ifndef _MEX_TRACKER_CONNECTION_H
#define _MEX_TRACKER_CONNECTION_H

#include <glib-object.h>

#include <tracker-sparql.h>

G_BEGIN_DECLS

#define MEX_TYPE_TRACKER_CONNECTION mex_tracker_connection_get_type()

#define MEX_TRACKER_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_TRACKER_CONNECTION, MexTrackerConnection))

#define MEX_TRACKER_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_TRACKER_CONNECTION, MexTrackerConnectionClass))

#define MEX_IS_TRACKER_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_TRACKER_CONNECTION))

#define MEX_IS_TRACKER_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_TRACKER_CONNECTION))

#define MEX_TRACKER_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_TRACKER_CONNECTION, MexTrackerConnectionClass))

typedef struct _MexTrackerConnection MexTrackerConnection;
typedef struct _MexTrackerConnectionClass MexTrackerConnectionClass;
typedef struct _MexTrackerConnectionPrivate MexTrackerConnectionPrivate;

struct _MexTrackerConnection
{
  GObject parent;

  MexTrackerConnectionPrivate *priv;
};

struct _MexTrackerConnectionClass
{
  GObjectClass parent_class;
};

GType mex_tracker_connection_get_type (void) G_GNUC_CONST;

MexTrackerConnection *mex_tracker_connection_new (void);

TrackerSparqlConnection *mex_tracker_connection_get_connection (MexTrackerConnection *connection);

G_END_DECLS

#endif /* _MEX_TRACKER_CONNECTION_H */
