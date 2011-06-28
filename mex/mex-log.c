/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Igalia S.L.
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

/**
 * SECTION:mex-log
 * @short_description: Log system
 *
 * This class stores information related to the log system
 */

#include "mex-log.h"
#include "mex-log-private.h"

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

struct _MexLogDomain {
  /*< private >*/
  MexLogLevel log_level;
  char *name;
};


static gchar **mex_log_env;          /* 'domain:level' array from MEX_LOG */

static MexLogLevel mex_default_log_level = MEX_LOG_LEVEL_WARNING;
static GSList *log_domains = NULL;  /* the list of MexLogDomain's */

/* Catch all log domain */
/* For clarity, it should not be re-#define'd in this file, code that wants to
 * log things with log_log_domain should do it explicitly using MEX_LOG(),
 * instead of using MEX_{DEBUG,INFO,MESSAGE,WARNING,ERROR}() */
MEX_LOG_DOMAIN(MEX_LOG_DOMAIN_DEFAULT);

MEX_LOG_DOMAIN(log_log_domain);

static MexLogDomain *
mex_log_domain_find_by_name (const gchar *name)
{
  GSList *list;

  for (list = log_domains; list; list = g_slist_next (list)) {
    MexLogDomain *log_domain = list->data;

    if (g_strcmp0 (log_domain->name, name) == 0)
      return log_domain;
  }

  return NULL;
}

static MexLogDomain *
_mex_log_domain_new_internal (const gchar *name)
{
  MexLogDomain *domain;

  if (*name == '\0' && MEX_LOG_DOMAIN_DEFAULT != NULL)
    return MEX_LOG_DOMAIN_DEFAULT;

  domain = g_slice_new (MexLogDomain);
  domain->log_level = mex_default_log_level;
  domain->name = g_strdup (name);

  log_domains = g_slist_prepend (log_domains, domain);

  if (*name == '\0' && MEX_LOG_DOMAIN_DEFAULT == NULL)
    /* Ensure MEX_LOG_DOMAIN_DEFAULT is set. */
    MEX_LOG_DOMAIN_DEFAULT = domain;

  return domain;
}

/**
 * mex_log_domain_new: (skip)
 * @name: The name for the new log domain
 *
 * Returns: The new log domain
 *
 * Since: 0.2
 */
MexLogDomain *
mex_log_domain_new (const gchar *name)
{
  MexLogDomain *domain;
  gchar **pair;

  g_return_val_if_fail (name, NULL);

  domain = _mex_log_domain_new_internal (name);

  /* If the MEX_LOG env variable contains @name, let's override that domain
   * verbosity */
  if (mex_log_env == NULL)
    return domain;

  pair = mex_log_env;

  while (*pair) {
    gchar **pair_info;
    gchar *domain_spec;

    pair_info = g_strsplit (*pair, ":", 2);
    domain_spec = pair_info[0];

    if (g_strcmp0 (domain_spec, name) == 0)
      mex_log_configure (*pair);

    g_strfreev (pair_info);
    pair++;
  }

  return domain;
}

static void
_mex_log_domain_free_internal (MexLogDomain *domain)
{
  log_domains = g_slist_remove (log_domains, domain);
  g_free (domain->name);
  g_slice_free (MexLogDomain, domain);
}

void
mex_log_domain_free (MexLogDomain *domain)
{
  g_return_if_fail (domain);

  /* domain can actually be MEX_LOG_DOMAIN_DEFAULT if the domain name given
   * in _new() was "", freeing the default domain is not possible from the
   * public API */
  if (domain == MEX_LOG_DOMAIN_DEFAULT)
    return;

  _mex_log_domain_free_internal (domain);
}

static void
mex_log_domain_set_level_all (MexLogLevel level)
{
  GSList *list;

  /* Set the default log level to be level, so newly created domains will
   * have the correct level */
  mex_default_log_level = level;

  for (list = log_domains; list; list = g_slist_next (list)) {
    MexLogDomain *log_domain = list->data;

    log_domain->log_level = level;
  }
}

