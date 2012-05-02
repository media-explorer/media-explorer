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
#include <glib-object.h>

#include "mex-directorysearch-plugin.h"

#include <glib/gi18n-lib.h>

static void model_provider_iface_init (MexModelProviderInterface *interface);
static void action_provider_iface_init (MexActionProviderInterface *iface);
static void _new_file_added (MexDirectorySearchManager *self,
                             const gchar *file_path,
                             gchar *mount_name);
static void _volume_monitor (MexDirectorySearchManager *self);



G_DEFINE_TYPE_WITH_CODE (MexDirectorySearchManager,
                         mex_directory_search_manager,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                action_provider_iface_init))

#define DIRECTORY_SEARCH_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_DIRECTORY_SEARCH_MANAGER,\
                                MexDirectorySearchManagerPrivate))

static void
_volume_monitor_mount_added_cb (GVolumeMonitor        *volume_monitor,
                                GMount                *mount,
                                MexDirectorySearchManager *self);


static void _volume_monitor_mount_removed_cb (GVolumeMonitor  *volume_monitor,
                                              GMount          *mount,
                                              MexDirectorySearchManager *self);
static void
dir_iterate (MexDirectorySearchManager *self, const gchar *dir_path);


static const gchar *mex_disc_mimetypes[] = { "video/dvd" , "video/vcd", NULL };

struct _MexDirectorySearchManagerPrivate
{
  GList *models;
  GList *actions;

  MexModel *videos_model;
  MexModel *pictures_model;
  MexModel *music_model;

  MexContent *content;

  GHashTable *drive_list_table;
};

GList *dir_list = NULL, *file_list = NULL;

/*
 * Add media contents to GList, dir_list for list of directories, file_list for
 * list of files. Send file list to new_file_added to add to contents.
 *
 */

static void 
directory_list (MexDirectorySearchManager *self, gchar *mount_name)
{
  GList *list_iterate;
  
  for (list_iterate = dir_list; list_iterate != NULL;
       list_iterate = list_iterate->next)
    {
      dir_iterate (self, list_iterate->data);
    }
  for (list_iterate = file_list; list_iterate != NULL;
       list_iterate = list_iterate->next)
    {
      _new_file_added (self, list_iterate->data, mount_name);
    }
  g_list_free_full (dir_list, g_free);
  dir_list = NULL;
  g_list_free_full (file_list, g_free);
  file_list = NULL;
}

/*
 * Use directory path to determine the file path, test path is used to separate
 * and add directories within a directory.
 *
 */

static void
dir_iterate (MexDirectorySearchManager *self, const gchar *dir_path)
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


