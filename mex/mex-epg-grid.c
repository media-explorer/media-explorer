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


#include "mex-epg-grid.h"

#include "mex-channel-manager.h"
#include "mex-epg-tile.h"
#include "mex-log.h"
#include "mex-marshal.h"
#include "mex-private.h"
#include "mex-utils.h"

#define MEX_LOG_DOMAIN_DEFAULT  epg_log_domain
MEX_LOG_DOMAIN_EXTERN(epg_log_domain);

static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexEpgGrid,
                         mex_epg_grid,
                         MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                mx_focusable_iface_init))

#define EPG_GRID_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_EPG_GRID, MexEpgGridPrivate))

#define HEADER_HEIGHT 50

enum
{
  SIGNAL_ROW_SELECTED,
  SIGNAL_EVENT_ACTIVATED,

  SIGNAL_LAST
};

struct _MexEpgGridPrivate
{
  guint has_focus_but_no_tile : 1;

  GPtrArray *header;        /* Array of MxFrames for the header row */
  GPtrArray *rows;          /* GPtrArray of GPtrArray of MexEpgTiles */

  guint pixels_for_5_mins;  /* The amount of pixels that represents 5 mins */

  GDateTime *first_date;    /* The first (chronologically) date we display */
  GDateTime *last_date;     /* the last ... */
  GDateTime *current_date;  /* Override the current date (instead of now) */

  guint focused_row_idx;    /* which row holds the focused tile */
  guint focused_tile_idx;   /* which tile in the row has the focus */
  GDateTime *focused_date;  /* the date we should focus on DOWN/UP events */

