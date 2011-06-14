#if !defined(__GLIB_CONTROLLER_H_INSIDE__) && !defined(GLIB_CONTROLLER_COMPILATION)
#error "Only <glib-controller/glib-controller.h> can be included directly."
#endif

#ifndef __G_CONTROLLER_REFERENCE_H__
#define __G_CONTROLLER_REFERENCE_H__

#include <glib-controller/gcontrollertypes.h>

G_BEGIN_DECLS

#define G_CONTROLLER_REFERENCE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_CONTROLLER_REFERENCE, GControllerReferenceClass))
#define G_IS_CONTROLLER_REFERENCE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_CONTROLLER_REFERENCE))
#define G_CONTROLLER_REFERENCE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_CONTROLLER_REFERENCE, GControllerReferenceClass))

typedef struct _GControllerReferencePrivate     GControllerReferencePrivate;
typedef struct _GControllerReferenceClass       GControllerReferenceClass;

struct _GControllerReference
{
  /*< private >*/
  GObject parent_instance;

  GControllerReferencePrivate *priv;
};

/**
 * GControllerReferenceClass:
 *
 * The <structname>GControllerReferenceClass</structname> structure
 * contains only private data
 */
struct _GControllerReferenceClass
{
  /*< private >*/
  GObjectClass parent_class;

  /* padding */
  void (*_g_controller_ref0) (void);
  void (*_g_controller_ref1) (void);
  void (*_g_controller_ref2) (void);
  void (*_g_controller_ref3) (void);
  void (*_g_controller_ref4) (void);
  void (*_g_controller_ref5) (void);
  void (*_g_controller_ref6) (void);
  void (*_g_controller_ref7) (void);
  void (*_g_controller_ref8) (void);
};

GType g_controller_reference_get_type (void) G_GNUC_CONST;

GController *         g_controller_reference_get_controller    (GControllerReference *ref);
GControllerAction     g_controller_reference_get_action        (GControllerReference *ref);
gint                  g_controller_reference_get_n_indices     (GControllerReference *ref);
GType                 g_controller_reference_get_index_type    (GControllerReference *ref);

void                  g_controller_reference_add_index_value   (GControllerReference *ref,
                                                                const GValue         *value);
gboolean              g_controller_reference_get_index_value   (GControllerReference *ref,
                                                                gint                  pos,
                                                                GValue               *value);

void                  g_controller_reference_add_index         (GControllerReference *ref,
                                                                ...);
gboolean              g_controller_reference_get_index         (GControllerReference *ref,
                                                                gint                  pos,
                                                                ...);

gint                  g_controller_reference_get_index_int     (GControllerReference *ref,
                                                                gint                  pos);
guint                 g_controller_reference_get_index_uint    (GControllerReference *ref,
                                                                gint                  pos);
const gchar *         g_controller_reference_get_index_string  (GControllerReference *ref,
                                                                gint                  pos);
gpointer              g_controller_reference_get_index_pointer (GControllerReference *ref,
                                                                gint                  pos);

G_END_DECLS

#endif /* __G_CONTROLLER_REFERENCE_H__ */
