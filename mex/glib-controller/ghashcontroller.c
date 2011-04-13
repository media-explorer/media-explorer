#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ghashcontroller.h"

/**
 * SECTION:ghashcontroller
 * @Title: GHashController
 * @Short_Description: A controller for GLib hash tables
 * @See_Also: #GController, #GHashTable
 *
 * The #GHashController object is a #GController implementation aimed
 * at controlling #GHashTable data structures
 */

struct _GHashControllerPrivate
{
  GHashTable *hash;
};

enum
{
  PROP_0,

  PROP_HASH
};

G_DEFINE_TYPE (GHashController, g_hash_controller, G_TYPE_CONTROLLER);

static void
g_hash_controller_set_property (GObject      *gobject,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GHashController *self = G_HASH_CONTROLLER (gobject);

  switch (prop_id)
    {
    case PROP_HASH:
      g_hash_controller_set_hash (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
g_hash_controller_get_property (GObject    *gobject,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GHashControllerPrivate *priv = G_HASH_CONTROLLER (gobject)->priv;

  switch (prop_id)
    {
    case PROP_HASH:
      g_value_set_boxed (value, priv->hash);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
g_hash_controller_finalize (GObject *gobject)
{
  GHashControllerPrivate *priv = G_HASH_CONTROLLER (gobject)->priv;

  if (priv->hash)
    g_hash_table_unref (priv->hash);

  G_OBJECT_CLASS (g_hash_controller_parent_class)->finalize (gobject);
}

static void
g_hash_controller_class_init (GHashControllerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  gobject_class->set_property = g_hash_controller_set_property;
  gobject_class->get_property = g_hash_controller_get_property;
  gobject_class->finalize = g_hash_controller_finalize;

  /**
   * GHashController:hash:
   *
   * The #GHashTable to be controlled by a #GHashController instance
   */
  pspec = g_param_spec_boxed ("hash",
                              "Hash",
                              "The GHashTable to be controlled",
                              G_TYPE_HASH_TABLE,
                              G_PARAM_READWRITE |
                              G_PARAM_CONSTRUCT |
                              G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_HASH, pspec);

  g_type_class_add_private (klass, sizeof (GHashControllerPrivate));
}

static void
g_hash_controller_init (GHashController *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            G_TYPE_HASH_CONTROLLER,
                                            GHashControllerPrivate);
}

/**
 * g_hash_controller_new:
 * @hash: (allow-none): a #GHashTable or %NULL
 *
 * Creates a new #GHashController controlling the @hash
 *
 * Return value: the newly created #GHashController
 */
GController *
g_hash_controller_new (GHashTable *hash)
{
  return g_object_new (G_TYPE_HASH_CONTROLLER,
                       "hash", hash,
                       NULL);
}

/**
 * g_hash_controller_set_hash:
 * @controller: a #GHashController
 * @hash: (allow-none): a #GHashTable or %NULL
 *
 * Sets the #GHashTable to be controlled by @controller
 *
 * The #GHashController will take a reference on the passed #GHashTable
 * which will be released when the #GHashController is disposed or when
 * unsetting the controlled hash by passing %NULL to this function
 */
void
g_hash_controller_set_hash (GHashController *controller,
                            GHashTable      *hash)
{
  g_return_if_fail (G_IS_HASH_CONTROLLER (controller));

  if (controller->priv->hash == hash)
    return;

  if (controller->priv->hash != NULL)
    g_hash_table_unref (controller->priv->hash);

  controller->priv->hash = hash;
  if (controller->priv->hash != NULL)
    g_hash_table_ref (controller->priv->hash);

  g_object_notify (G_OBJECT (controller), "hash");
}

/**
 * g_hash_controller_get_hash:
 * @controller: a #GHashController
 *
 * Retrieves the #GHashTable controlled by @controller
 *
 * Return value: (transfer none): a pointer to a #GHashTable, or %NULL
 */
GHashTable *
g_hash_controller_get_hash (GHashController *controller)
{
  g_return_val_if_fail (G_IS_HASH_CONTROLLER (controller), NULL);

  return controller->priv->hash;
}