static MexLogDomain *
get_domain_from_spec (const gchar *domain_spec)
{
  MexLogDomain *domain;

  domain = mex_log_domain_find_by_name (domain_spec);

  return domain;
}

static gchar *name2level[MEX_LOG_LEVEL_LAST] = {
  "none", "error", "warning", "message", "info", "debug"
};

static MexLogLevel
get_log_level_from_spec (const gchar *level_spec)
{
  guint i;
  long int level_num;
  char *tail;

  /* "-" or "none" (from name2level) can be used to disable all logging */
  if (strcmp (level_spec, "-") == 0) {
    return MEX_LOG_LEVEL_NONE;
  }

  /* '*' means everything */
  if (strcmp (level_spec, "*") == 0) {
    return MEX_LOG_LEVEL_LAST - 1;
  }

  errno = 0;
  level_num = strtol (level_spec, &tail, 0);
  if (!errno
      && tail != level_spec
      && level_num >= MEX_LOG_LEVEL_NONE
      && level_num <= MEX_LOG_LEVEL_LAST - 1)
      return (MexLogLevel) level_num;

  for (i = 0; i < MEX_LOG_LEVEL_LAST; i++)
    if (strcmp (level_spec, name2level[i]) == 0)
      return i;

  /* If the spec does not match one of our levels, just return the current
   * default log level */
  return mex_default_log_level;
}

static void
configure_log_domains (const gchar *domains)
{
  gchar **pairs;
  gchar **pair;
  gchar **pair_info ;
  gchar *domain_spec;
  gchar *level_spec;
  MexLogDomain *domain;
  MexLogLevel level;

  pair = pairs = g_strsplit (domains, ",", 0);

  while (*pair) {
    pair_info = g_strsplit (*pair, ":", 2);
    if (pair_info[0] && pair_info[1]) {
      domain_spec = pair_info[0];
      level_spec = pair_info[1];

      level = get_log_level_from_spec (level_spec);
      domain = get_domain_from_spec (domain_spec);

      if (strcmp (domain_spec, "*") == 0)
        mex_log_domain_set_level_all (level);

      if (domain == NULL) {
       g_strfreev (pair_info);
       pair++;
       continue;
      }

      domain->log_level = level;

      MEX_LOG (log_log_domain, MEX_LOG_LEVEL_DEBUG,
               "domain: '%s', level: '%s'", domain_spec, level_spec);

      g_strfreev (pair_info);
    } else {
      MEX_LOG (log_log_domain, MEX_LOG_LEVEL_WARNING,
               "Invalid log spec: '%s'", *pair);
    }
    pair++;
  }
  g_strfreev (pairs);
}

static void
mex_log_valist (MexLogDomain *domain,
                MexLogLevel   level,
                const gchar  *strloc,
                const gchar  *format,
                va_list       args)
{
  gchar *message;
  GLogLevelFlags level2flag[MEX_LOG_LEVEL_LAST] = {
    0,                    /* MEX_LOG_LEVEL_NONE    */
    G_LOG_LEVEL_CRITICAL, /* MEX_LOG_LEVEL_ERROR   */
    G_LOG_LEVEL_WARNING,  /* MEX_LOG_LEVEL_WARNING */
    G_LOG_LEVEL_MESSAGE,  /* MEX_LOG_LEVEL_MESSAGE */
    G_LOG_LEVEL_INFO,     /* MEX_LOG_LEVEL_INFO    */
    G_LOG_LEVEL_DEBUG     /* MEX_LOG_LEVEL_DEBUG   */
  };

  g_return_if_fail (domain);
  g_return_if_fail (level > 0 && level < MEX_LOG_LEVEL_LAST);
  g_return_if_fail (strloc);
  g_return_if_fail (format);

  message = g_strdup_vprintf (format, args);

  if (level <= domain->log_level)
    g_log (G_LOG_DOMAIN, level2flag[level],
           "[%s] %s: %s", domain->name, strloc, message);

  g_free (message);
}

void
mex_log (MexLogDomain *domain,
         MexLogLevel   level,
         const gchar  *strloc,
         const gchar  *format,
         ...)
{
  va_list var_args;

  va_start (var_args, format);
  mex_log_valist (domain, level, strloc, format, var_args);
  va_end (var_args);
}

