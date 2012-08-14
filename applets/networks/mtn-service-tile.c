/*
 * mex-networks - Connection Manager UI for Media Explorer 
 * Copyright © 2010-2011, Intel Corporation.
 * Copyright © 2012, sleep(5) ltd.
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

#include <glib/gi18n.h>

#include "mtn-service-tile.h"
#include "mtn-connman-service.h"

#define MTN_ICON_SIZE 26

G_DEFINE_TYPE (MtnServiceTile, mtn_service_tile, MX_TYPE_BUTTON);

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MTN_TYPE_SERVICE_TILE, MtnServiceTilePrivate))

enum {
    PROP_0,
    PROP_PATH,
};

struct _MtnServiceTilePrivate {
    MtnConnmanService *service;

    ClutterActor *title;
    ClutterActor *security_icon;
    ClutterActor *type_icon;
    ClutterActor *connection_icon;

    gboolean is_pressed;
};

static void
mtn_service_tile_get_property (GObject *object, guint property_id,
                               GValue *value, GParamSpec *pspec)
{
    MtnServiceTile *tile;

    tile = MTN_SERVICE_TILE (object);

    switch (property_id) {
    case PROP_PATH:
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_service_tile_set_property (GObject *object, guint property_id,
                                                            const GValue *value, GParamSpec *pspec)
{
    MtnServiceTile *tile;

    tile = MTN_SERVICE_TILE (object);

    switch (property_id) {
    case PROP_PATH:
        mtn_service_tile_set_object_path (tile,
                                          g_value_get_string (value));        
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mtn_service_tile_dispose (GObject *object)
{
    MtnServiceTile *tile;

    tile = MTN_SERVICE_TILE (object);

    G_OBJECT_CLASS (mtn_service_tile_parent_class)->dispose (object);
}

static void
mtn_service_tile_finalize (GObject *object)
{
    G_OBJECT_CLASS (mtn_service_tile_parent_class)->finalize (object);
}

static void
mtn_service_tile_class_init (MtnServiceTileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GParamSpec *pspec;

    g_type_class_add_private (klass, sizeof (MtnServiceTilePrivate));

    object_class->get_property = mtn_service_tile_get_property;
    object_class->set_property = mtn_service_tile_set_property;
    object_class->dispose = mtn_service_tile_dispose;
    object_class->finalize = mtn_service_tile_finalize;

    pspec = g_param_spec_string ("object-path",
                                 "Object path",
                                 "D-Bus object path of a Connman service",
                                 NULL,
                                 G_PARAM_WRITABLE | G_PARAM_CONSTRUCT);
    g_object_class_install_property (object_class, PROP_PATH, pspec);
}

static void
mtn_service_tile_init (MtnServiceTile *tile)
{
    ClutterActor *box;

    tile->priv = GET_PRIVATE (tile);

    box = mx_box_layout_new ();
    mx_bin_set_child (MX_BIN (tile), box);

    mx_bin_set_alignment (MX_BIN (tile),
                          MX_ALIGN_START,
                          MX_ALIGN_MIDDLE);
    mx_bin_set_fill (MX_BIN (tile), TRUE, TRUE);

    tile->priv->connection_icon = mx_icon_new ();
    clutter_actor_set_size (CLUTTER_ACTOR (tile->priv->connection_icon),
                            MTN_ICON_SIZE, MTN_ICON_SIZE);
    mx_icon_set_icon_size (MX_ICON (tile->priv->connection_icon),
                           MTN_ICON_SIZE);
    clutter_actor_insert_child_at_index (box, tile->priv->connection_icon, -1);

    tile->priv->title = mx_label_new ();
    mx_label_set_y_align (MX_LABEL (tile->priv->title),
                          MX_ALIGN_MIDDLE);
    mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (box),
                                                tile->priv->title,
                                                -1,
                                             "expand", TRUE,
                                             "x-fill", TRUE,
                                             NULL);


    tile->priv->security_icon = mx_icon_new ();
    clutter_actor_set_size (CLUTTER_ACTOR (tile->priv->security_icon),
                            MTN_ICON_SIZE, MTN_ICON_SIZE);
    mx_icon_set_icon_size (MX_ICON (tile->priv->security_icon),
                           MTN_ICON_SIZE);
    clutter_actor_insert_child_at_index (box, tile->priv->security_icon, -1);

    tile->priv->type_icon = mx_icon_new ();
    clutter_actor_set_size (CLUTTER_ACTOR (tile->priv->type_icon),
                            MTN_ICON_SIZE, MTN_ICON_SIZE);
    mx_icon_set_icon_size (MX_ICON (tile->priv->type_icon),
                           MTN_ICON_SIZE);
    clutter_actor_insert_child_at_index (box, tile->priv->type_icon, -1);
}

static void
_service_name_changed (MtnConnmanService *service,
                       GVariant          *value,
                       MtnServiceTile    *tile)
{
    const char *name;

    name = value ? g_variant_get_string (value, NULL) : "";
    mx_label_set_text (MX_LABEL (tile->priv->title), name);    
}

static void
_service_state_changed (MtnConnmanService *service,
                        GVariant          *value,
                        MtnServiceTile    *tile)
{
    const char *state;

    state = value ? g_variant_get_string (value, NULL) : "idle";

    /* TODO: possibly need a failed icon but need to figure out _when_
     * that is shown */

    if (g_strcmp0 (state, "ready") == 0 ||
        g_strcmp0 (state, "online") == 0) {
        mx_stylable_set_style_class (MX_STYLABLE (tile->priv->connection_icon),
                                     "mtn-service-connected");
    } else {
        mx_stylable_set_style_class (MX_STYLABLE (tile->priv->connection_icon),
                                     "mtn-service-idle");
    }
}

