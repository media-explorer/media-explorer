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


#ifndef __MEX_EPG_PROVIDER_H__
#define __MEX_EPG_PROVIDER_H__

#include <glib-object.h>
#include <mex/mex-channel.h>

G_BEGIN_DECLS

#define MEX_TYPE_EPG_PROVIDER (mex_epg_provider_get_type ())

#define MEX_EPG_PROVIDER(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                 \
                               MEX_TYPE_EPG_PROVIDER, \
                               MexEpgProvider))

#define MEX_IS_EPG_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_EPG_PROVIDER))

#define MEX_EPG_PROVIDER_IFACE(iface)                 \
  (G_TYPE_CHECK_CLASS_CAST ((iface),                  \
                            MEX_TYPE_EPG_PROVIDER,    \
                            MexEpgProviderInterface))

#define MEX_IS_EPG_PROVIDER_IFACE(iface) \
  (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_EPG_PROVIDER))

#define MEX_EPG_PROVIDER_GET_IFACE(obj)                     \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj),                    \
                                  MEX_TYPE_EPG_PROVIDER,    \
                                  MexEpgProviderInterface))

typedef struct _MexEpgProvider          MexEpgProvider;
typedef struct _MexEpgProviderInterface MexEpgProviderInterface;

typedef void (*MexEpgProviderReply) (MexEpgProvider *provider,
                                     MexChannel     *channel,
                                     GPtrArray      *events,
                                     gpointer        user_data);
struct _MexEpgProviderInterface
{
  GTypeInterface g_iface;

  /* virtual functions */
  const gchar *   (*get_timezone) (MexEpgProvider *provider);
  gboolean        (*is_ready)     (MexEpgProvider *provider);
  void            (*get_events)   (MexEpgProvider      *provider,
                                   MexChannel          *channel,
                                   GDateTime           *start_date,
                                   GDateTime           *end_date,
                                   MexEpgProviderReply  reply,
                                   gpointer             user_data);
};

GType         mex_epg_provider_get_type       (void) G_GNUC_CONST;

const gchar * mex_epg_provider_get_timezone   (MexEpgProvider *provider);
gboolean      mex_epg_provider_is_ready       (MexEpgProvider *provider);
void          mex_epg_provider_get_events     (MexEpgProvider      *provider,
                                               MexChannel          *channel,
                                               GDateTime           *start_date,
                                               GDateTime           *end_date,
                                               MexEpgProviderReply  reply,
                                               gpointer             user_data);

G_END_DECLS

#endif /* __MEX_EPG_PROVIDER_H__ */
