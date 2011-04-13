#if !defined(__GLIB_CONTROLLER_H_INSIDE__) && !defined(GLIB_CONTROLLER_COMPILATION)
#error "Only <glib-controller/glib-controller.h> can be included directly."
#endif

#ifndef __G_CONTROLLER_VERSION_H__
#define __G_CONTROLLER_VERSION_H__

#define G_CONTROLLER_MAJOR_VERSION      (0)

#define G_CONTROLLER_MINOR_VERSION      (1)

#define G_CONTROLLER_MICRO_VERSION      (1)

#define G_CONTROLLER_VERSION_S          "0.1.1"

#define G_CONTROLLER_VERSION_HEX        (G_CONTROLLER_MAJOR_VERSION << 24 | \
                                         G_CONTROLLER_MINOR_VERSION << 16 | \
                                         G_CONTROLLER_MICRO_VERSION <<  8)

#endif /* __G_CONTROLLER_VERSION_H__ */
