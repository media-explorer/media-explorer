#if !defined(__GLIB_CONTROLLER_H_INSIDE__) && !defined(GLIB_CONTROLLER_COMPILATION)
#error "Only <glib-controller.h> can be included directly."
#endif

#ifndef __G_ARRAY_CONTROLLER_H__
#define __G_ARRAY_CONTROLLER_H__

#include <glib-controller/gcontroller.h>

G_BEGIN_DECLS

#define G_ARRAY_CONTROLLER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_ARRAY_CONTROLLER, GArrayControllerClass))
#define G_IS_ARRAY_CONTROLLER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_ARRAY_CONTROLLER))
#define G_ARRAY_CONTROLLER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_ARRAY_CONTROLLER, GArrayControllerClass))

typedef struct _GArrayControllerPrivate GArrayControllerPrivate;
typedef struct _GArrayControllerClass   GArrayControllerClass;

struct _GArrayController
{
  /*< private >*/
  GController parent_instance;

  GArrayControllerPrivate *priv;
};

/**
 * GArrayControllerClass:
 *
 * The <structname>GArrayControllerClass</structname> structure contains
 * only private data.
 */
struct _GArrayControllerClass
{
  /*< private >*/
  GControllerClass parent_class;
};

GType g_array_controller_get_type (void) G_GNUC_CONST;

GController *g_array_controller_new       (GArray           *array);

void         g_array_controller_set_array (GArrayController *controller,
                                           GArray           *array);
GArray *     g_array_controller_get_array (GArrayController *controller);

G_END_DECLS

#endif /* __G_ARRAY_CONTROLLER_H__ */