  guint n_rows_to_load;     /* number of rows that still need to be loaded */
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
remove_header (gpointer data)
{

}

static void
free_ptr_array (gpointer data)
{
  GPtrArray *array = data;

  g_ptr_array_unref (array);
}

static void
remove_row (MexEpgGrid *grid,
            gint        position)
{
  MexEpgGridPrivate *priv = grid->priv;
  GPtrArray *row;
  guint i;

  row = g_ptr_array_index (priv->rows, position);
  for (i = 0; i < row->len; i++)
    {
      ClutterActor *tile = g_ptr_array_index (row, i);

      clutter_actor_unparent (tile);
    }
  g_ptr_array_free (row, TRUE);

  g_ptr_array_index (priv->rows, position) = NULL;
}

static void
create_header (MexEpgGrid *grid)
{
  MexEpgGridPrivate *priv = grid->priv;
  GTimeSpan diff;
  GDateTime *time_, *old_time;
  gint n_headers, i;
  gchar *time_str;

  diff = g_date_time_difference (priv->last_date, priv->first_date);
  n_headers = (diff * 1e-6 / 60. / 30) + 1; /* number of 30mins slices */

  if (MEX_DEBUG_ENABLED)
    {
      gchar *first_str, *last_str;

      first_str = mex_date_to_string (priv->first_date);
      last_str = mex_date_to_string (priv->last_date);
      MEX_DEBUG ("Creating header between %s and %s (%d columns)",
                 first_str, last_str, n_headers);
      g_free (first_str);
      g_free (last_str);
    }

  g_ptr_array_set_size (priv->header, n_headers);

  time_ = g_date_time_ref (priv->first_date);
  for (i = 0; i < n_headers; i++)
    {
      ClutterActor  *frame, *label;

      /* a Frame for 30 mins, each frame has a label in it */
      frame = mx_frame_new ();
      clutter_actor_set_parent (frame, CLUTTER_ACTOR (grid));
      mx_stylable_set_style_class (MX_STYLABLE (frame), "EpgHeader");
      clutter_actor_set_size (frame,
                              6 * priv->pixels_for_5_mins,
                              HEADER_HEIGHT);

      time_str = g_date_time_format (time_, "%H:%M");
      label = mx_label_new_with_text (time_str);
      mx_bin_set_child (MX_BIN (frame), label);
      mx_bin_set_alignment (MX_BIN (frame), MX_ALIGN_START, MX_ALIGN_MIDDLE);

      g_ptr_array_index (priv->header, i) = frame;

      g_free (time_str);
      old_time = time_;
      time_ = g_date_time_add_minutes (time_, 30);
      g_date_time_unref (old_time);
    }
  g_date_time_unref (time_);
}

static void
on_tile_clicked (MexEpgTile *tile,
                 MexEpgGrid *grid)
{
  MexEpgEvent *event;

  event = mex_epg_tile_get_event (tile);
  g_signal_emit (grid, signals[SIGNAL_EVENT_ACTIVATED], 0, event);
}

/*
 * MxFocusable implementation
 */

/* When first focussing the grid, or then when pressing right or left, we
 * record a date (the middle of the focused event) that will then be used
 * when pressing up/down to focus the relevant tile, the one that contains the
 * this focused date */
static void
update_focused_date (MexEpgGrid *grid,
                     MexEpgTile *tile)
{
  MexEpgGridPrivate *priv = grid->priv;
  GDateTime *start_date, *middle_date;
  MexEpgEvent *event;
  gint duration;

  if (priv->focused_date)
    g_date_time_unref (priv->focused_date);

  event = mex_epg_tile_get_event (tile);

  start_date = mex_epg_event_get_start_date (event);
  duration = mex_epg_event_get_duration (event);
  middle_date = g_date_time_add_seconds (start_date, duration / 2);

  priv->focused_date = middle_date;
}

static gint
find_similar_event (MexEpgGrid  *grid,
                    guint        row_idx,
                    MexEpgEvent *source)
{
  MexEpgGridPrivate *priv = grid->priv;
  MexEpgEvent *event;
  MexEpgTile *tile = NULL;
  GPtrArray *row;
  guint i = 0;

  if (priv->focused_date == NULL)
    return -1;

  row = g_ptr_array_index (priv->rows, row_idx);
  if (row == NULL)
    return -1;

  for (i = 0; i < row->len; i++)
    {
      tile = g_ptr_array_index (row, i);
      event = mex_epg_tile_get_event (tile);

      if (mex_epg_event_is_date_in_between (event, priv->focused_date))
        break;
    }

  if (i == row->len)
    i = row->len -1;

  return i;
}

static MxFocusable *
mex_epg_grid_move_focus (MxFocusable      *focusable,
                         MxFocusDirection  direction,
                         MxFocusable      *from)
{
  MexEpgGrid *grid = MEX_EPG_GRID (focusable);
  MexEpgGridPrivate *priv = grid->priv;
  MxFocusable *new_focused = NULL;
  GPtrArray *focused_row;
  MexEpgEvent *event;
  MxFocusHint hint = MX_FOCUS_HINT_FIRST;;
  gint new_tile_position = -1;

  focused_row = g_ptr_array_index (priv->rows, priv->focused_row_idx);
  if (focused_row == NULL)
    return NULL;

  /* just ignore the move if there is no tile in the grid */
  if (priv->has_focus_but_no_tile)
    return NULL;

  switch (direction)
    {
    case MX_FOCUS_DIRECTION_RIGHT:
    case MX_FOCUS_DIRECTION_NEXT:
      if (priv->focused_tile_idx == focused_row->len - 1)
          return NULL;

      priv->focused_tile_idx++;
      new_focused = g_ptr_array_index (focused_row, priv->focused_tile_idx);

      update_focused_date (grid, MEX_EPG_TILE (new_focused));
      break;

    case MX_FOCUS_DIRECTION_LEFT:
    case MX_FOCUS_DIRECTION_PREVIOUS:
      if (priv->focused_tile_idx == 0)
          return NULL;

      priv->focused_tile_idx--;
      new_focused = g_ptr_array_index (focused_row, priv->focused_tile_idx);

      update_focused_date (grid, MEX_EPG_TILE (new_focused));
      break;

    case MX_FOCUS_DIRECTION_UP:
      if (priv->focused_row_idx == 0)
        return NULL;

      /* from can be either the EpgGrid itself or an EpgTile */
      if (MEX_IS_EPG_TILE (from))
        {
          event = mex_epg_tile_get_event (MEX_EPG_TILE (from));
          new_tile_position = find_similar_event (grid,
                                                  priv->focused_row_idx - 1,
                                                  event);
        }

      if (new_tile_position == -1)
        return NULL;

      priv->focused_row_idx--;
      priv->focused_tile_idx = new_tile_position;
      focused_row = g_ptr_array_index (priv->rows, priv->focused_row_idx);
      new_focused = g_ptr_array_index (focused_row, priv->focused_tile_idx);

      g_signal_emit (focusable, signals[SIGNAL_ROW_SELECTED], 0,
                     priv->focused_row_idx);
      break;

    case MX_FOCUS_DIRECTION_DOWN:
      if (priv->focused_row_idx == priv->rows->len - 1)
        return NULL;

      /* from can be either the EpgGrid itself or an EpgTile */
      if (MEX_IS_EPG_TILE (from))
        {
          event = mex_epg_tile_get_event (MEX_EPG_TILE (from));
          new_tile_position = find_similar_event (grid,
                                                  priv->focused_row_idx + 1,
                                                  event);
        }

      if (new_tile_position == -1)
        return NULL;

      priv->focused_row_idx++;
      priv->focused_tile_idx = new_tile_position;
      focused_row = g_ptr_array_index (priv->rows, priv->focused_row_idx);
      new_focused = g_ptr_array_index (focused_row, priv->focused_tile_idx);

      g_signal_emit (focusable, signals[SIGNAL_ROW_SELECTED], 0,
                     priv->focused_row_idx);
      break;

    case MX_FOCUS_DIRECTION_OUT:
      priv->has_focus_but_no_tile = FALSE;
      break;

    default:
      break;
    }

  if (new_focused)
    return mx_focusable_accept_focus (new_focused, hint);

  return NULL;
}

static MxFocusable *
mex_epg_grid_accept_focus (MxFocusable *focusable,
                           MxFocusHint  hint)
{
  MexEpgGrid *grid = MEX_EPG_GRID (focusable);
  MexEpgGridPrivate *priv = grid->priv;
  MxFocusable *focused, *focused_tile;
  GPtrArray *focused_row;

  if (priv->focused_row_idx == -1)
    {
      priv->has_focus_but_no_tile = TRUE;
      return focusable;
    }

  focused_row = g_ptr_array_index (priv->rows, priv->focused_row_idx);
  if (G_UNLIKELY (focused_row == NULL))
    {
      priv->has_focus_but_no_tile = TRUE;
      return focusable;
    }

  focused_tile = g_ptr_array_index (focused_row, 0);

  update_focused_date (grid, MEX_EPG_TILE (focused_tile));

  focused = mx_focusable_accept_focus (focused_tile, hint);


  g_signal_emit (focusable, signals[SIGNAL_ROW_SELECTED], 0, 0);

  return focused;
}

static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->move_focus = mex_epg_grid_move_focus;
  iface->accept_focus = mex_epg_grid_accept_focus;
}

