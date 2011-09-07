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


/**
 * SECTION:mex-content
 * @short_description: Objects that can be put in #MexContentBox
 *
 * Implementing #MexContent means that the class can be displayed in a
 * #MexContentBox.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "mex-private.h"
#include "mex-enum-types.h"
#include "mex-content.h"

static void
mex_content_base_finalize (gpointer g_iface)
{
}

static void
mex_content_base_init (gpointer g_iface)
{
  static gboolean is_initialized = FALSE;
  GParamSpec *param;

  if (!is_initialized) {
    param = g_param_spec_boolean ("last-position-start",
                                  "Last position start",
                                  "Start media content from last position",
                                  TRUE,
                                  G_PARAM_READWRITE);
    g_object_interface_install_property (g_iface, param);

    is_initialized = TRUE;
  }
}

GType
mex_content_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    {
      GTypeInfo content_info = {
        sizeof (MexContentIface),
        mex_content_base_init,
        mex_content_base_finalize
      };

      our_type = g_type_register_static (G_TYPE_INTERFACE,
                                         I_("MexContent"),
                                         &content_info, 0);
    }

  return our_type;
}

/**
 * mex_content_get_property:
 * @content: a #MexContent
 * @key: a #MexContentMetadata
 *
 * Retrieves a #GParamSpec for @key for this content.
 *
 * Return value: The #GParamSpec of the property corresponding to the @key
 *
 * Since: 0.2
 */
GParamSpec *
mex_content_get_property (MexContent         *content,
                          MexContentMetadata  key)
{
  MexContentIface *iface;

  g_return_val_if_fail (MEX_IS_CONTENT (content), NULL);

  iface = MEX_CONTENT_GET_IFACE (content);

  if (G_LIKELY (iface->get_property))
    return iface->get_property (content, key);

  g_warning ("MexContent of type '%s' does not implement get_property()",
             g_type_name (G_OBJECT_TYPE (content)));

  return NULL;
}

/**
 * mex_content_get_metadata:
 * @content: a #MexContent
 * @key: a #MexContentMetadata
 *
 * Retrieves a string for @key for this content.
 *
 * Return value: The string corresponding to the @key
 *
 * Since: 0.2
 */
const gchar *
mex_content_get_metadata (MexContent         *content,
                          MexContentMetadata  key)
{
  MexContentIface *iface;

  g_return_val_if_fail (MEX_IS_CONTENT (content), NULL);

  iface = MEX_CONTENT_GET_IFACE (content);

  if (G_LIKELY (iface->get_metadata))
    return iface->get_metadata (content, key);

  g_warning ("MexContent of type '%s' does not implement get_metadata()",
             g_type_name (G_OBJECT_TYPE (content)));

  return NULL;
}

/**
 * mex_content_get_metadata_fallback:
 * @content: a #MexContent
 * @key: a #MexContentMetadata
 *
 * Retrieves a string for @key for this content.
 *
 * Return value: The string corresponding to the @key. Because this string
 * may be dynamically created it should be freed with g_free when no longer
 * needed.
 *
 * Since: 0.2
 */
gchar *
mex_content_get_metadata_fallback (MexContent        *content,
                                   MexContentMetadata key)
{
  MexContentIface *iface;

  g_return_val_if_fail (MEX_IS_CONTENT (content), NULL);

  iface = MEX_CONTENT_GET_IFACE (content);

  if (G_LIKELY (iface->get_metadata_fallback))
    return iface->get_metadata_fallback (content, key);

  return NULL;
}

/**
 * mex_content_set_metadata:
 * @content: a #MexContent
 * @key: a #MexContentMetadata
 * @value: a string corresponding to the new value of @key
 *
 * Sets a string for @key for this content.
 *
 * Since: 0.2
 */
void
mex_content_set_metadata (MexContent         *content,
                          MexContentMetadata  key,
                          const gchar        *value)
{
  MexContentIface *iface;

  g_return_if_fail (MEX_IS_CONTENT (content));

  iface = MEX_CONTENT_GET_IFACE (content);

  if (G_LIKELY (iface->set_metadata)) {
    iface->set_metadata (content, key, value);
    return;
  }

  g_warning ("MexContent of type '%s' does not implement set_metadata()",
             g_type_name (G_OBJECT_TYPE (content)));
}

