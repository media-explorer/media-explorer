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


#include "mex-explorer.h"
#include "mex-aggregate-model.h"
#include "mex-view-model.h"
#include "mex-column.h"
#include "mex-column-view.h"
#include "mex-content-box.h"
#include "mex-content-proxy.h"
#include "mex-content-view.h"
#include "mex-grid-view.h"
#include "mex-marshal.h"
#include "mex-resizing-hbox.h"
#include "mex-tile.h"
#include "mex-grid.h"

#include "mex-scene.h"

#include "mex-model-manager.h"

#define ANIMATION_DURATION 150

static void model_length_changed_cb (MexModel   *model,
                                     GParamSpec *pspec,
                                     MexColumn  *column);
static void mx_focusable_iface_init (MxFocusableIface *iface);

static MxFocusableIface *mex_explorer_focusable_parent_iface = NULL;

G_DEFINE_TYPE_WITH_CODE (MexExplorer, mex_explorer, MX_TYPE_STACK,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init))

#define EXPLORER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_EXPLORER, MexExplorerPrivate))

enum
{
  PAGE_CREATED,
  HEADER_ACTIVATED,

  LAST_SIGNAL
};

enum
{
  PROP_0,

  PROP_ROOT_MODEL,
  PROP_MODEL,
  PROP_N_PREVIEW_ITEMS,
  PROP_DEPTH,
  PROP_TOUCH_MODE
};

struct _MexExplorerPrivate
{
  guint         in_transition       : 1;
  guint         has_temporary_focus : 1;
  guint         touch_mode          : 1;
  MexModel     *root_model;
  GQueue        pages;
  GList        *to_destroy;
  ClutterActor *last_focus;
  gint          n_preview_items;

  ClutterActor *current_child;
  ClutterActor *previous_child;
};

static guint signals[LAST_SIGNAL] = { 0, };

static GQuark mex_explorer_model_quark = 0;
static GQuark mex_explorer_proxy_quark = 0;
static GQuark mex_explorer_container_quark = 0;
static GQuark mex_explorer_explorer_quark = 0;

static void
mex_explorer_model_added_cb (MexAggregateModel *aggregate,
                             MexModel          *model,
                             MexExplorer       *explorer);
static void
mex_explorer_model_removed_cb (MexAggregateModel *aggregate,
                               MexModel          *model,
                               MexExplorer       *explorer);


static MxFocusable *
mex_explorer_move_focus (MxFocusable      *focusable,
                         MxFocusDirection  direction,
                         MxFocusable      *from)
{
  MexExplorerPrivate *priv = MEX_EXPLORER (focusable)->priv;

  priv->has_temporary_focus = FALSE;

  return NULL;
}

static MxFocusable *
mex_explorer_accept_focus (MxFocusable *focusable, MxFocusHint hint)
{
  MexExplorerPrivate *priv = MEX_EXPLORER (focusable)->priv;

  /* If our child isn't focusable (this will happen in the case we're
   * currently showing a completely empty model), still accept focus
   * so we can push the focus to the child once it gets filled with data.
   */
  if (!priv->current_child)
    {
      priv->has_temporary_focus = TRUE;
      return focusable;
    }

  priv->has_temporary_focus = FALSE;

  return mx_focusable_accept_focus (MX_FOCUSABLE (priv->current_child), hint);
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  /* Store a pointer to the parent implementation so we can chain up */
  mex_explorer_focusable_parent_iface = g_type_interface_peek_parent (iface);

  iface->accept_focus = mex_explorer_accept_focus;
  iface->move_focus = mex_explorer_move_focus;
}

