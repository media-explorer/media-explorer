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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include <mex/mex-log.h>
#include <mex/mex-plugin.h>
#include <mex/mex-model-provider.h>
#include <mex/mex-action-provider.h>
#include <mex/mex-generic-model.h>
#include <mex/mex-utils.h>

#include "mex-transmission-plugin.h"
#include "mex-torrent.h"

#define MEX_LOG_DOMAIN_DEFAULT  transmission_log_domain
MEX_LOG_DOMAIN_STATIC (transmission_log_domain);

#define RPC_URL                       "http://localhost:9091/transmission/rpc"
#define TRANSMISSION_SESSION          "X-Transmission-Session-Id"
#define TRANSMISSION_REFRESH_TIMEOUT  10 /* in seconds */

G_DEFINE_TYPE (MexTransmissionPlugin, mex_transmission_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o)                                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                            \
                                MEX_TYPE_TRANSMISSION_PLUGIN,   \
                                MexTransmissionPluginPrivate))

struct _MexTransmissionPluginPrivate
{
  MexModel *transmission_model;

  SoupSession *session;
  gchar *session_id;

  GHashTable *id_to_torrent;

  /* we cache the update request to avoid repeatedly building it every
   * TRANSMISSION_REFRESH_TIMEOUT */
  gchar* update_request_data;
  gsize  update_request_length;
};

static void
free_int64 (gpointer data)
{
  g_slice_free (gint64, data);
}

static gboolean
array_has_id (GArray *array,
              gint64  id_to_check)
{
  guint i;

  for (i = 0; i < array->len; i++)
    {
      gint64 id = g_array_index (array, gint64, i);

      if (id == id_to_check)
        return TRUE;
    }

  return FALSE;
}

static void
decode_response (MexTransmissionPlugin *plugin,
                 const gchar           *data)
{
  MexTransmissionPluginPrivate *priv = plugin->priv;
  JsonParser *parser;
  JsonReader *reader;
  JsonNode *root;
  gint nb_torrents, i;
  guint u;
  GError *error = NULL;
  GArray *ids;

  parser = json_parser_new ();
  json_parser_load_from_data (parser, data, -1, &error);
  if (error)
    {
      g_warning ("could not parser response: %s\nmessage was %s",
                 error->message, data);
      goto parser_error;
    }

  root = json_parser_get_root (parser);
  reader = json_reader_new (root);

  json_reader_read_member (reader, "arguments");
  json_reader_read_member (reader, "torrents");
  nb_torrents = json_reader_count_elements (reader);

  /* ids will collect the ids currently known by the daemon to be able to
   * purge the torrents that have been removed from the server but that we
   * still have in our model */
  ids = g_array_sized_new (FALSE, FALSE, sizeof (gint64), nb_torrents);

  for (i = 0; i < nb_torrents; i++)
    {
      gint64 id, percent_done;
      const gchar *name;
      MexTorrent *torrent;

      json_reader_read_element (reader, i);

      json_reader_read_member (reader, "id");
      id = json_reader_get_int_value (reader);
      json_reader_end_member (reader);

      json_reader_read_member (reader, "name");
      name = json_reader_get_string_value (reader);
      json_reader_end_member (reader);

      json_reader_read_member (reader, "percentDone");
      percent_done = json_reader_get_int_value (reader);
      json_reader_end_member (reader);

      json_reader_end_element (reader);

      g_array_append_val (ids, id);

      torrent = g_hash_table_lookup (priv->id_to_torrent, &id);
      if (torrent)
        {
          /* we already have a MxTorrent for that ID, we update its fields */
          MEX_INFO ("(update) updating torrent %s", name);

          mex_torrent_set_percent_done (torrent, percent_done);
        }
      else
        {
          /* no MxTorrent for this ID, time to create one */
          gint64 *new_id;

          MEX_INFO ("(update) adding torrent %s", name);

          torrent = g_object_new (MEX_TYPE_TORRENT,
                                  "id", id,
                                  "name", name,
                                  "percent-done", percent_done,
                                  NULL);

          new_id = g_slice_new (gint64);
          *new_id = id;
          g_hash_table_insert (priv->id_to_torrent, new_id, torrent);

          mex_model_add_content (priv->transmission_model,
                                 MEX_CONTENT (torrent));
        }
    }
  json_reader_end_member (reader); /* torrents */
  json_reader_end_member (reader); /* arguments */

  /* purge time, remove the torrents that have disappeared */
  for (u = 0; u < mex_model_get_length (priv->transmission_model); u++)
    {
      MexTorrent *torrent;
      gint64 id;

      torrent = MEX_TORRENT (mex_model_get_content (priv->transmission_model,
                                                    u));
      id = mex_torrent_get_id (torrent);

      if (!array_has_id (ids, id))
        {
          MEX_INFO ("(update) remove torrent %s",
                    mex_torrent_get_name (torrent));

          mex_model_remove_content (priv->transmission_model,
                                    MEX_CONTENT (torrent));
          g_hash_table_remove (priv->id_to_torrent, &id);
        }
    }

  g_array_free (ids, TRUE);

  g_object_unref (reader);
parser_error:
  g_object_unref (parser);
}

