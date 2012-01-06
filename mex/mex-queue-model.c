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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <json-glib/json-glib.h>

#include "mex-queue-model.h"
#include "mex-program.h"

G_DEFINE_TYPE (MexQueueModel, mex_queue_model, MEX_TYPE_GENERIC_MODEL)

#define QUEUE_MODEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_QUEUE_MODEL, MexQueueModelPrivate))


static void mex_queue_model_save (MexQueueModel *model);
static void mex_queue_model_load (MexQueueModel *model);

static void _controller_changed_cb (GController          *controller,
                                    GControllerAction     action,
                                    GControllerReference *ref,
                                    MexQueueModel        *model);

struct _MexQueueModelPrivate
{
  GController *controller;
  guint serialise_idle_id;
};


static void
mex_queue_model_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_queue_model_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_queue_model_dispose (GObject *object)
{
  MexQueueModel *model = MEX_QUEUE_MODEL (object);
  MexQueueModelPrivate *priv = model->priv;

  if (priv->controller)
    {
      g_signal_handlers_disconnect_by_func (priv->controller,
                                            _controller_changed_cb,
                                            model);
      priv->controller = NULL;
    }

  G_OBJECT_CLASS (mex_queue_model_parent_class)->dispose (object);
}

static void
mex_queue_model_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_queue_model_parent_class)->finalize (object);
}

static void
mex_queue_model_class_init (MexQueueModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexQueueModelPrivate));

  object_class->get_property = mex_queue_model_get_property;
  object_class->set_property = mex_queue_model_set_property;
  object_class->dispose = mex_queue_model_dispose;
  object_class->finalize = mex_queue_model_finalize;
}

static gboolean
_serialise_idle_cb (MexQueueModel *model)
{
  MexQueueModelPrivate *priv = model->priv;

  mex_queue_model_save (model);

  priv->serialise_idle_id = 0;

  return FALSE;
}

static void
_controller_changed_cb (GController          *controller,
                        GControllerAction     action,
                        GControllerReference *ref,
                        MexQueueModel        *model)
{
  MexQueueModelPrivate *priv = model->priv;
  guint index_;
  MexContent *content;

  /*
   * MexGenericContent only does single items at a time so we have an
   * assumption here that our reference only contains a single item
   */

  if (action == G_CONTROLLER_ADD || action == G_CONTROLLER_REMOVE)
    {
      index_ = g_controller_reference_get_index_uint (ref, 0);
      content = mex_model_get_content (MEX_MODEL (model), index_);
    }

  if (action == G_CONTROLLER_ADD)
    {
      mex_content_set_metadata (content,
                                MEX_CONTENT_METADATA_QUEUED,
                                "yes");
    }
  else if (action == G_CONTROLLER_REMOVE)
    {
      mex_content_set_metadata (content,
                                MEX_CONTENT_METADATA_QUEUED,
                                NULL);
    }
  else if (action == G_CONTROLLER_CLEAR)
    {
      gint model_length;
      model_length = mex_model_get_length (MEX_MODEL (model));

      for (index_=0; index_ < model_length; index_++)
        {
          content = mex_model_get_content (MEX_MODEL (model), index_);

          mex_content_set_metadata (content,
                                    MEX_CONTENT_METADATA_QUEUED,
                                    NULL);
        }
    }
  else
    {
      GEnumClass *enum_class;

      enum_class = g_type_class_ref (G_TYPE_CONTROLLER_ACTION);

      g_critical (G_STRLOC ": Unexpected GController action: %s",
                  (g_enum_get_value (enum_class,
                                     action))->value_name);

      g_type_class_unref (enum_class);
    }

  if (priv->serialise_idle_id)
    return;

  /* Need to use a high priority idle here since we want to try and write
   * *after* the content has been removed from the model
   */
  priv->serialise_idle_id = g_idle_add_full (G_PRIORITY_DEFAULT,
                                             (GSourceFunc)_serialise_idle_cb,
                                             g_object_ref (model),
                                             g_object_unref);
}

static void
mex_queue_model_init (MexQueueModel *self)
{
  MexQueueModelPrivate *priv;

  self->priv = QUEUE_MODEL_PRIVATE (self);
  priv = self->priv;

  /* Load before setting up the controller otherwise .. BOOM! */
  mex_queue_model_load (self);

  priv->controller = mex_model_get_controller (MEX_MODEL (self));
  g_signal_connect (priv->controller,
                    "changed",
                    (GCallback)_controller_changed_cb,
                    self);

  g_object_set (self, "title", _("Queue"), NULL);
}

