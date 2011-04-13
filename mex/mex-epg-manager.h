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


#ifndef __MEX_EPG_MANAGER_H__
#define __MEX_EPG_MANAGER_H__

#include <glib-object.h>

#include <mex/mex-epg-event.h>
#include <mex/mex-epg-provider.h>

G_BEGIN_DECLS

#define MEX_TYPE_EPG_MANAGER mex_epg_manager_get_type()

#define MEX_EPG_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_EPG_MANAGER, MexEpgManager))

#define MEX_EPG_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_EPG_MANAGER, MexEpgManagerClass))

#define MEX_IS_EPG_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_EPG_MANAGER))

#define MEX_IS_EPG_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_EPG_MANAGER))

#define MEX_EPG_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_EPG_MANAGER, MexEpgManagerClass))

typedef struct _MexEpgManager MexEpgManager;
typedef struct _MexEpgManagerClass MexEpgManagerClass;
typedef struct _MexEpgManagerPrivate MexEpgManagerPrivate;

/** MexEpgManagerReply: Type of callback to give to mex_epg_manager_get_events()
 *
 * <note>The resulting array of #MexEpgEvents is owned by the library,
 * you need to take a reference to the array and/or some or all of the elements
 * if you want to keep them around in the client code.</note>
 */
typedef void (*MexEpgManagerReply) (MexEpgProvider *provider,
                                    MexChannel     *channel,
                                    GPtrArray      *events,
                                    gpointer        user_data);

struct _MexEpgManager
{
  GObject parent;

  MexEpgManagerPrivate *priv;
};

struct _MexEpgManagerClass
{
  GObjectClass parent_class;
};

GType           mex_epg_manager_get_type      (void) G_GNUC_CONST;

MexEpgManager * mex_epg_manager_get_default   (void);

void            mex_epg_manager_add_provider  (MexEpgManager  *manager,
                                               MexEpgProvider *provider);

void            mex_epg_manager_get_events    (MexEpgManager       *manager,
                                               MexChannel          *channel,
                                               GDateTime           *start_date,
                                               GDateTime           *end_date,
                                               MexEpgManagerReply  reply,
                                               gpointer             user_data);
void            mex_epg_manager_get_event_now (MexEpgManager      *manager,
                                               MexChannel         *channel,
                                               MexEpgManagerReply  reply,
                                               gpointer            user_data);

G_END_DECLS

#endif /* __MEX_EPG_MANAGER_H__ */
