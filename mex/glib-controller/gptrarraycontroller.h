#if !defined(__GLIB_CONTROLLER_H_INSIDE__) && !defined(GLIB_CONTROLLER_COMPILATION)
#error "Only <glib-controller.h> can be included directly."
#endif

#ifndef __G_PTR_ARRAY_CONTROLLER_H__
#define __G_PTR_ARRAY_CONTROLLER_H__

#include <glib-controller/gcontroller.h>

G_BEGIN_DECLS

#define G_PTR_ARRAY_CONTROLLER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_PTR_ARRAY_CONTROLLER, GPtrArrayControllerClass))
#define G_IS_PTR_ARRAY_CONTROLLER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_PTR_ARRAY_CONTROLLER))
#define G_PTR_ARRAY_CONTROLLER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_PTR_ARRAY_CONTROLLER, GPtrArrayControllerClass))

typedef struct _GPtrArrayControllerPrivate      GPtrArrayControllerPrivate;
typedef struct _GPtrArrayControllerClass        GPtrArrayControllerClass;

struct _GPtrArrayController
{
  /*< private >*/
  GController parent_instance;

  GPtrArrayControllerPrivate *priv;
};

/**
 * GPtrArrayControllerClass:
 *
 * The <structname>GPtrArrayControllerClass</structname> structure contains
 * only private data.
 */
struct _GPtrArrayControllerClass
{
  /*< private >*/
  GControllerClass parent_class;
};

GType g_ptr_array_controller_get_type (void) G_GNUC_CONST;

GController *g_ptr_array_controller_new       (GPtrArray           *array);

void         g_ptr_array_controller_set_array (GPtrArrayController *controller,
                                               GPtrArray           *array);
GPtrArray *  g_ptr_array_controller_get_array (GPtrArrayController *controller);

G_END_DECLS

#endif /* __G_PTR_ARRAY_CONTROLLER_H__ */
