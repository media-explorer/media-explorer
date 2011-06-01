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

#include "mex-tracker-utils.h"

/**/
#define TRACKER_UPNP_CLASS_REQUEST                      \
  "SELECT ?u WHERE "                                    \
  "{ "                                                  \
  "?u a rdfs:Class . "                                  \
  "FILTER(fn:ends-with(?u,\"upnp#UPnPDataObject\")) "   \
  "}"

/**/

static GHashTable *mex_to_sparql_mapping = NULL;
static GHashTable *sparql_to_mex_mapping = NULL;

/**/

static gchar *
build_flavored_key (gchar *key, const gchar *flavor)
{
  gint i = 0;

  while (key[i] != '\0')
    {
      if (!g_ascii_isalnum (key[i]))
        {
          key[i] = '_';
        }
      i++;
    }

  return g_strdup_printf ("%s_%s", key, flavor);
}

static void
set_orientation (TrackerSparqlCursor *cursor,
                 gint                 column,
                 MexContent          *content,
                 MexContentMetadata   metadata_key)
{
  const gchar *str = tracker_sparql_cursor_get_string (cursor, column, NULL);

  if (g_str_has_suffix (str, "nfo#orientation-top"))
    mex_content_set_metadata (content, metadata_key, "0");
  else if (g_str_has_suffix (str, "nfo#orientation-right"))
    mex_content_set_metadata (content, metadata_key, "90");
  else if (g_str_has_suffix (str, "nfo#orientation-bottom"))
    mex_content_set_metadata (content, metadata_key, "180");
  else if (g_str_has_suffix (str, "nfo#orientation-left"))
    mex_content_set_metadata (content, metadata_key, "270");
}

static MexSparqlAssoc *
insert_key_mapping (MexContentMetadata  metadata_key,
                    const gchar        *sparql_key_attr,
                    const gchar        *sparql_key_attr_call,
                    const gchar        *sparql_key_flavor)
{
  MexSparqlAssoc *assoc = g_new0 (MexSparqlAssoc, 1);
  GList *assoc_list = g_hash_table_lookup (mex_to_sparql_mapping,
                                           GINT_TO_POINTER (metadata_key));
  gchar *canon_name =
    g_strdup (mex_content_metadata_key_to_string (metadata_key));

  assoc->metadata_key         = metadata_key;
  assoc->sparql_key_name      = build_flavored_key (canon_name,
                                                    sparql_key_flavor);
  assoc->sparql_key_attr      = sparql_key_attr;
  assoc->sparql_key_attr_call = sparql_key_attr_call;
  assoc->sparql_key_flavor    = sparql_key_flavor;

  assoc_list = g_list_append (assoc_list, assoc);

  g_hash_table_insert (mex_to_sparql_mapping,
                       GINT_TO_POINTER (metadata_key),
                       assoc_list);
  g_hash_table_insert (sparql_to_mex_mapping,
                       (gpointer) assoc->sparql_key_name,
                       assoc);
  g_hash_table_insert (sparql_to_mex_mapping,
                       (gpointer) mex_content_metadata_key_to_string (metadata_key),
                       assoc);

  g_free (canon_name);

  return assoc;
}

static MexSparqlAssoc *
insert_key_mapping_with_setter (MexContentMetadata  metadata_key,
                                const gchar        *sparql_key_attr,
                                const gchar        *sparql_key_attr_call,
                                const gchar        *sparql_key_flavor,
                                MexTrackerSetterCb  setter)
{
  MexSparqlAssoc *assoc;

  assoc = insert_key_mapping (metadata_key,
                              sparql_key_attr,
                              sparql_key_attr_call,
                              sparql_key_flavor);

  assoc->set_value = setter;

  return assoc;
}

