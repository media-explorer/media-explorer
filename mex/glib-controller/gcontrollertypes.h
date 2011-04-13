#if !defined(__GLIB_CONTROLLER_H_INSIDE__) && !defined(GLIB_CONTROLLER_COMPILATION)
#error "Only <glib-controller.h> can be included directly."
#endif

#ifndef __G_CONTROLLER_TYPES_H__
#define __G_CONTROLLER_TYPES_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define G_TYPE_CONTROLLER       (g_controller_get_type ())
#define G_CONTROLLER(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_CONTROLLER, GController))
#define G_IS_CONTROLLER(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_CONTROLLER))

/**
 * GController:
 *
 * The <structname>GController</structname> structure contains only
 * private data and should be accessed using the provided API
 */
typedef struct _GController             GController;

#define G_TYPE_CONTROLLER_REFERENCE     (g_controller_reference_get_type ())
#define G_CONTROLLER_REFERENCE(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_CONTROLLER_REFERENCE, GControllerReference))
#define G_IS_CONTROLLER_REFERENCE(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_CONTROLLER_REFERENCE))

/**
 * GControllerReference:
 *
 * The <structname>GControllerReference</structname> structure contains
 * only private data and should be accessed using the provided API
 */
typedef struct _GControllerReference    GControllerReference;

#define G_TYPE_ARRAY_CONTROLLER         (g_array_controller_get_type ())
#define G_ARRAY_CONTROLLER(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_ARRAY_CONTROLLER, GArrayController))
#define G_IS_ARRAY_CONTROLLER(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_ARRAY_CONTROLLER))

/**
 * GArrayController:
 *
 * The <structname>GArrayController</structname> structure contains
 * only private data and should be accessed using the provided API
 */
typedef struct _GArrayController        GArrayController;

#define G_TYPE_PTR_ARRAY_CONTROLLER     (g_ptr_array_controller_get_type ())
#define G_PTR_ARRAY_CONTROLLER(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_PTR_ARRAY_CONTROLLER, GPtrArrayController))
#define G_IS_PTR_ARRAY_CONTROLLER(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_PTR_ARRAY_CONTROLLER))

/**
 * GPtrArrayController:
 *
 * The <structname>GPtrArrayController</structname> structure contains
 * only private data and should be accessed using the provided API
 */
typedef struct _GPtrArrayController     GPtrArrayController;

#define G_TYPE_HASH_CONTROLLER          (g_hash_controller_get_type ())
#define G_HASH_CONTROLLER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HASH_CONTROLLER, GHashController))
#define G_IS_HASH_CONTROLLER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_HASH_CONTROLLER))

/**
 * GHashController:
 *
 * The <structname>GHashController</structname> structure contains
 * only private data and should be accessed using the provided API
 */
typedef struct _GHashController         GHashController;

/**
 * GControllerAction:
 * @G_CONTROLLER_INVALID_ACTION: Marker for initial/error state
 * @G_CONTROLLER_ADD: New items have been added to the storage
 *   controlled by a #GController
 * @G_CONTROLLER_REMOVE: Items have been removed from the storage
 *   controlled by a #GController
 * @G_CONTROLLER_UPDATE: Items have been updated in the storage
 *   controlled by a #GController
 * @G_CONTROLLER_CLEAR: The storage controlled by a #GController
 *   has been cleared; semantically, it's the equivalent of a
 *   %G_CONTROLLER_REMOVE for every index of the storage
 * @G_CONTROLLER_REPLACE: The storage controlled by a #GController
 *   has been completely replaced; semantically, it's the equivalent
 *   of a %G_CONTROLLER_REMOVE for every index of the old storage
 *   and a %G_CONTROLLER_ADD for every index of the new storage
 *
 * The available actions supported by a #GController
 *
 * This enumeration might be extended at later date
 */
typedef enum { /*< prefix=G_CONTROLLER >*/
  G_CONTROLLER_INVALID_ACTION,

  G_CONTROLLER_ADD,
  G_CONTROLLER_REMOVE,
  G_CONTROLLER_UPDATE,
  G_CONTROLLER_CLEAR,
  G_CONTROLLER_REPLACE
} GControllerAction;

G_END_DECLS

#endif /* __G_CONTROLLER_TYPES_H__ */