static void
mex_explorer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  MexExplorer *self = MEX_EXPLORER (object);

  switch (property_id)
    {
    case PROP_ROOT_MODEL:
      g_value_set_object (value, mex_explorer_get_root_model (self));
      break;

    case PROP_MODEL:
      g_value_set_object (value, mex_explorer_get_model (self));
      break;

    case PROP_N_PREVIEW_ITEMS:
      g_value_set_int (value, mex_explorer_get_n_preview_items (self));
      break;

    case PROP_DEPTH:
      g_value_set_uint (value, mex_explorer_get_depth (self));
      break;

    case PROP_TOUCH_MODE:
      g_value_set_boolean (value, mex_explorer_get_touch_mode (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_explorer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  MexExplorer *self = MEX_EXPLORER (object);

  switch (property_id)
    {
    case PROP_ROOT_MODEL:
      mex_explorer_set_root_model (self,
                                   MEX_MODEL (g_value_get_object (value)));
      break;

    case PROP_N_PREVIEW_ITEMS:
      mex_explorer_set_n_preview_items (self, g_value_get_int (value));
      break;

    case PROP_TOUCH_MODE:
      mex_explorer_set_touch_mode (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_explorer_dispose (GObject *object)
{
  MexExplorerPrivate *priv = MEX_EXPLORER (object)->priv;

  if (priv->to_destroy)
    {
      g_list_free (priv->to_destroy);
      priv->to_destroy = NULL;
    }

  while (!g_queue_is_empty (&priv->pages))
    {
      GObject *page = g_queue_pop_head (&priv->pages);
      GObject *model = g_object_get_qdata (page, mex_explorer_model_quark);

      g_object_set_qdata (model, mex_explorer_proxy_quark, NULL);
      g_object_set_qdata (model, mex_explorer_container_quark, NULL);

      /* Note, no need to destroy the page, it's parented to this container
       * and will be destroyed when we chain up.
       */

      if (MEX_IS_AGGREGATE_MODEL (model))
        {
          g_signal_handlers_disconnect_by_func (model,
                                                mex_explorer_model_added_cb,
                                                object);
          g_signal_handlers_disconnect_by_func (model,
                                                mex_explorer_model_removed_cb,
                                                object);
        }
    }

  G_OBJECT_CLASS (mex_explorer_parent_class)->dispose (object);
}

static void
mex_explorer_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_explorer_parent_class)->finalize (object);
}

static void
mex_explorer_notify_focused_cb (MxFocusManager *manager,
                                GParamSpec     *pspec,
                                MexExplorer    *explorer)
{
  ClutterActor *focused;

  MexExplorerPrivate *priv = explorer->priv;

  focused = (ClutterActor *)mx_focus_manager_get_focused (manager);

  /* Keep track of our last focused descendant, for use in
   * mex_explorer_get_focused_model().
   */
  if (focused)
    {
      ClutterActor *parent, *model_child;

      model_child = NULL;
      while (focused)
        {
          parent = clutter_actor_get_parent (focused);

          if (!model_child && g_object_get_qdata (G_OBJECT (focused),
                                                  mex_explorer_model_quark))
            model_child = focused;

          if (parent == (ClutterActor *)explorer)
            {
              priv->last_focus = model_child;
              return;
            }

          focused = parent;
        }
    }
}

static void
mex_explorer_map (ClutterActor *actor)
{
  MxFocusManager *manager;

  CLUTTER_ACTOR_CLASS (mex_explorer_parent_class)->map (actor);

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  g_signal_connect (manager, "notify::focused",
                    G_CALLBACK (mex_explorer_notify_focused_cb), actor);
  mex_explorer_notify_focused_cb (manager, NULL, MEX_EXPLORER (actor));
}

static void
mex_explorer_unmap (ClutterActor *actor)
{
  MxFocusManager *manager;

  manager = mx_focus_manager_get_for_stage ((ClutterStage *)
                                            clutter_actor_get_stage (actor));
  g_signal_handlers_disconnect_by_func (manager,
                                        mex_explorer_notify_focused_cb,
                                        actor);

  CLUTTER_ACTOR_CLASS (mex_explorer_parent_class)->unmap (actor);
}

static void
mex_explorer_class_init (MexExplorerClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexExplorerPrivate));

  object_class->get_property = mex_explorer_get_property;
  object_class->set_property = mex_explorer_set_property;
  object_class->dispose = mex_explorer_dispose;
  object_class->finalize = mex_explorer_finalize;

  actor_class->map = mex_explorer_map;
  actor_class->unmap = mex_explorer_unmap;

  pspec = g_param_spec_object ("root-model",
                               "Root model",
                               "The Mex(Aggregate)Model that represents the "
                               "root node of the model hierarchy.",
                               G_TYPE_OBJECT,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ROOT_MODEL, pspec);

  pspec = g_param_spec_object ("model",
                               "Model",
                               "The MexModel currently being shown.",
                               G_TYPE_OBJECT,
                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MODEL, pspec);

  pspec = g_param_spec_int ("n-preview-items",
                            "N Preview Items",
                            "The number of preview items to show for "
                            "aggregate models. -1 for no limit.",
                            -1, G_MAXINT, 8,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_N_PREVIEW_ITEMS, pspec);

  pspec = g_param_spec_uint ("depth",
                             "Depth",
                             "The number of models currently in the explorer.",
                             0, G_MAXUINT, 0,
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DEPTH, pspec);

  pspec = g_param_spec_boolean ("touch-mode",
                                "Touch Mode",
                                "Enable touch-screen operation.",
                                FALSE,
                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TOUCH_MODE, pspec);

  signals[PAGE_CREATED] =
    g_signal_new ("page-created",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexExplorerClass, page_created),
                  NULL, NULL,
                  mex_marshal_VOID__OBJECT_POINTER,
                  G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_POINTER);

  signals[HEADER_ACTIVATED] =
    g_signal_new ("header-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexExplorerClass, header_activated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, G_TYPE_OBJECT);

  mex_explorer_model_quark = g_quark_from_static_string ("mex-explorer-model");
  mex_explorer_proxy_quark = g_quark_from_static_string ("mex-explorer-proxy");
  mex_explorer_container_quark =
    g_quark_from_static_string ("mex-explorer-container");
  mex_explorer_explorer_quark =
    g_quark_from_static_string ("mex-explorer-explorer");
}

static void
mex_explorer_prune_children (MexExplorer *self)
{
  ClutterStage *stage;
  MxActorManager *manager;

  MexExplorerPrivate *priv = self->priv;

  stage = (ClutterStage *)clutter_actor_get_stage (CLUTTER_ACTOR (self));
  manager = stage ? mx_actor_manager_get_for_stage (stage) : NULL;

  while (priv->to_destroy)
    {
      GObject *model;
      ClutterActor *page;

      page = priv->to_destroy->data;
      model = g_object_get_qdata (G_OBJECT (page),
                                  mex_explorer_model_quark);

      g_object_set_qdata (model, mex_explorer_proxy_quark, NULL);
      g_object_set_qdata (model, mex_explorer_container_quark, NULL);

      if (MEX_IS_AGGREGATE_MODEL (model))
        {
          g_signal_handlers_disconnect_by_func (model,
                                                mex_explorer_model_added_cb,
                                                self);
          g_signal_handlers_disconnect_by_func (model,
                                                mex_explorer_model_removed_cb,
                                                self);
        }

      if (manager && CLUTTER_IS_CONTAINER (page))
        {
          clutter_actor_set_opacity (page, 0);
          mx_actor_manager_remove_container (manager, CLUTTER_CONTAINER (page));
        }
      else
        clutter_actor_destroy (page);

      priv->to_destroy =
        g_list_delete_link (priv->to_destroy, priv->to_destroy);
    }
}

static void
mex_explorer_init (MexExplorer *self)
{
  MexExplorerPrivate *priv = self->priv = EXPLORER_PRIVATE (self);

  priv->n_preview_items = 8;

  g_queue_init (&priv->pages);
}

static void
mex_explorer_clear_cb (gpointer data, gpointer user_data)
{
  MexModel *model;

  ClutterActor *child = data;
  MexExplorer *self = user_data;
  MexExplorerPrivate *priv = self->priv;

  model = g_object_get_qdata (G_OBJECT (child), mex_explorer_model_quark);

  if ((!priv->root_model) || (model != priv->root_model))
    priv->to_destroy = g_list_prepend (priv->to_destroy, child);
}

static void
mex_explorer_column_object_created_cb (MexProxy     *proxy,
                                       MexContent   *content,
                                       ClutterActor *object,
                                       gpointer      column)
{
  const gchar *mime_type;

  /* filter out folders from column views */
  mime_type = mex_content_get_metadata (content,
                                        MEX_CONTENT_METADATA_MIMETYPE);

  if (g_strcmp0 (mime_type, "x-grl/box") == 0)
    {
      g_signal_stop_emission_by_name (proxy, "object-created");
      return;
    }
}

static void
mex_explorer_column_activated_cb (ClutterActor *column,
                                  MexExplorer  *explorer)
{
  MexModel *model = g_object_get_qdata (G_OBJECT (column),
                                        mex_explorer_model_quark);
  if (model)
    g_signal_emit (explorer, signals[HEADER_ACTIVATED], 0, model);
}

static void
mex_explorer_unset_container_cb (gpointer  model,
                                 GObject  *column)
{
  g_object_set_qdata (G_OBJECT (model), mex_explorer_container_quark, NULL);

  g_signal_handlers_disconnect_by_func (model, model_length_changed_cb, column);
}

static void
mex_explorer_show_maybe_focus (ClutterActor *column,
                               ClutterActor *box,
                               MexExplorer  *explorer)
{
  ClutterActor *column_view;
  MexExplorerPrivate *priv = explorer->priv;

  column_view = clutter_actor_get_parent (clutter_actor_get_parent (column));
  g_assert (MEX_IS_COLUMN_VIEW (column_view));

  /* Show the column-view the column is in */
  clutter_actor_show (column_view);

  if (priv->has_temporary_focus)
    {
      ClutterActor *stage = clutter_actor_get_stage (CLUTTER_ACTOR (explorer));

      /* An actor has been added and we had temporary focus - try to
       * re-focus ourselves.
       */
      if (stage)
        {
          MxFocusManager *manager =
            mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));
          mx_focus_manager_move_focus (manager, MX_FOCUS_DIRECTION_OUT);
          mx_focus_manager_push_focus_with_hint (manager,
                                                 MX_FOCUSABLE (explorer),
                                                 MX_FOCUS_HINT_PRIOR);
        }
    }
}

