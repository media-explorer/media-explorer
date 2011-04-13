/**
 * SECTION:gcontrollerreference
 * @Title: GControllerReference
 * @Short_Description: A reference to a storage change
 *
 * The #GControllerReference is an object created by a #GController whenever
 * a controlled data storage changes. The #GControllerReference stores the
 * the location of the changed data in a way that allows a View to query the
 * storage for the actual data.
 *
 * A #GControllerReference can only be created by a #GController using
 * g_controller_create_reference() and should be passed to the
 * #GController::changed signal emitter, g_controller_emit_changed()
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gcontrollerreference.h"
#include "gcontroller.h"
#include "gcontrollerenumtypes.h"
#include "gcontrollertypes.h"

#include <string.h>
#include <gobject/gvaluecollector.h>

struct _GControllerReferencePrivate
{
  GController *controller;

  GControllerAction action;

  GType index_type;
  GValueArray *indices;
};

enum
{
  PROP_0,

  PROP_CONTROLLER,
  PROP_ACTION,
  PROP_INDEX_TYPE,
  PROP_INDICES
};

G_DEFINE_TYPE (GControllerReference,
               g_controller_reference,
               G_TYPE_OBJECT);

static GValueArray *
add_indices (GValueArray *cur_indices,
             GValueArray *new_indices)
{
  gint i;

  if (new_indices == NULL)
    return cur_indices;

  if (cur_indices == NULL)
    cur_indices = g_value_array_new (new_indices->n_values);

  for (i = 0; i < new_indices->n_values; i++)
    g_value_array_append (cur_indices, g_value_array_get_nth (new_indices, i));

  return cur_indices;
}

static void
g_controller_reference_constructed (GObject *gobject)
{
  GControllerReferencePrivate *priv = G_CONTROLLER_REFERENCE (gobject)->priv;

  g_assert (G_IS_CONTROLLER (priv->controller));

  if (priv->action == G_CONTROLLER_INVALID_ACTION)
    {
      g_critical ("The constructed reference for the GController "
                  "of type '%s' does not have a valid action.",
                  G_OBJECT_TYPE_NAME (priv->controller));
    }

  if (priv->index_type == G_TYPE_INVALID)
    {
      g_critical ("The constructed reference for the GController "
                  "of type '%s' does not have a valid index type.",
                  G_OBJECT_TYPE_NAME (priv->controller));
    }
}

static void
g_controller_reference_set_property (GObject      *gobject,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GControllerReferencePrivate *priv = G_CONTROLLER_REFERENCE (gobject)->priv;

  switch (prop_id)
    {
    case PROP_CONTROLLER:
      priv->controller = g_object_ref (g_value_get_object (value));
      break;

    case PROP_ACTION:
      priv->action = g_value_get_enum (value);
      break;

    case PROP_INDEX_TYPE:
      priv->index_type = g_value_get_gtype (value);
      break;

    case PROP_INDICES:
      priv->indices = add_indices (priv->indices, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
g_controller_reference_get_property (GObject    *gobject,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GControllerReferencePrivate *priv = G_CONTROLLER_REFERENCE (gobject)->priv;

  switch (prop_id)
    {
    case PROP_CONTROLLER:
      g_value_set_object (value, priv->controller);
      break;

    case PROP_ACTION:
      g_value_set_enum (value, priv->action);
      break;

    case PROP_INDEX_TYPE:
      g_value_set_gtype (value, priv->index_type);
      break;

    case PROP_INDICES:
      g_value_set_boxed (value, priv->indices);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
g_controller_reference_dispose (GObject *gobject)
{
  GControllerReferencePrivate *priv = G_CONTROLLER_REFERENCE (gobject)->priv;

  if (priv->controller != NULL)
    {
      g_object_unref (priv->controller);
      priv->controller = NULL;
    }

  G_OBJECT_CLASS (g_controller_reference_parent_class)->finalize (gobject);
}

static void
g_controller_reference_finalize (GObject *gobject)
{
  GControllerReferencePrivate *priv = G_CONTROLLER_REFERENCE (gobject)->priv;

  if (priv->indices != NULL)
    g_value_array_free (priv->indices);

  G_OBJECT_CLASS (g_controller_reference_parent_class)->finalize (gobject);
}

static void
g_controller_reference_class_init (GControllerReferenceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (GControllerReferencePrivate));

  gobject_class->constructed = g_controller_reference_constructed;
  gobject_class->set_property = g_controller_reference_set_property;
  gobject_class->get_property = g_controller_reference_get_property;
  gobject_class->dispose = g_controller_reference_dispose;
  gobject_class->finalize = g_controller_reference_finalize;

  /**
   * GControllerReference:controller:
   *
   * The #GController instance that created this reference
   */
  pspec = g_param_spec_object ("controller",
                               "Controller",
                               "The controller instance that created the reference",
                               G_TYPE_CONTROLLER,
                               G_PARAM_READWRITE |
                               G_PARAM_CONSTRUCT_ONLY |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_CONTROLLER, pspec);

  /**
   * GControllerReference:action:
   *
   * The #GControllerAction that caused the creation of the reference
   */
  pspec = g_param_spec_enum ("action",
                             "Action",
                             "The action that caused the creation of the reference",
                             G_TYPE_CONTROLLER_ACTION,
                             G_CONTROLLER_INVALID_ACTION,
                             G_PARAM_READWRITE |
                             G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_ACTION, pspec);

  /**
   * GControllerReference:index-type:
   *
   * The #GType representation of an index stored by the reference
   */
  pspec = g_param_spec_gtype ("index-type",
                              "Index Type",
                              "The type of the indices",
                              G_TYPE_NONE,
                              G_PARAM_READWRITE |
                              G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_INDEX_TYPE, pspec);

  /**
   * GControllerReference:indices:
   *
   * A #GValueArray containing all the indices stored by the reference
   *
   * The indices are meaningful only for the data storage controlled
   * by the #GController that created this reference
   */
  pspec = g_param_spec_boxed ("indices",
                              "Indices",
                              "The indices inside the data storage",
                              G_TYPE_VALUE_ARRAY,
                              G_PARAM_READWRITE |
                              G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_INDICES, pspec);
}

