/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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



#include "mex-scene.h"

G_DEFINE_INTERFACE (MexScene, mex_scene, CLUTTER_TYPE_ACTOR)

static void
mex_scene_default_init (MexSceneInterface *interface)
{
}


void
mex_scene_open (MexScene        *scene,
                ClutterCallback  callback,
                gpointer         data)
{
  MexSceneInterface *iface;

  g_return_if_fail (MEX_IS_SCENE (scene));

  iface = MEX_SCENE_GET_IFACE (scene);

  if (iface->open)
    {
      iface->open (scene, callback, data);
      return;
    }

  g_warning ("MexScene of type '%s' does not implement open()",
             g_type_name (G_OBJECT_TYPE (scene)));
}

void
mex_scene_close (MexScene        *scene,
                 ClutterCallback  callback,
                 gpointer         data)
{
  MexSceneInterface *iface;

  g_return_if_fail (MEX_IS_SCENE (scene));

  iface = MEX_SCENE_GET_IFACE (scene);

  if (iface->close)
    {
      iface->close (scene, callback, data);
      return;
    }

  g_warning ("MexScene of type '%s' does not implement close()",
             g_type_name (G_OBJECT_TYPE (scene)));
}