static void
mex_tracker_setup_key_mappings (void)
{
  mex_to_sparql_mapping = g_hash_table_new (g_direct_hash, g_direct_equal);
  sparql_to_mex_mapping = g_hash_table_new (g_str_hash, g_str_equal);

  insert_key_mapping (MEX_CONTENT_METADATA_PRIVATE_ID,
                      NULL,
                      "?u",
                      "file");

  insert_key_mapping (MEX_CONTENT_METADATA_ALBUM,
                      NULL,
                      "nmm:albumTitle(nmm:musicAlbum(?u))",
                      "audio");

  insert_key_mapping (MEX_CONTENT_METADATA_ARTIST,
                      NULL,
                      "nmm:artistName(nmm:performer(?u))",
                      "audio");

  /* insert_key_mapping (MEX_METADATA_KEY_AUTHOR, */
  /*                     NULL, */
  /*                     "nmm:artistName(nmm:performer(?u))", */
  /*                     "audio"); */

  /* insert_key_mapping (MEX_METADATA_KEY_BITRATE, */
  /*                     "nfo:averageBitrate", */
  /*                     "nfo:averageBitrate(?u)", */
  /*                     "audio"); */

  /* insert_key_mapping (MEX_METADATA_KEY_CHILDCOUNT, */
  /*                     "nfo:entryCounter", */
  /*                     "nfo:entryCounter(?u)", */
  /*                     "directory"); */

  insert_key_mapping (MEX_CONTENT_METADATA_DATE,
                      "nfo:fileLastModified",
                      "nfo:fileLastModified(?u)",
                      "file");

  insert_key_mapping (MEX_CONTENT_METADATA_DURATION,
                      "nfo:duration",
                      "nfo:duration(?u)",
                      "audio");

  /* insert_key_mapping (MEX_METADATA_KEY_FRAMERATE, */
  /*                     "nfo:frameRate", */
  /*                     "nfo:frameRate(?u)", */
  /*                     "video"); */

  insert_key_mapping (MEX_CONTENT_METADATA_HEIGHT,
                      "nfo:height",
                      "nfo:height(?u)",
                      "video");

  insert_key_mapping (MEX_CONTENT_METADATA_ID,
                      "tracker:id",
                      "tracker:id(?u)",
                      "file");

  /* insert_key_mapping (MEX_METADATA_KEY_LAST_PLAYED, */
  /*                     "nfo:fileLastAccessed(?u)", */
  /*                     "file"); */

  insert_key_mapping (MEX_CONTENT_METADATA_MIMETYPE,
                      "nie:mimeType",
                      "nie:mimeType(?u)",
                      "file");

  insert_key_mapping (MEX_CONTENT_METADATA_STREAM,
                      "nie:url",
                      "nie:url(?u)",
                      "file");

  insert_key_mapping (MEX_CONTENT_METADATA_TITLE,
                      "nie:title",
                      "nie:title(?u)",
                      "audio");

  insert_key_mapping (MEX_CONTENT_METADATA_TITLE,
                      "nfo:fileName",
                      "nfo:fileName(?u)",
                      "file");

  insert_key_mapping (MEX_CONTENT_METADATA_URL,
                      "nie:url",
                      "nie:url(?u)",
                      "file");

  insert_key_mapping (MEX_CONTENT_METADATA_WIDTH,
                      "nfo:width",
                      "nfo:width(?u)",
                      "video");

  insert_key_mapping (MEX_CONTENT_METADATA_SEASON,
                      "nmm:season",
                      "nmm:season(?u)",
                      "video");

  insert_key_mapping (MEX_CONTENT_METADATA_EPISODE,
                      "nmm:episodeNumber",
                      "nmm:episodeNumber(?u)",
                      "video");

  insert_key_mapping (MEX_CONTENT_METADATA_CREATION_DATE,
                      "nie:contentCreated",
                      "nie:contentCreated(?u)",
                      "image");

  insert_key_mapping (MEX_CONTENT_METADATA_CAMERA_MODEL,
                      NULL,
                      "nfo:model(nfo:equipment(?u))",
                      "image");

  insert_key_mapping (MEX_CONTENT_METADATA_FLASH_USED,
                      "nmm:flash",
                      "nmm:flash(?u)",
                      "image");

  insert_key_mapping (MEX_CONTENT_METADATA_EXPOSURE_TIME,
                      "nmm:exposureTime",
                      "nmm:exposureTime(?u)",
                      "image");

  insert_key_mapping (MEX_CONTENT_METADATA_ISO_SPEED,
                      "nmm:isoSpeed",
                      "nmm:isoSpeed(?u)",
                      "image");

  insert_key_mapping_with_setter (MEX_CONTENT_METADATA_ORIENTATION,
                                  "nfo:orientation",
                                  "nfo:orientation(?u)",
                                  "image",
                                  set_orientation);

  insert_key_mapping (MEX_CONTENT_METADATA_PLAY_COUNT,
                      "nie:usageCounter",
                      "nie:usageCounter(?u)",
                      "media");

  insert_key_mapping (MEX_CONTENT_METADATA_LAST_PLAYED_DATE,
                      "nie:contentAccessed",
                      "nie:contentAccessed(?u)",
                      "media");

#ifdef TRACKER_0_10_5
  insert_key_mapping (MEX_CONTENT_METADATA_LAST_POSITION,
                      "nfo:lastPlayedPosition",
                      "nfo:lastPlayedPosition(?u)",
                      "media");
#endif
}