/* Add the item count to the title of the column */
static gboolean
transform_title_cb (GBinding *binding,
                    const GValue *source_value,
                    GValue *target_value,
                    MexModel *model)
{
  gchar *new_target_value;
  MexModel *orig_model = mex_model_get_model (model);

  new_target_value = g_strdup_printf ("%s (%d)",
                                      g_value_get_string (source_value),
                                      mex_model_get_length (orig_model));

  g_value_set_string (target_value, new_target_value);

  g_free (new_target_value);

 return TRUE;
}

/* Update the column title when the number of items changes */
static void
model_length_changed_cb (MexModel *model,
                         GParamSpec *pspec,
                         MexColumn *column)
{
  g_object_notify (G_OBJECT (model), "title");
}

static void
mex_explorer_model_added_cb (MexAggregateModel *aggregate,
                             MexModel          *model,
                             MexExplorer       *explorer)
{
  MexModel *view_model;
  MexProxy *proxy;
  gchar *placeholder_text;
  ClutterContainer *container;
  ClutterActor *column_view, *label;
  gboolean display_item_count, always_visible;
  gchar *title;
  MexColumn *column;
  const MexModelCategoryInfo *c_info;
  gchar *category;

  MexExplorerPrivate *priv = explorer->priv;

  view_model = mex_view_model_new (model);
  if (priv->n_preview_items >= 0)
    mex_view_model_set_limit (MEX_VIEW_MODEL (view_model),
                              priv->n_preview_items);

  /* get the category information for this model */
  g_object_get (model, "category", &category, NULL);
  c_info = mex_model_manager_get_category_info (mex_model_manager_get_default (),
                                                category);
  g_free (category);

  /* group using the primary group key */
  if (c_info->primary_group_by_key)
    mex_view_model_set_group_by (MEX_VIEW_MODEL (view_model),
                                 c_info->primary_group_by_key);


  container = g_object_get_qdata (G_OBJECT (aggregate),
                                  mex_explorer_container_quark);

  /* Create a new column for this model */
  column_view = mex_column_view_new (NULL, NULL);
  column = mex_column_view_get_column (MEX_COLUMN_VIEW (column_view));
  if (priv->touch_mode)
    mex_column_set_collapse_on_focus (column, FALSE);

  g_object_get (model, "display-item-count", &display_item_count,
                "title", &title, NULL);

  /* Cross reference column_view <-> model */
  g_object_set_qdata (G_OBJECT (column_view),
                      mex_explorer_model_quark,
                      model);
  g_object_set_qdata (G_OBJECT (model),
                      mex_explorer_container_quark,
                      column_view);
  g_object_set_qdata (G_OBJECT (model),
                      mex_explorer_explorer_quark,
                      explorer);

  mx_stylable_set_style_class (MX_STYLABLE (column_view), title);
  g_free (title);

  if (!display_item_count)
    g_object_bind_property (model, "title", column_view, "label",
                            G_BINDING_SYNC_CREATE);
  else
    g_object_bind_property_full (model, "title",
                                 column_view, "label",
                                 G_BINDING_SYNC_CREATE,
                                 (GBindingTransformFunc)transform_title_cb,
                                 NULL,
                                 view_model,
                                 NULL);

  g_signal_connect (model, "notify::length",
                    G_CALLBACK (model_length_changed_cb), column);

  g_object_bind_property (model, "icon-name",
                          column_view, "icon-name",
                          G_BINDING_SYNC_CREATE);

  g_object_get (G_OBJECT (model),
                "placeholder-text", &placeholder_text,
                "always-visible", &always_visible,
                NULL);

  /* placeholder actor for when there are no items */
  label = g_object_new (MX_TYPE_LABEL,
                        "style-class",
                        (placeholder_text && placeholder_text[0]) ? "placeholder-label" : "",
                        "natural-width", 426.0,
                        "natural-height", 239.0,
                        "line-wrap", TRUE,
                        NULL);
  g_object_bind_property (model, "placeholder-text",
                          label, "text",
                          G_BINDING_SYNC_CREATE);
  mex_column_view_set_placeholder_actor (MEX_COLUMN_VIEW (column_view), label);

  /* If there's no place-holder text, start hidden and show when there's an
   * actor added.
   */
  if ((!placeholder_text || !(*placeholder_text)) && !always_visible)
    {
      clutter_actor_hide (column_view);

      /* FIXME: This column will stay hidden if there's placeholder text
       *        set after this point
       */
      g_signal_connect (column, "actor-added",
                        G_CALLBACK (mex_explorer_show_maybe_focus),
                        explorer);
    }

  /* Create the proxy to create objects for the column */
  proxy = mex_content_proxy_new (view_model,
                                 CLUTTER_CONTAINER (column),
                                 MEX_TYPE_CONTENT_BOX);
  mex_content_proxy_set_stage (MEX_CONTENT_PROXY (proxy),
                               (ClutterStage *)clutter_actor_get_stage (
                                 CLUTTER_ACTOR (explorer)));

  /* Setup qdata and weak references so we can get references back to
   * the container/the explorer/the model and so that we can forward
   * the appropriate signals.
   */
  g_object_weak_ref (G_OBJECT (column_view),
                     (GWeakNotify)g_object_unref,
                     proxy);
  g_signal_connect (proxy, "object-created",
                    G_CALLBACK (mex_explorer_column_object_created_cb),
                    column);

  g_signal_connect (column_view, "activated",
                    G_CALLBACK (mex_explorer_column_activated_cb),
                    explorer);

  clutter_container_add_actor (container, column_view);

  g_object_weak_ref (G_OBJECT (column_view), mex_explorer_unset_container_cb, model);

  g_object_unref (view_model);
}

