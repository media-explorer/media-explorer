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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-info-bar.h"
#include "mex-tile.h"
#include "mex-generic-notification-source.h"
#include "mex-action-manager.h"

#include <glib/gi18n-lib.h>

#ifdef HAVE_GIO_UNIX
#include <gio/gdesktopappinfo.h>
#endif

static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexInfoBar, mex_info_bar, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init))

#define INFO_BAR_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_INFO_BAR, MexInfoBarPrivate))

struct _MexInfoBarPrivate
{
  ClutterActor *group;
  ClutterActor *settings_dialog;
  ClutterActor *settings_button;
  ClutterActor *power_button;
  ClutterActor *back_button;


  ClutterScript *script;

  gboolean settings_dialog_parented;

  MexGenericNotificationSource *notification_source;
};

enum
{
  CLOSE,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

/* MxFocusableIface */

static MxFocusable *
mex_info_bar_move_focus (MxFocusable      *focusable,
                         MxFocusDirection  direction,
                         MxFocusable      *from)
{
  return NULL;
}

static MxFocusable *
mex_info_bar_accept_focus (MxFocusable *focusable,
                           MxFocusHint  hint)
{
  MexInfoBarPrivate *priv = MEX_INFO_BAR (focusable)->priv;

  MxFocusable *result;

  ClutterActor *buttons_area;

 buttons_area =
    CLUTTER_ACTOR (clutter_script_get_object (priv->script, "buttons-area"));

  /* try the previous focusable first */
  result = mx_focusable_accept_focus (MX_FOCUSABLE (buttons_area),
                                      MX_FOCUS_HINT_PRIOR);

  if (!result)
    result = mx_focusable_accept_focus (MX_FOCUSABLE (buttons_area),
                                        MX_FOCUS_HINT_FIRST);

  return result;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_info_bar_move_focus;
  iface->accept_focus = mex_info_bar_accept_focus;
}

/* Actor implementation */

static void
mex_info_bar_get_preferred_width (ClutterActor *actor,
                                  gfloat        for_height,
                                  gfloat       *min_width_p,
                                  gfloat       *nat_width_p)
{
  MxPadding padding;
  MexInfoBarPrivate *priv = MEX_INFO_BAR (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (for_height >= 0)
    for_height = MAX (0, for_height - padding.top - padding.bottom);

  clutter_actor_get_preferred_width (priv->group,
                                     for_height,
                                     min_width_p,
                                     nat_width_p);

  if (min_width_p)
    *min_width_p += padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p += padding.left + padding.right;
}

static void
mex_info_bar_get_preferred_height (ClutterActor *actor,
                                   gfloat        for_width,
                                   gfloat       *min_height_p,
                                   gfloat       *nat_height_p)
{
  MxPadding padding;
  MexInfoBarPrivate *priv = MEX_INFO_BAR (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  if (for_width >= 0)
    for_width = MAX (0, for_width - padding.left - padding.right);

  clutter_actor_get_preferred_height (priv->group,
                                      for_width,
                                      min_height_p,
                                      nat_height_p);

  if (min_height_p)
    *min_height_p += padding.top + padding.bottom;
  if (nat_height_p)
    *nat_height_p += padding.top + padding.bottom;
}

static void
mex_info_bar_allocate (ClutterActor           *actor,
                       const ClutterActorBox  *box,
                       ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;
  MexInfoBarPrivate *priv = MEX_INFO_BAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_info_bar_parent_class)->
    allocate (actor, box, flags);

  mx_widget_get_available_area (MX_WIDGET (actor), box, &child_box);
  clutter_actor_allocate (priv->group, &child_box, flags);
}

static void
mex_info_bar_paint (ClutterActor *actor)
{
  MexInfoBarPrivate *priv = MEX_INFO_BAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_info_bar_parent_class)->paint (actor);

  clutter_actor_paint (priv->group);
}

static void
mex_info_bar_pick (ClutterActor       *actor,
                   const ClutterColor *color)
{
  MexInfoBarPrivate *priv = MEX_INFO_BAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_info_bar_parent_class)->pick (actor, color);

  clutter_actor_paint (priv->group);
}

static gboolean
_create_settings_dialog (MexInfoBar *self);

static void
mex_info_bar_map (ClutterActor *actor)
{
  MexInfoBarPrivate *priv = MEX_INFO_BAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_info_bar_parent_class)->map (actor);

  clutter_actor_map (priv->group);
}

static void
mex_info_bar_unmap (ClutterActor *actor)
{
  MexInfoBarPrivate *priv = MEX_INFO_BAR (actor)->priv;

  clutter_actor_unmap (priv->group);

  CLUTTER_ACTOR_CLASS (mex_info_bar_parent_class)->unmap (actor);
}

