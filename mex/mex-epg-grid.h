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


#ifndef __MEX_EPG_GRID_H__
#define __MEX_EPG_GRID_H__

#include <glib-object.h>
#include <mx/mx.h>

#include <mex/mex-epg-event.h>
#include <mex/mex-channel.h>

G_BEGIN_DECLS

#define MEX_TYPE_EPG_GRID mex_epg_grid_get_type()

#define MEX_EPG_GRID(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_EPG_GRID, MexEpgGrid))

#define MEX_EPG_GRID_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_EPG_GRID, MexEpgGridClass))

#define MEX_IS_EPG_GRID(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_EPG_GRID))

#define MEX_IS_EPG_GRID_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_EPG_GRID))

#define MEX_EPG_GRID_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_EPG_GRID, MexEpgGridClass))

typedef struct _MexEpgGrid MexEpgGrid;
typedef struct _MexEpgGridClass MexEpgGridClass;
typedef struct _MexEpgGridPrivate MexEpgGridPrivate;

struct _MexEpgGrid
{
  MxWidget parent;

  MexEpgGridPrivate *priv;
};

struct _MexEpgGridClass
{
  MxWidgetClass parent_class;
};

GType           mex_epg_grid_get_type               (void) G_GNUC_CONST;

ClutterActor *  mex_epg_grid_new                    (void);
void            mex_epg_grid_add_events             (MexEpgGrid *grid,
                                                     MexChannel *channel,
                                                     GPtrArray   *events);
void            mex_epg_grid_set_current_date_time  (MexEpgGrid *grid,
                                                     GDateTime  *date);
void            mex_epg_grid_set_date_time_span     (MexEpgGrid *grid,
                                                     GDateTime  *start,
                                                     GDateTime  *end);

G_END_DECLS

#endif /* __MEX_EPG_GRID_H__ */
