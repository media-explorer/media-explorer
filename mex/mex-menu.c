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


/**
 * SECTION:mex-menu
 * @short_description: A hierarchical menu widget
 *
 * #MexMenu is a widget that can be used to represent a hierarchical menu
 * structure. Menu items are created from #MxAction objects, and the current
 * menu depth can be incremented or decremented to represent different levels
 * of a tree hierarchy.
 *
 * #MexMenu presents menu items vertically, and the menu can expand either to
 * the right (where depth increases positively), or to the left (where depth
 * increases negatively).
 */

#include "mex-menu.h"
#include "mex-resizing-hbox-child.h"
#include "mex-scroll-view.h"
#include "mex-utils.h"

G_DEFINE_TYPE (MexMenu, mex_menu, MEX_TYPE_RESIZING_HBOX)

#define MENU_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_MENU, MexMenuPrivate))

enum
{
  PROP_0,

  PROP_DEPTH,
  PROP_MIN_WIDTH
};

struct _MexMenuPrivate
{
  /* Whether the actor contains the child that has focus */
  guint            has_focus    : 1;

  /* Whether to focus the next added menu item. This will also focus
   * the first menu item when removing a menu level.
   */
  guint            focus_on_add : 1;

  /* A convenience pointer to the MxBoxLayout containing the menu items
   * at the current depth.
   */
  ClutterActor    *layout;

  /* The current menu hierarchy depth. 0 is root, and a menu can extend to
   * the left (so this will be negative) or to the right (positive).
   */
  gint             depth;

  /* A hash-table mapping actions to menu-items - This also acts as
   * the list of represented actions.
   */
  GHashTable      *action_to_item;

  /* The minimum width for a menu */
  gfloat           min_width;
};


static void mex_menu_item_destroyed_cb (MexMenu  *self,
                                        gpointer  old_item);


/* A quark used to store the depth of a menu item box-layout */
static GQuark mex_menu_depth_quark = 0;

/* A quark to store if a child in this container is a menu item */
static GQuark mex_menu_item_quark = 0;


static void
mex_menu_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  MexMenuPrivate *priv = MEX_MENU (object)->priv;

  switch (property_id)
    {
    case PROP_DEPTH:
      g_value_set_int (value, priv->depth);
      break;

    case PROP_MIN_WIDTH:
      g_value_set_float (value, priv->min_width);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_menu_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  MexMenu *self = MEX_MENU (object);

  switch (property_id)
    {
    case PROP_MIN_WIDTH:
      mex_menu_set_min_width (self, g_value_get_float (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_menu_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_menu_parent_class)->dispose (object);
}

static void
mex_menu_finalize (GObject *object)
{
  MexMenuPrivate *priv = MEX_MENU (object)->priv;

  if (priv->action_to_item)
    {
      GHashTableIter iter;
      gpointer key, value;

      g_hash_table_iter_init (&iter, priv->action_to_item);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          g_object_weak_unref (G_OBJECT (value),
                               (GWeakNotify)mex_menu_item_destroyed_cb,
                               object);
        }

      g_hash_table_unref (priv->action_to_item);
      priv->action_to_item = NULL;
    }

  G_OBJECT_CLASS (mex_menu_parent_class)->finalize (object);
}

static void
mex_menu_notify_focused_cb (MxFocusManager *manager,
                            GParamSpec     *pspec,
                            MexMenu        *self)
{
  MxBoxLayout *layout;
  ClutterActor *focused;
  MexMenuPrivate *priv = self->priv;
  gint depth;

  focused = (ClutterActor *)mx_focus_manager_get_focused (manager);

  /* Check if our child received focus and store which child it was,
   * or lose focus.
   */
  if (focused)
    {
      ClutterActor *parent = clutter_actor_get_parent (focused);
      while (parent)
        {
          if (MX_IS_BOX_LAYOUT (focused))
            layout = (MxBoxLayout *)focused;

          if (parent == (ClutterActor *)self)
            {
              /* We have focus, check what depth we should be at */
              depth =
                GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (layout),
                                                     mex_menu_depth_quark));

              if ((priv->depth > 0) && (depth < priv->depth))
                while (mex_menu_pop (self) != depth);
              else if ((priv->depth < 0) && (depth > priv->depth))
                while (mex_menu_push (self) != depth);

              if (priv->has_focus)
                priv->focus_on_add = FALSE;
              else
                priv->has_focus = TRUE;

              return;
            }

          focused = parent;
          parent = clutter_actor_get_parent (focused);
        }
    }

  /* We don't have focus, switch to the root depth */
  priv->has_focus = priv->focus_on_add = FALSE;
  if (priv->depth > 0)
    while (mex_menu_pop (self));
  else if (priv->depth < 0)
    while (mex_menu_push (self));
}

