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
  MEX_DEBUG_MISC                = 1 << 0,
  MEX_DEBUG_RSS                 = 1 << 1,
  MEX_DEBUG_EPG                 = 1 << 2,
  MEX_DEBUG_APPLET_MANAGER      = 1 << 3,
  MEX_DEBUG_CHANNEL             = 1 << 4,
  MEX_DEBUG_DOWNLOAD_QUEUE      = 1 << 5
} MexDebugFlag;

#ifdef MEX_ENABLE_DEBUG

#ifdef __GNUC__
#define MEX_NOTE(type,x,a...)                     G_STMT_START {  \
        if (G_UNLIKELY (_mex_debug_flags & MEX_DEBUG_##type))     \
          { g_message ("[" #type "] " G_STRLOC ": " x, ##a); }    \
                                                  } G_STMT_END

#define MEX_WARN(type,x,a...)                     G_STMT_START {  \
        { g_message ("[" #type "] " G_STRLOC ": " x, ##a); }      \
                                                  } G_STMT_END

#define MEX_TIMESTAMP(type,x,a...)                G_STMT_START {  \
        if (G_UNLIKELY (_mex_debug_flags & MEX_DEBUG_##type))     \
          { g_message ("[" #type "]" " %li:"  G_STRLOC ": "       \
                       x, _mex_get_timestamp(), ##a); }           \
                                                  } G_STMT_END
#define MEX_DEBUG_ENABLED(type) \
  (G_UNLIKELY (_mex_debug_flags & MEX_DEBUG_##type))

#else
/* Try the C99 version; unfortunately, this does not allow us to pass
 * empty arguments to the macro, which means we have to
 * do an intemediate printf.
 */
#define MEX_NOTE(type,...)                        G_STMT_START {  \
        if (G_UNLIKELY (_mex_debug_flags & MEX_DEBUG_##type))     \
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
        if (G_UNLIKELY (_mex_debug_flags & MEX_DEBUG_##type))     \
          {                                                       \
            gchar * _fmt = g_strdup_printf (__VA_ARGS__);         \
            g_message ("[" #type "]" " %li:"  G_STRLOC ": %s",    \
                       _mex_get_timestamp(), _fmt);               \
            g_free (_fmt);                                        \
          }                                                       \
                                                  } G_STMT_END
#endif

#define MEX_MARK()      MEX_NOTE(MISC, "== mark ==")
#define MEX_DBG(x) { a }

/* We do not even define those (private) symbols when debug is disabled.
 * This is to ensure the debug code is not shiped with the library when
 * disabled */

extern guint _mex_debug_flags;

gulong    _mex_get_timestamp    (void);
gboolean  _mex_debug_init       (void);

#else /* !MEX_ENABLE_DEBUG */

#define MEX_NOTE(type,...)         G_STMT_START { } G_STMT_END
#define MEX_WARN(type,...)         G_STMT_START { } G_STMT_END
#define MEX_MARK()                 G_STMT_START { } G_STMT_END
#define MEX_DBG(x)                 G_STMT_START { } G_STMT_END
#define MEX_TIMESTAMP(type,...)    G_STMT_START { } G_STMT_END
#define MEX_DEBUG_ENABLED(type)    (0)

#endif /* MEX_ENABLE_DEBUG */

G_END_DECLS

#endif /* __MEX_DEBUG_H__ */
