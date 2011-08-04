#ifndef __MEX_BACKDROP_H__
#define __MEX_BACKDROP_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_BACKDROP            (mex_backdrop_get_type())
#define MEX_BACKDROP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_BACKDROP, MexBackdrop))
#define MEX_BACKDROP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_BACKDROP, MexBackdropClass))
#define MEX_IS_BACKDROP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_BACKDROP))
#define MEX_IS_BACKDROP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_BACKDROP))
#define MEX_BACKDROP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_BACKDROP, MexBackdropClass))

typedef struct _MexBackdrop            MexBackdrop;
typedef struct _MexBackdropClass       MexBackdropClass;
typedef struct _MexBackdropPrivate     MexBackdropPrivate;

struct _MexBackdrop
{
  /*< private >*/
  ClutterActor parent_instance;

  MexBackdropPrivate *priv;
};

struct _MexBackdropClass
{
  /*< private >*/
  ClutterActorClass parent_class;
};

GType mex_backdrop_get_type (void) G_GNUC_CONST;
ClutterActor *mex_backdrop_new        (void);

G_END_DECLS

#endif /* __MEX_BACKDROP_H__ */
