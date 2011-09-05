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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 */

/* Interface to run tracker queries and output them as JSON strings */

#include <stdlib.h>
#include <glib.h>
#include <json-glib/json-glib.h>

#include "tracker-client.h"

/* Returns the tracker query result in json format */
gchar *
tracker_interface_query (TrackerInterface *tracker_interface,
                         const gchar *query)
{
  GError *error=NULL;
  gchar *final_result;
  TrackerSparqlCursor *cursor;
  gint cols, i;
  const gchar **field_names;

  /* json-glib */
  JsonBuilder *json_builder;
  JsonGenerator *json_generator;

  /* Safety, we may have failed to create the tracker backend */
  if (!tracker_interface)
    return NULL;

  cursor = tracker_sparql_connection_query (tracker_interface->connection,
                                            query,
                                            NULL,
                                            &error);
  if (!cursor)
    {
      g_warning ("something went really wrong :'(");
      return g_strdup ("{}");
    }

  if (error)
    {
      g_warning ("Error in running query: %s", error->message);
      g_error_free (error);
      return g_strdup ("{}");
    }

  /* Do this once instead of for each item */

  cols = tracker_sparql_cursor_get_n_columns (cursor);

  field_names = g_malloc0 (cols * sizeof (gchar *));

  for (i=0; i < cols; i++)
    {
      field_names[i] = tracker_sparql_cursor_get_variable_name (cursor, i);
    }

  json_builder = json_builder_new ();

  json_builder_begin_array (json_builder);

  while (tracker_sparql_cursor_next (cursor, NULL, &error))
    {
      json_builder_begin_object (json_builder);

      for (i=0; i < cols; i++)
        {
          const gchar *value;

          /* "field" : "value", */
          value = tracker_sparql_cursor_get_string (cursor,
                                                    i,
                                                    NULL);

          json_builder_set_member_name (json_builder, field_names[i]);
          json_builder_add_string_value (json_builder, value);

        }
      json_builder_end_object (json_builder);
    }

  json_builder_end_array (json_builder);

  json_generator = json_generator_new ();

  json_generator_set_root (json_generator,
                           json_builder_get_root (json_builder));

  final_result = json_generator_to_data (json_generator, NULL);

  g_object_unref (json_builder);
  g_object_unref (json_generator);

  if (!final_result)
    final_result = g_strdup ("{}");

  return final_result;
}

TrackerInterface *
tracker_interface_new (void)
{
  TrackerInterface *tracker_interface;

  tracker_interface = g_new0 (TrackerInterface, 1);
  GError *error=NULL;

  /* connect to the tracker sparql backend */
  tracker_interface->connection =
    tracker_sparql_connection_get_direct (NULL, &error);

  if (error)
    {
      g_warning ("Failed to connect to tracker %s",
               error->message ? error->message : "Unknown");
      g_error_free (error);

      /* we're useless without a connection */
      tracker_interface_free (tracker_interface);
      return NULL;
    }
  return tracker_interface;
}


void tracker_interface_free (TrackerInterface *tracker_interface)
{
  if (tracker_interface->connection)
    {
      g_object_unref (tracker_interface->connection);
      tracker_interface->connection = NULL;
    }

  g_free (tracker_interface);
}
