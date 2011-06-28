/*
 * Mex - a media explorer
 *
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2003 Benjamin Otte <in7y118@public.uni-hamburg.de>
 *                    2010 Igalia S.L.
 *                    2010 Intel Corporation.
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
 *
 * Part of this code has been adapted from GStreamer gst/gstinfo.h
 */

#ifndef __MEX_LOG_H__
#define __MEX_LOG_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * MexLogLevel:
 * @MEX_LOG_LEVEL_NONE: Log level none
 * @MEX_LOG_LEVEL_ERROR: Log on error
 * @MEX_LOG_LEVEL_WARNING: Log on warning
 * @MEX_LOG_LEVEL_MESSAGE: Log on message
 * @MEX_LOG_LEVEL_INFO: Log on info
 * @MEX_LOG_LEVEL_DEBUG: Log on debug
 * @MEX_LOG_LEVEL_LAST: Last level of log
 *
 * Grilo log levels. Defines the level of verbosity selected in Grilo.
 */
typedef enum {
  MEX_LOG_LEVEL_NONE,
  MEX_LOG_LEVEL_ERROR,
  MEX_LOG_LEVEL_WARNING,
  MEX_LOG_LEVEL_MESSAGE,
  MEX_LOG_LEVEL_INFO,
  MEX_LOG_LEVEL_DEBUG,

  MEX_LOG_LEVEL_LAST
} MexLogLevel;

/* Opaque type */
typedef struct _MexLogDomain MexLogDomain;

extern MexLogDomain *MEX_LOG_DOMAIN_DEFAULT;

/**
 * MEX_LOG_DOMAIN:
 * @domain: the log domain
 *
 * Defines a MexLogDomain variable.
 */
#define MEX_LOG_DOMAIN(domain) MexLogDomain *domain = NULL

/**
 * MEX_LOG_DOMAIN_EXTERN:
 * @domain: the log domain
 *
 * Declares a MexLogDomain variable as extern. Use in header files.
 */
#define MEX_LOG_DOMAIN_EXTERN(domain) extern MexLogDomain *domain

/**
 * MEX_LOG_DOMAIN_STATIC:
 * @domain: the log domain
 *
 * Defines a static MexLogDomain variable.
 */
#define MEX_LOG_DOMAIN_STATIC(domain) static MexLogDomain *domain = NULL

/**
 * MEX_LOG_DOMAIN_INIT:
 * @domain: the log domain to initialize.
 * @name: the name of the log domain.
 *
 * Creates a new #MexLogDomain with the given name.
 */
#define MEX_LOG_DOMAIN_INIT(domain, name) G_STMT_START { \
  if (domain == NULL)                                    \
    domain = mex_log_domain_new (name);                  \
} G_STMT_END

/**
 * MEX_LOG_DOMAIN_FREE:
 * @domain: the log domain to free.
 *
 * Free a previously allocated #MexLogDomain.
 */
#define MEX_LOG_DOMAIN_FREE(domain) G_STMT_START {  \
  mex_log_domain_free (domain);                     \
  domain = NULL;                                    \
} G_STMT_END

/**
 * MEX_LOG:
 * @domain: the log domain to use
 * @level: the severity of the message
 * @...: A printf-style message to output
 *
 * Outputs a debugging message. This is the most general macro for outputting
 * debugging messages. You will probably want to use one of the ones described
 * below.
 */
#ifdef G_HAVE_ISO_VARARGS

#define MEX_LOG(domain, level, ...) G_STMT_START{       \
    mex_log ((domain), (level), G_STRLOC, __VA_ARGS__); \
}G_STMT_END

#elif G_HAVE_GNUC_VARARGS

#define MEX_LOG(domain, level, args...) G_STMT_START{ \
    mex_log ((domain), (level), G_STRLOC, ##args);    \
}G_STMT_END

#else /* no variadic macros, use inline */

static inline void
MEX_LOG_valist (MexLogDomain *domain,
                MexLogLevel   level,
                const char   *format,
                va_list       varargs)
{
  mex_log (domain, level, "", format, varargs);
}

static inline void
MEX_LOG (MexLogDomain *domain,
         MexLogLevel   level,
         const char   *format,
         ...)
{
  va_list varargs;

  va_start (varargs, format);
  MEX_LOG_DOMAIN_LOG_valist (domain, level, format, varargs);
  va_end (varargs);
}

#endif /* G_HAVE_ISO_VARARGS */

/**
 * MEX_ERROR:
 * @...: printf-style message to output
 *
 * Output an error message in the default log domain.
 */
/**
 * MEX_WARNING:
 * @...: printf-style message to output
 *
 * Output a warning message in the default log domain.
 */