static void
g_controller_reference_init (GControllerReference *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            G_TYPE_CONTROLLER_REFERENCE,
                                            GControllerReferencePrivate);

  self->priv->controller = NULL;

  self->priv->action = G_CONTROLLER_INVALID_ACTION;

  self->priv->index_type = G_TYPE_INVALID;
  self->priv->indices = NULL;
}

/**
 * g_controller_reference_get_n_indices:
 * @ref: a #GControllerReference
 *
 * Retrieves the number of indices stored by the @ref
 *
 * Return value: the number of indices
 */
gint
g_controller_reference_get_n_indices (GControllerReference *ref)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), 0);

  if (ref->priv->indices == NULL)
    return 0;

  return ref->priv->indices->n_values;
}

/**
 * g_controller_reference_get_index_type:
 * @ref: a #GControllerReference
 *
 * Retrieves the type of the indices stored by the @ref
 *
 * Return value: a #GType
 */
GType
g_controller_reference_get_index_type (GControllerReference *ref)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), G_TYPE_INVALID);

  return ref->priv->index_type;
}

/**
 * g_controller_reference_get_controller:
 * @ref: a #GControllerReference
 *
 * Retrieves a pointer to the #GController that created this reference
 *
 * Return value: (transfer none): a pointer to a #GController
 */
GController *
g_controller_reference_get_controller (GControllerReference *ref)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), NULL);

  return ref->priv->controller;
}

/**
 * g_controller_reference_get_action:
 * @ref: a #GControllerReference
 *
 * Retrieves the action that caused the creation of this reference
 *
 * Return value: a #GControllerAction
 */
GControllerAction
g_controller_reference_get_action (GControllerReference *ref)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), G_CONTROLLER_INVALID_ACTION);

  return ref->priv->action;
}

/**
 * g_controller_reference_add_index_value:
 * @ref: a #GControllerReference
 * @value: a #GValue containing an index
 *
 * Adds an index stored inside a #GValue.
 *
 * The #GValue must contain a value with the same type as the
 * #GControllerReference:index-type
 *
 * This function is mostly useful for bindings
 */
void
g_controller_reference_add_index_value (GControllerReference *ref,
                                        const GValue         *value)
{
  g_return_if_fail (G_IS_CONTROLLER_REFERENCE (ref));
  g_return_if_fail (G_VALUE_TYPE (value) == ref->priv->index_type);

  if (ref->priv->indices == NULL)
    ref->priv->indices = g_value_array_new (0);

  g_value_array_append (ref->priv->indices, value);
}

/**
 * g_controller_reference_get_index_value:
 * @ref: a #GControllerReference
 * @pos: the position of the index, between 0 and the number of
 *   indices stored by @ref
 * @value: an uninitialized #GValue
 *
 * Retrieves the index at @pos and stores it inside @value
 *
 * Return value: %TRUE on success
 */
gboolean
g_controller_reference_get_index_value (GControllerReference *ref,
                                        gint                  pos,
                                        GValue               *value)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  if (ref->priv->indices == NULL)
    return FALSE;

  if (pos < 0 || pos >= ref->priv->indices->n_values)
    return FALSE;

  g_value_init (value, ref->priv->index_type);
  g_value_copy (g_value_array_get_nth (ref->priv->indices, pos), value);

  return TRUE;
}

/**
 * g_controller_reference_add_index:
 * @ref: a #GControllerReference
 * @Varargs: the index to add
 *
 * Variadic arguments version of g_controller_reference_add_index_value(),
 * for the convenience of users of the C API
 */
