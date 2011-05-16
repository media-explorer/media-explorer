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


#ifndef __MEX_CONTENT_H__
#define __MEX_CONTENT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_CONTENT              (mex_content_get_type ())
#define MEX_CONTENT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_CONTENT, MexContent))
#define MEX_IS_CONTENT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_CONTENT))
#define MEX_CONTENT_IFACE(iface)      (G_TYPE_CHECK_CLASS_CAST ((iface), MEX_TYPE_CONTENT, MexContentIface))
#define MEX_IS_CONTENT_IFACE(iface)   (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_CONTENT))
#define MEX_CONTENT_GET_IFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MEX_TYPE_CONTENT, MexContentIface))

typedef struct _MexContent         MexContent;
typedef struct _MexContentIface    MexContentIface;
typedef struct _MexContentProperty MexContentProperty;

typedef enum {
    MEX_CONTENT_METADATA_NONE,
    MEX_CONTENT_METADATA_SERIES_NAME,
    MEX_CONTENT_METADATA_TITLE,
    MEX_CONTENT_METADATA_SUB_TITLE,
    MEX_CONTENT_METADATA_SEASON,
    MEX_CONTENT_METADATA_EPISODE,
    MEX_CONTENT_METADATA_STATION_ID,
    MEX_CONTENT_METADATA_STATION_LOGO,
    MEX_CONTENT_METADATA_STILL,
    MEX_CONTENT_METADATA_SYNOPSIS,
    MEX_CONTENT_METADATA_YEAR,
    MEX_CONTENT_METADATA_DURATION,
    MEX_CONTENT_METADATA_URL, /* This is the URL of the page
                                 with the player on it*/
    MEX_CONTENT_METADATA_PLAYER, /* This is the URL of the player object */
    MEX_CONTENT_METADATA_STREAM, /* This is the URL of the video stream */
    MEX_CONTENT_METADATA_STUDIO,
    MEX_CONTENT_METADATA_DIRECTOR,
    MEX_CONTENT_METADATA_MIMETYPE,
    MEX_CONTENT_METADATA_COPYRIGHT,
    MEX_CONTENT_METADATA_COPYRIGHT_URL,
    MEX_CONTENT_METADATA_PRICE,
    MEX_CONTENT_METADATA_PRICE_TYPE,
    MEX_CONTENT_METADATA_PRICE_CURRENCY,
    MEX_CONTENT_METADATA_LICENSE,
    MEX_CONTENT_METADATA_LICENSE_URL,
    MEX_CONTENT_METADATA_VALID_FROM,
    MEX_CONTENT_METADATA_VALID_UNTIL,
    MEX_CONTENT_METADATA_ID,
    MEX_CONTENT_METADATA_QUEUED, /* whether we're queued or not */
    MEX_CONTENT_METADATA_DATE,
    MEX_CONTENT_METADATA_CREATION_DATE,
    MEX_CONTENT_METADATA_CAMERA_MODEL,
    MEX_CONTENT_METADATA_ORIENTATION,
    MEX_CONTENT_METADATA_FLASH_USED,
    MEX_CONTENT_METADATA_EXPOSURE_TIME,
    MEX_CONTENT_METADATA_ISO_SPEED,
    MEX_CONTENT_METADATA_HEIGHT,
    MEX_CONTENT_METADATA_WIDTH,
    MEX_CONTENT_METADATA_LAST_POSITION,
    MEX_CONTENT_METADATA_PLAY_COUNT,
    MEX_CONTENT_METADATA_LAST_PLAYED_DATE,
    MEX_CONTENT_METADATA_ALBUM,
    MEX_CONTENT_METADATA_ARTIST,

    MEX_CONTENT_METADATA_LAST_ID
} MexContentMetadata;

struct _MexContentIface
{
  GTypeInterface g_iface;

  /* virtual functions */
  GParamSpec *  (*get_property)           (MexContent         *content,
                                           MexContentMetadata  key);
  const gchar * (*get_metadata)           (MexContent         *content,
                                           MexContentMetadata  key);
  void          (*set_metadata)           (MexContent         *content,
                                           MexContentMetadata  key,
                                           const gchar        *value);
  gchar *       (*get_metadata_fallback)  (MexContent         *content,
                                           MexContentMetadata  key);
  const gchar * (*get_property_name)      (MexContent          *content,
                                           MexContentMetadata  key);

  void          (*save_metadata)          (MexContent *content);
};

GType         mex_content_get_type                (void) G_GNUC_CONST;

GParamSpec *  mex_content_get_property            (MexContent         *list,
                                                   MexContentMetadata  key);
const gchar * mex_content_get_metadata            (MexContent         *list,
                                                   MexContentMetadata  key);
gchar *       mex_content_get_metadata_fallback   (MexContent        *list,
                                                   MexContentMetadata key);
void          mex_content_set_metadata            (MexContent         *list,
                                                   MexContentMetadata  key,
                                                   const gchar        *value);
void          mex_content_save_metadata           (MexContent         *list);

const char *  mex_content_get_property_name       (MexContent         *content,
                                                   MexContentMetadata  key);
void          mex_content_set_last_used_metadatas (MexContent *content);

const gchar * mex_content_metadata_key_to_string  (MexContentMetadata key);

G_END_DECLS

#endif /* __MEX_CONTENT_H__ */