MexSparqlAssoc *
mex_tracker_get_mapping_from_sparql (const gchar *sparql_key)
{
  return (MexSparqlAssoc *) g_hash_table_lookup (sparql_to_mex_mapping,
                                                 sparql_key);
}

static GList *
get_mapping_from_mex (const MexContentMetadata metadata_key)
{
  return (GList *) g_hash_table_lookup (mex_to_sparql_mapping,
                                        GINT_TO_POINTER (metadata_key));
}

gboolean
mex_tracker_key_is_supported (MexContentMetadata metadata_key)
{
  return g_hash_table_lookup (mex_to_sparql_mapping,
                              GINT_TO_POINTER (metadata_key)) != NULL;
}

/**/

gchar *
mex_tracker_get_select_string (const GList *metadata_keys)
{
  const GList *key = metadata_keys;
  GString *gstr = g_string_new ("");
  GList *assoc_list;
  MexSparqlAssoc *assoc;

  assoc_list = get_mapping_from_mex (MEX_CONTENT_METADATA_PRIVATE_ID);
  assoc = (MexSparqlAssoc *) assoc_list->data;
  g_string_append_printf (gstr, "%s AS %s ",
                          assoc->sparql_key_attr_call,
                          assoc->sparql_key_name);

  while (key != NULL)
    {
      assoc_list = get_mapping_from_mex (GPOINTER_TO_UINT (key->data));
      while (assoc_list != NULL)
        {
          assoc = (MexSparqlAssoc *) assoc_list->data;
          if (assoc != NULL)
            {
              g_string_append_printf (gstr, "%s AS %s ",
                                      assoc->sparql_key_attr_call,
                                      assoc->sparql_key_name);
            }
          assoc_list = assoc_list->next;
        }
      key = key->next;
    }

  return g_string_free (gstr, FALSE);
}

static void
gen_prop_insert_string (GString        *gstr,
                        MexSparqlAssoc *assoc,
                        MexContent     *content)
{
  gchar *tmp;

  tmp = g_strescape (mex_content_get_metadata (content, assoc->metadata_key), NULL);
  g_string_append_printf (gstr, "%s \"%s\"",
                          assoc->sparql_key_attr, tmp);
  g_free (tmp);
}

gchar *
mex_tracker_get_insert_string (MexContent *content,
                               const GList *metadata_keys)
{
  gboolean first = TRUE;
  const GList *key = metadata_keys, *assoc_list;
  MexSparqlAssoc *assoc;
  GString *gstr = g_string_new ("");
  gchar *ret;

  while (key != NULL)
    {
      assoc_list = get_mapping_from_mex (GPOINTER_TO_INT (key->data));
      while (assoc_list != NULL) {
        assoc = (MexSparqlAssoc *) assoc_list->data;
        if (assoc != NULL)
          {
            if (mex_content_get_metadata (content,
                                          GPOINTER_TO_INT (key->data)))
              {
                if (first)
                  {
                    gen_prop_insert_string (gstr, assoc, content);
                    first = FALSE;
                  }
                else
                  {
                    g_string_append (gstr, " ; ");
                    gen_prop_insert_string (gstr, assoc, content);
                  }
              }
          }
        assoc_list = assoc_list->next;
      }
      key = key->next;
    }

  ret = gstr->str;
  g_string_free (gstr, FALSE);

  return ret;
}

gchar *
mex_tracker_get_delete_string (const GList *metadata_keys)
{
  gboolean first = TRUE;
  const GList *key = metadata_keys, *assoc_list;
  MexSparqlAssoc *assoc;
  GString *gstr = g_string_new ("");
  gchar *ret;
  gint var_n = 0;

  while (key != NULL)
    {
      assoc_list = get_mapping_from_mex (GPOINTER_TO_INT (key->data));
      while (assoc_list != NULL)
        {
          assoc = (MexSparqlAssoc *) assoc_list->data;
          if (assoc != NULL)
            {
              if (first)
                {
                  g_string_append_printf (gstr, "%s ?v%i",
                                          assoc->sparql_key_attr, var_n);
                  first = FALSE;
                }
              else
                {
                  g_string_append_printf (gstr, " ; %s ?v%i",
                                          assoc->sparql_key_attr, var_n);
                }
              var_n++;
            }
          assoc_list = assoc_list->next;
        }
      key = key->next;
    }

  ret = gstr->str;
  g_string_free (gstr, FALSE);

  return ret;
}