void
g_controller_reference_add_index (GControllerReference *ref,
                                  ...)
{
  GValue value = { 0, };
  gchar *error = NULL;
  va_list args;

  g_return_if_fail (G_IS_CONTROLLER_REFERENCE (ref));

  va_start (args, ref);

  if (ref->priv->indices == NULL)
    ref->priv->indices = g_value_array_new (1);

#if GLIB_CHECK_VERSION (2, 24, 0)
  G_VALUE_COLLECT_INIT (&value, ref->priv->index_type, args, 0, &error);
#else
  g_value_init (&value, ref->priv->index_type);
  G_VALUE_COLLECT (&value, args, 0, &error);
#endif /* GLIB_CHECK_VERSION */

  if (error != NULL)
    {
      g_warning ("%s: %s", G_STRLOC, error);
      g_free (error);
      goto out;
    }

  ref->priv->indices = g_value_array_append (ref->priv->indices, &value);
  g_value_unset (&value);

out:
  va_end (args);
}

/**
 * g_controller_reference_get_index:
 * @ref: a #GControllerReference
 * @pos: the position of the index
 * @Varargs: return location for the index value at the given position
 *
 * Retrieves the index inside the @ref
 *
 * Return value: %TRUE on success
 */
gboolean
g_controller_reference_get_index (GControllerReference *ref,
                                  gint                  pos,
                                  ...)
{
  GValue *value;
  gchar *error = NULL;
  va_list args;
  gboolean res = FALSE;

  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), FALSE);

  if (ref->priv->indices == NULL)
    return FALSE;

  va_start (args, pos);

  value = g_value_array_get_nth (ref->priv->indices, pos);
  if (value == NULL)
    goto out;

  G_VALUE_LCOPY (value, args, 0, &error);

  if (error != NULL)
    {
      g_warning ("%s: %s", G_STRLOC, error);
      g_free (error);
      res = FALSE;
    }
  else
    res = TRUE;

out:
  va_end (args);

  return res;
}

/**
 * g_controller_reference_get_index_int:
 * @ref: a #GControllerReference
 * @pos: the position of the index
 *
 * Typed accessor for integer indexes stored inside the @ref
 *
 * Return value: an integer index at the given position
 */
gint
g_controller_reference_get_index_int (GControllerReference *ref,
                                      gint                  pos)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), 0);
  g_return_val_if_fail (ref->priv->index_type == G_TYPE_INT, 0);

  if (ref->priv->indices == NULL)
    return 0;

  if (pos < 0 || pos >= ref->priv->indices->n_values)
    return 0;

  return g_value_get_int (g_value_array_get_nth (ref->priv->indices, pos));
}

/**
 * g_controller_reference_get_index_uint:
 * @ref: a #GControllerReference
 * @pos: the position of the index
 *
 * Typed accessor for unsigned integer indexes stored inside the @ref
 *
 * Return value: an unsigned integer index at the given position
 */
guint
g_controller_reference_get_index_uint (GControllerReference *ref,
                                       gint                  pos)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), 0);
  g_return_val_if_fail (ref->priv->index_type == G_TYPE_UINT, 0);

  if (ref->priv->indices == NULL)
    return 0;

  if (pos < 0 || pos >= ref->priv->indices->n_values)
    return 0;

  return g_value_get_uint (g_value_array_get_nth (ref->priv->indices, pos));
}

/**
 * g_controller_reference_get_index_string:
 * @ref: a #GControllerReference
 * @pos: the position of the index
 *
 * Typed accessor for string indexes stored inside the @ref
 *
 * Return value: a string index at the given position
 */
G_CONST_RETURN gchar *
g_controller_reference_get_index_string (GControllerReference *ref,
                                         gint                  pos)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), NULL);
  g_return_val_if_fail (ref->priv->index_type == G_TYPE_STRING, NULL);

  if (ref->priv->indices == NULL)
    return 0;

  if (pos < 0 || pos >= ref->priv->indices->n_values)
    return 0;

  return g_value_get_string (g_value_array_get_nth (ref->priv->indices, pos));
}

/**
 * g_controller_reference_get_index_pointer:
 * @ref: a #GControllerReference
 * @pos: the position of the index
 *
 * Typed accessor for pointer indexes stored inside the @ref
 *
 * Return value: a pointer index at the given position
 */
gpointer
g_controller_reference_get_index_pointer (GControllerReference *ref,
                                          gint                  pos)
{
  g_return_val_if_fail (G_IS_CONTROLLER_REFERENCE (ref), NULL);
  g_return_val_if_fail (ref->priv->index_type == G_TYPE_POINTER, NULL);

  if (ref->priv->indices == NULL)
    return 0;

  if (pos < 0 || pos >= ref->priv->indices->n_values)
    return 0;

  return g_value_get_pointer (g_value_array_get_nth (ref->priv->indices, pos));
}
