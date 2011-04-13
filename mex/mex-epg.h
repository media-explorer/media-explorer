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


#ifndef __MEX_EPG_H__
#define __MEX_EPG_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_EPG mex_epg_get_type()

#define MEX_EPG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_EPG, MexEpg))

#define MEX_EPG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_EPG, MexEpgClass))

#define MEX_IS_EPG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_EPG))

#define MEX_IS_EPG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_EPG))

#define MEX_EPG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_EPG, MexEpgClass))

typedef struct _MexEpg MexEpg;
typedef struct _MexEpgClass MexEpgClass;
typedef struct _MexEpgPrivate MexEpgPrivate;

struct _MexEpg
{
  MxWidget parent;

  MexEpgPrivate *priv;
};

struct _MexEpgClass
{
  MxWidgetClass parent_class;
};

GType           mex_epg_get_type                  (void) G_GNUC_CONST;

ClutterActor *  mex_epg_new                       (void);

guint           mex_epg_get_event_range           (MexEpg *epg);
void            mex_epg_set_event_range           (MexEpg *epg,
                                                   guint   event_range);

void            mex_epg_show_events_for_datetime  (MexEpg    *epg,
                                                   GDateTime *start_date);
void            mex_epg_show_events_now           (MexEpg *epg);

G_END_DECLS

#endif /* __MEX_EPG_H__ */
