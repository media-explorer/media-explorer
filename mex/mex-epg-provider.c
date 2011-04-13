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


/**
 * SECTION:mex-epg-provider
 * @short_description: Interface for sources of epgs
 *
 * Implementing #MexEpgProvider means that the class can provide a list
 * of epgs to the application.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-debug.h"
#include "mex-epg-provider.h"

G_DEFINE_INTERFACE (MexEpgProvider, mex_epg_provider, G_TYPE_INVALID);

static void
mex_epg_provider_default_init (MexEpgProviderInterface *klass)
{
  g_signal_new ("epg-provider-ready",
                G_TYPE_FROM_INTERFACE (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE, 0);
}

/**
 * mex_epg_provider_get_timezone:
 * @provider: a #MexEpgProvider
 *
 * Retrieves the time zone that must be used to query the @provider. If %NULL
 * if returned, the local time zone should be used.
 *
 * Return value: The Olson's database timezone name
 *
 * <note>In case the implementor of the interface does not implement this
 * funtion, the default implementation will return %NULL ie. the local time
 * zone</note>
 *
 * Since: 0.2
 */
const gchar *
mex_epg_provider_get_timezone (MexEpgProvider *provider)
{
  MexEpgProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_EPG_PROVIDER (provider), 0);

  iface = MEX_EPG_PROVIDER_GET_IFACE (provider);

  if (iface->get_timezone)
    return iface->get_timezone (provider);

  return NULL;
}

/**
 * mex_epg_provider_is_ready:
 * @provider: a #MexEpgProvider
 *
 * Ask if the #MexEpgProvider is ready to be queried. If it's not ready yet
 * you can connect to the "epg-provider-ready" signal that will be fired when
 * the provider is ready.
 *
 * Return value: %TRUE is @provider is ready, %FALSE otherwise
 *
 * <note>In case the implementor of the interface does not implement this
 * funtion, the default implementation will return %TRUE ie. the
 * #MexEpgProvider is ready</note>
 *
 * Since: 0.2
 */
gboolean
mex_epg_provider_is_ready (MexEpgProvider *provider)
{
  MexEpgProviderInterface *iface;

  g_return_val_if_fail (MEX_IS_EPG_PROVIDER (provider), 0);

  iface = MEX_EPG_PROVIDER_GET_IFACE (provider);

  if (iface->is_ready)
    return iface->is_ready (provider);

  return TRUE;
}

/**
 * mex_epg_provider_get_events:
 * @provider: a #MexEpgProvider
 * @channel: the #MexChannel you want the epg data for
 * @start_date: Start date of the wanted EPG events
 * @end_date: End date of the wanted EPG events
 *
 * Retrives a list of #MexEpgEvents for @channel between @start_date and
 * @end_date
 *
 * Return value: A #GPtrArray of #MexEpgEvent
 *
 * Since: 0.2
 */
void
mex_epg_provider_get_events (MexEpgProvider      *provider,
                             MexChannel          *channel,
                             GDateTime           *start_date,
                             GDateTime           *end_date,
                             MexEpgProviderReply  reply,
                             gpointer             user_data)
{
  MexEpgProviderInterface *iface;

  g_return_if_fail (MEX_IS_EPG_PROVIDER (provider));

  iface = MEX_EPG_PROVIDER_GET_IFACE (provider);

  if (MEX_DEBUG_ENABLED (EPG))
    {
      gchar *start, *end;

      start = g_date_time_format (start_date, "%d/%m/%y %H:%M");
      end = g_date_time_format (end_date, "%d/%m/%y %H:%M");
      MEX_NOTE (EPG, "Asking for events between %s and %s", start, end);
      g_free (start);
      g_free (end);
    }

  if (G_LIKELY (iface->get_events))
    {
      iface->get_events (provider, channel, start_date, end_date,
                         reply, user_data);
      return;
    }

  g_warning ("MexEpgProvider of type '%s' does not implement get_events()",
             g_type_name (G_OBJECT_TYPE (provider)));
}