/**
 * mex_content_get_property_name:
 * @key: The key ID
 *
 * Retrieves the property name for @key.
 *
 * It's possible to return %NULL when the #MexContent does not want to
 * associate a GObject property with a #MexContentMetadata key. This means,
 * for instance that it will disable the #GBindings that are setup
 * automatically by wigets like #MexContentBox or #MexContentButton.
 *
 * The default implementation returns %NULL for any @key value given.
 *
 * Return value: The property name or %NULL.
 */
const char *
mex_content_get_property_name (MexContent         *content,
                               MexContentMetadata  key)
{
  MexContentIface *iface;

  g_return_val_if_fail (MEX_IS_CONTENT (content), NULL);

  iface = MEX_CONTENT_GET_IFACE (content);

  if (iface->get_property_name)
    return iface->get_property_name (content, key);

  return NULL;
}

/**
 * mex_content_save_metadata:
 * @content: a #MexContent
 *
 * Save all metadata into underlaying backend.
 *
 * Since: 0.2
 */
void
mex_content_save_metadata (MexContent *content)
{
  MexContentIface *iface;

  g_return_if_fail (MEX_IS_CONTENT (content));

  iface = MEX_CONTENT_GET_IFACE (content);

  if (iface->save_metadata) {
    iface->save_metadata (content);
    return;
  }

  g_warning ("MexContent of type '%s' does not implement save_metadata()",
             g_type_name (G_OBJECT_TYPE (content)));
}

void
mex_content_foreach_metadata (MexContent           *content,
                              MexContentMetadataCb  callback,
                              gpointer              data)
{
  MexContentIface *iface;

  g_return_if_fail (MEX_IS_CONTENT (content));
  g_return_if_fail (callback != NULL);

  iface = MEX_CONTENT_GET_IFACE (content);

  if (iface->foreach_metadata) {
    iface->foreach_metadata (content, callback, data);
    return;
  }

  g_warning ("MexContent of type '%s' does not implement foreach_metadata()",
             g_type_name (G_OBJECT_TYPE (content)));
}

/**
 * mex_content_open:
 * @content: a #MexContent
 *
 * Open a content.
 *
 * Since: 0.2
 */
void
mex_content_open (MexContent *content, MexModel *context)
{
  MexContentIface *iface;

  g_return_if_fail (MEX_IS_CONTENT (content));

  iface = MEX_CONTENT_GET_IFACE (content);

  if (iface->open) {
    iface->open (content, context);
    return;
  }

  g_warning ("MexContent of type '%s' does not implement open()",
             g_type_name (G_OBJECT_TYPE (content)));
}

void
mex_content_set_last_used_metadatas (MexContent *content)
{
  guint count;
  gchar str[20], *nstr;
  const gchar *play_count;
  GDateTime *datetime;
  GTimeVal tv;

  play_count = mex_content_get_metadata (content,
                                         MEX_CONTENT_METADATA_PLAY_COUNT);
  if (play_count) {
    count = atoi (play_count);
    count++;
  } else
    count = 1;
  snprintf (str, sizeof (str), "%u", count);
  mex_content_set_metadata (content, MEX_CONTENT_METADATA_PLAY_COUNT, str);

  datetime = g_date_time_new_now_local ();
  if (datetime) {
    if (g_date_time_to_timeval (datetime, &tv)) {
      tv.tv_usec = 0;
      nstr = g_time_val_to_iso8601 (&tv);
      if (nstr) {
        mex_content_set_metadata (content,
                                  MEX_CONTENT_METADATA_LAST_PLAYED_DATE,
                                  nstr);
        g_free (nstr);
      }
    }
    g_date_time_unref (datetime);
  }
}

const gchar *
mex_content_metadata_key_to_string (MexContentMetadata key)
{
  return mex_enum_to_string (MEX_TYPE_CONTENT_METADATA, key);
}
