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

#ifndef _MEX_TRACKER_UTILS_H_
#define _MEX_TRACKER_UTILS_H_

#include <tracker-sparql.h>

#include <mex-content.h>

#include "mex-tracker-cache.h"
#include "mex-tracker-connection.h"
#include "mex-tracker-content.h"
#include "mex-tracker-queue.h"

/* ------- Definitions ------- */

#define RDF_TYPE_ALBUM    "nmm#MusicAlbum"
#define RDF_TYPE_ARTIST   "nmm#Artist"
#define RDF_TYPE_AUDIO    "nfo#Audio"
#define RDF_TYPE_MUSIC    "nmm#MusicPiece"
#define RDF_TYPE_IMAGE    "nmm#Photo"
#define RDF_TYPE_VIDEO    "nmm#Video"
#define RDF_TYPE_FOLDER   "nfo#Folder"
#define RDF_TYPE_DOCUMENT "nfo#Document"
#define RDF_TYPE_BOX      "grilo#Box"

#define RDF_TYPE_VOLUME "tracker#Volume"
#define RDF_TYPE_UPNP   "upnp#ContentDirectory"

/**/

typedef void (*MexTrackerSetterCb) (TrackerSparqlCursor *cursor,
                                    gint                 column,
                                    MexContent          *content,
                                    MexContentMetadata   key);

typedef struct {
  MexContentMetadata metadata_key;
  const gchar *sparql_key_name;
  const gchar *sparql_key_attr;
  const gchar *sparql_key_attr_call;
  const gchar *sparql_key_flavor;

  MexTrackerSetterCb set_value;
} MexSparqlAssoc;

/**/
gboolean mex_tracker_key_is_supported (MexContentMetadata key);

const GList *mex_tracker_get_supported_keys (void);

MexSparqlAssoc *mex_tracker_get_mapping_from_sparql (const gchar *key);

MexContent *mex_tracker_build_content (TrackerSparqlCursor *cursor);

void mex_tracker_update_content (MexContent *content,
                                 TrackerSparqlCursor *cursor);

gchar *mex_tracker_get_select_string (const GList *keys);

gchar *mex_tracker_get_insert_string (MexContent *media, const GList *keys);

gchar *mex_tracker_get_delete_string (const GList *keys);

gchar *mex_tracker_get_delete_conditional_string (const gchar *urn,
                                                  const GList *keys);

/* gchar *mex_tracker_get_media_name (const gchar *rdf_type, */
/*                                    const gchar *uri, */
/*                                    const gchar *datasource, */
/*                                    const gchar *datasource_name); */

MexTrackerConnection *mex_tracker_get_connection (void);

MexTrackerQueue *mex_tracker_get_queue (void);

MexTrackerCache *mex_tracker_get_cache (void);

void mex_tracker_connection_init (void);

#endif /* _MEX_TRACKER_UTILS_H_ */