/**
 * MEX_MESSAGE:
 * @...: printf-style message to output
 *
 * Output a logging message in the default log domain.
 */
/**
 * MEX_INFO:
 * @...: printf-style message to output
 *
 * Output an informational message in the default log domain.
 */
/**
 * MEX_DEBUG:
 * @...: printf-style message to output
 *
 * Output a debugging message in the default log domain.
 */

#if G_HAVE_ISO_VARARGS

#define MEX_ERROR(...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_ERROR, __VA_ARGS__)
#define MEX_WARNING(...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define MEX_MESSAGE(...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_MESSAGE, __VA_ARGS__)
#define MEX_INFO(...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_INFO, __VA_ARGS__)
#define MEX_DEBUG(...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_DEBUG, __VA_ARGS__)

#elif G_HAVE_GNUC_VARARGS

#define MEX_ERROR(args...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_ERROR, ##args)
#define MEX_WARNING(args...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_WARNING, ##args)
#define MEX_MESSAGE(args...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_MESSAGE, ##args)
#define MEX_INFO(args...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_INFO, ##args)
#define MEX_DEBUG(args...) \
  MEX_LOG (MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_DEBUG, ##args)

#else /* no variadic macros, use inline */

static inline void
MEX_ERROR (MexLogDomain *domain, const char *format, ...)
{
  va_list varargs;

  va_start (varargs, format);
  MEX_LOG_valist (domain, MEX_LOG_LEVEL_ERROR, format, varargs);
  va_end (varargs);
}

static inline void
MEX_WARNING (MexLogDomain *domain, const char *format, ...)
{
  va_list varargs;

  va_start (varargs, format);
  MEX_LOG_valist (domain, MEX_LOG_LEVEL_WARNING, format, varargs);
  va_end (varargs);
}

static inline void
MEX_MESSAGE (MexLogDomain *domain, const char *format, ...)
{
  va_list varargs;

  va_start (varargs, format);
  MEX_LOG_valist (domain, MEX_LOG_LEVEL_MESSAGE, format, varargs);
  va_end (varargs);
}

static inline void
MEX_INFO (MexLogDomain *domain, const char *format, ...)
{
  va_list varargs;

  va_start (varargs, format);
  MEX_LOG_valist (domain, MEX_LOG_LEVEL_INFO, format, varargs);
  va_end (varargs);
}

static inline void
MEX_DEBUG (MexLogDomain *domain, const char *format, ...)
{
  va_list varargs;

  va_start (varargs, format);
  MEX_LOG_valist (domain, MEX_LOG_LEVEL_DEBUG, format, varargs);
  va_end (varargs);
}

#endif /* G_HAVE_ISO_VARARGS */

/**
 * MEX_DEBUG_ENABLED:
 *
 * Evaluates to true if the debug level is enabled for the default log domain.
 *
 * Since: 0.2
 */
#define MEX_DEBUG_ENABLED \
  mex_log_enabled(MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_DEBUG)

/**
 * MEX_INFO_ENABLED:
 *
 * Evaluates to true if the info level is enabled for the default log domain.
 *
 * Since: 0.2
 */
#define MEX_INFO_ENABLED \
  mex_log_enabled(MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_INFO)

/**
 * MEX_MESSAGE_ENABLED:
 *
 * Evaluates to true if the message level is enabled for the default log domain.
 *
 * Since: 0.2
 */
#define MEX_MESSAGE_ENABLED \
  mex_log_enabled(MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_MESSAGE)

/**
 * MEX_WARNING_ENABLED:
 *
 * Evaluates to true if the warning level is enabled for the default log domain.
 *
 * Since: 0.2
 */
#define MEX_WARNING_ENABLED \
  mex_log_enabled(MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_WARNING)

/**
 * MEX_ERROR_ENABLED:
 *
 * Evaluates to true if the error level is enabled for the default log domain.
 *
 * Since: 0.2
 */
#define MEX_ERROR_ENABLED \
  mex_log_enabled(MEX_LOG_DOMAIN_DEFAULT, MEX_LOG_LEVEL_ERROR)

MexLogDomain *  mex_log_domain_new    (const gchar *name);
void            mex_log_domain_free   (MexLogDomain *domain);

void            mex_log_configure     (const gchar  *config);
gboolean        mex_log_enabled       (MexLogDomain *domain,
                                       MexLogLevel   level);
void            mex_log               (MexLogDomain *domain,
                                       MexLogLevel   level,
                                       const gchar  *strloc,
                                       const gchar  *format,
                                       ...) G_GNUC_PRINTF (4, 5) G_GNUC_NO_INSTRUMENT;

G_END_DECLS

#endif /* __MEX_LOG_H__ */