#define DOMAIN_INIT(domain, name) G_STMT_START {  \
    domain = _mex_log_domain_new_internal (name); \
} G_STMT_END

void
_mex_log_init_core_domains (void)
{
  const gchar *log_env;

  DOMAIN_INIT (MEX_LOG_DOMAIN_DEFAULT, "");
  DOMAIN_INIT (log_log_domain, "log");
  DOMAIN_INIT (epg_log_domain, "epg");
  DOMAIN_INIT (applet_manager_log_domain, "applet-manager");
  DOMAIN_INIT (channel_log_domain, "channel");
  DOMAIN_INIT (download_queue_log_domain, "download-queue");

  /* Retrieve the MEX_DEBUG environment variable, initialize core domains from
   * it if applicable and keep it for mex_log_domain_new(). Plugins are using
   * mex_log_domain_new() in their init() functions to initialize their log
   * domains. At that time, we'll look at the saved MEX_DEBUG to overrive the
   * verbosity */
  log_env = g_getenv ("MEX_DEBUG");
  if (log_env) {
    MEX_LOG (log_log_domain, MEX_LOG_LEVEL_DEBUG,
             "Using log configuration from MEX_DEBUG: %s", log_env);
    configure_log_domains (log_env);
    mex_log_env = g_strsplit (log_env, ",", 0);
  }

}

#undef DOMAIN_INIT

#define DOMAIN_FREE(domain) G_STMT_START {  \
    _mex_log_domain_free_internal (domain);   \
} G_STMT_END

void
_mex_log_free_core_domains (void)
{
  DOMAIN_FREE (MEX_LOG_DOMAIN_DEFAULT);
  DOMAIN_FREE (log_log_domain);
  DOMAIN_FREE (epg_log_domain);
  DOMAIN_FREE (applet_manager_log_domain);
  DOMAIN_FREE (channel_log_domain);
  DOMAIN_FREE (download_queue_log_domain);

  g_strfreev (mex_log_env);
}

#undef DOMAIN_FREE

/**
 * mex_log_configure:
 * @config: A string describing the wanted log configuration
 *
 * Configure a set of log domains. The default configuration is to display
 * warning and error messages only for all the log domains.
 *
 * The configuration string follows the following grammar:
 *
 * |[
 *   config-list: config | config ',' config-list
 *   config: domain ':' level
 *   domain: '*' | [a-zA-Z0-9]+
 *   level: '*' | '-' | named-level | num-level
 *   named-level: "none" | "error" | "warning" | "message" | "info" | "debug"
 *   num-level: [0-5]
 * ]|
 *
 * examples:
 * <itemizedlist>
 *   <listitem><para>"*:*": maximum verbosity for all the log domains</para>
 *   </listitem>
 *   <listitem><para>"*:-": don't print any message</para></listitem>
 *   <listitem><para>"download-queue:debug,epg:debug": prints debug,
 *   info, message, warning and error messages for the download-queue and
 *   epg domains</para></listitem>
 * </itemizedlist>
 *
 * <note>It's possible to override the log configuration at runtime by
 * defining the MEX_DEBUG environment variable to a configuration string
 * as described above</note>
 *
 * Since: 0.2
 */
void
mex_log_configure (const gchar *config)
{
  configure_log_domains (config);
}

/**
 * mex_log_enabled:
 * @domain: a log domain
 * @level: a #MexLogLevel
 *
 * Checks if the @domain has the @level "enabled", ie. if the user has
 * configured the log domain to output @level messages.
 *
 * Return: %TRUE if @domain has its verbosity setup to print @level message,
 * %FALSE otherwise
 *
 * Since: 0.2
 */
gboolean
mex_log_enabled (MexLogDomain *domain,
                 MexLogLevel   level)
{
  g_return_val_if_fail (domain, FALSE);
  g_return_val_if_fail (level > 0 && level < MEX_LOG_LEVEL_LAST, FALSE);

  return level <= domain->log_level;
}