gchar *
mex_tracker_get_delete_conditional_string (const gchar *urn,
                                           const GList *metadata_keys)
{
  gboolean first = TRUE;
  const GList *key = metadata_keys, *assoc_list;
  MexSparqlAssoc *assoc;
  GString *gstr = g_string_new ("");
  gchar *ret;
  gint var_n = 0;

  while (key != NULL)
    {
      assoc_list = get_mapping_from_mex (GPOINTER_TO_INT (key->data));
      while (assoc_list != NULL)
        {
          assoc = (MexSparqlAssoc *) assoc_list->data;
          if (assoc != NULL)
            {
              if (first)
                {
                  g_string_append_printf (gstr, "OPTIONAL { <%s>  %s ?v%i }",
                                          urn,  assoc->sparql_key_attr, var_n);
                  first = FALSE;
                }
              else
                {
                  g_string_append_printf (gstr, " . OPTIONAL { <%s> %s ?v%i }",
                                          urn, assoc->sparql_key_attr, var_n);
                }
              var_n++;
            }
          assoc_list = assoc_list->next;
        }
      key = key->next;
    }

  ret = gstr->str;
  g_string_free (gstr, FALSE);

  return ret;
}

/**/

void
mex_tracker_update_content (MexContent *content, TrackerSparqlCursor *cursor)
{
  int i, nb_col;

  g_return_if_fail (MEX_IS_CONTENT (content));
  g_return_if_fail (TRACKER_SPARQL_IS_CURSOR (cursor));

  nb_col = tracker_sparql_cursor_get_n_columns (cursor);

  for (i = 0; i < nb_col; i++)
    {
      const gchar *sparql_key =
        tracker_sparql_cursor_get_variable_name (cursor, i);
      MexSparqlAssoc *assoc = mex_tracker_get_mapping_from_sparql (sparql_key);

      /* No data */
      if (!tracker_sparql_cursor_is_bound (cursor, i))
        continue;

      /* Metadata already here */
      if (mex_content_get_metadata (content, assoc->metadata_key))
        continue;

      mex_content_set_metadata (content, assoc->metadata_key,
                                tracker_sparql_cursor_get_string (cursor,
                                                                  i, NULL));
    }
}

MexContent *
mex_tracker_build_content (TrackerSparqlCursor *cursor)
{
  MexContent *content = NULL;
  const gchar *urn;

  g_return_val_if_fail (TRACKER_SPARQL_IS_CURSOR (cursor), NULL);

  urn = tracker_sparql_cursor_get_string (cursor, 0, NULL);
  if (urn != NULL)
    content = mex_tracker_cache_lookup (mex_tracker_get_cache (), urn);

  if (!content)
    content = mex_tracker_content_new ();

  mex_tracker_update_content (content, cursor);
  mex_tracker_content_set_in_setup (MEX_TRACKER_CONTENT (content), FALSE);

  return content;
}

/**/

/* static gchar * */
/* get_tracker_volume_name (const gchar *uri, */
/* 			 const gchar *datasource) */
/* { */
/*   gchar *source_name = NULL; */
/*   GVolumeMonitor *volume_monitor; */
/*   GList *mounts, *mount; */
/*   GFile *file; */

/*   if (uri != NULL && *uri != '\0') { */
/*     volume_monitor = g_volume_monitor_get (); */
/*     mounts = g_volume_monitor_get_mounts (volume_monitor); */
/*     file = g_file_new_for_uri (uri); */

/*     mount = mounts; */
/*     while (mount != NULL) { */
/*       GFile *m_file = g_mount_get_root (G_MOUNT (mount->data)); */

/*       if (g_file_equal (m_file, file)) { */
/*         gchar *m_name = g_mount_get_name (G_MOUNT (mount->data)); */
/*         g_object_unref (G_OBJECT (m_file)); */
/*         source_name = g_strdup_printf ("Removable - %s", m_name); */
/*         g_free (m_name); */
/*         break; */
/*       } */
/*       g_object_unref (G_OBJECT (m_file)); */

