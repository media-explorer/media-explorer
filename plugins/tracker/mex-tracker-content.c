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

#include "mex-debug.h"
#include "mex-tracker-content.h"
#include "mex-tracker-queue.h"
#include "mex-tracker-utils.h"

#define TRACKER_SAVE_REQUEST                            \
  "DELETE { <%s> %s } WHERE { <%s> a nfo:Media . %s } " \
  "INSERT { <%s> a nfo:Media ; %s . }"


static void mex_content_iface_init (MexContentIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexTrackerContent, mex_tracker_content,
                         MEX_TYPE_GENERIC_CONTENT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                mex_content_iface_init))

#define TRACKER_CONTENT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TRACKER_CONTENT, MexTrackerContentPrivate))

enum {
  PROP_0,

  PROP_IN_SETUP
};

struct _MexTrackerContentPrivate
{
  MexContentIface *parent_iface;

  gboolean in_setup;

  GList *modified_keys; /* list(MexContentMetadata) */
};

static void mex_tracker_content_set_metadata  (MexContent         *content,
                                               MexContentMetadata  key,
                                               const gchar        *value);
static void mex_tracker_content_save_metadata (MexContent *content);

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->set_metadata = mex_tracker_content_set_metadata;
  iface->save_metadata = mex_tracker_content_save_metadata;
}

static void
mex_tracker_content_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MexTrackerContentPrivate *priv = TRACKER_CONTENT_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IN_SETUP:
      g_value_set_boolean (value, priv->in_setup);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tracker_content_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MexTrackerContentPrivate *priv = TRACKER_CONTENT_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IN_SETUP:
      priv->in_setup = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_tracker_content_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_content_parent_class)->dispose (object);
}

static void
mex_tracker_content_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_tracker_content_parent_class)->finalize (object);
}

static void
mex_tracker_content_constructed (GObject *object)
{
  MexTrackerContentPrivate *priv = TRACKER_CONTENT_PRIVATE (object);
  MexContentIface          *iface;

  if (G_OBJECT_CLASS (mex_tracker_content_parent_class)->constructed)
    G_OBJECT_CLASS (mex_tracker_content_parent_class)->constructed (object);

  iface = MEX_CONTENT_GET_IFACE (object);
  priv->parent_iface = g_type_interface_peek_parent (iface);
}

static void
mex_tracker_content_class_init (MexTrackerContentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexTrackerContentPrivate));

  object_class->get_property = mex_tracker_content_get_property;
  object_class->set_property = mex_tracker_content_set_property;
  object_class->dispose = mex_tracker_content_dispose;
  object_class->finalize = mex_tracker_content_finalize;
  object_class->constructed = mex_tracker_content_constructed;

  pspec = g_param_spec_boolean ("in-setup", "In Setup",
                                "Indicate whether the current content is being setup",
                                TRUE,
                                G_PARAM_READWRITE |
                                G_PARAM_STATIC_STRINGS |
                                G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_IN_SETUP, pspec);
}

static void
mex_tracker_content_init (MexTrackerContent *self)
{
  MexTrackerContentPrivate *priv = TRACKER_CONTENT_PRIVATE (self);

  self->priv = priv;
}

MexContent *
mex_tracker_content_new (void)
{
  return g_object_new (MEX_TYPE_TRACKER_CONTENT, NULL);
}

static void
mex_tracker_content_set_metadata  (MexContent         *content,
                                   MexContentMetadata  key,
                                   const gchar        *value)

{
  MexTrackerContentPrivate *priv = TRACKER_CONTENT_PRIVATE (content);
  GList *metadata_key = priv->modified_keys;

  if (!priv->in_setup)
    {
      while (metadata_key)
        {
          if (GPOINTER_TO_INT (metadata_key->data) == key)
            goto set_metadata;

          metadata_key = metadata_key->next;
        }

      priv->modified_keys = g_list_append (priv->modified_keys,
                                           GINT_TO_POINTER (key));
    }

 set_metadata:
  priv->parent_iface->set_metadata (content, key, value);
}

static void
mex_tracker_content_save_metadata_cb (GObject      *source_object,
                                      GAsyncResult *result,
                                      MexTrackerOp *os)
{
  GError *error = NULL;
  MexTrackerConnection *conn = mex_tracker_get_connection ();

  tracker_sparql_connection_update_finish (
    mex_tracker_connection_get_connection (conn),
    result,
    &error);

  if (error)
    {
      MEX_WARN (TRACKER, "could not save metadata: %s", error->message);
      g_error_free (error);
    }

  mex_tracker_queue_done (mex_tracker_get_queue (), os);
}

static void
mex_tracker_content_save_metadata (MexContent *content)
{
  MexTrackerContentPrivate *priv = TRACKER_CONTENT_PRIVATE (content);
  gchar *sparql_delete, *sparql_cdelete, *sparql_insert, *sparql_final;
  const gchar *urn = mex_content_get_metadata (content,
                                               MEX_CONTENT_METADATA_PRIVATE_ID);
  MexTrackerOp *os;

  sparql_delete = mex_tracker_get_delete_string (priv->modified_keys);
  sparql_cdelete = mex_tracker_get_delete_conditional_string (urn,
                                                              priv->modified_keys);
  sparql_insert = mex_tracker_get_insert_string (content,
                                                 priv->modified_keys);
  sparql_final = g_strdup_printf (TRACKER_SAVE_REQUEST,
                                  urn, sparql_delete,
                                  urn, sparql_cdelete,
                                  urn, sparql_insert);

  os = mex_tracker_op_initiate_set_metadata (sparql_final,
          (GAsyncReadyCallback) mex_tracker_content_save_metadata_cb,
          content);

  g_list_free (priv->modified_keys);
  priv->modified_keys = NULL;

  g_message ("request: '%s'", sparql_final);

  mex_tracker_queue_push (mex_tracker_get_queue (), os);

  g_free (sparql_delete);
  g_free (sparql_cdelete);
  g_free (sparql_insert);
}

void
mex_tracker_content_set_in_setup (MexTrackerContent *content, gboolean in_setup)
{
  MexTrackerContentPrivate *priv;

  g_return_if_fail (MEX_IS_TRACKER_CONTENT (content));

  priv = content->priv;

  priv->in_setup = in_setup;
}

gboolean
mex_tracker_content_get_in_setup (MexTrackerContent *content)
{
  MexTrackerContentPrivate *priv;

  g_return_if_fail (MEX_IS_TRACKER_CONTENT (content));

  priv = content->priv;

  return priv->in_setup;
}
