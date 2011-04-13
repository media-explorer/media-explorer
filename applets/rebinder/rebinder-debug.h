/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */


#ifndef __MEX_DEBUG_H__
#define __MEX_DEBUG_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  MEX_DEBUG_REMAPPING           = 1 << 0,
  MEX_DEBUG_EVDEV               = 1 << 1,
  MEX_DEBUG_FAKE                = 1 << 2,
  MEX_DEBUG_STATE               = 1 << 3
} MexDebugFlag;

#ifdef REBINDER_ENABLE_DEBUG

#define MEX_HAS_DEBUG(type) \
  ((rebinder_debug_flags & MEX_DEBUG_##type) != FALSE)

#ifdef __GNUC__
#define MEX_NOTE(type,x,a...)                     G_STMT_START {  \
        if (G_UNLIKELY (rebinder_debug_flags & MEX_DEBUG_##type)) \
          { g_message ("[" #type "] " G_STRLOC ": " x, ##a); }    \
                                                  } G_STMT_END

#define MEX_WARN(type,x,a...)                     G_STMT_START {  \
        { g_message ("[" #type "] " G_STRLOC ": " x, ##a); }      \
                                                  } G_STMT_END

#define MEX_TIMESTAMP(type,x,a...)                G_STMT_START {  \
        if (G_UNLIKELY (rebinder_debug_flags & MEX_DEBUG_##type)) \
          { g_message ("[" #type "]" " %li:"  G_STRLOC ": "       \
                       x, rebinder_get_timestamp(), ##a); }       \
                                                  } G_STMT_END
#define MEX_DEBUG_ENABLED(type) \
  (G_UNLIKELY (rebinder_debug_flags & MEX_DEBUG_##type))

#else
/* Try the C99 version; unfortunately, this does not allow us to pass
 * empty arguments to the macro, which means we have to
 * do an intemediate printf.
 */
#define MEX_NOTE(type,...)                        G_STMT_START {  \
        if (G_UNLIKELY (rebinder_debug_flags & MEX_DEBUG_##type)) \
          {                                                       \
            gchar * _fmt = g_strdup_printf (__VA_ARGS__);         \
            g_message ("[" #type "] " G_STRLOC ": %s",_fmt);      \
            g_free (_fmt);                                        \
          }                                                       \
                                                  } G_STMT_END

#define MEX_WARN(type,...)                        G_STMT_START {  \
        gchar * _fmt = g_strdup_printf (__VA_ARGS__);             \
        g_warning ("[" #type "] " G_STRLOC ": %s",_fmt);          \
        g_free (_fmt);                                            \
                                                  } G_STMT_END

#define MEX_TIMESTAMP(type,...)                   G_STMT_START {  \
        if (G_UNLIKELY (rebinder_debug_flags & MEX_DEBUG_##type)) \
          {                                                       \
            gchar * _fmt = g_strdup_printf (__VA_ARGS__);         \
            g_message ("[" #type "]" " %li:"  G_STRLOC ": %s",    \
                       rebinder_get_timestamp(), _fmt);           \
            g_free (_fmt);                                        \
          }                                                       \
                                                  } G_STMT_END
#endif

#define MEX_MARK()      MEX_NOTE(MISC, "== mark ==")
#define MEX_DBG(x) { a }

/* We do not even define those (private) symbols when debug is disabled.
 * This is to ensure the debug code is not shiped with the library when
 * disabled */

extern guint rebinder_debug_flags;

gulong    rebinder_get_timestamp    (void);
gboolean  rebinder_debug_init       (void);

#else /* !REBINDER_ENABLE_DEBUG */

#define MEX_HAS_DEBUG(type)        FALSE
#define MEX_NOTE(type,...)         G_STMT_START { } G_STMT_END
#define MEX_WARN(type,...)         G_STMT_START { } G_STMT_END
#define MEX_MARK()                 G_STMT_START { } G_STMT_END
#define MEX_DBG(x)                 G_STMT_START { } G_STMT_END
#define MEX_TIMESTAMP(type,...)    G_STMT_START { } G_STMT_END
#define MEX_DEBUG_ENABLED(type)    (0)

#endif /* REBINDER_ENABLE_DEBUG */

G_END_DECLS

#endif /* __MEX_DEBUG_H__ */