static void
mex_explorer_model_removed_cb (MexAggregateModel *aggregate,
                               MexModel          *model,
                               MexExplorer       *explorer)
{
  ClutterActor *column_view = g_object_get_qdata (G_OBJECT (model),
                                                  mex_explorer_container_quark);
  ClutterActor *parent = clutter_actor_get_parent (column_view);
  ClutterActor *column = (ClutterActor*) mex_column_view_get_column (MEX_COLUMN_VIEW (column_view));

  /* Disconnect container */
  g_signal_handlers_disconnect_by_func (column,
                                        G_CALLBACK (mex_explorer_show_maybe_focus),
                                        explorer);

  /* Remove the weak reference - it's possible (and likely) the model will
   * disappear before we get to the callback.
   */
  g_object_weak_unref (G_OBJECT (column_view),
                       mex_explorer_unset_container_cb, model);
  g_object_set_qdata (G_OBJECT (model), mex_explorer_container_quark, NULL);
  g_object_set_qdata (G_OBJECT (model), mex_explorer_explorer_quark, NULL);

  /* Remove the column representing this model */
  clutter_container_remove_actor (CLUTTER_CONTAINER (parent), column_view);
}

static void
mex_explorer_open_child_complete (ClutterActor *child,
                                  gpointer explorer)
{
  MexExplorerPrivate *priv = MEX_EXPLORER (explorer)->priv;

  priv->in_transition = FALSE;
}

