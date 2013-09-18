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

#include "mex-info-bar.h"
#include "mex-tile.h"
#include "mex-generic-notification-source.h"
#include "mex-action-manager.h"
#include "mex-info-bar-component.h"

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

  guint settings_dialog_parented : 1;
  guint plugin_manager_set       : 1;
  guint no_settings_helpers      : 1;

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

  return mx_focusable_accept_focus (MX_FOCUSABLE (priv->group),
                                    MX_FOCUS_HINT_FIRST);
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

  /* use opacity to show and hide the buttons to ensure the correct amount of
   * space is always allocated for them */

  clutter_actor_set_opacity (priv->settings_button, 0xff * visible);
  clutter_actor_set_opacity (priv->back_button, 0xff * !visible);

  mx_widget_set_disabled (MX_WIDGET (priv->settings_button), !visible);
  mx_widget_set_disabled (MX_WIDGET (priv->back_button), visible);
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

static gboolean
_create_settings_dialog (MexInfoBar *self)
{
  MexInfoBarPrivate *priv = self->priv;

  ClutterActor *dialog;
  ClutterActor *dialog_layout, *dialog_label;
  ClutterActor *no_settings;

  MxAction *close_dialog;

  dialog = mx_dialog_new ();
  mx_stylable_set_style_class (MX_STYLABLE (dialog), "MexInfoBarDialog");

  dialog_label = mx_label_new_with_text (_("Settings"));
  mx_stylable_set_style_class (MX_STYLABLE (dialog_label), "DialogHeader");

  close_dialog = mx_action_new_full ("close", _("Close"),
                                     G_CALLBACK (_close_dialog_cb), self);

  dialog_layout = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (dialog_layout), 10);
  mx_table_set_row_spacing (MX_TABLE (dialog_layout), 30);

  mx_table_insert_actor (MX_TABLE (dialog_layout),
                         CLUTTER_ACTOR (dialog_label), 0, 0);

  no_settings = mx_label_new_with_text (_("No settings helpers installed"));

  clutter_actor_hide (priv->settings_button);

  mx_table_insert_actor (MX_TABLE (dialog_layout),
                         CLUTTER_ACTOR (no_settings), 1, 0);

  priv->no_settings_helpers = TRUE;

  clutter_actor_add_child (CLUTTER_ACTOR (dialog), dialog_layout);
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

static void
mex_info_bar_add_settings_item (MexInfoBar   *self,
                                ClutterActor *item,
                                gint          idx)
{
  MexInfoBarPrivate *priv = self->priv;
  ClutterActor      *dialog_layout;
  int                rows;
  ClutterActorIter   iter;
  ClutterActor      *child;

  g_return_if_fail (priv->settings_dialog);

  /* TODO -- handle the index, at least 0 for prepend and <0 for append. */
  dialog_layout = clutter_actor_get_first_child (CLUTTER_ACTOR (priv->settings_dialog));
  rows = mx_table_get_row_count (MX_TABLE (dialog_layout));

  mx_table_insert_actor (MX_TABLE (dialog_layout), item, rows + 1, 1);

  if (priv->no_settings_helpers)
    {
      clutter_actor_show (priv->settings_button);
      /*
       * Remove the 'No settings helpers installed' notice.
       */
      clutter_actor_iter_init (&iter, dialog_layout);

      while (clutter_actor_iter_next (&iter, &child))
        {
          int row = mx_table_child_get_row (MX_TABLE (dialog_layout), child);
          int col = mx_table_child_get_column (MX_TABLE (dialog_layout), child);

          if (row == 1 && col == 0)
            {
              clutter_actor_remove_child (dialog_layout, child);
              priv->no_settings_helpers = FALSE;
              break;
            }
        }
    }
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
  GSettings *settings;

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

  /* ensure the notification area is above any other actors */
  clutter_actor_set_child_above_sibling (clutter_actor_get_parent (notification_area),
                                         notification_area, NULL);

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

  settings = g_settings_new ("org.media-explorer.Shell");
  if (!g_settings_get_boolean (settings, "close-button-visible"))
    clutter_actor_hide (priv->power_button);
  g_clear_object (&settings);
}

ClutterActor *
mex_info_bar_get_default (void)
{
  static ClutterActor *singleton = NULL;

  if (singleton)
    return singleton;

  return singleton = g_object_new (MEX_TYPE_INFO_BAR, NULL);
}

static void
mex_info_bar_plugin_loaded_cb (MexPluginManager *mgr,
                               GObject          *plugin,
                               MexInfoBar       *self)
{
  MexInfoBarPrivate   *priv = self->priv;
  MexInfoBarLocation   location;
  int                  idx;
  ClutterActor        *actor, *stage;
  MexInfoBarComponent *comp;

  if (!MEX_IS_INFO_BAR_COMPONENT (plugin))
    return;

  comp = MEX_INFO_BAR_COMPONENT (plugin);

  location = mex_info_bar_component_get_location (comp);
  g_return_if_fail (location != MEX_INFO_BAR_LOCATION_UNDEFINED);

  idx = mex_info_bar_component_get_location_index (comp);

  stage = clutter_actor_get_stage (CLUTTER_ACTOR (self));

  switch (location)
    {
    default:
      g_warning ("Unknown info bar location %d", location);
      break;
    case MEX_INFO_BAR_LOCATION_BAR:
      actor = mex_info_bar_component_create_ui (comp, stage);
      g_return_if_fail (actor);
      clutter_actor_insert_child_at_index (priv->group, actor, idx);
      break;
    case MEX_INFO_BAR_LOCATION_SETTINGS:
      actor = mex_info_bar_component_create_ui (comp, stage);
      g_return_if_fail (actor);
      mex_info_bar_add_settings_item (self, actor, idx);
      break;
    }
}

void
mex_info_bar_set_plugin_manager (MexInfoBar *self, MexPluginManager *mgr)
{
  g_return_if_fail (MEX_IS_INFO_BAR (self) && MEX_IS_PLUGIN_MANAGER (mgr));
  g_return_if_fail (!self->priv->plugin_manager_set);

  self->priv->plugin_manager_set = TRUE;

  g_signal_connect (mgr, "plugin-loaded",
                    G_CALLBACK (mex_info_bar_plugin_loaded_cb),
                    self);
}