static void
mex_menu_map (ClutterActor *actor)
{
  MxFocusManager *manager;
  MexMenu *self = MEX_MENU (actor);

  CLUTTER_ACTOR_CLASS (mex_menu_parent_class)->map (actor);

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    {
      g_signal_connect (manager, "notify::focused",
                        G_CALLBACK (mex_menu_notify_focused_cb), self);
      mex_menu_notify_focused_cb (manager, NULL, self);
    }
}

static void
mex_menu_unmap (ClutterActor *actor)
{
  MxFocusManager *manager;

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  if (manager)
    g_signal_handlers_disconnect_by_func (manager,
                                          mex_menu_notify_focused_cb,
                                          actor);

  CLUTTER_ACTOR_CLASS (mex_menu_parent_class)->unmap (actor);
}

static void
mex_menu_class_init (MexMenuClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexMenuPrivate));

  object_class->get_property = mex_menu_get_property;
  object_class->set_property = mex_menu_set_property;
  object_class->dispose = mex_menu_dispose;
  object_class->finalize = mex_menu_finalize;

  actor_class->map = mex_menu_map;
  actor_class->unmap = mex_menu_unmap;

  pspec = g_param_spec_int ("depth",
                            "Depth",
                            "The depth of the active menu item.",
                            -G_MAXINT, G_MAXINT, 0,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DEPTH, pspec);

  pspec = g_param_spec_float ("min-menu-width",
                              "Minimum Menu Width",
                              "The minimum width of any menu layout.",
                              -1.f, G_MAXFLOAT, -1.f,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MIN_WIDTH, pspec);

  mex_menu_depth_quark = g_quark_from_static_string ("mex-menu-depth");
  mex_menu_item_quark = g_quark_from_static_string ("mex-menu-item");
}

static MxAction *
mex_menu_find_action (MexMenu       *menu,
                      const gchar   *action_name,
                      ClutterActor **item)
{
  gpointer key, value;
  GHashTableIter iter;

  MexMenuPrivate *priv = menu->priv;

  g_hash_table_iter_init (&iter, priv->action_to_item);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      MxAction *action = key;

      if (g_strcmp0 (mx_action_get_name (action), action_name) == 0)
        {
          if (item)
            *item = value;

          return action;
        }
    }

  if (item)
    *item = NULL;

  return NULL;
}

static ClutterActor *
mex_menu_find_item (MexMenu *menu, const gchar *action_name)
{
  ClutterActor *item;

  mex_menu_find_action (menu, action_name, &item);

  return item;
}

static void
mex_menu_item_set_detail (ClutterActor *item, const gchar *text)
{
  MxLabel *label = g_object_get_data (G_OBJECT (item), "detail-label");
  clutter_actor_show (CLUTTER_ACTOR (label));
  mx_label_set_text (label, text ? text : "");
}

static const gchar *
mex_menu_item_get_detail (ClutterActor *item)
{
  MxLabel *label = g_object_get_data (G_OBJECT (item), "detail-label");
  return mx_label_get_text (label);
}

static void
mex_menu_item_set_toggled (ClutterActor *item,
                           gboolean      toggled)
{
  MxStylable *icon = g_object_get_data (G_OBJECT (item), "toggle-icon");
  if (toggled)
    mx_stylable_style_pseudo_class_add (icon, "checked");
  else
    mx_stylable_style_pseudo_class_remove (icon, "checked");
}

static gboolean
mex_menu_item_get_toggled (ClutterActor *item)
{
  MxStylable *icon = g_object_get_data (G_OBJECT (item), "toggle-icon");
  return mx_stylable_style_pseudo_class_contains (icon, "checked");
}

static void
mex_menu_item_clicked_cb (ClutterActor *item,
                          MxAction     *action)
{
  g_signal_emit_by_name (action, "activated");
}