static void
mex_info_bar_dispose (GObject *object)
{
  MexInfoBar *self = MEX_INFO_BAR (object);
  MexInfoBarPrivate *priv = self->priv;

  if (priv->settings_dialog)
    {
      clutter_actor_destroy (priv->settings_dialog);
      priv->settings_dialog = NULL;
    }

  if (priv->group)
    {
      clutter_actor_destroy (priv->group);
      priv->group = NULL;
    }

  if (priv->script)
    {
      g_object_unref (priv->script);
      priv->script = NULL;
    }

  G_OBJECT_CLASS (mex_info_bar_parent_class)->dispose (object);
}

static void
mex_info_bar_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_info_bar_parent_class)->finalize (object);
}

static void
mex_info_bar_class_init (MexInfoBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexInfoBarPrivate));

  object_class->dispose = mex_info_bar_dispose;
  object_class->finalize = mex_info_bar_finalize;

  actor_class->get_preferred_width = mex_info_bar_get_preferred_width;
  actor_class->get_preferred_height = mex_info_bar_get_preferred_height;
  actor_class->allocate = mex_info_bar_allocate;
  actor_class->paint = mex_info_bar_paint;
  actor_class->pick = mex_info_bar_pick;
  actor_class->map = mex_info_bar_map;
  actor_class->unmap = mex_info_bar_unmap;

  signals[CLOSE] = g_signal_new ("close-request",
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST,
                                 0, NULL, NULL,
                                 g_cclosure_marshal_VOID__VOID,
                                 G_TYPE_NONE, 0);
}

static gboolean
_close_dialog_cb (gpointer unused, MexInfoBar *self)
{
  MexInfoBarPrivate *priv = self->priv;

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->settings_dialog))
    clutter_actor_hide (priv->settings_dialog);

  mex_push_focus (MX_FOCUSABLE (self));

  return FALSE;
}

void
mex_info_bar_show_notifications (MexInfoBar *self, gboolean visible)
{
  MexInfoBarPrivate *priv = self->priv;
  ClutterActor *notification_area;

  notification_area =
    CLUTTER_ACTOR (clutter_script_get_object (priv->script,
                                              "notification-area"));
  if (!visible)
    clutter_actor_set_opacity (notification_area, 0x00);
  else
    clutter_actor_set_opacity (notification_area, 0xff);
}

gboolean mex_info_bar_dialog_visible (MexInfoBar *self)
{
  MexInfoBarPrivate *priv = self->priv;

  return CLUTTER_ACTOR_IS_VISIBLE (priv->settings_dialog);
}

void
mex_info_bar_show_buttons (MexInfoBar *self, gboolean visible)
{
  MexInfoBarPrivate *priv = self->priv;

  if (!visible)
    {
      clutter_actor_hide (priv->settings_button);
      clutter_actor_show (priv->back_button);
    }
  else
    {
      clutter_actor_show (priv->settings_button);
      clutter_actor_hide (priv->back_button);
    }
}

void
mex_info_bar_close_dialog (MexInfoBar *self)
{
  _close_dialog_cb (NULL, self);
}

static gboolean
_close_request_cb (gpointer unused, MexInfoBar *self)
{
  g_signal_emit (self, signals[CLOSE], 0);
  return TRUE;
}

#ifdef HAVE_GIO_UNIX
static gboolean
_app_launcher_cb (ClutterActor *actor, gpointer command)
{
  GError *error=NULL;

  if (!g_spawn_command_line_async ((const gchar *)command, &error))
    {
      g_warning (G_STRLOC ": Error launching: %s", error->message);
      g_error_free (error);
    }

  return TRUE;
}

static MxAction *
_action_new_from_desktop_file (const gchar *desktop_file_id)
{
  GDesktopAppInfo *dai;

  dai = g_desktop_app_info_new (desktop_file_id);

  if (dai)
    {
      MxAction *action;
      GAppInfo *ai;
      GIcon *icon;

      ai = G_APP_INFO (dai);

      action = mx_action_new_full (g_app_info_get_name (ai),
                                   g_app_info_get_display_name (ai),
                                   G_CALLBACK (_app_launcher_cb),
                                   (gpointer)g_app_info_get_commandline (ai));

     icon = g_app_info_get_icon (ai);
     if (icon)
       {
         gchar *icon_name;
         icon_name =  g_icon_to_string (icon);

         mx_action_set_icon (action, icon_name);

         g_free (icon_name);
       }

      return action;
    }
  return NULL;
}
#else
static MxAction *
_action_new_from_desktop_file (const gchar *desktop_file_id)
{
  return NULL;
}
#endif

