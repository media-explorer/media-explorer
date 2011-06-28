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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "mex-epg.h"

#include "mex-aspect-frame.h"
#include "mex-channel-manager.h"
#include "mex-column.h"
#include "mex-content-box.h"
#include "mex-content-view.h"
#include "mex-download-queue.h"
#include "mex-epg-grid.h"
#include "mex-epg-manager.h"
#include "mex-log.h"
#include "mex-marshal.h"
#include "mex-private.h"
#include "mex-scroll-view.h"
#include "mex-shadow.h"

#define MEX_LOG_DOMAIN_DEFAULT  epg_log_domain
MEX_LOG_DOMAIN(epg_log_domain);

static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexEpg,
                         mex_epg,
                         MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init))

#define EPG_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_EPG, MexEpgPrivate))

#define CHANNEL_HEIGHT  64
#define INDICATOR_SIZE  12

enum
{
  SIGNAL_EVENT_ACTIVATED,

  SIGNAL_LAST
};

enum
{
  PROP_0,

  PROP_EVENT_RANGE
};

struct _MexEpgPrivate
{
  ClutterActor *channel_box;
  ClutterActor *grid_scrollview;
  ClutterActor *grid;
  ClutterActor *selection_indicator;

  GPtrArray    *channels;           /* Array of channel actors */
  gint         focused_channel;     /* index of focused channel in channels */

  guint pixels_for_5_mins;
  guint event_range;                /* in minutes */

  GDateTime *start_date, *end_date;
};

static guint signals[SIGNAL_LAST] = { 0, };

/*
 * Create a new date from @date rounded to the previous or next hour depending
 * on @next being FALSE/TRUE.
 */
static GDateTime *
round_to_30min (GDateTime *date,
                gboolean   next)
{
  gint seconds, minutes;

  /* seconds will collect how many seconds we need to add to round @date */
  seconds = -g_date_time_get_seconds (date);

  minutes = g_date_time_get_minute (date);
  if (next == FALSE)
    if (minutes >= 30)
      seconds -= (minutes - 30) * 60;
    else
      seconds -= minutes * 60;
  else
    if (minutes >= 30)
      seconds += (60 - minutes) * 60;
    else
      seconds += (30 - minutes) * 60;

  return g_date_time_add_seconds(date, seconds);
}

static void
on_get_events_reply (MexEpgProvider *provider,
                     MexChannel     *channel,
                     GPtrArray      *events,
                     gpointer        user_data)
{
  MexEpg *epg = MEX_EPG (user_data);
  MexEpgPrivate *priv = epg->priv;

  if (G_UNLIKELY (events == NULL))
    {
      MEX_WARNING ("Could not find EPG events for channel %s",
                   mex_channel_get_name (channel));
      /* signal the grid that we have no data for that row */
      mex_epg_grid_add_events (MEX_EPG_GRID (priv->grid), channel, NULL);
      return;
    }

  MEX_DEBUG ("Received %d events for %s", events->len,
             mex_channel_get_name (channel));

  mex_epg_grid_add_events (MEX_EPG_GRID (priv->grid), channel, events);
}

#if 0
static void
on_mex_epg_logo_downloaded (MexDownloadQueue *queue,
                            const char       *uri,
                            const char       *buffer,
                            gsize             count,
                            const GError     *dq_error,
                            gpointer          userdata)
{
  ClutterContainer *logo_frame = CLUTTER_CONTAINER (userdata);
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf;
  ClutterActor *texture;
  GError *error = NULL;

  loader = gdk_pixbuf_loader_new ();
  gdk_pixbuf_loader_write (loader, (const guchar *) buffer, count, &error);
  if (error)
    {
      g_warning ("Error loading %s: %s", uri, error->message);
      g_object_unref (loader);
      g_error_free (error);

      return;
    }

  gdk_pixbuf_loader_close (loader, &error);
  if (error)
    {
      g_warning ("Error loading %s: %s", uri, error->message);
      g_object_unref (loader);
      g_error_free (error);

      return;
    }

  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

  if (pixbuf)
    {
      texture = clutter_texture_new ();
      clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (texture),
                                         gdk_pixbuf_get_pixels (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf),
                                         gdk_pixbuf_get_width (pixbuf),
                                         gdk_pixbuf_get_height (pixbuf),
                                         gdk_pixbuf_get_rowstride (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf) ? 4: 3,
                                         0, &error);
      if (error)
        {
          g_warning ("Unable to set pixbuf for %s: %s", uri, error->message);
          g_error_free (error);
          goto cleanup;
        }

      clutter_container_add_actor (logo_frame, texture);
    }

cleanup:
  g_object_unref (loader);
}
#endif

