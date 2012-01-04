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

#ifndef __MEX_SCENE_H__
#define __MEX_SCENE_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_SCENE              (mex_scene_get_type ())
#define MEX_SCENE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_SCENE, MexScene))
#define MEX_IS_SCENE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_SCENE))
#define MEX_SCENE_IFACE(iface)      (G_TYPE_CHECK_CLASS_CAST ((iface), MEX_TYPE_SCENE, MexSceneInterface))
#define MEX_IS_SCENE_IFACE(iface)   (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_SCENE))
#define MEX_SCENE_GET_IFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MEX_TYPE_SCENE, MexSceneInterface))

typedef struct _MexScene MexScene;
typedef struct _MexSceneInterface MexSceneInterface;

struct _MexSceneInterface
{
  GTypeInterface g_iface;

  /* virtual functions */
  void (*open) (MexScene              *scene,
                const ClutterActorBox *origin,
                ClutterCallback        callback,
                gpointer               data);

  void (*close) (MexScene              *scene,
                 const ClutterActorBox *target,
                 ClutterCallback        callback,
                 gpointer               data);

  void (*get_current_target) (MexScene        *scene,
                              ClutterActorBox *box);
};


GType mex_scene_get_type (void) G_GNUC_CONST;

void mex_scene_open (MexScene              *scene,
                     const ClutterActorBox *origin,
                     ClutterCallback        callback,
                     gpointer               data);
void mex_scene_close (MexScene              *scene,
                      const ClutterActorBox *target,
                      ClutterCallback        callback,
                      gpointer               data);

void mex_scene_get_current_target (MexScene        *scene,
                                   ClutterActorBox *box);

G_END_DECLS

#endif /* __MEX_SCENE_H__ */
