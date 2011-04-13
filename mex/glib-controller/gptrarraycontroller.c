#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gptrarraycontroller.h"

/**
 * SECTION:gptrarraycontroller
 * @Title: GPtrArrayController
 * @Short_Description: A controller for GLib arrays
 * @See_Also: #GController, #GPtrArray
 *
 * The #GPtrArrayController object is a #GController implementation aimed
 * at controlling #GPtrArray data structures
 */

struct _GPtrArrayControllerPrivate
{
  GPtrArray *array;
};

enum
{
  PROP_0,

  PROP_ARRAY
};

G_DEFINE_TYPE (GPtrArrayController, g_ptr_array_controller, G_TYPE_CONTROLLER);

static void
g_ptr_array_controller_set_property (GObject      *gobject,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GPtrArrayController *self = G_PTR_ARRAY_CONTROLLER (gobject);

  switch (prop_id)
    {
    case PROP_ARRAY:
      g_ptr_array_controller_set_array (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
g_ptr_array_controller_get_property (GObject    *gobject,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GPtrArrayControllerPrivate *priv = G_PTR_ARRAY_CONTROLLER (gobject)->priv;

  switch (prop_id)
    {
    case PROP_ARRAY:
      g_value_set_boxed (value, priv->array);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
g_ptr_array_controller_finalize (GObject *gobject)
{
  GPtrArrayControllerPrivate *priv = G_PTR_ARRAY_CONTROLLER (gobject)->priv;

  if (priv->array)
    g_ptr_array_unref (priv->array);

  G_OBJECT_CLASS (g_ptr_array_controller_parent_class)->finalize (gobject);
}

static void
g_ptr_array_controller_class_init (GPtrArrayControllerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  gobject_class->set_property = g_ptr_array_controller_set_property;
  gobject_class->get_property = g_ptr_array_controller_get_property;
  gobject_class->finalize = g_ptr_array_controller_finalize;

  /**
   * GPtrArrayController:array:
   *
   * The #GPtrArray to be controlled by a #GPtrArrayController instance
   */
  pspec = g_param_spec_boxed ("array",
                              "Array",
                              "The GPtrArray to be controlled",
                              G_TYPE_PTR_ARRAY,
                              G_PARAM_READWRITE |
                              G_PARAM_CONSTRUCT |
                              G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_ARRAY, pspec);

  g_type_class_add_private (klass, sizeof (GPtrArrayControllerPrivate));
}

static void
g_ptr_array_controller_init (GPtrArrayController *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            G_TYPE_PTR_ARRAY_CONTROLLER,
                                            GPtrArrayControllerPrivate);
}

/**
 * g_ptr_array_controller_new:
 * @array: (allow-none): a #GPtrArray or %NULL
 *
 * Creates a new #GPtrArrayController controlling the @array
 *
 * Return value: the newly created #GPtrArrayController
 */
GController *
g_ptr_array_controller_new (GPtrArray *array)
{
  return g_object_new (G_TYPE_PTR_ARRAY_CONTROLLER,
                       "array", array,
                       NULL);
}

/**
 * g_ptr_array_controller_set_array:
 * @controller: a #GPtrArrayController
 * @array: (allow-none): a #GPtrArray or %NULL
 *
 * Sets the #GPtrArray to be controlled by @controller
 *
 * The #GPtrArrayController will take a reference on the passed #GPtrArray
 * which will be released when the #GPtrArrayController is disposed or
 * when unsetting the controlled array by passing %NULL to this
 * function
 */
void
g_ptr_array_controller_set_array (GPtrArrayController *controller,
                                  GPtrArray           *array)
{
  g_return_if_fail (G_IS_PTR_ARRAY_CONTROLLER (controller));

  if (controller->priv->array == array)
    return;

  if (controller->priv->array != NULL)
    g_ptr_array_unref (controller->priv->array);

  controller->priv->array = array;
  if (controller->priv->array != NULL)
    g_ptr_array_ref (controller->priv->array);

  g_object_notify (G_OBJECT (controller), "array");
}

/**
 * g_ptr_array_controller_get_array:
 * @controller: a #GPtrArrayController
 *
 * Retrieves the #GPtrArray controlled by @controller
 *
 * Return value: (transfer none): a pointer to a #GPtrArray, or %NULL
 */
GPtrArray *
g_ptr_array_controller_get_array (GPtrArrayController *controller)
{
  g_return_val_if_fail (G_IS_PTR_ARRAY_CONTROLLER (controller), NULL);

  return controller->priv->array;
}
