/*
 * mex-networks - Connection Manager UI for Media Explorer 
 * Copyright © 2010-2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#include "mtn-services-view.h"
#include "mtn-connman.h"
#include "mtn-service-model.h"
#include "mtn-service-tile.h"

struct _MtnServicesViewPrivate {
    MtnServiceModel *model;
};

G_DEFINE_TYPE (MtnServicesView, mtn_services_view, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MTN_TYPE_SERVICES_VIEW, MtnServicesViewPrivate))

enum
{
    PROP_0,
    PROP_CONNMAN,
};

enum
{
  CONNECTION_REQUESTED,
  LAST_SIGNAL
};
static guint mtn_services_view_signals[LAST_SIGNAL] = {0};

static void
mtn_services_view_get_property (GObject *object, guint property_id,
                               GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_services_view_set_property (GObject *object, guint property_id,
                               const GValue *value, GParamSpec *pspec)
{
    MtnServicesView *view;

    view = MTN_SERVICES_VIEW (object);

    switch (property_id) {
    case PROP_CONNMAN:
        mtn_services_view_set_connman (view, MTN_CONNMAN (g_value_get_object (value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_tile_clicked (MtnServiceTile *tile, MtnServicesView *view)
{
    g_signal_emit (view,
                   mtn_services_view_signals[CONNECTION_REQUESTED],
                   0,
                   mtn_service_tile_get_object_path (tile));
}

static void
_row_added (MtnServiceModel  *model,
            ClutterModelIter *iter,
            MtnServicesView  *view)
{
    guint index;
    ClutterActor *actor;

    clutter_model_iter_get (iter,
                            MTN_SERVICE_MODEL_COL_INDEX, &index,
                            MTN_SERVICE_MODEL_COL_WIDGET, &actor,
                            -1);

    mx_box_layout_add_actor (MX_BOX_LAYOUT (view), actor, index);
    g_signal_connect (actor, "clicked",
                      G_CALLBACK (_tile_clicked), view);
}

static void
_row_changed (MtnServiceModel  *model,
              ClutterModelIter *iter,
              MtnServicesView  *view)
{
    guint index;
    ClutterActor *actor, *sibling;
    GList *children;

    clutter_model_iter_get (iter,
                            MTN_SERVICE_MODEL_COL_INDEX, &index,
                            MTN_SERVICE_MODEL_COL_WIDGET, &actor,
                            -1);

    children = clutter_container_get_children (CLUTTER_CONTAINER (view));
    sibling = CLUTTER_ACTOR (g_list_nth_data (children, index)); 
    g_list_free (children);

    clutter_container_raise_child (CLUTTER_CONTAINER (view),
                                   actor,
                                   sibling);
}

static void
_row_removed (MtnServiceModel  *model,
              ClutterModelIter *iter,
              MtnServicesView  *view)
{
    ClutterActor *actor;

    clutter_model_iter_get (iter,
                            MTN_SERVICE_MODEL_COL_WIDGET, &actor,
                            -1);
    
    clutter_container_remove_actor (CLUTTER_CONTAINER (view), actor);
}

static void
mtn_services_view_dispose (GObject *object)
{
    MtnServicesView *view;

    view = MTN_SERVICES_VIEW (object);
    if (view->priv->model) {
        g_object_unref (view->priv->model);
        view->priv->model = NULL;
    }

    G_OBJECT_CLASS (mtn_services_view_parent_class)->dispose (object);
}

static void
mtn_services_view_finalize (GObject *object)
{
    G_OBJECT_CLASS (mtn_services_view_parent_class)->finalize (object);
}

static void
mtn_services_view_class_init (MtnServicesViewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MtnServicesViewPrivate));

    object_class->get_property = mtn_services_view_get_property;
    object_class->set_property = mtn_services_view_set_property;
    object_class->dispose = mtn_services_view_dispose;
    object_class->finalize = mtn_services_view_finalize;

    g_object_class_install_property (object_class,
                                     PROP_CONNMAN,
                                     g_param_spec_object ("connman",
                                                          "Connman",
                                                          "The MtnConnman instance to use",
                                                          MTN_TYPE_CONNMAN,
                                                          G_PARAM_WRITABLE));
    mtn_services_view_signals[CONNECTION_REQUESTED] =
        g_signal_new ("connection-requested",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MtnServicesViewClass,
                                       connection_requested),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, G_TYPE_STRING);

}

static void
mtn_services_view_init (MtnServicesView *view)
{
    view->priv = GET_PRIVATE (view);

    mx_box_layout_set_orientation (MX_BOX_LAYOUT (view),
                                   MX_ORIENTATION_VERTICAL);

    view->priv->model = mtn_service_model_new (NULL);
    g_signal_connect (view->priv->model, "row-added",
                      G_CALLBACK (_row_added), view);
    g_signal_connect (view->priv->model, "row-changed",
                      G_CALLBACK (_row_changed), view);
    g_signal_connect (view->priv->model, "row-removed",
                      G_CALLBACK (_row_removed), view);
    clutter_model_foreach (CLUTTER_MODEL (view->priv->model),
                           (ClutterModelForeachFunc)_row_added,
                           view);
}

void
mtn_services_view_set_connman (MtnServicesView *view,
                               MtnConnman *connman)
{
    if (view->priv->model) {
        mtn_service_model_set_connman (view->priv->model, connman);
    }
}

ClutterActor*
mtn_services_view_new (void)
{
    return g_object_new (MTN_TYPE_SERVICES_VIEW, NULL);
}
