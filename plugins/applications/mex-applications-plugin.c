/*
 * Mex - a media explorer
 *
 * Copyright © , 2010, 2011 Intel Corporation.
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

#include <gio/gdesktopappinfo.h>

#include "mex-applications-plugin.h"
#include <mex/mex-model-provider.h>
#include <mex/mex-action-provider.h>
#include <mex/mex-generic-model.h>
#include <mex/mex-utils.h>
#include <mex/mex-application.h>

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>

static void model_provider_iface_init (MexModelProviderInterface *iface);
static void action_provider_iface_init (MexActionProviderInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MexApplicationsPlugin,
                         mex_applications_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL_PROVIDER,
                                                model_provider_iface_init)
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_ACTION_PROVIDER,
                                                action_provider_iface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_APPLICATIONS_PLUGIN, MexApplicationsPluginPrivate))

typedef struct _MexApplicationsPluginPrivate MexApplicationsPluginPrivate;

struct _MexApplicationsPluginPrivate {
  GList *models;
  GList *actions;

  MexModel *applications_model;
};

static void
mex_applications_plugin_dispose (GObject *object)
{
  MexApplicationsPluginPrivate *priv = GET_PRIVATE (object);

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

  G_OBJECT_CLASS (mex_applications_plugin_parent_class)->dispose (object);
}

static void
mex_applications_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_applications_plugin_parent_class)->finalize (object);
}

static void
mex_applications_plugin_class_init (MexApplicationsPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexApplicationsPluginPrivate));

  object_class->dispose = mex_applications_plugin_dispose;
  object_class->finalize = mex_applications_plugin_finalize;
}

static const gchar *app_mimetypes[] = { "x-mex-application", NULL };

static void
_applications_plugin_launch_action_cb (MxAction *action,
                                       gpointer  userdata)
{
  MexContent *content = mex_action_get_content (action);
  MexApplication *application = MEX_APPLICATION (content);
  const gchar *cmd_line;
  GError *error = NULL;

  cmd_line = mex_application_get_executable (application);

  if (!g_spawn_command_line_async (cmd_line, &error))
    {
      g_warning (G_STRLOC ": Error launching: %s",
                 error->message);
      g_clear_error (&error);
    }
}

static MexApplication *
_application_new_from_desktop_file (const gchar *desktop_file_id)
{
  GDesktopAppInfo *dai;
  GAppInfo        *ai;

  dai = g_desktop_app_info_new (desktop_file_id);
  ai = G_APP_INFO (dai);

  if (dai)
    {
      MexApplication *application;

      application = g_object_new (MEX_TYPE_APPLICATION,
                                  "name", g_app_info_get_name (ai),
                                  "desktop-file",
                                        g_desktop_app_info_get_filename (dai),
                                  "executable",
                                        g_app_info_get_commandline (ai),
                                  "description",
                                        g_app_info_get_description (ai),
                                  "icon", NULL,
                                  NULL);

      return application;
    }

  return NULL;
}

static void
_populate_model (MexApplicationsPlugin *self)
{
  MexApplicationsPluginPrivate *priv = GET_PRIVATE (self);
  MexApplication *application;

  application = _application_new_from_desktop_file ("mex-networks.desktop");
  if (application)
    mex_model_add_content (priv->applications_model,
                           MEX_CONTENT (application));

  application = _application_new_from_desktop_file ("mex-browser.desktop");
  if (application)
    mex_model_add_content (priv->applications_model,
                           MEX_CONTENT (application));

  application = _application_new_from_desktop_file ("mex-rebinder-config.desktop");
  if (application)
    mex_model_add_content (priv->applications_model,
                           MEX_CONTENT (application));

  application = _application_new_from_desktop_file ("gnome-terminal.desktop");
  if (application)
    mex_model_add_content (priv->applications_model,
                           MEX_CONTENT (application));
}

static void
mex_applications_plugin_init (MexApplicationsPlugin *self)
{
  MexApplicationsPluginPrivate *priv = GET_PRIVATE (self);
  MexActionInfo *action_info;
  MexModelInfo *model_info;

  priv->applications_model = mex_generic_model_new (_("Applications"),
                                                    "icon-applications");

  _populate_model (self);

  action_info = g_new0 (MexActionInfo, 1);
  action_info->action = mx_action_new_full ("launch",
                                            _("Launch"),
                                            (GCallback)_applications_plugin_launch_action_cb,
                                            self);
  mx_action_set_icon (action_info->action, "x-mex-app-launch-mex");
  action_info->mime_types = g_strdupv ((gchar **)app_mimetypes);
  action_info->priority = 100;
  priv->actions = g_list_append (priv->actions, action_info);

  model_info = mex_model_info_new_with_sort_funcs (priv->applications_model,
                                                   "applications", 0);
  g_object_unref (priv->applications_model);

  priv->models = g_list_append (priv->models, model_info);
}

static const GList *
mex_applications_plugin_get_actions (MexActionProvider *action_provider)
{
  MexApplicationsPluginPrivate *priv = GET_PRIVATE (action_provider);

  return priv->actions;
}

static const GList *
mex_applications_plugin_get_models (MexModelProvider *model_provider)
{
  MexApplicationsPluginPrivate *priv = GET_PRIVATE (model_provider);

  return priv->models;
}

static void
model_provider_iface_init (MexModelProviderInterface *iface)
{
  iface->get_models = mex_applications_plugin_get_models;
}

static void
action_provider_iface_init (MexActionProviderInterface *iface)
{
  iface->get_actions = mex_applications_plugin_get_actions;
}

G_MODULE_EXPORT const GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_APPLICATIONS_PLUGIN;
}
