/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
 * Copyright © 2012, sleep(5) ltd.
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

#include "mex-networks-plugin.h"
#include "mtn-app.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <mex/mex.h>
#include <mex/mex-info-bar-component.h>

static void mex_info_bar_component_iface_init (MexInfoBarComponentIface *iface);
static void mex_networks_plugin_dispose (GObject *object);
static void mex_networks_plugin_finalize (GObject *object);

G_DEFINE_TYPE_WITH_CODE (MexNetworksPlugin, mex_networks_plugin, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_INFO_BAR_COMPONENT,
                                            mex_info_bar_component_iface_init));

#define MEX_NETWORKS_PLUGIN_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_NETWORKS_PLUGIN, MexNetworksPluginPrivate))

struct _MexNetworksPluginPrivate
{
  MtnApp       *app;
  ClutterActor *button;
  ClutterActor *transient_for;

  guint disposed : 1;
};

static void
mex_networks_plugin_class_init (MexNetworksPluginClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  g_type_class_add_private (klass, sizeof (MexNetworksPluginPrivate));

  object_class->dispose      = mex_networks_plugin_dispose;
  object_class->finalize     = mex_networks_plugin_finalize;
}

static void
mex_networks_plugin_init (MexNetworksPlugin *self)
{
  self->priv = MEX_NETWORKS_PLUGIN_GET_PRIVATE (self);
}

static void
mex_networks_plugin_dispose (GObject *object)
{
  MexNetworksPlugin        *self = (MexNetworksPlugin*) object;
  MexNetworksPluginPrivate *priv = self->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->app)
    {
      g_object_unref (priv->app);
      priv->app = NULL;
    }

  G_OBJECT_CLASS (mex_networks_plugin_parent_class)->dispose (object);
}

static void
mex_networks_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_networks_plugin_parent_class)->finalize (object);
}

static MexInfoBarLocation
mex_networks_plugin_get_location (MexInfoBarComponent *comp)
{
  return MEX_INFO_BAR_LOCATION_SETTINGS;
}

static int
mex_networks_plugin_get_location_index (MexInfoBarComponent *comp)
{
  return -1;
}

static void
mex_networks_plugin_close_cb (MtnApp *app, MexNetworksPlugin *self)
{
  MexNetworksPluginPrivate *priv = self->priv;

  g_print ("%s:%d\n", __FUNCTION__, __LINE__);

  mex_push_focus (MX_FOCUSABLE (priv->button));

  priv->app = NULL;
  g_object_unref (app);
}

static void
mex_networks_plugin_activated_cb (MxAction *action, MexNetworksPlugin *self)
{
  MexNetworksPluginPrivate *priv = self->priv;
  ClutterActor             *dialog;

  if (priv->app)
    g_object_unref (priv->app);

  priv->app = mtn_app_new ();
  g_signal_connect (priv->app, "close",
                    G_CALLBACK (mex_networks_plugin_close_cb), self);

  dialog = mtn_app_get_dialog (priv->app);

  if (priv->transient_for)
    mx_dialog_set_transient_parent (MX_DIALOG (dialog), priv->transient_for);

  clutter_actor_show (dialog);
}

static ClutterActor *
mex_networks_plugin_create_ui (MexInfoBarComponent *comp,
                               ClutterActor        *transient_for)
{
  MexNetworksPlugin *self;
  ClutterActor *network_graphic, *network_tile, *network_button;
  gchar        *tmp;
  MxAction     *network_action;

  g_return_val_if_fail (MEX_IS_NETWORKS_PLUGIN (comp), NULL);

  self = MEX_NETWORKS_PLUGIN (comp);

  /*
   * The actual dialog is being constructed on demand, so store the
   * transient parent.
   */
  self->priv->transient_for = transient_for;

  /*
   * Make the button for the Settings dialog.
   */
  network_graphic = mx_image_new ();
  mx_stylable_set_style_class (MX_STYLABLE (network_graphic),
                               "NetworkGraphic");

  tmp = g_build_filename (mex_get_data_dir (), "style",
                          "graphic-network.png", NULL);
  mx_image_set_from_file (MX_IMAGE (network_graphic), tmp, NULL);
  g_free (tmp);

  network_tile = mex_tile_new ();
  mex_tile_set_label (MEX_TILE (network_tile), _("Network"));
  mex_tile_set_important (MEX_TILE (network_tile), TRUE);

  network_button = mx_button_new ();

  network_action =
    mx_action_new_full ("network-settings",
                        _("Networks"),
                        G_CALLBACK (mex_networks_plugin_activated_cb),
                        self);

  mx_button_set_action (MX_BUTTON (network_button), network_action);

  mx_bin_set_child (MX_BIN (network_tile), network_button);
  mx_bin_set_child (MX_BIN (network_button), network_graphic);

  self->priv->button = network_tile;

  return network_tile;
}

static void
mex_info_bar_component_iface_init (MexInfoBarComponentIface *iface)
{
  iface->get_location       = mex_networks_plugin_get_location;
  iface->get_location_index = mex_networks_plugin_get_location_index;
  iface->create_ui          = mex_networks_plugin_create_ui;
}

static GType
mex_networks_get_type (void)
{
  return MEX_TYPE_NETWORKS_PLUGIN;
}

MEX_DEFINE_PLUGIN ("Networks",
		   "Network Configuration",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Tomas Frydrych <tomas@sleepfive.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   mex_networks_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