/*
 * ClutterActor implementation
 */

#define TILE_HEIGHT   64

static guint
time_span_to_pixels (MexEpgGrid *grid,
                     GTimeSpan   span)
{
  return span * 1e-6 / 60 / 5 * grid->priv->pixels_for_5_mins;
}

static void
mex_epg_grid_get_preferred_width (ClutterActor *actor,
                                  gfloat        for_height,
                                  gfloat       *min_width_p,
                                  gfloat       *nat_width_p)
{
  MexEpgGrid *grid = MEX_EPG_GRID (actor);
  MexEpgGridPrivate *priv = grid->priv;
  MxPadding padding;
  GTimeSpan diff;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (priv->first_date == NULL || priv->last_date == NULL)
    diff = 0;
  else
    diff = g_date_time_difference (priv->last_date, priv->first_date);

  if (min_width_p)
    *min_width_p = 0;

  if (nat_width_p)
    *nat_width_p = time_span_to_pixels (grid, diff) + padding.left +
                   padding.right;
}

static void
mex_epg_grid_get_preferred_height (ClutterActor *actor,
                                   gfloat        for_width,
                                   gfloat       *min_height_p,
                                   gfloat       *nat_height_p)
{
  MexEpgGrid *grid = MEX_EPG_GRID (actor);
  MexEpgGridPrivate *priv = grid->priv;
  MxPadding padding;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_height_p)
    *min_height_p = 0;

  if (nat_height_p)
    *nat_height_p = priv->rows->len * TILE_HEIGHT + padding.top +
                    padding.bottom;
}