static gboolean
_create_settings_dialog (MexInfoBar *self)
{
  MexInfoBarPrivate *priv = self->priv;

  ClutterActor *dialog, *network_graphic;
  ClutterActor *network_tile, *dialog_layout, *dialog_label;
  ClutterActor *network_button;

  MxAction *close_dialog, *network_settings;

  dialog = mx_dialog_new ();
  mx_stylable_set_style_class (MX_STYLABLE (dialog), "MexInfoBarDialog");

  dialog_label = mx_label_new_with_text (_("Settings"));
  mx_stylable_set_style_class (MX_STYLABLE (dialog_label), "DialogHeader");

  /* Create actions for settings dialog */
  network_settings =
    _action_new_from_desktop_file ("mex-networks.desktop");

  close_dialog = mx_action_new_full ("close", _("Close"),
                                     G_CALLBACK (_close_dialog_cb), self);

  dialog_layout = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (dialog_layout), 10);
  mx_table_set_row_spacing (MX_TABLE (dialog_layout), 30);

  mx_table_insert_actor (MX_TABLE (dialog_layout),
                         CLUTTER_ACTOR (dialog_label), 0, 0);

  if (network_settings)
    {
      gchar *tmp;
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

      mx_button_set_action (MX_BUTTON (network_button), network_settings);

      mx_bin_set_child (MX_BIN (network_tile), network_button);
      mx_bin_set_child (MX_BIN (network_button), network_graphic);

      mx_table_insert_actor (MX_TABLE (dialog_layout),
                             CLUTTER_ACTOR (network_tile), 1, 1);
    }

  if (!network_settings)
    {
      ClutterActor *no_settings;
      no_settings = mx_label_new_with_text (_("No settings helpers installed"));

      clutter_actor_destroy (priv->settings_button);

      mx_table_insert_actor (MX_TABLE (dialog_layout),
                             CLUTTER_ACTOR (no_settings), 1, 0);
    }


  mx_bin_set_child (MX_BIN (dialog), dialog_layout);
  mx_dialog_add_action (MX_DIALOG (dialog), close_dialog);

  priv->settings_dialog = g_object_ref (dialog);

  return TRUE;
}

static gboolean
_show_settings_dialog_cb (ClutterActor *actor, MexInfoBar *self)
{
  MexInfoBarPrivate *priv = self->priv;

  if (!priv->settings_dialog_parented)
    {
      ClutterActor *stage;

      stage = clutter_actor_get_stage (CLUTTER_ACTOR (self));
      mx_dialog_set_transient_parent (MX_DIALOG (priv->settings_dialog), stage);
      priv->settings_dialog_parented = TRUE;
    }

  clutter_actor_show (priv->settings_dialog);

  return TRUE;
}

static void
_back_button_cb (ClutterActor *actor,
                 MexInfoBar   *bar)
{
  GList *actions, *l;

  actions = mex_action_manager_get_actions (mex_action_manager_get_default ());

  /* find the back action */
  for (l = actions; l; l = g_list_next (l))
    {
      MxAction *action = l->data;

      if (!g_strcmp0 (mx_action_get_name (action), "go-back"))
        g_action_activate (G_ACTION (action), NULL);
    }

  g_list_free (actions);
}

void
mex_info_bar_new_notification (MexInfoBar *self,
                               const gchar *message,
                               gint timeout)
{
  MexInfoBarPrivate *priv = self->priv;

  mex_generic_notification_new_notification (priv->notification_source,
                                             message,
                                             timeout);
}

static void
mex_info_bar_init (MexInfoBar *self)
{
  ClutterScript *script;
  ClutterActor *notification_area;
  GError *err = NULL;
  gchar *tmp;

  MexInfoBarPrivate *priv = self->priv = INFO_BAR_PRIVATE (self);

  priv->script = script = clutter_script_new ();

  tmp = g_build_filename (mex_get_data_dir (), "json", "info-bar.json", NULL);
  clutter_script_load_from_file (script, tmp, &err);
  g_free (tmp);

  if (err)
    g_error ("Could not load info bar: %s", err->message);

  priv->group =
    CLUTTER_ACTOR (clutter_script_get_object (script, "main-group"));

  clutter_actor_set_parent (priv->group, CLUTTER_ACTOR (self));


  priv->settings_button =
    CLUTTER_ACTOR (clutter_script_get_object (script, "settings-button"));

  priv->power_button =
    CLUTTER_ACTOR (clutter_script_get_object (script, "power-button"));

  priv->back_button =
    CLUTTER_ACTOR (clutter_script_get_object (script, "back-button"));

  priv->notification_source = mex_generic_notification_source_new ();

  notification_area =
    CLUTTER_ACTOR (clutter_script_get_object (priv->script,
                                              "notification-area"));

  mex_notification_area_add_source (MEX_NOTIFICATION_AREA (notification_area),
                                    MEX_NOTIFICATION_SOURCE (priv->notification_source));

  g_signal_connect (priv->settings_button,
                    "clicked",
                    G_CALLBACK (_show_settings_dialog_cb), self);

  g_signal_connect (priv->power_button,
                    "clicked",
                    G_CALLBACK (_close_request_cb), self);

  g_signal_connect (priv->back_button,
                    "clicked",
                    G_CALLBACK (_back_button_cb), self);

  _create_settings_dialog (MEX_INFO_BAR (self));
}

ClutterActor *
mex_info_bar_get_default (void)
{
  static ClutterActor *singleton = NULL;

  if (singleton)
    return singleton;

  return singleton = g_object_new (MEX_TYPE_INFO_BAR, NULL);
}
