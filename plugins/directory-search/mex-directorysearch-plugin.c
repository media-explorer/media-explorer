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

/* Small object to Add and Remove a MexContent item that represents a DVD */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>
#include <mex/mex.h>
#include <clutter/clutter.h>
#include <sys/types.h>
#include <stdlib.h>
#include <gst/gst.h>

#include "mex-directorysearch-plugin.h"

#include <glib/gi18n-lib.h>

static void model_provider_iface_init (MexModelProviderInterface *interface);
static void action_provider_iface_init (MexActionProviderInterface *iface);
static void _new_file_added (MexDirectorySearchManager *self, const gchar *file_path);


G_DEFINE_TYPE_WITH_CODE (MexDirectorySearchManager,
                         mex_directory_search_manager,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                action_provider_iface_init))

#define DIRECTORY_SEARCH_MANAGER_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_DIRECTORY_SEARCH_MANAGER, MexDirectorySearchManagerPrivate))

static void _volume_monitor_mount_added_cb (GVolumeMonitor           *volume_monitor,
                                            GMount                   *mount,
                                            MexDirectorySearchManager *self);

static void _volume_monitor_mount_removed_cb (GVolumeMonitor           *volume_monitor,
                                              GMount                   *mount,
                                              MexDirectorySearchManager *self);

static const gchar *mex_disc_mimetypes[] = { "video/dvd" , "video/vcd", NULL };

struct _MexDirectorySearchManagerPrivate
{
  GList *models;
  GList *actions;

  MexModel *videos_model;
  MexModel *pictures_model;
  MexModel *music_model;

  MexContent *content;
};

GList *dir_list = NULL, *file_list = NULL;

static void
dir_iterate (const gchar *dir_path)
{
  GError *errormsg = NULL;
  GDir *dir;
  gchar *test_path;
  const gchar *entry;
  

  dir = g_dir_open (dir_path, 0, &errormsg);
  
  if (errormsg != NULL)
    g_warning("Error! %s", errormsg->message);

    
  if (dir != NULL)
    {
     while ((entry = g_dir_read_name (dir)) != NULL)
       {
         test_path = g_strconcat (dir_path, G_DIR_SEPARATOR_S, entry, NULL);
         if (g_file_test (test_path, G_FILE_TEST_IS_DIR) == TRUE)
           {
             dir_list = g_list_append (dir_list, test_path);
           }
         else if (g_file_test (test_path, (G_FILE_TEST_IS_REGULAR
                                           & ~G_FILE_TEST_IS_SYMLINK) == TRUE))
           file_list = g_list_append (file_list, test_path);
       }
    }
  g_dir_close (dir);
}

static void directory_list(MexDirectorySearchManager *self)
{
  GList *list_iterate;
  const gchar *dir_music_path, *dir_pictures_path, *dir_videos_path;

  dir_music_path = g_get_user_special_dir (G_USER_DIRECTORY_MUSIC);
  dir_pictures_path = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  dir_videos_path = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);

  dir_iterate (dir_music_path);
  dir_iterate (dir_pictures_path);
  dir_iterate (dir_videos_path);
  
  for (list_iterate = dir_list; list_iterate != NULL;
       list_iterate = list_iterate->next)
    {
      dir_iterate (list_iterate->data);
    }
  for (list_iterate = file_list; list_iterate != NULL;
       list_iterate = list_iterate->next)
    {
      //g_print("File: %s\n", (gchar *)list_iterate->data);
      _new_file_added (self, list_iterate->data);
    }
  g_list_free_full (dir_list, g_free);
  g_list_free_full (file_list, g_free);
}