static
void get_tile_box (MexEpgGrid      *grid,
                   MexEpgTile      *tile,
                   guint            row,
                   MxPadding       *padding,
                   ClutterActorBox *box)
{
  MexEpgGridPrivate *priv = grid->priv;
  MexEpgEvent *event;
  GDateTime *start_date;
  GTimeSpan diff;
  gint duration;

  event = mex_epg_tile_get_event (tile);
  start_date = mex_epg_event_get_start_date (event);

  if (g_date_time_compare (start_date, priv->first_date) < 0)
    diff = 0;
  else
    diff = g_date_time_difference (start_date, priv->first_date);   /* us */
  duration = mex_epg_event_get_duration (event);                  /*  s */

  box->x1 = padding->left + time_span_to_pixels (grid, diff);
  box->y1 = padding->top + HEADER_HEIGHT + TILE_HEIGHT * row;
  box->x2 = box->x1 + duration / 60 / 5 * priv->pixels_for_5_mins;
  box->y2 = box->y1 + TILE_HEIGHT;

#if 0
  g_message ("Allocating tile (%f,%f) (%f,%f)",
             box->x1, box->y1, box->x2, box->y2);
#endif
}

static void
mex_epg_grid_allocate (ClutterActor           *actor,
                       const ClutterActorBox  *box,
                       ClutterAllocationFlags  flags)
{
  MexEpgGrid *grid = MEX_EPG_GRID (actor);
  MexEpgGridPrivate *priv = grid->priv;
  ClutterActorBox tile_box, header_box;
  MxPadding padding;
  guint i, j;

  CLUTTER_ACTOR_CLASS (mex_epg_grid_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  /* header */
  for (i = 0; i < priv->header->len; i++)
    {
      ClutterActor *header = g_ptr_array_index (priv->header, i);

      header_box.x1 = padding.left + i * 6 * priv->pixels_for_5_mins;
      header_box.y1 = padding.top;
      header_box.x2 = header_box.x1 + 6 * priv->pixels_for_5_mins;
      header_box.y2 = header_box.y1 + HEADER_HEIGHT;

#if 0
      g_message ("Allocating header (%f,%f) (%f,%f)",
                 header_box.x1, header_box.y1,
                 header_box.x2, header_box.y2);
#endif

      clutter_actor_allocate (header, &header_box, flags);
    }

  /* tiles */
  for (i = 0; i < priv->rows->len; i++)
    {
      GPtrArray *tiles = g_ptr_array_index (priv->rows, i);

      if (tiles == NULL)
        continue;

      for (j = 0; j < tiles->len; j++)
        {
          MexEpgTile *tile = g_ptr_array_index (tiles, j);

          get_tile_box (grid, tile, i, &padding, &tile_box);
          clutter_actor_allocate (CLUTTER_ACTOR (tile), &tile_box, flags);
        }
    }
}

static void
mex_epg_grid_paint (ClutterActor *actor)
{
  MexEpgGrid *grid = MEX_EPG_GRID (actor);
  MexEpgGridPrivate *priv = grid->priv;
  guint i, j;

  CLUTTER_ACTOR_CLASS (mex_epg_grid_parent_class)->paint (actor);

  /* header */
  for (i = 0; i < priv->header->len; i++)
    {
      ClutterActor *header = g_ptr_array_index (priv->header, i);

      clutter_actor_paint (header);
    }

  /* tiles */
  for (i = 0; i < priv->rows->len; i++)
    {
      GPtrArray *tiles = g_ptr_array_index (priv->rows, i);

      if (tiles == NULL)
        continue;

      for (j = 0; j < tiles->len; j++)
        {
          ClutterActor *tile = g_ptr_array_index (tiles, j);

          clutter_actor_paint (tile);
        }
    }
}

static void
mex_epg_grid_map (ClutterActor *actor)
{
  MexEpgGrid *grid = MEX_EPG_GRID (actor);
  MexEpgGridPrivate *priv = grid->priv;
  guint i, j;

  CLUTTER_ACTOR_CLASS (mex_epg_grid_parent_class)->map (actor);

  /* header */
  for (i = 0; i < priv->header->len; i++)
    {
      ClutterActor *header = g_ptr_array_index (priv->header, i);

      clutter_actor_map (header);
    }

  /* tiles */
  for (i = 0; i < priv->rows->len; i++)
    {
      GPtrArray *tiles = g_ptr_array_index (priv->rows, i);

      if (tiles == NULL)
        continue;

      for (j = 0; j < tiles->len; j++)
        {
          ClutterActor *tile = g_ptr_array_index (tiles, j);

          clutter_actor_map (tile);
        }
    }
}

static void
mex_epg_grid_unmap (ClutterActor *actor)
{
  MexEpgGrid *grid = MEX_EPG_GRID (actor);
  MexEpgGridPrivate *priv = grid->priv;
  guint i, j;

  /* header */
  for (i = 0; i < priv->header->len; i++)
    {
      ClutterActor *header = g_ptr_array_index (priv->header, i);

      clutter_actor_unmap (header);
    }

  /* tiles */
  for (i = 0; i < priv->rows->len; i++)
    {
      GPtrArray *tiles = g_ptr_array_index (priv->rows, i);

      if (tiles == NULL)
        continue;

      for (j = 0; j < tiles->len; j++)
        {
          ClutterActor *tile = g_ptr_array_index (tiles, j);

          clutter_actor_unmap (tile);
        }
    }

  CLUTTER_ACTOR_CLASS (mex_epg_grid_parent_class)->unmap (actor);
}

/*
 * GObject implementation
 */

static void
mex_epg_grid_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_grid_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_grid_finalize (GObject *object)
{
  MexEpgGrid *grid = MEX_EPG_GRID (object);
  MexEpgGridPrivate *priv = grid->priv;

  if (priv->current_date)
    g_date_time_unref (priv->current_date);
  if (priv->first_date)
    g_date_time_unref (priv->first_date);
  if (priv->last_date)
    g_date_time_unref (priv->last_date);

  G_OBJECT_CLASS (mex_epg_grid_parent_class)->finalize (object);
}

static void
mex_epg_grid_class_init (MexEpgGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexEpgGridPrivate));

  object_class->get_property = mex_epg_grid_get_property;
  object_class->set_property = mex_epg_grid_set_property;
  object_class->finalize = mex_epg_grid_finalize;

  actor_class->get_preferred_width = mex_epg_grid_get_preferred_width;
  actor_class->get_preferred_height = mex_epg_grid_get_preferred_height;
  actor_class->allocate = mex_epg_grid_allocate;
  actor_class->paint = mex_epg_grid_paint;
  actor_class->map = mex_epg_grid_map;
  actor_class->unmap = mex_epg_grid_unmap;

  signals[SIGNAL_ROW_SELECTED] = g_signal_new ("row-selected",
                                               MEX_TYPE_EPG_GRID,
                                               G_SIGNAL_RUN_FIRST,
                                               0,
                                               NULL,
                                               NULL,
                                               mex_marshal_VOID__INT,
                                               G_TYPE_NONE,
                                               1,
                                               G_TYPE_INT);

  signals[SIGNAL_EVENT_ACTIVATED] =
    g_signal_new ("event-activated",
                  MEX_TYPE_EPG_GRID,
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
mex_epg_grid_init (MexEpgGrid *self)
{
  MexEpgGridPrivate *priv;
  MexChannelManager *channel_manager;

  self->priv = priv = EPG_GRID_PRIVATE (self);

  priv->pixels_for_5_mins = 32;

  channel_manager = mex_channel_manager_get_default ();

  priv->n_rows_to_load = mex_channel_manager_get_n_channels (channel_manager);
  priv->header = g_ptr_array_new_with_free_func (remove_header);

  priv->rows = g_ptr_array_new_with_free_func (free_ptr_array);
  g_ptr_array_set_size (priv->rows, priv->n_rows_to_load);
}

ClutterActor *
mex_epg_grid_new (void)
{
  return g_object_new (MEX_TYPE_EPG_GRID, NULL);
}

static void
row_loaded (MexEpgGrid *grid,
            gint        position)
{
  MexEpgGridPrivate *priv = grid->priv;

  if (G_UNLIKELY (priv->n_rows_to_load) == 0)
    {
      MEX_WARNING ("A new row was loaded, but we've already loaded every row");
      return;
    }

  priv->n_rows_to_load--;
}

void
mex_epg_grid_add_events (MexEpgGrid *grid,
                         MexChannel *channel,
                         GPtrArray  *events)
{
  MexEpgGridPrivate *priv;
  MexChannelManager *channel_manager;
  MxFocusManager *focus_manager;
  GPtrArray *row;
  gint position;
  guint i;

  g_return_if_fail (MEX_IS_EPG_GRID (grid));
  g_return_if_fail (MEX_IS_CHANNEL (channel));
  g_return_if_fail (events);
  priv = grid->priv;

  channel_manager = mex_channel_manager_get_default ();
  position = mex_channel_manager_get_channel_position (channel_manager,
                                                       channel);
  if (G_UNLIKELY (position == -1))
    {
      MEX_WARNING ("Could not find position of channel %s",
                   mex_channel_get_name (channel));
      return;
    }

  /* no events for this channel */
  if (G_UNLIKELY (events->len == 0))
    {
      /* signal that we won't have data for that row */
      row_loaded (grid, position);
      return;
    }

  /* we insert tiles in bulk, removing the existing tiles if needed */
  row = g_ptr_array_index (priv->rows, position);

  /* If we already have data for that row, we assume the caller wants to
   * replace the data. If we don't we signal that a new row is loaded */
  if (row)
    remove_row (grid, position);
  else
    row_loaded (grid, position);

  row = g_ptr_array_new ();
  g_ptr_array_set_size (row, events->len);

  /* We are adding events, it's a good time to check if we have a valid
   * current date */
  if (priv->current_date == NULL)
    priv->current_date = g_date_time_new_now_local ();

  for (i = 0; i < events->len; i++)
    {
      MexEpgEvent *event = g_ptr_array_index (events, i);
      ClutterActor *tile;

      tile = mex_epg_tile_new_with_event (event);
      g_signal_connect (tile, "clicked",
                        G_CALLBACK (on_tile_clicked), grid);
      clutter_actor_set_parent (tile, CLUTTER_ACTOR (grid));

      g_ptr_array_index (row, i) = tile;

#if 0
      /* Disabled because we don't need to style differently the events that
       * are occuring now in the current design, might come back though */
      if (mex_epg_event_is_date_in_between (event, priv->current_date))
        mx_stylable_set_style_class (MX_STYLABLE (tile), "EpgTileNow");
#endif
    }

  g_ptr_array_index (priv->rows, position) = row;

  /* If the EpgGrid had the focus before we had a chance to add events,
   * it's a good time to push the focus to one of the EpgTile */
  /* FIXME: default to the channel we are watching, not row 0 */
  if (priv->has_focus_but_no_tile && position == 0 && row->len > 0)
    {
      ClutterActor *focused_tile = g_ptr_array_index (row, 0);
      ClutterActor *stage;

      stage = clutter_actor_get_stage (focused_tile);
      focus_manager = mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));
      mx_focus_manager_push_focus (focus_manager, MX_FOCUSABLE (focused_tile));

      g_signal_emit (grid, signals[SIGNAL_ROW_SELECTED], 0, 0);

      priv->has_focus_but_no_tile = FALSE;
    }

  /* We have a new row, relayout */
  clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
}

/* Used to override the current time */
/* FIXME: check if the new date is between the span first/last{_date} */
void
mex_epg_grid_set_current_date_time (MexEpgGrid *grid,
                                    GDateTime  *date)
{
  MexEpgGridPrivate *priv;

  g_return_if_fail (MEX_IS_EPG_GRID (grid));
  g_return_if_fail (date);
  priv = grid->priv;

  if (priv->current_date)
    g_date_time_unref (priv->current_date);

  priv->current_date = g_date_time_ref (date);

  /* FIXME: We need to reset the classes of the tiles to take the new current
   * time into account, for now it works only if set_current_date_time()
   * is called before add_events(), which is good enough */
}

/* FIXME this expects start and end aligned to 30mins marks do the rounding
 * here rather than epg-grid? -> yes */
void
mex_epg_grid_set_date_time_span (MexEpgGrid *grid,
                                 GDateTime  *start,
                                 GDateTime  *end)
{
  MexEpgGridPrivate *priv;

  g_return_if_fail (MEX_IS_EPG_GRID (grid));
  g_return_if_fail (start && end);
  priv = grid->priv;

  priv->first_date = g_date_time_ref (start);
  priv->last_date = g_date_time_ref (end);

  create_header (grid);
  clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
}