static void
mex_explorer_open_child (ClutterActor *old_child,
                         gpointer      explorer)
{
  MexExplorerPrivate *priv = MEX_EXPLORER (explorer)->priv;
  ClutterActorBox box;


  /* swap the two scenes */
  clutter_actor_hide (priv->previous_child);
  clutter_actor_show (priv->current_child);

  /* ensure the child has focus */
  mex_push_focus (MX_FOCUSABLE (priv->current_child));

  mex_scene_get_current_target (MEX_SCENE (priv->current_child), &box);

  mex_scene_open (MEX_SCENE (priv->current_child), &box,
                  mex_explorer_open_child_complete, explorer);


  clutter_actor_animate (priv->current_child, CLUTTER_EASE_OUT_QUINT,
                         ANIMATION_DURATION,
                         "opacity", (guchar) 255,
                         NULL);

  clutter_actor_animate (priv->previous_child, CLUTTER_EASE_IN_QUINT,
                         ANIMATION_DURATION,
                         "opacity", (guchar) 0, NULL);


  mex_explorer_prune_children (explorer);
}

static void
mex_explorer_present (MexExplorer  *explorer,
                      ClutterActor *child)
{
  MexExplorerPrivate *priv = explorer->priv;
  ClutterActorBox target;

  if (priv->in_transition)
    return;

  priv->previous_child = priv->current_child;
  priv->current_child = child;

  if (!priv->previous_child)
    return;

  /* remove focus from the current child */
  mex_push_focus (MX_FOCUSABLE (child));

  /* transition between the old child and the new */
  priv->in_transition = TRUE;

  if (!CLUTTER_ACTOR_IS_VISIBLE (explorer))
    {
      mex_explorer_open_child (priv->current_child, explorer);
      return;
    }

  /* get the target position */
  mex_scene_get_current_target (MEX_SCENE (child), &target);

  /* close the old scene */
  mex_scene_close (MEX_SCENE (priv->previous_child), &target,
                   mex_explorer_open_child, explorer);

  /* hide the new child until the old child has "closed" */
  clutter_actor_hide (child);
}

/* public functions */

ClutterActor *
mex_explorer_new (void)
{
  return g_object_new (MEX_TYPE_EXPLORER, NULL);
}

void
mex_explorer_set_root_model (MexExplorer *explorer, MexModel *model)
{
  MexExplorerPrivate *priv;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));
  g_return_if_fail (MEX_IS_MODEL (model));

  priv = explorer->priv;
  if (priv->root_model == model)
    return;

  if (priv->root_model)
    {
      priv->root_model = NULL;
      clutter_container_foreach (CLUTTER_CONTAINER (explorer),
                                 CLUTTER_CALLBACK (mex_explorer_clear_cb),
                                 explorer);
    }

  if (model)
    {
      /* We adopt the object reference, which is unref'd when the associated
       * page is destroyed (via a weak reference).
       */
      priv->root_model = model;
      mex_explorer_push_model (explorer, model);
    }

  g_object_notify (G_OBJECT (explorer), "root-model");
}

MexModel *
mex_explorer_get_root_model (MexExplorer *explorer)
{
  g_return_val_if_fail (MEX_IS_EXPLORER (explorer), NULL);
  return explorer->priv->root_model;
}