static void
mex_epg_add_channel (MexEpg     *epg,
                     MexChannel *channel,
                     guint       position)
{
  MexEpgPrivate *priv = epg->priv;
  ClutterActor *channel_frame, *label;

  /* Add the channel to the hbox */
  channel_frame = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (channel_frame), "EpgChannel");

  label = mx_label_new_with_text (mex_channel_get_name (channel));
  mx_bin_set_child (MX_BIN (channel_frame), label);
  mx_bin_set_alignment (MX_BIN (channel_frame),
                        MX_ALIGN_START,
                        MX_ALIGN_MIDDLE);
  clutter_actor_set_height (channel_frame, CHANNEL_HEIGHT);

  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->channel_box), channel_frame, position);

  /* keep a weak pointer to the channel for the channel h/l following what's
   * focused in the grid */
  g_ptr_array_index (priv->channels, position) = channel_frame;
}

static void
on_epg_grid_row_selected (MexEpgGrid *grid,
                          gint        row,
                          MexEpg     *epg)
{
  MexEpgPrivate *priv = epg->priv;
  ClutterActor *channel;

  if (row < 0 && row > priv->channels->len)
    {
      g_warning ("Row index %d is outside the range of channels", row);
      return;
    }

  channel = g_ptr_array_index (priv->channels, priv->focused_channel);
  mx_stylable_style_pseudo_class_remove (MX_STYLABLE (channel), "focus");

  channel = g_ptr_array_index (priv->channels, row);
  mx_stylable_style_pseudo_class_add (MX_STYLABLE (channel), "focus");

  priv->focused_channel = row;
}

static void
on_epg_grid_event_activated (MexEpgGrid  *grid,
                             MexEpgEvent *event,
                             MexEpg      *epg)
{
  g_signal_emit (epg, signals[SIGNAL_EVENT_ACTIVATED], 0, event);
}

/*
 * MxFocusable implementation
 */

static MxFocusable *
mex_epg_accept_focus (MxFocusable *focusable,
                      MxFocusHint  hint)
{
  MexEpg *epg = MEX_EPG (focusable);
  MexEpgPrivate *priv = epg->priv;
  MxFocusable *focused;

  focused = mx_focusable_accept_focus (MX_FOCUSABLE (priv->grid), hint);

  return focused;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = mex_epg_accept_focus;
}

/*
 * ClutterActor implementation
 */