static void
mex_directory_search_manager_dispose (GObject *object)
{
  MexDirectorySearchManager *directory_search_manager =
    MEX_DIRECTORY_SEARCH_MANAGER (object);
  MexDirectorySearchManagerPrivate *priv =
    MEX_DIRECTORY_SEARCH_MANAGER (directory_search_manager)->priv;

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
  if (priv->drive_list_table)
    {
      g_hash_table_destroy (priv->drive_list_table);
      priv->drive_list_table = NULL;
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
      mex_content_set_metadata (content,
                                MEX_CONTENT_METADATA_STILL,
                                thumb_uri);
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

/*
 * Apply metadata to filename from media, this will deduce the filename from
 * displaying as a file path to displaying only the name of the file in MEX
 */

static void
set_metadata_from_media (MexContent *content,
                         const gchar *uri_path,
                         MexContentMetadata mex_key)
{
  MexDirectorySearchManagerPrivate *priv;
  gchar *showname = NULL, *title, *season_str;
  gint season, episode, year = 0;
  gchar *filename, *replacement;
  const gchar *mimetype;

  if (uri_path)
    {
      if (mex_key == MEX_CONTENT_METADATA_TITLE)
        {
          GRegex *regex;
          replacement = g_filename_from_uri (uri_path, NULL, NULL);
          title = g_path_get_basename (replacement);
          regex = g_regex_new ("\\.....?$",
                               0,
                               0,
                               NULL);
          filename = g_regex_replace (regex,
                                      title,
                                      -1,
                                      0,
                                      "",
                                      0,
                                      NULL);
          mex_content_set_metadata (content,
                                    MEX_CONTENT_METADATA_TITLE,
                                    filename);

          g_regex_unref (regex);

        }
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
  /*set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_SYNOPSIS);
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
  set_metadata_from_media (content, uri_path, MEX_CONTENT_METADATA_ARTIST);*/
}

/*
 * Volume monitor determines the drives connected to the current system, it will
 * search the files in the systems local pictures/videos/music locations first,
 * then externally discover items from mounted drives. 
 */

static void _volume_monitor (MexDirectorySearchManager *self)
{
  gchar *drive_name;
  const gchar *mount_path, *dir_music_path, *dir_pictures_path, *dir_videos_path;
  GList *mountlist;
  GVolumeMonitor *volumemonitor;
  GDrive *drive;
  GMount *mount;
  GVolume *volume;
  gchar mount_name;

  dir_music_path = g_get_user_special_dir (G_USER_DIRECTORY_MUSIC);
  dir_pictures_path = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  dir_videos_path = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);

  dir_iterate (self, dir_music_path);
  dir_iterate (self, dir_pictures_path);
  dir_iterate (self, dir_videos_path);

  volumemonitor = g_volume_monitor_get ();

  mountlist = g_volume_monitor_get_mounts (volumemonitor);

  directory_list (self, NULL);

  if ((g_list_length (mountlist) > 0))
    {
      GList *iter = NULL;
      for (iter = mountlist; iter != NULL; iter = iter->next)
        {
          _volume_monitor_mount_added_cb (volumemonitor, 
                                          G_MOUNT (iter->data), self);
        }
    }

  g_signal_connect (volumemonitor, "mount-added", 
                    G_CALLBACK(_volume_monitor_mount_added_cb), 
                    self);

  g_signal_connect (volumemonitor, "mount-removed",
                    G_CALLBACK(_volume_monitor_mount_removed_cb), self);
  g_list_free (mountlist);

}

/*
 * File path sent from directory search function is then used to obtain contents
 * contents are then checked for mimetype in order to separate into necessary
 * models. Thumbnail and metadata are extracted using the uri path. Also, if
 * content is from mounted drive, then content is stored in hash table for quick
 * removal upon removal of respective media.
 */

static void
_new_file_added (MexDirectorySearchManager *self, const gchar *file_path,
                 gchar *mount_name)
{
  GError *errormsg = NULL;
  gchar *uri_path, *name, *filename, *basename;
  const gchar *mimetype;
  GQueue *mount_queue;
  MexDirectorySearchManagerPrivate *priv =
    MEX_DIRECTORY_SEARCH_MANAGER (self)->priv;

  uri_path = g_filename_to_uri (file_path, NULL, &errormsg);

  if (errormsg != NULL)
    g_warning("Error! %s", errormsg->message);

  priv->content = mex_content_from_uri (uri_path);

  mimetype = mex_content_get_metadata (priv->content,
                                       MEX_CONTENT_METADATA_MIMETYPE);
  mex_directory_search_thumbnail (priv->content, mimetype, uri_path);

  mex_update_content_from_media (priv->content, uri_path);

   if (g_str_has_prefix (mimetype, "video/"))
    {
      mex_model_add_content (priv->videos_model, priv->content);
    }
  else if (g_str_has_prefix (mimetype, "image/"))
    mex_model_add_content (priv->pictures_model, priv->content);
  else if (g_str_has_prefix (mimetype, "audio/"))
    mex_model_add_content (priv->music_model, priv->content);

  if (mount_name)
    {
      mount_queue = g_hash_table_lookup (priv->drive_list_table, mount_name);
      g_queue_push_tail (mount_queue, priv->content);
    }

  priv->content = NULL;
  g_free (uri_path);
}

/*
 * The mounted content is now added to the hashtable, as well as added to the
 * models via directory list function
 */
static void
_content_type_resolved (GObject                             *mount,
                        GAsyncResult                        *result,
                        MexDirectorySearchManager           *self)
{
  GError *error = NULL;
  gchar **content_types, *mount_name;
  GQueue *mount_queue;
  MexDirectorySearchManagerPrivate *priv =
    MEX_DIRECTORY_SEARCH_MANAGER (self)->priv;

  content_types =
    g_mount_guess_content_type_finish (G_MOUNT (mount), result, &error);

  if (content_types)
    {
      if (g_strcmp0 (content_types[0], "x-content/video-dvd") != 0
          || g_strcmp0 (content_types[0], "x-content/video-vcd") != 0)
        {
          gchar *mount_path;
          GFile *mount_file;

          mount_queue = g_queue_new ();
          mount_name = g_mount_get_name (G_MOUNT (mount));
          g_hash_table_insert (priv->drive_list_table, mount_name, mount_queue);

          mount_file = g_mount_get_default_location (mount);
          mount_path = g_file_get_path (mount_file);

          dir_iterate (self, mount_path);
          directory_list (self, mount_name);
        }
    }

  g_strfreev (content_types);
  g_clear_error (&error);
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

/*
 * Removes content added from mounted drives, from the models
 */
static void
remove_queue_item (gpointer data, gpointer user_data)
{
  MexDirectorySearchManagerPrivate *priv =
    MEX_DIRECTORY_SEARCH_MANAGER (user_data)->priv;

  g_object_unref (G_OBJECT(data));
  mex_model_remove_content (priv->videos_model, MEX_CONTENT (data));
  mex_model_remove_content (priv->pictures_model, MEX_CONTENT (data));
  mex_model_remove_content (priv->music_model, MEX_CONTENT (data));
}

static void
_volume_monitor_mount_removed_cb (GVolumeMonitor        *volume_monitor,
                                  GMount                *mount,
                                  MexDirectorySearchManager *self)
{
  MexDirectorySearchManagerPrivate *priv =
    MEX_DIRECTORY_SEARCH_MANAGER (self)->priv;
  gchar *mount_name;
  GQueue *mount_queue;

  mount_name = g_mount_get_name (G_MOUNT (mount));

  mount_queue = g_hash_table_lookup (priv->drive_list_table, mount_name);

  g_queue_foreach (mount_queue, remove_queue_item, self);

  g_queue_free (mount_queue);
}

static void
mex_directory_search_manager_init (MexDirectorySearchManager *self)
{
  MexDirectorySearchManagerPrivate *priv;

  priv = self->priv = DIRECTORY_SEARCH_MANAGER_PRIVATE (self);

  priv->drive_list_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                  g_free, g_queue_free);

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

  _volume_monitor (self);
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
                   "Media integration",
                   PACKAGE_VERSION,
                   "LGPLv2.1+",
                   "Chirag Harendra <chirag.harendra@intel.com>,"
                   "Michael Wood <michael.g.wood@intel.com>",
                   MEX_API_MAJOR, MEX_API_MINOR,
                   mex_directorysearch_get_type,
                   MEX_PLUGIN_PRIORITY_NORMAL)