void
mex_explorer_push_model (MexExplorer *explorer,
                         MexModel    *model)
{
  MexExplorerPrivate *priv;
  ClutterActor *page;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));
  g_return_if_fail (MEX_IS_MODEL (model));

  page = NULL;
  priv = explorer->priv;

  if (priv->in_transition)
    return;

  if (MEX_IS_AGGREGATE_MODEL (model) &&
      (model != mex_explorer_get_model (explorer)))
    {
      GList *m;
      const GList *models;
      ClutterActor *resizing_hbox;

      /* Create container actors */
      resizing_hbox = mex_resizing_hbox_new ();
      mx_stylable_set_style_class (MX_STYLABLE (resizing_hbox),
                                   "column-view");

      if (model != priv->root_model)
        {
          mex_resizing_hbox_set_max_depth (
            MEX_RESIZING_HBOX (resizing_hbox), 1);
          mex_resizing_hbox_set_vertical_depth_scale (
            MEX_RESIZING_HBOX (resizing_hbox), 0.98f);
        }

      /* Store the container on the aggregate model */
      g_object_set_qdata (G_OBJECT (model), mex_explorer_container_quark,
                          resizing_hbox);

      /* Create columns for sub-models */
      models = mex_aggregate_model_get_models (MEX_AGGREGATE_MODEL (model));
      for (m = (GList *)models; m; m = m->next)
        mex_explorer_model_added_cb (MEX_AGGREGATE_MODEL (model),
                                     MEX_MODEL (m->data),
                                     explorer);

      /* Connect to added/removed signals */
      g_signal_connect (model, "model-added",
                        G_CALLBACK (mex_explorer_model_added_cb), explorer);
      g_signal_connect (model, "model-removed",
                        G_CALLBACK (mex_explorer_model_removed_cb), explorer);

      page = resizing_hbox;
    }
  else
    {
      page = mex_grid_view_new (model);
    }

  if (page)
    {
      g_object_weak_ref (G_OBJECT (page), (GWeakNotify)g_object_unref, model);
      g_object_set_qdata (G_OBJECT (page), mex_explorer_model_quark, model);

      g_queue_push_tail (&priv->pages, page);
      clutter_container_add_actor (CLUTTER_CONTAINER (explorer), page);

      g_object_notify (G_OBJECT (explorer), "model");
      g_object_notify (G_OBJECT (explorer), "depth");
    }

  mex_explorer_present (explorer, page);
}

void
mex_explorer_replace_model (MexExplorer *explorer,
                            MexModel    *model)
{
  GObject *page;
  MexModel *old_model;
  MexExplorerPrivate *priv;
  ClutterContainer *container;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));
  g_return_if_fail (MEX_IS_MODEL (model));

  priv = explorer->priv;
  old_model = mex_explorer_get_model (explorer);

  if (priv->in_transition)
    return;

  if (model == old_model)
    return;

  if (MEX_IS_AGGREGATE_MODEL (old_model) != MEX_IS_AGGREGATE_MODEL (model))
    {
      g_warning (G_STRLOC ": Cannot replace mismatching model types");
      return;
    }

  /* Store the container on the new model */
  container = g_object_get_qdata (G_OBJECT (old_model),
                                  mex_explorer_container_quark);
  g_object_set_qdata (G_OBJECT (model), mex_explorer_container_quark,
                      container);

  if (MEX_IS_AGGREGATE_MODEL (old_model))
    {
      GList *m;
      const GList *models;

      /* Disconnect added/removed signals from old model */
      g_signal_handlers_disconnect_by_func (old_model,
                                            mex_explorer_model_added_cb,
                                            explorer);
      g_signal_handlers_disconnect_by_func (old_model,
                                            mex_explorer_model_removed_cb,
                                            explorer);

      /* Remove columns for sub-models of old model */
      models = mex_aggregate_model_get_models (MEX_AGGREGATE_MODEL (old_model));
      for (m = (GList *)models; m; m = m->next)
        mex_explorer_model_removed_cb (MEX_AGGREGATE_MODEL (old_model),
                                       MEX_MODEL (m->data),
                                       explorer);

      /* We've now disconnected the old model, now connect the new model */

      /* Create columns for sub-models of the new model */
      models = mex_aggregate_model_get_models (MEX_AGGREGATE_MODEL (model));
      for (m = (GList *)models; m; m = m->next)
        mex_explorer_model_added_cb (MEX_AGGREGATE_MODEL (model),
                                     MEX_MODEL (m->data),
                                     explorer);

      /* Connect to added/removed signals of the new model */
      g_signal_connect (model, "model-added",
                        G_CALLBACK (mex_explorer_model_added_cb), explorer);
      g_signal_connect (model, "model-removed",
                        G_CALLBACK (mex_explorer_model_removed_cb), explorer);
    }
  else
    {
      /* In the non-aggregate case, it's slightly easier - we can just
       * set the model on the proxy and that should handle most everything.
       */

      /* Set proxy qdata on new model */
      MexProxy *proxy = g_object_get_qdata (G_OBJECT (old_model),
                                            mex_explorer_proxy_quark);
      g_object_set_qdata (G_OBJECT (model), mex_explorer_proxy_quark, proxy);

      /* Replace model on proxy */
      g_object_set (G_OBJECT (proxy), "model", model, NULL);

      /* Remove proxy qdata from old model */
      g_object_set_qdata (G_OBJECT (old_model), mex_explorer_proxy_quark, NULL);
    }

  /* Remove qdata from old model */
  g_object_set_qdata (G_OBJECT (old_model), mex_explorer_container_quark, NULL);

  /* Associate the new model with the current page */
  page = g_queue_peek_tail (&priv->pages);
  g_object_weak_unref (page, (GWeakNotify)g_object_unref, old_model);
  g_object_weak_ref (page, (GWeakNotify)g_object_unref, model);
  g_object_set_qdata (page, mex_explorer_model_quark, model);

  g_object_unref (old_model);
}