static void
mex_epg_allocate (ClutterActor           *actor,
                  const ClutterActorBox  *box,
                  ClutterAllocationFlags  flags)
{
  MexEpgPrivate *priv = MEX_EPG (actor)->priv;
  MxPadding padding;
  ClutterActorBox column_box, scrollview_box, indicator_box;
  gfloat available_width, available_height, column_width;
/*  gfloat indicator_width; */

  CLUTTER_ACTOR_CLASS (mex_epg_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  available_width = box->x2 - box->x1 - padding.left - padding.right;
  available_height = box->y2 - box->y1 - padding.top - padding.bottom;

#if 0
  g_message ("Space available %lfx%lf", available_width, available_height);
#endif

  /* column */
  column_box.x1 = padding.left;
  column_box.y1 = padding.top;
  clutter_actor_get_preferred_width (priv->channel_box,
                                     -1,
                                     NULL,
                                     &column_box.x2);
  column_box.x2 += column_box.x1;
  column_box.y2 = column_box.y1 + available_height;

#if 0
  g_message ("allocate column (%lf,%lf) (%lf,%lf)",
             column_box.x1, column_box.y1,
             column_box.x2, column_box.y2);
#endif

  clutter_actor_allocate (priv->channel_box, &column_box, flags);

  column_width = (column_box.x2 - column_box.x1);

  /* grid_scrollview */
  scrollview_box.x1 = column_box.x2;
  scrollview_box.y1 = column_box.y1;
  scrollview_box.x2 = scrollview_box.x1 + available_width - column_width;
  scrollview_box.y2 = scrollview_box.y1 + available_height;

#if 0
  g_message ("allocate scrollview (%lf,%lf) (%lf,%lf)",
             scrollview_box.x1, scrollview_box.y1,
             scrollview_box.x2, scrollview_box.y2);
#endif

  clutter_actor_allocate (priv->grid_scrollview, &scrollview_box, flags);

  /* selection_indicator */
  /* indicator_width = clutter_actor_get_width (priv->selection_indicator); */
  //indicator_box.x1 = column_box.x1 - (int) (indicator_width / 2);
  indicator_box.x1 = 0;
  indicator_box.y1 = 0;
#if 0
  indicator_box.x2 = indicator_box.x1 + indicator_width;
  indicator_box.y2 = indicator_box.y1 + indicator_width;
#endif
  indicator_box.x2 = 100;
  indicator_box.y2 = 100;

  clutter_actor_allocate (priv->selection_indicator, &indicator_box, flags);
}

static void
mex_epg_paint (ClutterActor *actor)
{
  MexEpgPrivate *priv = MEX_EPG (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_epg_parent_class)->paint (actor);

  clutter_actor_paint (priv->grid_scrollview);
  clutter_actor_paint (priv->channel_box);
  clutter_actor_paint (priv->selection_indicator);
}

static void
mex_epg_map (ClutterActor *actor)
{
  MexEpgPrivate *priv = MEX_EPG (actor)->priv;

  CLUTTER_ACTOR_CLASS (mex_epg_parent_class)->map (actor);

  clutter_actor_map (priv->grid_scrollview);
  clutter_actor_map (priv->channel_box);
  clutter_actor_map (priv->selection_indicator);
}

static void
mex_epg_unmap (ClutterActor *actor)
{
  MexEpgPrivate *priv = MEX_EPG (actor)->priv;

  clutter_actor_unmap (priv->selection_indicator);
  clutter_actor_unmap (priv->channel_box);
  clutter_actor_unmap (priv->grid_scrollview);

  CLUTTER_ACTOR_CLASS (mex_epg_parent_class)->unmap (actor);
}

/*
 * GObject implementation
 */

static void
mex_epg_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  MexEpg *epg = MEX_EPG (object);

  switch (property_id)
    {
    case PROP_EVENT_RANGE:
      g_value_set_uint (value, mex_epg_get_event_range (epg));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  MexEpg *epg = MEX_EPG (object);

  switch (property_id)
    {
    case PROP_EVENT_RANGE:
      mex_epg_set_event_range (epg, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_finalize (GObject *object)
{
  MexEpg *epg = MEX_EPG (object);
  MexEpgPrivate *priv = epg->priv;

  if (priv->start_date)
    g_date_time_unref (priv->start_date);
  if (priv->end_date)
    g_date_time_unref (priv->end_date);
  g_ptr_array_unref (priv->channels);

  G_OBJECT_CLASS (mex_epg_parent_class)->finalize (object);
}

static void
mex_epg_class_init (MexEpgClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexEpgPrivate));

  object_class->get_property = mex_epg_get_property;
  object_class->set_property = mex_epg_set_property;
  object_class->finalize = mex_epg_finalize;

  actor_class->allocate = mex_epg_allocate;
  actor_class->paint = mex_epg_paint;
  actor_class->map = mex_epg_map;
  actor_class->unmap = mex_epg_unmap;

  pspec = g_param_spec_uint ("event-range",
                             "Event Range",
                             "Range of events to display (in minutes)",
                             0, G_MAXUINT, 4320, /* 3 days */
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_EVENT_RANGE, pspec);

  signals[SIGNAL_EVENT_ACTIVATED] =
    g_signal_new ("event-activated",
                  MEX_TYPE_EPG,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  mex_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  MEX_TYPE_EPG_EVENT);
}

static void
mex_epg_init (MexEpg *self)
{
  MexEpgPrivate *priv;
  MexChannelManager *manager;
  const GPtrArray *channels;
  ClutterActor *viewport;
  ClutterColor color = {0xff, 0, 0, 0};
  guint i;

  self->priv = priv = EPG_PRIVATE (self);

  priv->pixels_for_5_mins = 14;
  priv->event_range = 840;

  /* column */
  priv->channel_box = g_object_new (MX_TYPE_BOX_LAYOUT,
                                    "orientation", MX_ORIENTATION_VERTICAL,
                                    NULL);
  clutter_actor_set_name (priv->channel_box, "epg-channel-list");

  clutter_actor_set_parent (priv->channel_box, CLUTTER_ACTOR (self));

  /* scrollview */
  priv->grid_scrollview = g_object_new (MEX_TYPE_SCROLL_VIEW, NULL);
  mex_scroll_view_set_indicators_hidden (MEX_SCROLL_VIEW (priv->grid_scrollview),
                                         TRUE);
  mex_scroll_view_set_follow_recurse (MEX_SCROLL_VIEW (priv->grid_scrollview),
                                      TRUE);
  clutter_actor_set_parent (priv->grid_scrollview, CLUTTER_ACTOR (self));

  /* viewport (for the table) */
  viewport = mx_viewport_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->grid_scrollview),
                               viewport);

  /* grid (table) */
  priv->grid = mex_epg_grid_new ();
  g_signal_connect (priv->grid, "row-selected",
                    G_CALLBACK (on_epg_grid_row_selected), self);
  g_signal_connect (priv->grid, "event-activated",
                    G_CALLBACK (on_epg_grid_event_activated), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (viewport), priv->grid);

  /* Add shadows */
  clutter_actor_add_effect (priv->channel_box,
                            CLUTTER_EFFECT (mex_shadow_new ()));

  /* selection_indicator */
  priv->selection_indicator = clutter_rectangle_new_with_color (&color);
  clutter_actor_set_name (priv->selection_indicator, "selection-indicator");
  clutter_actor_set_parent (priv->selection_indicator, CLUTTER_ACTOR (self));
  clutter_actor_set_size (priv->selection_indicator, 200, 200);

  /* We assume the channels are already configured */
  manager = mex_channel_manager_get_default ();
  channels = mex_channel_manager_get_channels (manager);
  priv->channels = g_ptr_array_sized_new (channels->len);
  for (i = 0; i < channels->len; i++)
    {
      MexChannel *channel = g_ptr_array_index (channels, i);

      mex_epg_add_channel (self, channel, i);
    }

}