static void
on_response_received (SoupSession *session,
                      SoupMessage *message,
                      gpointer     user_data)
{
  MexTransmissionPlugin *plugin = MEX_TRANSMISSION_PLUGIN (user_data);
  MexTransmissionPluginPrivate *priv = plugin->priv;

  if (message->status_code == SOUP_STATUS_CONFLICT)
    {
      const gchar *session_id;

      session_id = soup_message_headers_get_one (message->response_headers,
                                                 TRANSMISSION_SESSION);
      if (session_id == NULL)
        {
          g_warning ("could not retrieve session header");
          return;
        }

      priv->session_id = g_strdup (session_id);

      MEX_INFO ("Session Id is %s", priv->session_id);

      /* resend the message with the updated session id */
      soup_message_headers_append (message->request_headers,
                                   TRANSMISSION_SESSION,
                                   priv->session_id);
      g_object_ref (message);
      soup_session_queue_message (priv->session, message,
                                  on_response_received, plugin);
    }
  else if (message->status_code == SOUP_STATUS_OK)
    {
      decode_response (plugin, message->response_body->data);
    }
  else
    {
      g_warning ("Error %d while talking to the transmission daemon",
                 message->status_code);
    }
}

static gchar *
request_to_data (const gchar *method,
                 JsonNode    *arguments,
                 gsize       *length_out)
{
  JsonGenerator *generator;
  JsonBuilder *builder;
  gsize length = 0;
  JsonNode *root;
  gchar *data;

  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "method");
  json_builder_add_string_value (builder, method);
  json_builder_set_member_name (builder, "arguments");
  json_builder_add_value (builder, arguments);
  json_builder_end_object (builder);

  root = json_builder_get_root (builder);
  g_object_unref (builder);

  generator = json_generator_new ();
  json_generator_set_root (generator, root);
  data = json_generator_to_data (generator, &length);
  g_object_unref (generator);

  if (length_out)
    *length_out = length;

  return data;
}

#if 0
static void
mex_transmission_send_request (MexTransmissionPlugin *plugin,
                               const gchar           *method,
                               JsonNode              *arguments)
{
  MexTransmissionPluginPrivate *priv = plugin->priv;
  SoupMessage *message;
  gsize length = 0;
  gchar *data;

  /* build the json data */
  data = request_to_data (method, arguments, &length);

  /* send the message */
  message = soup_message_new ("POST", RPC_URL);
  soup_message_set_request (message, "application/json", SOUP_MEMORY_TAKE,
                            data, length);
  soup_session_queue_message (priv->session, message,
                              on_response_received, plugin);
}
#endif

