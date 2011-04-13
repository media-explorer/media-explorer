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


#include "mex-private.h"

#include "mex-epg-event.h"

G_DEFINE_TYPE (MexEpgEvent, mex_epg_event, G_TYPE_OBJECT)

#define EPG_EVENT_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_EPG_EVENT, MexEpgEventPrivate))

enum
{
  PROP_0,

  PROP_START_DATE,
  PROP_DURATION
};

struct _MexEpgEventPrivate
{
  GDateTime *start_date;
  gint duration;              /* in seconds */
  MexProgram *program;
  MexChannel *channel;
};

/*
 * GObject implementation
 */

static void
mex_epg_event_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MexEpgEvent *event = MEX_EPG_EVENT (object);
  MexEpgEventPrivate *priv = event->priv;

  switch (property_id)
    {
    case PROP_START_DATE:
      g_value_set_boxed (value, priv->start_date);
      break;
    case PROP_DURATION:
      g_value_set_int (value, priv->duration);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_event_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MexEpgEvent *event = MEX_EPG_EVENT (object);

  switch (property_id)
    {
    case PROP_START_DATE:
      mex_epg_event_set_start_date (event, g_value_get_boxed (value));
      break;
    case PROP_DURATION:
      mex_epg_event_set_duration (event, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_event_finalize (GObject *object)
{
  MexEpgEvent *event = MEX_EPG_EVENT (object);
  MexEpgEventPrivate *priv = event->priv;

  if (priv->start_date)
    g_date_time_unref (priv->start_date);
  if (priv->program)
    g_object_unref (priv->program);

  G_OBJECT_CLASS (mex_epg_event_parent_class)->finalize (object);
}

static void
mex_epg_event_class_init (MexEpgEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexEpgEventPrivate));

  object_class->get_property = mex_epg_event_get_property;
  object_class->set_property = mex_epg_event_set_property;
  object_class->finalize = mex_epg_event_finalize;

  /* FIXME: When we can depend on glib 2.26, start time and duration should be
   * at least CONSTRUCT properties and then appear in _new() */

  pspec = g_param_spec_boxed ("start-date",
                              "Start date",
                              "When the event starts", 
                              G_TYPE_DATE_TIME,
                              MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_START_DATE, pspec);

  pspec = g_param_spec_int ("duration",
                            "Duration",
                            "Duration of the event in seconds",
                            0, G_MAXINT, 
                            60,
                            MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DURATION, pspec);
}

static void
mex_epg_event_init (MexEpgEvent *self)
{
  self->priv = EPG_EVENT_PRIVATE (self);
}

MexEpgEvent *
mex_epg_event_new (void)
{
  return g_object_new (MEX_TYPE_EPG_EVENT, NULL);
}

MexEpgEvent *
mex_epg_event_new_with_date_time (GDateTime *datetime,
                                  gint       duration)
{
  MexEpgEvent *event;

  event = mex_epg_event_new ();
  mex_epg_event_set_start_date (event, datetime);
  event->priv->duration = duration;

  return event;
}

MexEpgEvent *
mex_epg_event_new_full (GTimeZone *time_zone,
                        gint       year,
                        gint       month,
                        gint       day,
                        gint       hour,
                        gint       minute,
                        gint       seconds,
                        gint       duration)
{
  MexEpgEvent *event;
  GDateTime *datetime;

  event = mex_epg_event_new ();
  datetime = g_date_time_new (time_zone,
                              year, month, day,
                              hour, minute, seconds);
  mex_epg_event_set_start_date (event, datetime);
  mex_epg_event_set_duration (event, duration);

  return event;
}

MexEpgEvent *
mex_epg_event_new_local (gint       year,
                         gint       month,
                         gint       day,
                         gint       hour,
                         gint       minute,
                         gint       seconds,
                         gint       duration)
{
  MexEpgEvent *event;
  GDateTime *datetime;

  event = mex_epg_event_new ();
  datetime = g_date_time_new_local (year, month, day,
                                    hour, minute, (gdouble) seconds);
  mex_epg_event_set_start_date (event, datetime);
  mex_epg_event_set_duration (event, duration);

  return event;
}

GDateTime *
mex_epg_event_get_start_date (MexEpgEvent *event)
{
  g_return_val_if_fail (MEX_IS_EPG_EVENT (event), NULL);

  return event->priv->start_date;
}

void
mex_epg_event_set_start_date (MexEpgEvent *event,
                              GDateTime   *start_date)
{
  MexEpgEventPrivate *priv;

  g_return_if_fail (MEX_IS_EPG_EVENT (event));
  priv = event->priv;

  if (priv->start_date)
    g_date_time_unref (priv->start_date);

  priv->start_date = g_date_time_ref (start_date);

  g_object_notify (G_OBJECT (event), "start-date");
}

GDateTime *
mex_epg_event_get_end_date (MexEpgEvent *event)
{
  MexEpgEventPrivate *priv;

  g_return_val_if_fail (MEX_IS_EPG_EVENT (event), NULL);
  priv = event->priv;

  return g_date_time_add_seconds (priv->start_date, priv->duration);
}

gint
mex_epg_event_get_duration (MexEpgEvent *event)
{
  g_return_val_if_fail (MEX_IS_EPG_EVENT (event), 0);

  return event->priv->duration;
}

void
mex_epg_event_set_duration (MexEpgEvent *event,
                            gint         duration)
{
  g_return_if_fail (MEX_IS_EPG_EVENT (event));

  event->priv->duration = duration;

  g_object_notify (G_OBJECT (event), "duration");
}

void
mex_epg_event_set_program (MexEpgEvent *event,
                           MexProgram  *program)
{
  MexEpgEventPrivate *priv;

  g_return_if_fail (MEX_IS_EPG_EVENT (event));
  g_return_if_fail (MEX_IS_PROGRAM (program));
  priv = event->priv;

  if (priv->program)
    g_object_unref (program);

  priv->program = g_object_ref (program);
}

MexProgram *
mex_epg_event_get_program (MexEpgEvent *event)
{
  g_return_val_if_fail (MEX_IS_EPG_EVENT (event), NULL);

  return event->priv->program;
}

void
mex_epg_event_set_channel (MexEpgEvent *event,
                           MexChannel  *channel)
{
  MexEpgEventPrivate *priv;

  g_return_if_fail (MEX_IS_EPG_EVENT (event));
  g_return_if_fail (MEX_IS_CHANNEL (channel));
  priv = event->priv;

  if (priv->channel)
    g_object_unref (channel);

  priv->channel = g_object_ref (channel);
}

MexChannel *
mex_epg_event_get_channel (MexEpgEvent *event)
{
  g_return_val_if_fail (MEX_IS_EPG_EVENT (event), NULL);

  return event->priv->channel;
}

gboolean
mex_epg_event_is_date_in_between (MexEpgEvent *event,
                                  GDateTime   *date)
{
  MexEpgEventPrivate *priv;
  GDateTime *end_date;
  gboolean in_between;

  g_return_val_if_fail (MEX_IS_EPG_EVENT (event), FALSE);
  priv = event->priv;

  end_date = g_date_time_add_seconds (priv->start_date, priv->duration);

  in_between = g_date_time_compare (priv->start_date, date) < 0 &&
               g_date_time_compare (date, end_date) <= 0;

  g_date_time_unref (end_date);

  return in_between;
}