static void
mex_directory_search_manager_dispose (GObject *object)
{
  MexDirectorySearchManager *directory_search_manager = MEX_DIRECTORY_SEARCH_MANAGER (object);
  MexDirectorySearchManagerPrivate *priv = MEX_DIRECTORY_SEARCH_MANAGER (directory_search_manager)->priv;

  if (priv->content)
    {
      g_object_unref (priv->content);
      priv->content = NULL;
    }
  while (priv->models)
    {
      g_object_unref (priv->models->data);
      priv->models = g_list_delete_link (priv->models, priv->models);
    }

  while (priv->actions)
    {
      MexActionInfo *info = priv->actions->data;

      g_object_unref (info->action);
      g_strfreev (info->mime_types);
      g_free (info);

      priv->actions = g_list_delete_link (priv->actions, priv->actions);
    }
}

static void
mex_directory_search_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_directory_search_manager_parent_class)->finalize (object);
}

static void
mex_directory_search_manager_class_init (MexDirectorySearchManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexDirectorySearchManagerPrivate));

  object_class->dispose = mex_directory_search_manager_dispose;
  object_class->finalize = mex_directory_search_manager_finalize;
}
/*
 * Callback from mex_thumbnailer_generate, when a thumbnail has been generated.
 */
static void
thumbnail_cb (const char *uri, gpointer user_data)
{
  MexContent *content;
  MexDirectorySearchManagerPrivate *priv;
  char *thumb_path;

  content = MEX_CONTENT (user_data);

  thumb_path = mex_get_thumbnail_path_for_uri (uri);

  if (g_file_test (thumb_path, G_FILE_TEST_EXISTS))
    {
      gchar *thumb_uri = g_filename_to_uri (thumb_path, NULL, NULL);

      //priv->in_update = TRUE;

      mex_content_set_metadata (content,
                                MEX_CONTENT_METADATA_STILL,
                                thumb_uri);

      //priv->in_update = FALSE;

      g_free (thumb_uri);
    }

  g_free (thumb_path);
}

/*
 * If we need to, generate a thumbnail.
 *
 * If the URL is not file:/// then we assume it's okay to use.
 *
 */
static void
mex_directory_search_thumbnail (MexContent *content, const gchar *mime_type, 
                                const gchar *uri_path)
{
  const char *old_thumb_url;
  char *thumb_path;
  static gchar *folder_thumb_uri = NULL;

  /* If the media isn't local, then we'll ignore it for now */

  old_thumb_url = mex_content_get_metadata (content,
                                            MEX_CONTENT_METADATA_STILL);
 
  thumb_path = mex_get_thumbnail_path_for_uri (uri_path);

  if (g_file_test (thumb_path, G_FILE_TEST_EXISTS))
    {
      gchar *thumb_url = g_filename_to_uri (thumb_path, NULL, NULL);
      if (!old_thumb_url || strcmp (thumb_url, old_thumb_url) != 0)
        mex_content_set_metadata (content, MEX_CONTENT_METADATA_STILL,
                                  thumb_url);
      g_free (thumb_url);
    }
  else
    {
      mex_thumbnailer_generate (uri_path, mime_type,
                                thumbnail_cb, content);
    }
  g_free (thumb_path);
}

static void
set_metadata_from_media (MexContent *content,
                         const gchar *uri_path,
                         MexContentMetadata mex_key)
{
  MexDirectorySearchManagerPrivate *priv;
  gchar *showname = NULL, *title, *season_str;
  gint season, episode, year = 0;
  gchar *replacement;
  const gchar *mimetype;

  if (uri_path)
    {
      if (mex_key == MEX_CONTENT_METADATA_TITLE)
        {
          mimetype = mex_content_get_metadata (content,
                                               MEX_CONTENT_METADATA_MIMETYPE);
          if (!mimetype)
            mimetype = "";

          if (g_str_has_prefix (mimetype, "video/"))
            {
              mex_metadata_from_uri (uri_path, &title, &showname, &year,
                                     &season, &episode);
            }

          if (showname)
            {
              replacement = g_strdup_printf (_("Episode %d"), episode);
            }
          else
            {
              GRegex *regex;

              /* strip off any file extensions */
              regex = g_regex_new ("\\.....?$", 0, 0, NULL);
              replacement = g_regex_replace (regex, uri_path, -1, 0, "", 0, NULL);

              g_regex_unref (regex);
            }

          if (!replacement)
            replacement = g_strdup (uri_path);

          mex_content_set_metadata (content, MEX_CONTENT_METADATA_TITLE, replacement);
          mex_content_set_metadata (content, MEX_CONTENT_METADATA_SERIES_NAME,
                                    showname);
          season_str = g_strdup_printf (_("Season %d"), season);
          mex_content_set_metadata (content, MEX_CONTENT_METADATA_SEASON,
                                    season_str);
          g_free (season_str);

          if (year)
            {
              replacement = g_strdup_printf ("%d", year);
              mex_content_set_metadata (content, MEX_CONTENT_METADATA_YEAR,
                                        replacement);

              g_free (replacement);
            }
        }
      else
        mex_content_set_metadata (content, mex_key, uri_path);
    }
}