static void
mex_transmission_send_static_message (MexTransmissionPlugin *plugin,
                                      const gchar           *data,
                                      gsize                  length)
{
  MexTransmissionPluginPrivate *priv = plugin->priv;
  SoupMessage *message;

  /* send the message */
  message = soup_message_new ("POST", RPC_URL);
  soup_message_set_request (message, "application/json", SOUP_MEMORY_STATIC,
                            data, length);
  soup_session_queue_message (priv->session, message,
                              on_response_received, plugin);
}

static void
build_update_request (MexTransmissionPlugin *plugin)
{
  MexTransmissionPluginPrivate *priv = plugin->priv;
  JsonBuilder *builder;
  JsonNode *arguments;

  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "fields");
  json_builder_begin_array (builder);
  json_builder_add_string_value (builder, "id");
  json_builder_add_string_value (builder, "name");
  json_builder_add_string_value (builder, "percentDone");
  json_builder_end_array (builder);
  json_builder_end_object (builder);

  arguments = json_builder_get_root (builder);
  g_object_unref (builder);

  priv->update_request_data = request_to_data ("torrent-get",
                                               arguments,
                                               &priv->update_request_length);
}

static gboolean
mex_transmission_update_model (gpointer data)
{
  MexTransmissionPlugin *self = data;
  MexTransmissionPluginPrivate *priv = self->priv;

  mex_transmission_send_static_message (self,
                                        priv->update_request_data,
                                        priv->update_request_length);

  return TRUE;
}

/*
 * GObject implementation
 */

static void
mex_transmission_plugin_finalize (GObject *object)
{
  MexTransmissionPlugin *plugin = MEX_TRANSMISSION_PLUGIN (object);
  MexTransmissionPluginPrivate *priv = plugin->priv;

  if (priv->id_to_torrent)
    g_hash_table_unref (priv->id_to_torrent);

  if (priv->session)
    g_object_unref (priv->session);

  g_free (priv->session_id);

  G_OBJECT_CLASS (mex_transmission_plugin_parent_class)->finalize (object);
}

static void
mex_transmission_plugin_class_init (MexTransmissionPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexTransmissionPluginPrivate));

  object_class->finalize = mex_transmission_plugin_finalize;
}

static void
mex_transmission_plugin_init (MexTransmissionPlugin *self)
{
  MexTransmissionPluginPrivate *priv;
  MexModelManager *manager;
  MexModelInfo *model_info;
  MexModelCategoryInfo downloads =
    {
      "downloads", _("Downloads"), "icon-apps", 60,
      _("Add torrents with the Transmission web interface, monitor and play "
        "them from here.")
    };

  self->priv = priv = GET_PRIVATE (self);

  MEX_LOG_DOMAIN_INIT (transmission_log_domain, "transmission");

  priv->id_to_torrent = g_hash_table_new_full (g_int64_hash, g_int64_equal,
                                               free_int64, NULL);
  priv->session = soup_session_async_new ();

  priv->transmission_model = mex_generic_model_new (_("Downloads"),
                                                     "icon-apps");

  model_info = mex_model_info_new_with_sort_funcs (priv->transmission_model,
                                                   "downloads", 0);
  g_object_unref (priv->transmission_model);

  manager = mex_model_manager_get_default ();
  mex_model_manager_add_category (manager, &downloads);
  mex_model_manager_add_model (manager, model_info);
  mex_model_info_free (model_info);

  /* build the update request and cache it */
  build_update_request (self);

  mex_transmission_update_model (self);
  g_timeout_add_seconds (TRANSMISSION_REFRESH_TIMEOUT,
                         mex_transmission_update_model, self);
}

static GType
mex_transmission_get_type (void)
{
  return MEX_TYPE_TRANSMISSION_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Transmission",
		   "Transmission BitTorrent integration",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Damien Lespiau <damien.lespiau@intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_transmission_get_type,
		   MEX_PLUGIN_PRIORITY_DEBUG)
