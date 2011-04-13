#if !defined(__GLIB_CONTROLLER_H_INSIDE__) && !defined(GLIB_CONTROLLER_COMPILATION)
#error "Only <glib-controller/glib-controller.h> can be included directly."
#endif

#ifndef __G_CONTROLLER_H__
#define __G_CONTROLLER_H__

#include <glib-controller/gcontrollertypes.h>

G_BEGIN_DECLS

#define G_CONTROLLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_CONTROLLER, GControllerClass))
#define G_IS_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_CONTROLLER))
#define G_CONTROLLER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_CONTROLLER, GControllerClass))

typedef struct _GControllerClass        GControllerClass;

struct _GController
{
  /*< private >*/
  GObject parent_instance;
};

/**
 * GControllerClass:
 * @create_reference: virtual function; sub-classes should create a new
 *   #GControllerReference instance for the given indexes
 * @changed: class handler for the #GController::changed signal
 *
 * The <structname>GControllerClass</structname> structure contains only
 * private data
 */
struct _GControllerClass
{
  /*< private >*/
  GObjectClass parent_instance;

  /*< public >*/

  /* virtual functions */
  GControllerReference *(* create_reference) (GController          *controller,
                                              GControllerAction     action,
                                              GType                 index_type,
                                              GValueArray          *indexes);

  /* signals */
  void                  (* changed)          (GController          *controller,
                                              GControllerAction     action,
                                              GControllerReference *reference);

  /*< private >*/

  /* padding, for future expansion */
  void (*_g_controller0) (void);
  void (*_g_controller1) (void);
  void (*_g_controller2) (void);
  void (*_g_controller3) (void);
  void (*_g_controller4) (void);
  void (*_g_controller5) (void);
  void (*_g_controller6) (void);
  void (*_g_controller7) (void);
};

GType g_controller_get_type (void) G_GNUC_CONST;

GControllerReference *g_controller_create_reference  (GController          *controller,
                                                      GControllerAction     action,
                                                      GType                 index_type,
                                                      gint                  n_indices,
                                                      ...);
GControllerReference *g_controller_create_referencev (GController          *controller,
                                                      GControllerAction     action,
                                                      GType                 index_type,
                                                      GValueArray          *indices);

void                  g_controller_emit_changed      (GController          *controller,
                                                      GControllerReference *reference);

G_END_DECLS

#endif /* __G_CONTROLLER_H__ */
