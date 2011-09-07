#include "mex-grilo.h"

#include <stdlib.h>
#include "mex-metadata-utils.h"

static GHashTable *mex_to_grl;

/**/

static void
_insert_grl_mex_link (GrlKeyID grl_key, MexContentMetadata mex_key)
{
  /* g_hash_table_insert (grl_to_mex, */
  /*                      GSIZE_TO_POINTER (grl_key), */
  /*                      GSIZE_TO_POINTER (mex_key)); */
  g_hash_table_insert (mex_to_grl,
                       GSIZE_TO_POINTER (mex_key),
                       GSIZE_TO_POINTER (grl_key));
}

void
mex_grilo_init (int *p_argc, char **p_argv[])
{
  grl_init (p_argc, p_argv);

  /* grl_to_mex = g_hash_table_new (g_direct_hash, g_direct_equal); */
  mex_to_grl = g_hash_table_new (g_direct_hash, g_direct_equal);

  _insert_grl_mex_link (GRL_METADATA_KEY_SHOW,
                        MEX_CONTENT_METADATA_SERIES_NAME);
  _insert_grl_mex_link (GRL_METADATA_KEY_DESCRIPTION,
                        MEX_CONTENT_METADATA_SYNOPSIS);
  _insert_grl_mex_link (GRL_METADATA_KEY_TITLE,
                        MEX_CONTENT_METADATA_TITLE);
  _insert_grl_mex_link (GRL_METADATA_KEY_SEASON,
                        MEX_CONTENT_METADATA_SEASON);
  _insert_grl_mex_link (GRL_METADATA_KEY_EPISODE,
                        MEX_CONTENT_METADATA_EPISODE);
  _insert_grl_mex_link (GRL_METADATA_KEY_EPISODE,
                        MEX_CONTENT_METADATA_EPISODE);
  _insert_grl_mex_link (GRL_METADATA_KEY_DURATION,
                        MEX_CONTENT_METADATA_DURATION);
  _insert_grl_mex_link (GRL_METADATA_KEY_URL,
                        MEX_CONTENT_METADATA_STREAM);
  _insert_grl_mex_link (GRL_METADATA_KEY_DATE,
                        MEX_CONTENT_METADATA_DATE);
  _insert_grl_mex_link (GRL_METADATA_KEY_MIME,
                        MEX_CONTENT_METADATA_MIMETYPE);
  _insert_grl_mex_link (GRL_METADATA_KEY_THUMBNAIL,
                        MEX_CONTENT_METADATA_STILL);
  _insert_grl_mex_link (GRL_METADATA_KEY_LAST_POSITION,
                        MEX_CONTENT_METADATA_LAST_POSITION);
  _insert_grl_mex_link (GRL_METADATA_KEY_PLAY_COUNT,
                        MEX_CONTENT_METADATA_PLAY_COUNT);
  _insert_grl_mex_link (GRL_METADATA_KEY_LAST_PLAYED,
                        MEX_CONTENT_METADATA_LAST_PLAYED_DATE);
  _insert_grl_mex_link (GRL_METADATA_KEY_WIDTH,
                        MEX_CONTENT_METADATA_WIDTH);
  _insert_grl_mex_link (GRL_METADATA_KEY_HEIGHT,
                        MEX_CONTENT_METADATA_HEIGHT);
  _insert_grl_mex_link (GRL_METADATA_KEY_CAMERA_MODEL,
                        MEX_CONTENT_METADATA_CAMERA_MODEL);
  _insert_grl_mex_link (GRL_METADATA_KEY_ORIENTATION,
                        MEX_CONTENT_METADATA_ORIENTATION);
  _insert_grl_mex_link (GRL_METADATA_KEY_FLASH_USED,
                        MEX_CONTENT_METADATA_FLASH_USED);
  _insert_grl_mex_link (GRL_METADATA_KEY_EXPOSURE_TIME,
                        MEX_CONTENT_METADATA_EXPOSURE_TIME);
  _insert_grl_mex_link (GRL_METADATA_KEY_ISO_SPEED,
                        MEX_CONTENT_METADATA_ISO_SPEED);
  _insert_grl_mex_link (GRL_METADATA_KEY_CREATION_DATE,
                        MEX_CONTENT_METADATA_CREATION_DATE);
  _insert_grl_mex_link (GRL_METADATA_KEY_ARTIST,
                        MEX_CONTENT_METADATA_ARTIST);
  _insert_grl_mex_link (GRL_METADATA_KEY_ALBUM,
                        MEX_CONTENT_METADATA_ALBUM);
}

static GrlKeyID
_get_grl_key_from_mex (MexContentMetadata mex_key)
{
  return (GrlKeyID) g_hash_table_lookup (mex_to_grl,
                                         GSIZE_TO_POINTER (mex_key));
}

