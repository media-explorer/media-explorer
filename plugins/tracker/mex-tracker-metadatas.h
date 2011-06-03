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

#ifndef _MEX_TRACKER_METADATAS_H
#define _MEX_TRACKER_METADATAS_H

#include <glib-object.h>

#include <mex-content.h>

G_BEGIN_DECLS

#define MEX_TYPE_TRACKER_METADATAS mex_tracker_metadatas_get_type()

#define MEX_TRACKER_METADATAS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_TRACKER_METADATAS, MexTrackerMetadatas))

#define MEX_TRACKER_METADATAS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_TRACKER_METADATAS, MexTrackerMetadatasClass))

#define MEX_IS_TRACKER_METADATAS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_TRACKER_METADATAS))

#define MEX_IS_TRACKER_METADATAS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_TRACKER_METADATAS))

#define MEX_TRACKER_METADATAS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_TRACKER_METADATAS, MexTrackerMetadatasClass))

typedef struct _MexTrackerMetadatas MexTrackerMetadatas;
typedef struct _MexTrackerMetadatasClass MexTrackerMetadatasClass;
typedef struct _MexTrackerMetadatasPrivate MexTrackerMetadatasPrivate;

struct _MexTrackerMetadatas
{
  GObject parent;

  MexTrackerMetadatasPrivate *priv;
};

struct _MexTrackerMetadatasClass
{
  GObjectClass parent_class;
};

GType mex_tracker_metadatas_get_type (void) G_GNUC_CONST;

MexTrackerMetadatas *mex_tracker_metadatas_new (MexContentMetadata meta, ...);

GList *mex_tracker_metadatas_get_metadata_list (MexTrackerMetadatas *metadatas);

void mex_tracker_metadatas_set_metadata_list (MexTrackerMetadatas *metadatas,
                                              GList *metadata_list);



G_END_DECLS

#endif /* _MEX_TRACKER_METADATAS_H */
