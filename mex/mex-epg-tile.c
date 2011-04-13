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


#include "mex-epg-tile.h"

#include "mex-private.h"

G_DEFINE_TYPE (MexEpgTile, mex_epg_tile, MX_TYPE_BUTTON)

#define EPG_TILE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_EPG_TILE, MexEpgTilePrivate))

enum
{
  PROP_0,

  PROP_EVENT
};

struct _MexEpgTilePrivate
{
  MexEpgEvent *event;
};

/*
 * GObject implementation
 */

static void
mex_epg_tile_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  MexEpgTile *tile = MEX_EPG_TILE (object);
  MexEpgTilePrivate *priv = tile->priv;

  switch (property_id)
    {
    case PROP_EVENT:
      g_value_set_object (value, priv->event);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_tile_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  MexEpgTile *tile = MEX_EPG_TILE (object);

  switch (property_id)
    {
    case PROP_EVENT:
      mex_epg_tile_set_event (tile, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_epg_tile_finalize (GObject *object)
{
  MexEpgTile *tile = MEX_EPG_TILE (object);
  MexEpgTilePrivate *priv = tile->priv;

  if (priv->event)
    g_object_unref (priv->event);

  G_OBJECT_CLASS (mex_epg_tile_parent_class)->finalize (object);
}

static void
mex_epg_tile_class_init (MexEpgTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexEpgTilePrivate));

  object_class->get_property = mex_epg_tile_get_property;
  object_class->set_property = mex_epg_tile_set_property;
  object_class->finalize = mex_epg_tile_finalize;

  pspec = g_param_spec_object ("event",
                               "Event",
                               "Event that the tile represents",
                               MEX_TYPE_EPG_EVENT,
                               MEX_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_EVENT, pspec);
}

static void
mex_epg_tile_init (MexEpgTile *self)
{
  self->priv = EPG_TILE_PRIVATE (self);
}

ClutterActor *
mex_epg_tile_new (void)
{
  return g_object_new (MEX_TYPE_EPG_TILE, NULL);
}

ClutterActor *
mex_epg_tile_new_with_event (MexEpgEvent *event)
{
  return g_object_new (MEX_TYPE_EPG_TILE,
                       "event", event,
                       NULL);
}

MexEpgEvent *
mex_epg_tile_get_event (MexEpgTile *tile)
{
  g_return_val_if_fail (MEX_IS_EPG_TILE (tile), NULL);

  return tile->priv->event;
}

void
mex_epg_tile_set_event (MexEpgTile  *tile,
                        MexEpgEvent *event)
{
  MexEpgTilePrivate *priv;
  MexProgram *program;
  const gchar *title;

  g_return_if_fail (MEX_IS_EPG_TILE (tile));
  priv = tile->priv;

  if (priv->event)
    {
      g_object_unref (priv->event);
      priv->event = NULL;
    }

  if (event)
    priv->event = g_object_ref (event);

  /* Update the label of the button */
  program = mex_epg_event_get_program (event);
  title = mex_program_get_metadata (program, MEX_CONTENT_METADATA_TITLE);
  mx_button_set_label (MX_BUTTON (tile), title);
  mx_bin_set_alignment (MX_BIN (tile), MX_ALIGN_START, MX_ALIGN_MIDDLE);

  g_object_notify (G_OBJECT (tile), "event");
}