static void
mex_menu_item_destroyed_cb (MexMenu  *self,
                            gpointer  old_item)
{
  gpointer key, value;
  GHashTableIter iter;

  MexMenuPrivate *priv = self->priv;

  g_hash_table_iter_init (&iter, priv->action_to_item);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      if (value == old_item)
        {
          g_hash_table_iter_remove (&iter);
          return;
        }
    }
}

static ClutterActor *
mex_menu_item_new (MexMenu *self, MxAction *action, MexMenuActionType type)
{
  ClutterActor *button, *layout, *icon, *vbox, *label, *arrow = NULL;

  button = mx_button_new ();
  mx_button_set_is_toggle (MX_BUTTON (button), TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (button), "Item");

  layout = mx_box_layout_new ();
  mx_bin_set_child (MX_BIN (button), layout);
  mx_bin_set_fill (MX_BIN (button), TRUE, FALSE);

  if (type == MEX_MENU_LEFT)
    {
      arrow = mx_icon_new ();
      mx_stylable_set_style_class (MX_STYLABLE (arrow), "Left");
      clutter_container_add_actor (CLUTTER_CONTAINER (layout), arrow);

    }

  vbox = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (vbox), MX_ORIENTATION_VERTICAL);

  label = mx_label_new ();
  mx_label_set_fade_out (MX_LABEL (label), TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (label), "Action");
  g_object_bind_property (action, "display-name", label, "text",
                          G_BINDING_SYNC_CREATE);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), label);

  label = mx_label_new ();
  mx_label_set_fade_out (MX_LABEL (label), TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (label), "Detail");
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), label);
  clutter_actor_hide (label);
  g_object_set_data (G_OBJECT (button), "detail-label", label);

  clutter_container_add_actor (CLUTTER_CONTAINER (layout), vbox);
  clutter_container_child_set (CLUTTER_CONTAINER (layout), vbox,
                               "expand", TRUE,
                               "x-fill", FALSE,
                               "x-align", MX_ALIGN_START,
                               "y-fill", FALSE,
                               NULL);

  icon = mx_icon_new ();
  g_object_bind_property (action, "icon", icon, "icon-name",
                          G_BINDING_SYNC_CREATE);
  clutter_container_add_actor (CLUTTER_CONTAINER (layout), icon);

  if (type == MEX_MENU_RIGHT)
    {
      arrow = mx_icon_new ();
      mx_stylable_set_style_class (MX_STYLABLE (arrow), "Right");
      clutter_container_add_actor (CLUTTER_CONTAINER (layout), arrow);
    }
  else if (type == MEX_MENU_TOGGLE)
    {
      ClutterActor *toggle = mx_icon_new ();
      mx_stylable_set_style_class (MX_STYLABLE (toggle), "Toggle");
      clutter_container_add_actor (CLUTTER_CONTAINER (layout), toggle);
      g_object_set_data (G_OBJECT (button), "toggle-icon", toggle);
    }

  if (arrow)
    clutter_container_child_set (CLUTTER_CONTAINER (layout),
                                 arrow,
                                 "expand", FALSE,
                                 "y-align", MX_ALIGN_MIDDLE,
                                 "y-fill", FALSE,
                                 NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (mex_menu_item_clicked_cb), action);

  g_object_weak_ref (G_OBJECT (button),
                     (GWeakNotify)mex_menu_item_destroyed_cb,
                     self);

  /* Set the item qdata on the button to mark that we created it */
  g_object_set_qdata (G_OBJECT (button), mex_menu_item_quark,
                      GINT_TO_POINTER (TRUE));

  return button;
}

static ClutterActor *
mex_menu_create_layout (MexMenu *menu, gboolean lower)
{
  ClutterActor *layout;

  MexMenuPrivate *priv = menu->priv;

  layout = mx_box_layout_new ();
  if (priv->min_width >= 0)
    g_object_set (G_OBJECT (layout), "min-width", priv->min_width, NULL);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (layout),
                                 MX_ORIENTATION_VERTICAL);
  mx_stylable_set_style_class (MX_STYLABLE (layout), "Menu");

  clutter_container_add_actor (CLUTTER_CONTAINER (menu), layout);

  return layout;
}

