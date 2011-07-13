/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
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
 * SECTION:mex-resizing-hbox-child
 * @short_description: meta data associated with a #MexResizingHBox child.
 *
 * #MexResizingHBoxChild is a #ClutterChildMeta implementation that stores the
 * child properties for children inside a #MexResizingHBox.
 */

#include "mex-resizing-hbox-child.h"

G_DEFINE_TYPE (MexResizingHBoxChild, mex_resizing_hbox_child,
               CLUTTER_TYPE_CHILD_META)

#define RESIZING_HBOX_CHILD_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_RESIZING_HBOX_CHILD, \
                                MexResizingHBoxChildPrivate))

static void
mex_resizing_hbox_child_dispose (GObject *object)
{
  MexResizingHBoxChild *self = MEX_RESIZING_HBOX_CHILD (object);

  if (self->timeline)
    {
      clutter_timeline_stop (self->timeline);
      g_object_unref (self->timeline);
      self->timeline = NULL;
    }

  G_OBJECT_CLASS (mex_resizing_hbox_child_parent_class)->dispose (object);
}

static void
mex_resizing_hbox_child_class_init (MexResizingHBoxChildClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = mex_resizing_hbox_child_dispose;
}

static void
mex_resizing_hbox_child_init (MexResizingHBoxChild *self)
{
}
