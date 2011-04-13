/**
 * SECTION:gcontroller
 * @Title: GController
 * @Short_Description: A controller for data storage changes
 *
 * #GController is an abstract class for creating controllers (in the
 * "model-view-controller" pattern sense) for GLib data types.
 *
 * A #GController is associated to a data storage, and whenever the
 * storage changes it will emit the #GController::changed signal,
 * detailing:
 *
 * <itemizedlist>
 *   <listitem><simpara>what kind of change it is</simpara></listitem>
 *   <listitem><simpara>where to access the changed data</simpara></listitem>
 * </itemizedlist>
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gcontroller.h"
#include "gcontrollerenumtypes.h"
#include "gcontrollermarshal.h"
#include "gcontrollerreference.h"

#include <string.h>
#include <gobject/gvaluecollector.h>

enum
{
  CHANGED,

  LAST_SIGNAL
};

static guint controller_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_ABSTRACT_TYPE (GController, g_controller, G_TYPE_OBJECT);

static GControllerReference *
create_reference (GController       *controller,
                  GControllerAction  action,
                  GType              index_type,
                  GValueArray       *indices)
{
  g_assert (index_type != G_TYPE_INVALID);

  return g_object_new (G_TYPE_CONTROLLER_REFERENCE,
                       "controller", controller,
                       "action", action,
                       "index-type", index_type,
                       "indices", indices,
                       NULL);
}

static void
g_controller_class_init (GControllerClass *klass)
{
  klass->create_reference = create_reference;

  /**
   * GController::changed:
   * @controller: the #GController that emitted the signal
   * @action: the action on the data storage controlled by @controller
   * @reference: a reference to the indices changed
   *
   * The ::changed signal is emitted each time the data storage controlled
   * by a #GController instance changes. The type of change is detailed by
   * the #GControllerAction enumeration and passed as the @action argument
   * to the signal handlers.
   *
   * The @reference object contains the information necessary for retrieving
   * the data that was affected by the change from the controlled storage;
   * the type of the information depends on the #GController.
   *
   * Views using a #GController to monitor changes inside a data storage
   * should connect to the #GController::changed signal and update themselves
   * whenever the signal is emitted.
   */
  controller_signals[CHANGED] =
    g_signal_new (g_intern_static_string ("changed"),
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GControllerClass, changed),
                  NULL, NULL,
                  _gcontroller_marshal_VOID__ENUM_OBJECT,
                  G_TYPE_NONE, 2,
                  G_TYPE_CONTROLLER_ACTION,
                  G_TYPE_CONTROLLER_REFERENCE);
}

static void
g_controller_init (GController *self)
{
}

static GControllerReference *
g_controller_create_reference_internal (GController       *controller,
                                        GControllerAction  action,
                                        GType              index_type,
                                        GValueArray       *indices)
{
  return G_CONTROLLER_GET_CLASS (controller)->create_reference (controller,
                                                                action,
                                                                index_type,
                                                                indices);
}

/**
 * g_controller_create_referencev
 * @controller: a #GController
 * @action: the action for the reference
 * @index_type: the type of the indices
 * @indices: (allow-none): a #GValueArray containing the indices
 *
 * Creates a new #GControllerReference for the given indices
 *
 * The passed @indices array is copied inside the #GControllerReference
 * and can be safely freed afterwards.
 *
 * If @indices is %NULL a new empty reference will be created; it is
 * possible to add indexes to it by using g_controller_reference_add_index()
 * or g_controller_reference_add_index_value().
 *
 * This is a vector-based variant of g_controller_create_reference()
 *
 * Return value: (transfer full): a newly created #GControllerReference
 *   instance. Use g_object_unref() when done using the returned object
 */
GControllerReference *
g_controller_create_referencev (GController       *controller,
                                GControllerAction  action,
                                GType              index_type,
                                GValueArray       *indices)
{
  g_return_val_if_fail (G_IS_CONTROLLER (controller), NULL);
  g_return_val_if_fail (index_type != G_TYPE_INVALID, NULL);

  return g_controller_create_reference_internal (controller, action, index_type, indices);
}

/**
 * g_controller_create_reference:
 * @controller: a #GController
 * @action: the action for the reference
 * @index_type: the type of the indices
 * @n_indices: the number of indices, or 0 to create an empty reference
 * @Varargs: the indices
 *
 * Creates a new #GControllerReference for the given indices
 *
 * This is a variadic arguments version of g_controller_create_referencev(),
 * for the convenience of users of the C API
 *
 * Return value: (transfer full): a newly created #GControllerReference
 *   instance. Use g_object_unref() when done using the returned object
 */
GControllerReference *
g_controller_create_reference (GController       *controller,
                               GControllerAction  action,
                               GType              index_type,
                               gint               n_indices,
                               ...)
{
  GControllerReference *ref;
  GValueArray *indices;
  va_list args;
  gint i;

  g_return_val_if_fail (G_IS_CONTROLLER (controller), NULL);
  g_return_val_if_fail (index_type != G_TYPE_INVALID, NULL);

  if (n_indices == 0)
    return g_controller_create_reference_internal (controller, action, index_type, NULL);

  indices = g_value_array_new (n_indices);

  va_start (args, n_indices);

  for (i = 0; i < n_indices; i++)
    {
      GValue idx = { 0, };
      gchar *error;

#if GLIB_CHECK_VERSION (2, 24, 0)
      G_VALUE_COLLECT_INIT (&idx, index_type,
                            args, 0,
                            &error);
#else
      g_value_init (&idx, index_type);
      G_VALUE_COLLECT (&idx, args, 0, &error);
#endif /* GLIB_CHECK_VERSION */

      if (error != NULL)
        {
          g_warning ("%s: %s", G_STRLOC, error);
          g_free (error);
          break;
        }

      /* XXX - there should really be an insert_nocopy() variant */
      g_value_array_insert (indices, i, &idx);
      g_value_unset (&idx);
    }

  va_end (args);

  ref = g_controller_create_reference_internal (controller,
                                                action,
                                                index_type,
                                                indices);

  g_value_array_free (indices);

  return ref;
}

/**
 * g_controller_emit_changed:
 * @controller: a #GController
 * @reference: the reference to the changed data
 *
 * Emits the #GController::changed signal with the given
 * parameters
 */
void
g_controller_emit_changed (GController          *controller,
                           GControllerReference *reference)
{
  g_return_if_fail (G_IS_CONTROLLER (controller));
  g_return_if_fail (G_IS_CONTROLLER_REFERENCE (reference));

  g_signal_emit (controller, controller_signals[CHANGED], 0,
                 g_controller_reference_get_action (reference),
                 reference);
}