/**
 * mex_queue_model_dup_singleton:
 *
 * Returns: a referenced queue model, you should use g_object_unref when you
 * have finished with it.
 */
MexModel *
mex_queue_model_dup_singleton (void)
{
  static MexModel *model = NULL;

  if (model)
    return g_object_ref (model);

  model = g_object_new (MEX_TYPE_QUEUE_MODEL, NULL);
  g_object_add_weak_pointer (G_OBJECT (model), (gpointer)&model);

  return model;
}

static JsonGenerator *
_model_to_generator (MexQueueModel *model)
{
  gint i;
  JsonArray *json_array;
  JsonNode *root;
  JsonGenerator *generator;

  json_array = json_array_sized_new (mex_model_get_length (MEX_MODEL (model)));

  for (i = 0; i < mex_model_get_length (MEX_MODEL (model)); i++)
    {
      MexContent *content;
      JsonNode *content_node;

      content = mex_model_get_content (MEX_MODEL (model), i);

      content_node = json_gobject_serialize (G_OBJECT (content));
      json_array_add_element (json_array, content_node);
    }

  generator = json_generator_new ();
  root = json_node_new (JSON_NODE_ARRAY);
  json_node_set_array (root, json_array);

  json_generator_set_root (generator, root);

  json_array_unref (json_array);
  json_node_free (root);

  return generator;
}

static gchar *
_queue_file_name (void)
{
  gchar *directory;
  gchar *filename;

  directory = g_build_filename (g_get_user_data_dir (),
                               "mex",
                               NULL);
  g_mkdir_with_parents (directory, 0775);

  filename = g_build_filename (g_get_user_data_dir (),
                               "mex",
                               "queue.json",
                               NULL);

  g_free (directory);

  return filename;
}

static void
_file_replaced_cb (GFile        *f,
                   GAsyncResult *res,
                   gchar        *buf)
{
  GError *error = NULL;

  if (!g_file_replace_contents_finish (f, res, NULL, &error))
    {
      g_warning (G_STRLOC ": Unable to replace the queue file: %s",
                 error->message);
      g_clear_error (&error);
    }

  g_free (buf);
}

/* This function is asynchronous .. slightly worried about re-entrancy here */
static void
mex_queue_model_save (MexQueueModel *model)
{
  GFile *f;
  gchar *filename;
  JsonGenerator *generator;
  gchar *buf;
  gsize buf_len;

  filename = _queue_file_name ();
  f = g_file_new_for_path (filename);

  if (mex_model_get_length (MEX_MODEL (model)) == 0)
    {
      GError *error = NULL;

      if (!g_file_delete (f, NULL, &error))
        {
          g_warning (G_STRLOC ": Unable to delete file: %s",
                     error->message);
          g_clear_error (&error);
        }

      g_object_unref (f);
      g_free (filename);
      return;
    }

  generator = _model_to_generator (model);

  buf = json_generator_to_data (generator, &buf_len);

  g_file_replace_contents_async (f,
                                 buf,
                                 buf_len,
                                 NULL,
                                 FALSE,
                                 G_FILE_CREATE_REPLACE_DESTINATION,
                                 NULL,
                                 (GAsyncReadyCallback)_file_replaced_cb,
                                 buf);

  g_object_unref (f);
  g_free (filename);
  g_object_unref (generator);
}

/* This function is synchronous! Blocking once at startup seems pretty
 * reasonable and allows us to avoid any complexity re. races
 */
static void
mex_queue_model_load (MexQueueModel *model)
{
  JsonParser *parser;
  gchar *filename;
  GError *error = NULL;
  JsonNode *root;
  JsonArray *array;
  gint i = 0;

  filename = _queue_file_name ();

  if (!g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      g_free (filename);

      return;
    }

  parser = json_parser_new ();
  if (!json_parser_load_from_file (parser, filename, &error))
    {
      g_warning (G_STRLOC ": error populating from file: %s",
                 error->message);
      g_clear_error (&error);
      goto out;
    }

  root = json_parser_get_root (parser);

  if (!JSON_NODE_HOLDS_ARRAY (root))
    {
      g_warning (G_STRLOC ": JSON data not of expected format!");

      goto out;
    }

  array = json_node_get_array (root);

  for (i = 0; i < json_array_get_length (array); i++)
    {
      MexContent *content;
      JsonNode *node;

      node = json_array_get_element (array, i);
      content = (MexContent *)json_gobject_deserialize (MEX_TYPE_PROGRAM,
                                                        node);

      mex_model_add_content (MEX_MODEL (model), content);
    }

out:
  g_free (filename);
  g_object_unref (parser);
}