void
mex_update_content_from_media (MexContent *content,
                              const gchar *uri_path)
{
  MexDirectorySearchManagerPrivate *priv;
  //g_return_if_fail (MEX_IS_CONTENT (content));

  /* FIXME: This list is just hard-coded and needs to be the same as
   *        the default set of keys in MexGriloFeed... Grilo is likely
   *        to add an API to retrieve all setted keys, we might want
   *        to use that.
   */
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_TITLE);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_SYNOPSIS);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_MIMETYPE);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_STILL);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_STREAM);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_WIDTH);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_HEIGHT);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_DATE);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_DURATION);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_LAST_POSITION);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_PLAY_COUNT);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_LAST_PLAYED_DATE);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_CAMERA_MODEL);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_ORIENTATION);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_FLASH_USED);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_EXPOSURE_TIME);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_ISO_SPEED);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_CREATION_DATE);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_ALBUM);
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_ARTIST);
}


static void
_new_file_added (MexDirectorySearchManager *self, const gchar *file_path)
{
  GError *errormsg = NULL;
  gchar *uri_path, *name, *filename, *basename, *p;
  const gchar *mimetype;
  MexDirectorySearchManagerPrivate *priv = MEX_DIRECTORY_SEARCH_MANAGER (self)->priv;

  uri_path = g_filename_to_uri (file_path, NULL, &errormsg);

  //g_print ("Uri-path:%s\n", uri_path);

  if (errormsg != NULL)
    g_warning("Error! %s", errormsg->message);

  priv->content = mex_content_from_uri (uri_path);
  mimetype = mex_content_get_metadata (priv->content,
                                       MEX_CONTENT_METADATA_MIMETYPE);
  g_print ("Mimetype: %s\n", mimetype);

  mex_directory_search_thumbnail (priv->content, mimetype, uri_path);
   if (g_str_has_prefix (mimetype, "video/"))
     {
    mex_model_add_content (priv->videos_model, priv->content);
    g_print ("GDB: Video added %p\n", priv->content);
     }
  else if (g_str_has_prefix (mimetype, "image/"))
    mex_model_add_content (priv->pictures_model, priv->content);
  else if (g_str_has_prefix (mimetype, "audio/"))
    mex_model_add_content (priv->music_model, priv->content);

  mex_update_content_from_media (priv->content, uri_path);

  g_free (uri_path);

}

static void
_content_type_resolved (GObject               *mount,
                        GAsyncResult          *result,
                        MexDirectorySearchManager *self)
{
  GError *error = NULL;
  gchar **content_types;

  content_types =
    g_mount_guess_content_type_finish (G_MOUNT (mount), result, &error);

  /* Possible types we could consider when gstreamer supports them:
   * x-content/video-bluray;
   * x-content/video-hddvd;
   * x-content/video-svcd;
   
  if (content_types)
    {
      if (g_strcmp0 (content_types[0], "x-content/video-dvd") == 0
          || g_strcmp0 (content_types[0], "x-content/video-vcd") == 0)
        {
          _new_disc_added (self, G_MOUNT (mount), content_types[0]);
        }
    }
  g_strfreev (content_types);*/
}

