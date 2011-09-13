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

#ifndef __MEX_METADATA_UTILS_H__
#define __MEX_METADATA_UTILS_H__

#include <glib.h>

#include <mex/mex-content.h>

typedef gboolean (*MexMetadataInfoVisibleCb) (const gchar *value, gpointer user_data);

typedef struct
{
  MexContentMetadata        key;
  const gchar              *key_string;
  gint                      priority;
  const gchar              *value;
  MexMetadataInfoVisibleCb  visible_cb;
  gpointer                  visible_data;
} MexMetadataInfo;

void mex_metadata_from_uri (const gchar *uri,
                            gchar      **title,
                            gchar      **showname,
                            GDate      **date,
                            gint        *season,
                            gint        *episode);

gchar * mex_metadata_humanise_duration (const gchar *duration);

gchar *mex_metadata_humanise_date (const gchar *iso8601_date);

gchar *mex_metadata_humanise_time (const gchar *time);

void mex_metadata_get_metadata (GList **metadata_template, MexContent *content);

MexMetadataInfo *mex_metadata_info_new (MexContentMetadata key,
                                        const gchar *key_string,
                                        gint priority);

MexMetadataInfo *mex_metadata_info_new_with_visibility (MexContentMetadata key,
                                                        const gchar *key_string,
                                                        gint priority,
                                                        MexMetadataInfoVisibleCb visible_cb,
                                                        gpointer user_data);

void mex_metadata_info_free (MexMetadataInfo *info);

gboolean mex_metadata_info_get_visible (MexMetadataInfo *info,
                                        const gchar *value);

#endif