void
mex_explorer_pop_model (MexExplorer *explorer)
{
  MexExplorerPrivate *priv;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));

  priv = explorer->priv;
  if (g_queue_get_length (&priv->pages) <= 1)
    return;

  if (priv->in_transition)
    return;

  priv->to_destroy = g_list_prepend (priv->to_destroy,
                                     g_queue_pop_tail (&priv->pages));

  mex_explorer_present (explorer,
                        (ClutterActor *)g_queue_peek_tail (&priv->pages));

  g_object_notify (G_OBJECT (explorer), "model");
  g_object_notify (G_OBJECT (explorer), "depth");
}

void
mex_explorer_pop_to_root (MexExplorer *explorer)
{
  MexExplorerPrivate *priv;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));

  priv = explorer->priv;
  if (g_queue_get_length (&priv->pages) <= 1)
    return;


  if (priv->in_transition)
    return;

  while (g_queue_get_length (&priv->pages) > 1)
    priv->to_destroy = g_list_prepend (priv->to_destroy,
                                       g_queue_pop_tail (&priv->pages));

  mex_explorer_present (explorer,
                        (ClutterActor *)g_queue_peek_tail (&priv->pages));

  g_object_notify (G_OBJECT (explorer), "model");
  g_object_notify (G_OBJECT (explorer), "depth");
}

MexModel *
mex_explorer_get_model (MexExplorer *explorer)
{
  GObject *object;
  g_return_val_if_fail (MEX_IS_EXPLORER (explorer), NULL);
  object = g_queue_peek_tail (&explorer->priv->pages);
  if (object)
    return g_object_get_qdata (object, mex_explorer_model_quark);
  return NULL;
}

GList *
mex_explorer_get_models (MexExplorer *explorer)
{
  GList *models, *p;
  MexExplorerPrivate *priv;

  g_return_val_if_fail (MEX_IS_EXPLORER (explorer), NULL);

  priv = explorer->priv;
  models = NULL;

  for (p = priv->pages.tail; p; p = p->prev)
    {
      GObject *page = p->data;
      MexModel *model = g_object_get_qdata (page, mex_explorer_model_quark);

      if (model)
        models = g_list_prepend (models, model);
      else
        g_warning (G_STRLOC ": Found page with no associated model");
    }

  return models;
}

static gint
mex_explorer_find_model_cb (gconstpointer a,
                            gconstpointer b)
{
  if (g_object_get_qdata (G_OBJECT (a), mex_explorer_model_quark) == b)
    return 0;
  else
    return -1;
}

void
mex_explorer_remove_model (MexExplorer *explorer,
                           MexModel    *model)
{
  GList *p;
  MexExplorerPrivate *priv;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));
  g_return_if_fail (MEX_IS_MODEL (model));

  priv = explorer->priv;

  /* Don't allow popping of the root model */
  if (priv->root_model == model)
    {
      g_warning (G_STRLOC ": Attempt to remove root model");
      return;
    }

  /* If the model is the tail page, just pop */
  if (mex_explorer_get_model (explorer) == model)
    {
      mex_explorer_pop_model (explorer);
      return;
    }

  /* Check if the model is in the to-destroy list, if so, ignore */
  for (p = priv->to_destroy; p; p = p->next)
    if (g_object_get_qdata (p->data, mex_explorer_model_quark) == model)
      return;

  /* The model is on a non-visible page, so we can just remove it from
   * the queue. (alternatively, the model isn't in the explorer - we'll
   * find out and warn if so).
   */
  p = g_queue_find_custom (&priv->pages, model, mex_explorer_find_model_cb);
  if (!p)
    {
      g_warning (G_STRLOC ": Attempt to remove unknown model");
      return;
    }

  /* Remove qdata on model, destroy page and remove it from the pages list */
  g_object_set_qdata (G_OBJECT (model), mex_explorer_proxy_quark, NULL);
  g_object_set_qdata (G_OBJECT (model), mex_explorer_container_quark, NULL);

  if (MEX_IS_AGGREGATE_MODEL (model))
    {
      g_signal_handlers_disconnect_by_func (model,
                                            mex_explorer_model_added_cb,
                                            explorer);
      g_signal_handlers_disconnect_by_func (model,
                                            mex_explorer_model_removed_cb,
                                            explorer);
    }

  clutter_actor_destroy (p->data);
  g_queue_delete_link (&priv->pages, p);
}

guint
mex_explorer_get_depth (MexExplorer *explorer)
{
  g_return_val_if_fail (MEX_IS_EXPLORER (explorer), 0);
  return explorer->priv->pages.length;
}

MexModel *
mex_explorer_get_focused_model (MexExplorer *explorer)
{
  MexExplorerPrivate *priv;

  g_return_val_if_fail (MEX_IS_EXPLORER (explorer), NULL);

  priv = explorer->priv;
  return priv->last_focus ?
    g_object_get_qdata (G_OBJECT (priv->last_focus), mex_explorer_model_quark) :
    mex_explorer_get_model (explorer);
}