static void
_service_security_changed (MtnConnmanService *service,
                           GVariant          *value,
                           MtnServiceTile    *tile)
{
    const char **securities, **security;
    const char *class;

    class = "mtn-service-security-none";

    securities = g_variant_get_strv (value, NULL);
    if (securities) {
      security = securities;
      while (*security) {
        if (g_strcmp0 (*security, "none") != 0)
          class = "mtn-service-security-secure";
        security++;
      }
      g_free (securities);
    }

    mx_stylable_set_style_class (MX_STYLABLE (tile->priv->security_icon),
                                 class);
}

static void
_update_type_icon (MtnServiceTile *tile,
                   const char *type,
                   uint strength)
{
    char *class;

    /* TODO might want to tweak the strength values... */
    if (g_strcmp0 (type, "ethernet") == 0)
        class = g_strdup ("mtn-service-type-wired");
    else
        class = g_strdup_printf ("mtn-service-type-generic-%u",
                                 CLAMP (strength / 25, 0, 3));

    mx_stylable_set_style_class (MX_STYLABLE (tile->priv->type_icon),
                                 class);
    g_free (class);
}

static void
_service_type_changed (MtnConnmanService *service,
                       GVariant          *value,
                       MtnServiceTile    *tile)
{
    GVariant *strength_val;
    const char *type;
    uint strength;

    type = value ? g_variant_get_string (value, NULL) : "";

    strength_val = mtn_connman_service_get_property (tile->priv->service,
                                                     "Strength");
    strength = strength_val ? g_variant_get_byte (strength_val) : 0;

    _update_type_icon (tile, type, strength);
}

static void
_service_strength_changed (MtnConnmanService *service,
                           GVariant          *value,
                           MtnServiceTile    *tile)
{
    GVariant *type_val;
    const char *type;
    uint strength;

    strength = value ? g_variant_get_byte (value) : 0;

    type_val = mtn_connman_service_get_property (tile->priv->service,
                                                 "Type");
    type = type_val ? g_variant_get_string (type_val, NULL) : "";

    _update_type_icon (tile, type, strength);
}


static void
mtn_service_tile_init_connman_service (MtnServiceTile *tile)
{
    GVariant *value;

    if (!tile->priv->service)
        return;

    value = mtn_connman_service_get_property (tile->priv->service,
                                              "Name");
    _service_name_changed (tile->priv->service, value, tile);

    value = mtn_connman_service_get_property (tile->priv->service,
                                              "State");
    _service_state_changed (tile->priv->service, value, tile);

    value = mtn_connman_service_get_property (tile->priv->service,
                                              "Security");
    _service_security_changed (tile->priv->service, value, tile);

    value = mtn_connman_service_get_property (tile->priv->service,
                                              "Type");
    _service_type_changed (tile->priv->service, value, tile);

    value = mtn_connman_service_get_property (tile->priv->service,
                                              "Strength");
    _service_strength_changed (tile->priv->service, value, tile);
}

static void
_name_owner_notify_cb (MtnConnmanService *service,
                       GParamSpec        *pspec,
                       MtnServiceTile    *tile)
{
    const char *name;

    g_object_get (service, "g-name-owner", &name, NULL);

    if (!name) {
        g_warning ("Lost D-Bus name owner for Connman service\n");
    }

    mtn_service_tile_init_connman_service (tile);
}

static void
_service_new_cb (GObject *object,
                 GAsyncResult *res,
                 gpointer user_data)
{
    GError *error;
    MtnServiceTile *tile;

    tile = MTN_SERVICE_TILE (user_data);

    error = NULL;
    tile->priv->service = mtn_connman_service_new_finish (res, &error);
    if (!tile->priv->service) {
        g_warning ("Failed to create Connman service proxy: %s", error->message);
        g_error_free (error);
        return;
    }

    g_signal_connect (tile->priv->service, "notify::g-name-owner",
                      G_CALLBACK (_name_owner_notify_cb), tile);
    g_signal_connect (tile->priv->service, "property-changed::Name",
                      G_CALLBACK (_service_name_changed), tile);
    g_signal_connect (tile->priv->service, "property-changed::State",
                      G_CALLBACK (_service_state_changed), tile);
    g_signal_connect (tile->priv->service, "property-changed::Security",
                      G_CALLBACK (_service_security_changed), tile);
    g_signal_connect (tile->priv->service, "property-changed::Type",
                      G_CALLBACK (_service_type_changed), tile);
    g_signal_connect (tile->priv->service, "property-changed::Strength",
                      G_CALLBACK (_service_strength_changed), tile);

    mtn_service_tile_init_connman_service (tile);
}

void
mtn_service_tile_set_object_path (MtnServiceTile *tile, const char *object_path)
{
    if (tile->priv->service) {
        g_object_unref (tile->priv->service);
        tile->priv->service = NULL;
    }

    if (object_path) {
        mtn_connman_service_new (object_path, NULL,
                                 _service_new_cb, tile);
    }
}

const char*
mtn_service_tile_get_object_path (MtnServiceTile *tile)
{
    if (!tile->priv->service)
        return NULL;

    return g_dbus_proxy_get_object_path (G_DBUS_PROXY (tile->priv->service));
}

MtnServiceTile*
mtn_service_tile_new (const char *object_path)
{
    return g_object_new (MTN_TYPE_SERVICE_TILE,
                         "object-path", object_path,
                         NULL);
}