static void
_volume_monitor_mount_added_cb (GVolumeMonitor        *volume_monitor,
                                GMount                *mount,
                                MexDirectorySearchManager *self)
{
  g_mount_guess_content_type (mount,
                              FALSE,
                              NULL,
                              (GAsyncReadyCallback)_content_type_resolved,
                              self);
}

static void
_volume_monitor_mount_removed_cb (GVolumeMonitor        *volume_monitor,
                                  GMount                *mount,
                                  MexDirectorySearchManager *self)
{
  g_mount_guess_content_type (mount,
                              FALSE,
                              NULL,
                              (GAsyncReadyCallback)_content_type_resolved,
                              self);
}

static void
_eject_ready_cb (GObject *object,
                 GAsyncResult *result,
                 MexDirectorySearchManager *self)
{
  GError *error = NULL;

  g_volume_eject_with_operation_finish (G_VOLUME (object), result, &error);

  if (error)
    {
      g_warning ("Problem ejecting: %s", error->message);
      g_error_free (error);
    }
}

static void
_eject_disc (MxAction *action, MexDirectorySearchManager *self)
{
  MexDirectorySearchManagerPrivate *priv = MEX_DIRECTORY_SEARCH_MANAGER (self)->priv;

  /*g_volume_eject_with_operation (priv->volume,
                                 0,
                                 NULL,
                                 NULL,
                                 (GAsyncReadyCallback)_eject_ready_cb,
                                 self);
*/
}


static void
mex_directory_search_manager_init (MexDirectorySearchManager *self)
{
  MexDirectorySearchManagerPrivate *priv;

  priv = self->priv = DIRECTORY_SEARCH_MANAGER_PRIVATE (self);

  priv->videos_model = mex_generic_model_new (_("Directory Searchs"),
                                                     "icon-panelheader-videos");
  
  priv->pictures_model = mex_generic_model_new (_("Directory Searchs"),
                                                     "icon-panelheader-pictures");
 
  priv->music_model = mex_generic_model_new (_("Directory Searchs"),
                                                     "icon-panelheader-music");

  g_object_set (G_OBJECT (priv->videos_model),
                "category", "videos",
                "priority", 100,
                NULL);

  g_object_set (G_OBJECT (priv->pictures_model),
                "category", "pictures",
                "priority", 100,
                NULL);
  g_object_set (G_OBJECT (priv->music_model),
                "category", "music",
                "priority", 100,
                NULL);


  priv->models = g_list_append (priv->models, priv->videos_model);
  priv->models = g_list_append (priv->models, priv->pictures_model);
  priv->models = g_list_append (priv->models, priv->music_model);

  directory_list(self);
}

static const GList *
mex_directory_search_manager_get_models (MexModelProvider *model_provider)
{
  MexDirectorySearchManagerPrivate *priv =
    MEX_DIRECTORY_SEARCH_MANAGER (model_provider)->priv;

  return priv->models;
}

static const GList *
mex_directory_search_manager_get_actions (MexActionProvider *action_provider)
{
  MexDirectorySearchManagerPrivate *priv =
    MEX_DIRECTORY_SEARCH_MANAGER (action_provider)->priv;

  return priv->actions;
}

static void
model_provider_iface_init (MexModelProviderInterface *iface)
{
  iface->get_models = mex_directory_search_manager_get_models;
}

static void
action_provider_iface_init (MexActionProviderInterface *iface)
{
  iface->get_actions = mex_directory_search_manager_get_actions;
}

static GType
mex_directorysearch_get_type (void)
{
  return MEX_TYPE_DIRECTORY_SEARCH_MANAGER;
}




MEX_DEFINE_PLUGIN ("Directory Search",
		   "DVD integration",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Michael Wood <michael.g.wood@linux.intel.com>,"
                   "Thomas Wood <thomas.wood@intel.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_directorysearch_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