ClutterActor *
mex_epg_new (void)
{
  return g_object_new (MEX_TYPE_EPG, NULL);
}

guint
mex_epg_get_event_range (MexEpg *epg)
{
  g_return_val_if_fail (MEX_IS_EPG (epg), 0);

  return epg->priv->event_range;
}

void
mex_epg_set_event_range (MexEpg *epg,
                         guint   event_range)
{
  MexEpgPrivate *priv;

  g_return_if_fail (MEX_IS_EPG (epg));
  priv = epg->priv;

  priv->event_range = event_range;

  g_object_notify (G_OBJECT (epg), "event-range");
}


void
mex_epg_show_events_for_datetime (MexEpg    *epg,
                                  GDateTime *start_date)
{
  MexEpgPrivate *priv;
  MexEpgManager *epg_manager;
  MexChannelManager *channel_manager;
  GDateTime *start_plus_range;
  const GPtrArray *channels;
  guint i;

  g_return_if_fail (MEX_IS_EPG (epg));
  priv = epg->priv;

  /* FIXME cancel current get_events() requests */

  mex_epg_grid_set_current_date_time (MEX_EPG_GRID (priv->grid), start_date);

  if (priv->start_date)
    g_date_time_unref (priv->start_date);
  if (priv->end_date)
    g_date_time_unref (priv->end_date);

  /* Set up the time span of the EpgGrid */
  priv->start_date = round_to_30min (start_date, FALSE);
  start_plus_range = g_date_time_add_minutes (priv->start_date,
                                              priv->event_range);
  priv->end_date = round_to_30min (start_plus_range, FALSE);
  g_date_time_unref (start_plus_range);
  mex_epg_grid_set_date_time_span (MEX_EPG_GRID (priv->grid),
                                   priv->start_date,
                                   priv->end_date);

  /* Start the process by querying the needed EpgEvents */
  epg_manager = mex_epg_manager_get_default ();
  channel_manager = mex_channel_manager_get_default ();
  channels = mex_channel_manager_get_channels (channel_manager);
  for (i = 0; i < channels->len; i++)
    {
      MexChannel *channel = g_ptr_array_index (channels, i);

      mex_epg_manager_get_events (epg_manager, channel,
                                  priv->start_date, priv->end_date,
                                  on_get_events_reply, epg);
    }
}

void
mex_epg_show_events_now (MexEpg *epg)
{
  GDateTime *middle_date;

  g_return_if_fail (MEX_IS_EPG (epg));

  middle_date = g_date_time_new_now_local ();
  mex_epg_show_events_for_datetime (epg, middle_date);

  g_date_time_unref (middle_date);
}

#if defined (ENABLE_TESTS)

#include "mex-test-internal.h"

void
mex_test_epg_round_30min (void)
{
  GDateTime *rounded, *to_round;
  gchar *str;

  to_round = g_date_time_new_local (2010, 12, 10, 15, 54, 4);

  rounded = round_to_30min (to_round, FALSE);
  str = g_date_time_format (rounded, "%d/%m/%Y %H:%M:%S");
  g_assert_cmpstr (str, ==, "10/12/2010 15:30:00");
  g_free (str);
  g_date_time_unref (rounded);

  rounded = round_to_30min (to_round, TRUE);
  str = g_date_time_format (rounded, "%d/%m/%Y %H:%M:%S");
  g_assert_cmpstr (str, ==, "10/12/2010 16:00:00");
  g_free (str);
  g_date_time_unref (rounded);

  g_date_time_unref (to_round);
  to_round = g_date_time_new_local (2010, 12, 10, 23, 15, 32);

  rounded = round_to_30min (to_round, FALSE);
  str = g_date_time_format (rounded, "%d/%m/%Y %H:%M:%S");
  g_assert_cmpstr (str, ==, "10/12/2010 23:00:00");
  g_free (str);
  g_date_time_unref (rounded);

  rounded = round_to_30min (to_round, TRUE);
  str = g_date_time_format (rounded, "%d/%m/%Y %H:%M:%S");
  g_assert_cmpstr (str, ==, "10/12/2010 23:30:00");
  g_free (str);
  g_date_time_unref (rounded);

  g_date_time_unref (to_round);
}
#endif
