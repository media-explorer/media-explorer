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

#include "mex-opticaldisc-plugin.h"

#include <glib/gi18n-lib.h>

static void model_provider_iface_init (MexModelProviderInterface *interface);
static void action_provider_iface_init (MexActionProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MexOpticalDiscManager,
                         mex_optical_disc_manager,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                action_provider_iface_init))

#define OPTICAL_DISC_MANAGER_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_OPTICAL_DISC_MANAGER, MexOpticalDiscManagerPrivate))

static void _volume_monitor_mount_added_cb (GVolumeMonitor           *volume_monitor,
                                            GMount                   *mount,
                                            MexOpticalDiscManager *self);

static void _volume_monitor_mount_removed_cb (GVolumeMonitor           *volume_monitor,
                                              GMount                   *mount,
                                              MexOpticalDiscManager *self);

static const gchar *mex_disc_mimetypes[] = { "video/dvd" , "video/vcd", NULL };

struct _MexOpticalDiscManagerPrivate
{
  GVolumeMonitor *volume_monitor;

  GList *models;
  GList *actions;

  MexModel *optical_discs_model;

  MexContent *content;
  GVolume *volume;
};


static void
mex_optical_disc_manager_dispose (GObject *object)
{
  MexOpticalDiscManager *optical_disc_manager = MEX_OPTICAL_DISC_MANAGER (object);
  MexOpticalDiscManagerPrivate *priv = MEX_OPTICAL_DISC_MANAGER (optical_disc_manager)->priv;

  if (priv->volume_monitor)
    {
      g_signal_handlers_disconnect_by_func (priv->volume_monitor,
                                            _volume_monitor_mount_added_cb,
                                            object);
      g_signal_handlers_disconnect_by_func (priv->volume_monitor,
                                            _volume_monitor_mount_removed_cb,
                                            object);
      g_object_unref (priv->volume_monitor);
      priv->volume_monitor = NULL;
    }

  if (priv->content)
    {
      g_object_unref (priv->content);
      priv->content = NULL;
    }

  if (priv->volume)
    {
      g_object_unref (priv->volume);
      priv->volume = NULL;
    }

  while (priv->models)
    {
      mex_model_info_free (priv->models->data);
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
mex_optical_disc_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_optical_disc_manager_parent_class)->finalize (object);
}

static void
mex_optical_disc_manager_class_init (MexOpticalDiscManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexOpticalDiscManagerPrivate));

  object_class->dispose = mex_optical_disc_manager_dispose;
  object_class->finalize = mex_optical_disc_manager_finalize;
}

static void
_new_disc_added (MexOpticalDiscManager *self, GMount *mount, gchar *type)
{
  MexOpticalDiscManagerPrivate *priv = MEX_OPTICAL_DISC_MANAGER (self)->priv;
  GVolume *volume;
  gchar *name;

  volume = g_mount_get_volume (mount);

  if (volume)
    name = g_volume_get_identifier (volume, G_VOLUME_IDENTIFIER_KIND_LABEL);

  if (!name)
    name = g_mount_get_name (mount);


  /* Keep the volume for this disc so that we can eject it later */
  if (priv->volume)
    {
      g_object_unref (priv->volume);
      priv->volume = NULL;
    }

  priv->volume = g_mount_get_volume (G_MOUNT (mount));

  /* Remove an existing content - we only consider one disc at a time */
  if (priv->content)
    {
      mex_model_remove_content (priv->optical_discs_model,
                                priv->content);
      priv->content = NULL;
    }
  else
    {
      if (g_strcmp0 (type, "x-content/video-dvd") == 0)
        priv->content = mex_content_from_uri ("dvd://");
      else if (g_strcmp0 (type, "x-content/video-vcd") == 0)
        priv->content = mex_content_from_uri ("vcd://");

      mex_content_set_metadata (priv->content,
                                MEX_CONTENT_METADATA_TITLE,
                                name);

      mex_model_add_content (priv->optical_discs_model, priv->content);
    }
}


static void
_content_type_resolved (GObject               *mount,
                        GAsyncResult          *result,
                        MexOpticalDiscManager *self)
{
  MexOpticalDiscManagerPrivate *priv = MEX_OPTICAL_DISC_MANAGER (self)->priv;
  GError *error = NULL;
  gchar **content_types;

  content_types =
    g_mount_guess_content_type_finish (G_MOUNT (mount), result, &error);

  /* Possible types we could consider when gstreamer supports them:
   * x-content/video-bluray;
   * x-content/video-hddvd;
   * x-content/video-svcd;
   */
  if (content_types)
    {
      if (g_strcmp0 (content_types[0], "x-content/video-dvd") == 0
          || g_strcmp0 (content_types[0], "x-content/video-vcd") == 0)
        {
          _new_disc_added (self, G_MOUNT (mount), content_types[0]);
        }
    }
  g_strfreev (content_types);
}