void
mex_explorer_set_focused_model (MexExplorer *explorer,
                                MexModel    *model)
{
  ClutterActor *stage;
  MxFocusManager *manager;
  GObject *object;
  GList *c, *children;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));

  object = g_queue_peek_tail (&explorer->priv->pages);
  if (!object)
    return;

  if (!CLUTTER_IS_CONTAINER (object))
    {
      mex_push_focus (MX_FOCUSABLE (object));
      return;
    }

  children = clutter_container_get_children (CLUTTER_CONTAINER (object));
  for (c = children; c; c = c->next)
    {
      MexModel *m;

      ClutterActor *column_view = (ClutterActor *) c->data;

      m = g_object_get_qdata (G_OBJECT (column_view), mex_explorer_model_quark);

      if (m == model)
        {
          stage = clutter_actor_get_stage (CLUTTER_ACTOR (explorer));


          manager = mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));

          mx_focus_manager_push_focus_with_hint (manager,
                                                 MX_FOCUSABLE (column_view),
                                                 MX_FOCUS_HINT_FIRST);
          break;
        }
    }
  g_list_free (children);
}

void
mex_explorer_focus_content (MexExplorer       *explorer,
                            const MexContent  *content)
{
  GObject *object;
  GList *c, *children;
  ClutterContainer *container;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));
  g_return_if_fail (MEX_IS_CONTENT (content));

  object = g_queue_peek_tail (&explorer->priv->pages);
  if (!object)
    return;

  container = g_object_get_qdata (object, mex_explorer_container_quark);
  if (!container)
    return;

  children = clutter_container_get_children (container);
  for (c = children; c; c = c->next)
    {
      if (MEX_IS_RESIZING_HBOX (container))
        {
          GList *s, *sub_children;

          gboolean found = FALSE;
          ClutterContainer *sub_container;

          /* assert that the assumption about the actor hierarchy is correct */
          g_assert (MEX_IS_COLUMN_VIEW (c->data));
          sub_container =
            CLUTTER_CONTAINER (mex_column_view_get_column (c->data));
          g_assert (MEX_IS_COLUMN (sub_container));

          sub_children = clutter_container_get_children (sub_container);

          for (s = sub_children; s; s = s->next)
            {
              MexContentView *cv = s->data;

              if (MEX_IS_CONTENT_VIEW (s->data) &&
                  (mex_content_view_get_content (cv) == content))
                {
                  found = TRUE;
                  if (MX_IS_FOCUSABLE (s->data))
                    mex_push_focus (MX_FOCUSABLE (s->data));
                  break;
                }
            }
          g_list_free (sub_children);

          if (found)
            break;
        }
      else if (MEX_IS_GRID (container))
        {
          MexContentView *cv = c->data;

          if (MEX_IS_CONTENT_VIEW (c->data) &&
              mex_content_view_get_content (cv) == content)
            {
              if (MX_IS_FOCUSABLE (c->data))
                mex_push_focus (MX_FOCUSABLE (c->data));
              break;
            }
        }
    }
  g_list_free (children);
}

ClutterContainer *
mex_explorer_get_container_for_model (MexExplorer *explorer,
                                      MexModel    *model)
{
  g_return_val_if_fail (MEX_IS_EXPLORER (explorer), NULL);
  g_return_val_if_fail (MEX_IS_MODEL (model), NULL);

  return g_object_get_qdata (G_OBJECT (model), mex_explorer_container_quark);
}

void
mex_explorer_set_n_preview_items (MexExplorer *explorer,
                                  gint         n_preview_items)
{
  MexExplorerPrivate *priv;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));

  priv = explorer->priv;
  if (priv->n_preview_items != n_preview_items)
    {
      priv->n_preview_items = n_preview_items;
      g_object_notify (G_OBJECT (explorer), "n-preview-items");
    }
}

gint
mex_explorer_get_n_preview_items (MexExplorer *explorer)
{
  g_return_val_if_fail (MEX_IS_EXPLORER (explorer), 0);
  return explorer->priv->n_preview_items;
}

static void
mex_explorer_set_touch_mode_recursive (GList    *children,
                                       gboolean  on)
{
  GList *l;

  for (l = children; l; l = l->next)
    {
      if (MEX_IS_COLUMN_VIEW (l->data))
        mex_column_set_collapse_on_focus (
                  mex_column_view_get_column (MEX_COLUMN_VIEW (l->data)), !on);
      else if (CLUTTER_IS_CONTAINER (l->data))
        {
          GList *sub_children = clutter_container_get_children (l->data);
          mex_explorer_set_touch_mode_recursive (sub_children, on);
          g_list_free (sub_children);
        }
    }
}

void
mex_explorer_set_touch_mode (MexExplorer *explorer,
                             gboolean     on)
{
  MexExplorerPrivate *priv;

  g_return_if_fail (MEX_IS_EXPLORER (explorer));

  priv = explorer->priv;
  if (priv->touch_mode != on)
    {
      priv->touch_mode = on;
      mex_explorer_set_touch_mode_recursive (priv->pages.head, on);
      g_object_notify (G_OBJECT (explorer), "touch-mode");
    }
}

gboolean
mex_explorer_get_touch_mode (MexExplorer *explorer)
{
  g_return_val_if_fail (MEX_IS_EXPLORER (explorer), FALSE);
  return explorer->priv->touch_mode;
}
