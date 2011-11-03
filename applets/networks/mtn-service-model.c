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

#include "mtn-service-model.h"
#include "mtn-service-tile.h"

G_DEFINE_TYPE (MtnServiceModel, mtn_service_model, CLUTTER_TYPE_LIST_MODEL)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MTN_TYPE_SERVICE_MODEL, MtnServiceModelPrivate))

struct _MtnServiceModelPrivate {
    MtnConnman *connman;
};

enum
{
    PROP_0,
    PROP_CONNMAN,
};

GType MTN_SERVICE_MODEL_COLUMN_TYPES[MTN_SERVICE_MODEL_COUNT_COLS] = {
    G_TYPE_UINT,
    G_TYPE_BOOLEAN,
    G_TYPE_BOOLEAN,
    G_TYPE_STRING,
    G_TYPE_OBJECT,
};

static void
mtn_service_model_get_property (GObject *object, guint property_id,
                                GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_service_model_set_property (GObject *object, guint property_id,
                                const GValue *value, GParamSpec *pspec)
{
    MtnServiceModel *model;

    model = MTN_SERVICE_MODEL (object);

    switch (property_id) {
    case PROP_CONNMAN:
        mtn_service_model_set_connman (model,
                                       MTN_CONNMAN (g_value_get_object (value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_service_model_dispose (GObject *object)
{
    G_OBJECT_CLASS (mtn_service_model_parent_class)->dispose (object);
}

static void
mtn_service_model_class_init (MtnServiceModelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MtnServiceModelPrivate));

    object_class->get_property = mtn_service_model_get_property;
    object_class->set_property = mtn_service_model_set_property;
    object_class->dispose = mtn_service_model_dispose;

    g_object_class_install_property (object_class,
                                     PROP_CONNMAN,
                                     g_param_spec_object ("connman",
                                                          "Connman",
                                                          "The MtnConnman instance to use",
                                                          MTN_TYPE_CONNMAN,
                                                          G_PARAM_WRITABLE));
}

static int
_sort_by_index (ClutterModel *model,
                const GValue *a,
                const GValue *b,
                gpointer user_data)
{
    g_assert (G_VALUE_HOLDS (a, G_TYPE_UINT));
    g_assert (G_VALUE_HOLDS (b, G_TYPE_UINT));

    return g_value_get_uint (a) - g_value_get_uint (b);
}

static void
mtn_service_model_init (MtnServiceModel *model)
{
    model->priv = GET_PRIVATE (model);

    clutter_model_set_types (CLUTTER_MODEL (model),
                             MTN_SERVICE_MODEL_COUNT_COLS,
                             MTN_SERVICE_MODEL_COLUMN_TYPES);
    clutter_model_set_sort (CLUTTER_MODEL (model),
                            MTN_SERVICE_MODEL_COL_INDEX,
                            _sort_by_index,
                            NULL, NULL);
}


static ClutterModelIter*
mtn_service_model_lookup (MtnServiceModel *model,
                          const char      *object_path)
{
    ClutterModelIter *iter;
    char *row_path;
    /* NOTE could have internal hashtable, but that's not totally trivial.
     * TODO if needed... */

    iter = clutter_model_get_first_iter (CLUTTER_MODEL (model));
    if (!iter) {
        return NULL;
    }

    while (!clutter_model_iter_is_last (iter)) {
        clutter_model_iter_get (iter,
                                MTN_SERVICE_MODEL_COL_OBJECT_PATH, &row_path,
                                -1);
        if (g_strcmp0 (row_path, object_path) == 0) {
            break;
        }
        iter = clutter_model_iter_next (iter);
    }

    if (clutter_model_iter_is_last (iter)) {
        g_object_unref (iter);
        iter = NULL;
    }

    return iter;
}


static void
mtn_service_model_add_or_update (MtnServiceModel *model,
                                 const char      *path,
                                 guint            index)
{
    ClutterModelIter *model_iter;

    if (!path)
        return;

    model_iter = mtn_service_model_lookup (model, path);
    if (!model_iter) {
g_debug ("add service %d: %s", index, path);
        clutter_model_insert (CLUTTER_MODEL (model), index,
                              MTN_SERVICE_MODEL_COL_INDEX, index,
                              MTN_SERVICE_MODEL_COL_PRESENT, TRUE,
                              MTN_SERVICE_MODEL_COL_HIDDEN, FALSE,
                              MTN_SERVICE_MODEL_COL_OBJECT_PATH, path,
                              MTN_SERVICE_MODEL_COL_WIDGET, mtn_service_tile_new (path),
                              -1);
    } else {
g_debug ("update service %d: %s", index, path);
        clutter_model_iter_set (model_iter,
                                MTN_SERVICE_MODEL_COL_INDEX, index,
                                MTN_SERVICE_MODEL_COL_PRESENT, TRUE,
                                -1);
        g_object_unref (model_iter);
    }
}

static void
mtn_service_model_update (MtnServiceModel *model, GVariant *services)
{
    ClutterModelIter *model_iter;
    GVariantIter *iter;
    char *path;
    guint index;

    if (services) {
        index = 0;
        g_variant_get (services, "ao", &iter);
        while (g_variant_iter_next (iter, "&o", &path)) {
            mtn_service_model_add_or_update (model, path, index);
            index++;
        }
    }

    /* remove the rows that were not present in the Services list */
    index = 0;
    while ((model_iter = clutter_model_get_iter_at_row (CLUTTER_MODEL (model),
                                                        index))) {
        gboolean present, hidden;

        clutter_model_iter_get (model_iter,
                                MTN_SERVICE_MODEL_COL_PRESENT, &present,
                                MTN_SERVICE_MODEL_COL_HIDDEN, &hidden,
                                -1);
        if (present) {
            /* initialize to present=FALSE for next _services_changed() call */
            if (!hidden) {
                clutter_model_iter_set (model_iter,
                                        MTN_SERVICE_MODEL_COL_PRESENT, FALSE,
                                        -1);              
            }
            index++;
        } else {
            clutter_model_remove (CLUTTER_MODEL (model), index);
        }

        g_object_unref (model_iter);
    }
}

static void
_connman_services_changed_cb (MtnConnman      *connman,
                              GVariant        *var,
                              MtnServiceModel *model)
{
    mtn_service_model_update (model, var);
}

static void
mtn_service_model_init_connman (MtnServiceModel *model)
{
    GVariant *var;

    if (!model->priv->connman)
        return;

    var = mtn_connman_get_property (model->priv->connman,
                                    "Services");
    mtn_service_model_update (model, var);
}

static void
_name_owner_notify_cb (MtnConnman      *connman,
                       GParamSpec      *pspec,
                       MtnServiceModel *model)
{
    mtn_service_model_init_connman (model);
}

void
mtn_service_model_set_connman (MtnServiceModel *model,
                               MtnConnman *connman)
{
    if (model->priv->connman) {
        g_signal_handlers_disconnect_by_func (model->priv->connman,
                                              _connman_services_changed_cb,
                                              model);
        g_object_unref (model->priv->connman);
    }

    if (!connman) {
        model->priv->connman =  NULL;
    } else {
        model->priv->connman = g_object_ref (connman);
        g_signal_connect (connman,
                          "property-changed::Services",
                          G_CALLBACK (_connman_services_changed_cb),
                          model);
        g_signal_connect (connman, "notify::g-name-owner",
                          G_CALLBACK (_name_owner_notify_cb), model);

        mtn_service_model_init_connman (model);
    }
}

MtnServiceModel*
mtn_service_model_new (MtnConnman *connman)
{
    MtnServiceModel *model;

    model = g_object_new (MTN_TYPE_SERVICE_MODEL,
                          "connman", connman,
                          NULL);        
    return model;
}
