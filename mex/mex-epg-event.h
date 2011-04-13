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


#ifndef __MEX_EPG_EVENT_H__
#define __MEX_EPG_EVENT_H__

#include <glib-object.h>

#include <mex/mex-channel.h>
#include <mex/mex-program.h>

G_BEGIN_DECLS

#define MEX_TYPE_EPG_EVENT mex_epg_event_get_type()

#define MEX_EPG_EVENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_EPG_EVENT, MexEpgEvent))

#define MEX_EPG_EVENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_EPG_EVENT, MexEpgEventClass))

#define MEX_IS_EPG_EVENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_EPG_EVENT))

#define MEX_IS_EPG_EVENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_EPG_EVENT))

#define MEX_EPG_EVENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_EPG_EVENT, MexEpgEventClass))

typedef struct _MexEpgEvent MexEpgEvent;
typedef struct _MexEpgEventClass MexEpgEventClass;
typedef struct _MexEpgEventPrivate MexEpgEventPrivate;

struct _MexEpgEvent
{
  GObject parent;

  MexEpgEventPrivate *priv;
};

struct _MexEpgEventClass
{
  GObjectClass parent_class;
};

GType             mex_epg_event_get_type            (void) G_GNUC_CONST;

MexEpgEvent *     mex_epg_event_new                 (void);
MexEpgEvent *     mex_epg_event_new_with_date_time  (GDateTime *datetime,
                                                     gint       duration);
MexEpgEvent *     mex_epg_event_new_full            (GTimeZone *time_zone,
                                                     gint       year,
                                                     gint       month,
                                                     gint       day,
                                                     gint       hour,
                                                     gint       minute,
                                                     gint       seconds,
                                                     gint       duration);
MexEpgEvent *     mex_epg_event_new_local           (gint       year,
                                                     gint       month,
                                                     gint       day,
                                                     gint       hour,
                                                     gint       minute,
                                                     gint       seconds,
                                                     gint       duration);
GDateTime *       mex_epg_event_get_start_date      (MexEpgEvent *event);
void              mex_epg_event_set_start_date      (MexEpgEvent *event,
                                                     GDateTime   *start_date);
GDateTime *       mex_epg_event_get_end_date        (MexEpgEvent *event);
gint              mex_epg_event_get_duration        (MexEpgEvent *event);
void              mex_epg_event_set_duration        (MexEpgEvent *event,
                                                     gint         duration);
void              mex_epg_event_set_program         (MexEpgEvent *event,
                                                     MexProgram  *program);
MexProgram *      mex_epg_event_get_program         (MexEpgEvent *event);
void              mex_epg_event_set_channel         (MexEpgEvent *event,
                                                     MexChannel  *channel);
MexChannel *      mex_epg_event_get_channel         (MexEpgEvent *event);

gboolean          mex_epg_event_is_date_in_between  (MexEpgEvent *event,
                                                     GDateTime   *date);


G_END_DECLS

#endif /* __MEX_EPG_EVENT_H__ */