static void
mex_menu_init (MexMenu *self)
{
  MexMenuPrivate *priv = self->priv = MENU_PRIVATE (self);

  priv->min_width = -1.f;

  /* Create a hash-table for mapping actions to menu-items */
  priv->action_to_item = g_hash_table_new_full (NULL, NULL,
                                                g_object_unref, NULL);

  /* Disable horizontally resizing children */
  mex_resizing_hbox_set_depth_index (MEX_RESIZING_HBOX (self), 0);
  mex_resizing_hbox_set_horizontal_depth_scale (MEX_RESIZING_HBOX (self), 1.f);
  mex_resizing_hbox_set_vertical_depth_scale (MEX_RESIZING_HBOX (self), 0.99f);
  mex_resizing_hbox_set_depth_fade (MEX_RESIZING_HBOX (self), FALSE);

  /* Initialise the root menu item */
  priv->layout = mex_menu_create_layout (self, FALSE);
}

/**
 * mex_menu_new:
 *
 * Creates a new MexMenu.
 *
 * Returns: a newly allocated #MexMenu
 */
ClutterActor *
mex_menu_new (void)
{
  return g_object_new (MEX_TYPE_MENU, NULL);
}

/**
 * mex_menu_add_action:
 * @menu: A #MexMenu
 * @action: A #MxAction
 * @type: The menu action type
 *
 * Adds a menu item to @menu at the current depth. @action must be a uniquely
 * named action, if an action with that name already exists in the menu, this
 * function will do nothing.
 */
void
mex_menu_add_action (MexMenu           *menu,
                     MxAction          *action,
                     MexMenuActionType  type)
{
  ClutterActor *item;
  MexMenuPrivate *priv;

  g_return_if_fail (MEX_IS_MENU (menu));
  g_return_if_fail (MX_IS_ACTION (action));

  priv = menu->priv;

  if (mex_menu_find_action (menu, mx_action_get_name (action), NULL))
    {
      g_warning (G_STRLOC ": Action '%s' is already contained in this menu",
                 mx_action_get_name (action));
      return;
    }

  item = mex_menu_item_new (menu, action, type);
  g_hash_table_insert (priv->action_to_item, action, item);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->layout), item);

  if (priv->focus_on_add)
    {
      mex_push_focus (MX_FOCUSABLE (item));
      priv->focus_on_add = FALSE;
    }
}

/**
 * mex_menu_action_set_detail:
 * @menu: A #MexMenu
 * @action: The action name
 * @detail: The detail text
 *
 * Adds detail text to a particular menu item. This is the text that is
 * displayed below the display-name of the menu item.
 */
void
mex_menu_action_set_detail (MexMenu     *menu,
                            const gchar *action,
                            const gchar *detail)
{
  ClutterActor *item;

  g_return_if_fail (MEX_IS_MENU (menu));
  g_return_if_fail (action);

  item = mex_menu_find_item (menu, action);

  if (!item)
    {
      g_warning (G_STRLOC ": Action '%s' not found", action);
      return;
    }

  mex_menu_item_set_detail (item, detail);
}

const gchar *
mex_menu_action_get_detail (MexMenu     *menu,
                            const gchar *action)
{
  ClutterActor *item;

  g_return_val_if_fail (MEX_IS_MENU (menu), NULL);
  g_return_val_if_fail (action, NULL);

  item = mex_menu_find_item (menu, action);

  if (!item)
    {
      g_warning (G_STRLOC ": Action '%s' not found", action);
      return NULL;
    }

  return mex_menu_item_get_detail (item);
}

void
mex_menu_action_set_toggled (MexMenu     *menu,
                             const gchar *action,
                             gboolean     toggled)
{
  ClutterActor *item;

  g_return_if_fail (MEX_IS_MENU (menu));
  g_return_if_fail (action);

  item = mex_menu_find_item (menu, action);

  if (!item)
    {
      g_warning (G_STRLOC ": Action '%s' not found", action);
      return;
    }

  mex_menu_item_set_toggled (item, toggled);
}

gboolean
mex_menu_action_get_toggled (MexMenu     *menu,
                             const gchar *action)
{
  ClutterActor *item;

  g_return_val_if_fail (MEX_IS_MENU (menu), FALSE);
  g_return_val_if_fail (action, FALSE);

  item = mex_menu_find_item (menu, action);

  if (!item)
    {
      g_warning (G_STRLOC ": Action '%s' not found", action);
      return FALSE;
    }

  return mex_menu_item_get_toggled (item);
}