static void
_volume_monitor_mount_added_cb (GVolumeMonitor        *volume_monitor,
                                GMount                *mount,
                                MexOpticalDiscManager *self)
{
  MexOpticalDiscManagerPrivate *priv = MEX_OPTICAL_DISC_MANAGER (self)->priv;

  g_mount_guess_content_type (mount,
                              FALSE,
                              NULL,
                              (GAsyncReadyCallback)_content_type_resolved,
                              self);
}

static void
_volume_monitor_mount_removed_cb (GVolumeMonitor        *volume_monitor,
                                  GMount                *mount,
                                  MexOpticalDiscManager *self)
{
  MexOpticalDiscManagerPrivate *priv = MEX_OPTICAL_DISC_MANAGER (self)->priv;

  g_mount_guess_content_type (mount,
                              FALSE,
                              NULL,
                              (GAsyncReadyCallback)_content_type_resolved,
                              self);
}

static void
_eject_ready_cb (GObject *object,
                 GAsyncResult *result,
                 MexOpticalDiscManager *self)
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
_eject_disc (MxAction *action, MexOpticalDiscManager *self)
{
  MexOpticalDiscManagerPrivate *priv = MEX_OPTICAL_DISC_MANAGER (self)->priv;

  g_volume_eject_with_operation (priv->volume,
                                 0,
                                 NULL,
                                 NULL,
                                 (GAsyncReadyCallback)_eject_ready_cb,
                                 self);
}

static void
mex_optical_disc_manager_init (MexOpticalDiscManager *self)
{
  MexOpticalDiscManagerPrivate *priv;
  MexModelInfo *model_info;
  MexModel *optical_discs_model;
  MexActionInfo *eject_action;

  GList *mounts = NULL;
  GList *mounts_n = NULL;

  priv = self->priv = OPTICAL_DISC_MANAGER_PRIVATE (self);

  priv->volume_monitor = g_volume_monitor_get ();

  g_signal_connect (self->priv->volume_monitor,
                    "mount-added",
                    (GCallback)_volume_monitor_mount_added_cb,
                    self);
  g_signal_connect (self->priv->volume_monitor,
                    "mount-removed",
                    (GCallback)_volume_monitor_mount_removed_cb,
                    self);

  priv->optical_discs_model = mex_generic_model_new (_("Optical Discs"),
                                                     "icon-panelheader-videos");
  model_info =
    mex_model_info_new_with_sort_funcs (MEX_MODEL (priv->optical_discs_model),
                                        "videos", 100);
  priv->models = g_list_append (priv->models, model_info);

  mounts = g_volume_monitor_get_mounts (priv->volume_monitor);

  for (mounts_n = mounts; mounts_n; mounts_n = mounts_n->next)
    {
      g_mount_guess_content_type (mounts_n->data,
                                  FALSE,
                                  NULL,
                                  (GAsyncReadyCallback)_content_type_resolved,
                                  self);
    }
  /* Setup the eject action */
  eject_action = g_new0 (MexActionInfo, 1);


  /* TODO: Eject disc requires that we give it the volume we can just say we only ever have one volume and always eject that? */
  eject_action->action = mx_action_new_full ("eject",
                                            _("Eject"),
                                            (GCallback)_eject_disc,
                                            self);

  eject_action->mime_types = g_strdupv ((gchar **)mex_disc_mimetypes);
  eject_action->priority = 200;

  priv->actions = g_list_append (priv->actions, eject_action);
}

static MexOpticalDiscManager *
mex_optical_disc_manager_new (void)
{
  return g_object_new (MEX_TYPE_OPTICAL_DISC_MANAGER, NULL);
}

static const GList *
mex_optical_disc_manager_get_models (MexModelProvider *model_provider)
{
  MexOpticalDiscManagerPrivate *priv =
    MEX_OPTICAL_DISC_MANAGER (model_provider)->priv;

  return priv->models;
}

static const GList *
mex_optical_disc_manager_get_actions (MexActionProvider *action_provider)
{
  MexOpticalDiscManagerPrivate *priv =
    MEX_OPTICAL_DISC_MANAGER (action_provider)->priv;

  return priv->actions;
}

static void
model_provider_iface_init (MexModelProviderInterface *iface)
{
  iface->get_models = mex_optical_disc_manager_get_models;
}

static void
action_provider_iface_init (MexActionProviderInterface *iface)
{
  iface->get_actions = mex_optical_disc_manager_get_actions;
}


G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_OPTICAL_DISC_MANAGER;
}


