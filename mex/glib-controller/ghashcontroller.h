#if !defined(__GLIB_CONTROLLER_H_INSIDE__) && !defined(GLIB_CONTROLLER_COMPILATION)
#error "Only <glib-controller.h> can be included directly."
#endif

#ifndef __G_HASH_CONTROLLER_H__
#define __G_HASH_CONTROLLER_H__

#include <glib-controller/gcontroller.h>

G_BEGIN_DECLS

#define G_HASH_CONTROLLER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_HASH_CONTROLLER, GHashControllerClass))
#define G_IS_HASH_CONTROLLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_HASH_CONTROLLER))
#define G_HASH_CONTROLLER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HASH_CONTROLLER, GHashControllerClass))

typedef struct _GHashControllerPrivate GHashControllerPrivate;
typedef struct _GHashControllerClass   GHashControllerClass;

struct _GHashController
{
  /*< private >*/
  GController parent_instance;

  GHashControllerPrivate *priv;
};

/**
 * GHashControllerClass:
 *
 * The <structname>GHashControllerClass</structname> structure contains
 * only private data.
 */
struct _GHashControllerClass
{
  /*< private >*/
  GControllerClass parent_class;
};

GType g_hash_controller_get_type (void) G_GNUC_CONST;

GController *g_hash_controller_new      (GHashTable      *hash);

void         g_hash_controller_set_hash (GHashController *controller,
                                         GHashTable      *hash);
GHashTable * g_hash_controller_get_hash (GHashController *controller);

G_END_DECLS

#endif /* __G_HASH_CONTROLLER_H__ */