static void
mex_menu_count_children_cb (ClutterActor *actor,
                            gpointer      data)
{
  gint *count = data;
  *count = *count + 1;
}

/**
 * mex_menu_remove_action:
 * @menu: A #MexMenu
 * @action: The action name
 *
 * Remove the menu item that represents the given action name.
 */
void
mex_menu_remove_action (MexMenu     *menu,
                        const gchar *action_name)
{
  gpointer key, value;
  GHashTableIter iter;
  MexMenuPrivate *priv;

  g_return_if_fail (MEX_IS_MENU (menu));
  g_return_if_fail (action_name);

  priv = menu->priv;

  g_hash_table_iter_init (&iter, priv->action_to_item);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      gint n_children;
      ClutterActor *parent;

      MxAction *action = key;
      ClutterActor *item = value;

      if (g_strcmp0 (action_name, mx_action_get_name (action)) != 0)
        continue;

      g_hash_table_iter_remove (&iter);

      parent = clutter_actor_get_parent (item);
      clutter_actor_destroy (item);

      n_children = 0;
      clutter_container_foreach (CLUTTER_CONTAINER (parent),
                                 mex_menu_count_children_cb,
                                 &n_children);

      if (!n_children)
        {
          if (priv->depth > 0)
            mex_menu_pop (menu);
          else if (priv->depth < 0)
            mex_menu_push (menu);
        }

      return;
    }

  g_warning (G_STRLOC ": Action '%s' not found", action_name);
}

/**
 * mex_menu_get_actions:
 * @menu: A #MexMenu
 * @depth: The menu depth
 *
 * Retrieves the actions represented at the given depth of the menu. If the
 * given depth doesn't exist, or there are no menu items, this function will
 * return %NULL.
 *
 * The returned #MxAction objects are owned by @menu. The list should be freed
 * with g_list_free().
 *
 * Returns: a newly allocated list of #MxAction objects
 */
GList *
mex_menu_get_actions (MexMenu *menu,
                      gint     depth)
{
  GList *actions;
  gpointer key, value;
  GHashTableIter iter;
  MexMenuPrivate *priv;

  g_return_val_if_fail (MEX_IS_MENU (menu), NULL);

  priv = menu->priv;

  actions = NULL;
  g_hash_table_iter_init (&iter, priv->action_to_item);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      ClutterActor *item = value;
      ClutterActor *parent = clutter_actor_get_parent (item);
      gint item_depth =
        GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (parent),
                                             mex_menu_depth_quark));

      if (item_depth == depth)
        actions = g_list_prepend (actions, key);
    }

  return actions;
}

static void
mex_menu_uncheck_buttons (MexMenu *menu)
{
  GList *children;

  MexMenuPrivate *priv = menu->priv;

  children = clutter_container_get_children (CLUTTER_CONTAINER (priv->layout));
  while (children)
    {
      if (g_object_get_qdata (children->data, mex_menu_item_quark))
        {
          mx_button_set_toggled (MX_BUTTON (children->data), FALSE);

          if (priv->focus_on_add)
            {
              mex_push_focus (MX_FOCUSABLE (children->data));
              priv->focus_on_add = FALSE;
            }
        }

      children = g_list_delete_link (children, children);
    }
}

/**
 * mex_menu_push:
 * @menu: A #MexMenu
 *
 * Increments the current depth of the menu. If the current depth is %0,
 * or positive, this will add a new menu level. If the depth is negative,
 * this will remove a menu level.
 *
 * Returns: The new menu depth
 */
gint
mex_menu_push (MexMenu *menu)
{
  MexMenuPrivate *priv;

  g_return_val_if_fail (MEX_IS_MENU (menu), 0);

  priv = menu->priv;
  if (priv->depth < 0)
    {
      GList *l;
      GList *children =
        clutter_container_get_children (CLUTTER_CONTAINER (menu));

      l = g_list_find (children, clutter_actor_get_parent (priv->layout));
      priv->layout = l->next->data;
      clutter_container_remove_actor (CLUTTER_CONTAINER (menu),
                                      CLUTTER_ACTOR (l->data));
      g_list_free (children);

      priv->depth ++;
      priv->focus_on_add = priv->has_focus;
      mex_menu_uncheck_buttons (menu);
    }
  else
    {
      priv->depth ++;
      priv->layout = mex_menu_create_layout (menu, FALSE);
      g_object_set_qdata (G_OBJECT (priv->layout),
                          mex_menu_depth_quark,
                          GINT_TO_POINTER (priv->depth));

      if (priv->has_focus)
        priv->focus_on_add = TRUE;
    }

  g_object_notify (G_OBJECT (menu), "depth");

  return priv->depth;
}

