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

enum
{
  PROP_0,

  PROP_EXPAND,
  PROP_PUSH
};

static void
mex_resizing_hbox_child_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  MexResizingHBoxChild *child = MEX_RESIZING_HBOX_CHILD (object);

  switch (property_id)
    {
    case PROP_EXPAND:
      g_value_set_boolean (value, child->expand);
      break;
    case PROP_PUSH:
      g_value_set_boolean (value, child->push);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_resizing_hbox_child_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  MexResizingHBoxChild *child = MEX_RESIZING_HBOX_CHILD (object);
  ClutterActor *actor =
    clutter_child_meta_get_actor (CLUTTER_CHILD_META (object));

  switch (property_id)
    {
    case PROP_EXPAND:
      child->expand = g_value_get_boolean (value);
      break;

    case PROP_PUSH:
      child->push = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }

  clutter_actor_queue_relayout (actor);
}

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
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = mex_resizing_hbox_child_get_property;
  object_class->set_property = mex_resizing_hbox_child_set_property;
  object_class->dispose = mex_resizing_hbox_child_dispose;

  pspec = g_param_spec_boolean ("expand", "Expand",
                                "Whether the child should receive priority "
                                "when the container is allocating spare space.",
                                TRUE,
                                G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_EXPAND, pspec);

  pspec = g_param_spec_boolean ("push", "push",
                                "Whether the child should 'push aside' other "
                                "children.",
                                FALSE,
                                G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PUSH, pspec);
}

static void
mex_resizing_hbox_child_init (MexResizingHBoxChild *self)
{
  self->expand = TRUE;
  self->push = FALSE;
}

static MexResizingHBoxChild *
_get_child_meta (MexResizingHBox *resizing_hbox,
                 ClutterActor    *child)
{
  MexResizingHBoxChild *meta;

  meta = (MexResizingHBoxChild*)
    clutter_container_get_child_meta (CLUTTER_CONTAINER (resizing_hbox), child);

  return meta;
}

/**
 * mex_resizing_hbox_child_get_expand:
 * @resizing_hbox: A #MexResizingHBox
 * @child: A #ClutterActor
 *
 * Get the value of the #MexResizingHBoxChild:expand property.
 *
 * Returns: the current value of the "expand" property.
 */
gboolean
mex_resizing_hbox_child_get_expand (MexResizingHBox *resizing_hbox,
                                    ClutterActor    *child)
{
  MexResizingHBoxChild *meta;

  g_return_val_if_fail (MEX_IS_RESIZING_HBOX (resizing_hbox), FALSE);
  g_return_val_if_fail (CLUTTER_IS_ACTOR (child), FALSE);

  meta = _get_child_meta (resizing_hbox, child);

  return meta->expand;
}

/**
 * mex_resizing_hbox_child_set_expand:
 * @resizing_hbox: A #MexResizingHBox
 * @child: A #ClutterActor
 * @expand: A #gboolean
 *
 * Set the value of the #MexResizingHBoxChild:expand property.
 */
void
mex_resizing_hbox_child_set_expand (MexResizingHBox *resizing_hbox,
                                    ClutterActor    *child,
                                    gboolean         expand)
{
  MexResizingHBoxChild *meta;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (resizing_hbox));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  meta = _get_child_meta (resizing_hbox, child);

  meta->expand = expand;

  clutter_actor_queue_relayout (child);
}

/**
 * mex_resizing_hbox_child_get_push:
 * @resizing_hbox: An #MexResizingHBox
 * @child: A #ClutterActor
 *
 * Get the value of the #MexResizingHBoxChild:push property
 *
 * Returns: the current value of the "push" property
 */
gboolean
mex_resizing_hbox_child_get_push (MexResizingHBox *resizing_hbox,
                                  ClutterActor    *child)
{
  MexResizingHBoxChild *meta;

  g_return_val_if_fail (MEX_IS_RESIZING_HBOX (resizing_hbox), FALSE);
  g_return_val_if_fail (CLUTTER_IS_ACTOR (child), FALSE);

  meta = _get_child_meta (resizing_hbox, child);

  return meta->push;
}

/**
 * mex_resizing_hbox_child_set_push:
 * @resizing_hbox: An #MexResizingHBox
 * @child: A #ClutterActor
 * @push: A #gboolean
 *
 * Set the value of the #MexResizingHBoxChild:push property.
 */
void
mex_resizing_hbox_child_set_push (MexResizingHBox *resizing_hbox,
                                  ClutterActor    *child,
                                  gboolean         push)
{
  MexResizingHBoxChild *meta;

  g_return_if_fail (MEX_IS_RESIZING_HBOX (resizing_hbox));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  meta = _get_child_meta (resizing_hbox, child);

  meta->push = push;

  clutter_actor_queue_relayout (child);
}