/* static MexContentMetadata */
/* _get_mex_key_from_grl (GrlKeyID grl_key) */
/* { */
/*   return (MexContentMetadata) g_hash_table_lookup (grl_to_mex, */
/*                                                    GSIZE_TO_POINTER (grl_key)); */
/* } */

static void
set_metadata_from_media (MexContent          *content,
                         GrlMedia            *media,
                         MexContentMetadata   mex_key)
{
  gchar       *string;
  const gchar *cstring;
  GrlKeyID     grl_key = _get_grl_key_from_mex (mex_key);
  gint n;

  if (!grl_key)
    return;

  switch (G_PARAM_SPEC (grl_key)->value_type) {
  case G_TYPE_STRING:
    cstring = grl_data_get_string (GRL_DATA (media), grl_key);

    if (cstring)
      {
        if (mex_key == MEX_CONTENT_METADATA_TITLE)
          {
            gchar *showname = NULL, *title;
            gint season, episode;
            gchar *replacement;
            const gchar *mimetype;

            mimetype = mex_content_get_metadata (content,
                                                 MEX_CONTENT_METADATA_MIMETYPE);

            if (!mimetype)
              mimetype = "";

            if (g_str_has_prefix (mimetype, "video/"))
              {
                mex_metadata_from_uri (cstring, &title, &showname, NULL,
                                       &season, &episode);
              }

            if (showname)
              {
                replacement = g_strdup_printf ("%s - S%d - E%d",
                                               showname, season, episode);
              }
            else
              {
                GRegex *regex;

                /* strip off any file extensions */
                regex = g_regex_new ("\\.....?$", 0, 0, NULL);
                replacement = g_regex_replace (regex, cstring, -1, 0, "", 0, NULL);

                g_regex_unref (regex);
              }

            if (!replacement)
              replacement = g_strdup (cstring);

            mex_content_set_metadata (content, mex_key, replacement);

            g_free (replacement);
          }
        else
          mex_content_set_metadata (content, mex_key, cstring);
      }
    break;

  case G_TYPE_INT:
    n = grl_data_get_int (GRL_DATA (media), grl_key);

    if (n > 0)
      {
        string = g_strdup_printf ("%i", n);
        mex_content_set_metadata (content, mex_key, string);
        g_free (string);
      }
    break;

  case G_TYPE_FLOAT:
    string = g_strdup_printf ("%f", grl_data_get_float (GRL_DATA (media),
                                                        grl_key));
    mex_content_set_metadata (content, mex_key, string);
    g_free (string);
    break;
  }
}

void
mex_grilo_set_media_content_metadata (GrlMedia           *media,
                                      MexContentMetadata  mex_key,
                                      const gchar        *value)
{
  int      ival;
  float    fval;
  GrlKeyID grl_key;

  g_return_if_fail (GRL_IS_MEDIA (media));
  g_return_if_fail (mex_key < MEX_CONTENT_METADATA_LAST_ID);

  grl_key = _get_grl_key_from_mex (mex_key);

  if (!grl_key) {
    g_warning ("No grilo key to handle %s",
               mex_content_metadata_key_to_string (mex_key));
    return;
  }

  switch (G_PARAM_SPEC (grl_key)->value_type) {
  case G_TYPE_STRING:
    grl_data_set_string (GRL_DATA (media), grl_key, value);
    break;

  case G_TYPE_INT:
    ival = atoi (value);
    grl_data_set_int (GRL_DATA (media), grl_key, ival);
    break;

  case G_TYPE_FLOAT:
    fval = atof (value);
    grl_data_set_float (GRL_DATA (media), grl_key, fval);
    break;
  }
}

void
mex_grilo_update_content_from_media (MexContent *content,
                                     GrlMedia   *media)
{
  g_return_if_fail (MEX_IS_CONTENT (content));
  g_return_if_fail (GRL_IS_MEDIA (media));

  /* FIXME: This list is just hard-coded and needs to be the same as
   *        the default set of keys in MexGriloFeed... Grilo is likely
   *        to add an API to retrieve all setted keys, we might want
   *        to use that.
   */
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_TITLE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_SYNOPSIS);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_MIMETYPE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_STILL);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_STREAM);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_WIDTH);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_HEIGHT);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_DATE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_DURATION);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_LAST_POSITION);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_PLAY_COUNT);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_LAST_PLAYED_DATE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_CAMERA_MODEL);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_ORIENTATION);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_FLASH_USED);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_EXPOSURE_TIME);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_ISO_SPEED);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_CREATION_DATE);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_ALBUM);
  set_metadata_from_media (content, media, MEX_CONTENT_METADATA_ARTIST);
}