/**
 * mex_menu_pop:
 * @menu: A #MexMenu
 *
 * Decrements the current depth of the menu. If the current depth is %0,
 * or negative, this will add a new menu level. If the depth is positive,
 * this will remove a menu level.
 *
 * Returns: The new menu depth
 */
gint
mex_menu_pop (MexMenu *menu)
{
  MexMenuPrivate *priv;

  g_return_val_if_fail (MEX_IS_MENU (menu), 0);

  priv = menu->priv;
  if (priv->depth > 0)
    {
      GList *l;
      GList *children =
        clutter_container_get_children (CLUTTER_CONTAINER (menu));

      l = g_list_find (children, priv->layout);
      priv->layout = l->prev->data;
      clutter_container_remove_actor (CLUTTER_CONTAINER (menu),
                                      CLUTTER_ACTOR (l->data));
      g_list_free (children);

      priv->depth --;
      priv->focus_on_add = priv->has_focus;
      mex_menu_uncheck_buttons (menu);
    }
  else
    {
      priv->depth --;
      priv->layout = mex_menu_create_layout (menu, TRUE);
      g_object_set_qdata (G_OBJECT (priv->layout),
                          mex_menu_depth_quark,
                          GINT_TO_POINTER (priv->depth));

      if (priv->has_focus)
        priv->focus_on_add = TRUE;
    }

  g_object_notify (G_OBJECT (menu), "depth");

  return priv->depth;
}

/**
 * mex_menu_clear_all:
 * @menu: A #MexMenu
 *
 * Removes all items from the menu.
 */
void
mex_menu_clear_all (MexMenu *menu)
{
  gboolean direction;
  GList *l, *children;
  MexMenuPrivate *priv;

  g_return_if_fail (MEX_IS_MENU (menu));

  priv = menu->priv;

  if (priv->depth == 0)
    return;

  children = clutter_container_get_children (CLUTTER_CONTAINER (menu));
  direction = (priv->depth >= 0);

  for (l = g_list_find (children, clutter_actor_get_parent (priv->layout)); l;
       l = direction ? l->next : l->prev)
    {
      ClutterActor *child = l->data;
      clutter_container_remove_actor (CLUTTER_CONTAINER (menu), child);
      if (--priv->depth == 0)
        break;
    }

  g_list_free (children);

  priv->layout = mex_menu_create_layout (menu, FALSE);

  g_object_notify (G_OBJECT (menu), "depth");
}

MxBoxLayout *
mex_menu_get_layout (MexMenu *menu)
{
  g_return_val_if_fail (MEX_IS_MENU (menu), NULL);
  return MX_BOX_LAYOUT (menu->priv->layout);
}

void
mex_menu_set_min_width (MexMenu *menu,
                        gfloat   min_width)
{
  MexMenuPrivate *priv;

  g_return_if_fail (MEX_IS_MENU (menu));

  priv = menu->priv;
  if (priv->min_width != min_width)
    {
      gint depth;
      gboolean direction;
      GList *l, *children;

      priv->min_width = min_width;

      children = clutter_container_get_children (CLUTTER_CONTAINER (menu));
      direction = (priv->depth >= 0);
      depth = priv->depth;

      for (l = g_list_find (children, clutter_actor_get_parent (priv->layout));
           l; l = direction ? l->next : l->prev)
        {
          g_object_set (G_OBJECT (l->data),
                        "min-width", priv->min_width, NULL);
          if (--depth == 0)
            break;
        }

      g_list_free (children);

      g_object_notify (G_OBJECT (menu), "min-menu-width");
    }
}

gfloat
mex_menu_get_min_width (MexMenu *menu)
{
  g_return_val_if_fail (MEX_IS_MENU (menu), -1.f);
  return menu->priv->min_width;
}