/*       mount = mount->next; */
/*     } */
/*     g_list_foreach (mounts, (GFunc) g_object_unref, NULL); */
/*     g_list_free (mounts); */
/*     g_object_unref (G_OBJECT (file)); */
/*     g_object_unref (G_OBJECT (volume_monitor)); */
/*   } else { */
/*     source_name = g_strdup ("Local files"); */
/*   } */

/*   return source_name; */
/* } */

/* static gchar * */
/* get_tracker_upnp_name (const gchar *datasource_name) */
/* { */
/*   return g_strdup_printf ("UPnP - %s", datasource_name); */
/* } */

/* gchar * */
/* mex_tracker_get_media_name (const gchar *rdf_type, */
/*                             const gchar *uri, */
/*                             const gchar *datasource, */
/*                             const gchar *datasource_name) */
/* { */
/*   gchar *source_name = NULL; */
/*   gchar **rdf_single_type; */
/*   gint i; */

/*   /\* As rdf_type can be formed by several types, split them *\/ */
/*   rdf_single_type = g_strsplit (rdf_type, ",", -1); */
/*   i = g_strv_length (rdf_single_type) - 1; */

/*   while (i >= 0) { */
/*     if (g_str_has_suffix (rdf_single_type[i], RDF_TYPE_VOLUME)) { */
/*       source_name = get_tracker_volume_name (uri, datasource); */
/*       break; */
/*     } else if (g_str_has_suffix (rdf_single_type[i], RDF_TYPE_UPNP)) { */
/*       source_name = get_tracker_upnp_name (datasource_name); */
/*       break; */
/*     } */
/*     i--; */
/*   } */

/*   g_strfreev (rdf_single_type); */

/*   return source_name; */
/* } */

const GList *
mex_tracker_get_supported_keys (void)
{
  static GList *supported_keys = NULL;

  if (!supported_keys)
    supported_keys =  g_hash_table_get_keys (mex_to_sparql_mapping);

  return supported_keys;
}

static void
mex_tracker_get_upnp_class_cb (GObject      *object,
                               GAsyncResult *result,
                               gpointer      data)
{
  GError *error = NULL;
  TrackerSparqlConnection *connection = (TrackerSparqlConnection *) data;
  TrackerSparqlCursor *cursor;

  cursor =
    tracker_sparql_connection_query_finish (connection, result, &error);

  if (error) {
    g_warning ("Could not execute sparql query for upnp class: %s",
               error->message);
    g_error_free (error);
  } else {
    if (tracker_sparql_cursor_next (cursor, NULL, NULL)) {
      insert_key_mapping (MEX_CONTENT_METADATA_STILL,
                          "upnp:thumbnail",
                          "upnp:thumbnail(?u)",
                          "media");
    }
  }

  if (cursor)
    g_object_unref (cursor);
}

static void
mex_tracker_get_connection_cb (MexTrackerConnection *conn,
                               TrackerSparqlConnection *tconn,
                               gpointer data)
{
  GError *error = NULL;

  tracker_sparql_connection_query_async (tconn,
                                         TRACKER_UPNP_CLASS_REQUEST,
                                         NULL,
                                         mex_tracker_get_upnp_class_cb,
                                         tconn);
}

void
mex_tracker_connection_init (void)
{
  static gboolean initialized = FALSE;
  MexTrackerConnection *conn;

  if (G_LIKELY (initialized))
    {
      g_warning ("Connection already initialized.");
      return;
    }

  initialized = TRUE;
  mex_tracker_setup_key_mappings ();
  conn = mex_tracker_get_connection ();

  g_signal_connect (conn, "connected",
                    G_CALLBACK (mex_tracker_get_connection_cb), NULL);
}

MexTrackerConnection *
mex_tracker_get_connection (void)
{
  static MexTrackerConnection *connection = NULL;

  if (G_UNLIKELY (!connection))
    connection = mex_tracker_connection_new ();

  return connection;
}

MexTrackerQueue *
mex_tracker_get_queue (void)
{
  static MexTrackerQueue *queue = NULL;

  if (G_UNLIKELY (!queue))
    queue = mex_tracker_queue_new ();

  return queue;
}

MexTrackerCache *
mex_tracker_get_cache (void)
{
  static MexTrackerCache *cache = NULL;

  if (G_UNLIKELY (!cache))
    cache = mex_tracker_cache_new ();

  return cache;
}
