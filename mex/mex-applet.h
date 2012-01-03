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


#ifndef __MEX_APPLET_H__
#define __MEX_APPLET_H__

#include <glib-object.h>
#include <mex/mex-generic-content.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MEX_TYPE_APPLET mex_applet_get_type()

#define MEX_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_APPLET, MexApplet))

#define MEX_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_APPLET, MexAppletClass))

#define MEX_IS_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_APPLET))

#define MEX_IS_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_APPLET))

#define MEX_APPLET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_APPLET, MexAppletClass))

typedef enum
{
  MEX_APPLET_PRESENT_NONE   = 0,
  MEX_APPLET_PRESENT_OPAQUE = 1 << 0,
  MEX_APPLET_PRESENT_DIALOG = 2 << 1,
} MexAppletPresentationFlags;

typedef struct {
  MexGenericContent parent;
} MexApplet;

typedef struct {
  MexGenericContentClass parent_class;

  /* vfuncs */
  const gchar *(*get_id) (MexApplet *applet);
  const gchar *(*get_name) (MexApplet *applet);
  const gchar *(*get_description) (MexApplet *applet);
  const gchar *(*get_thumbnail) (MexApplet *applet);

  /* signals */
  void (*activated) (MexApplet *applet);
  void (*request_close) (MexApplet *applet, ClutterActor *actor);
  void (*closed) (MexApplet *applet, ClutterActor *actor);
  void (*present_actor) (MexApplet *applet,
                         MexAppletPresentationFlags flags,
                         ClutterActor *actor);
} MexAppletClass;

GType mex_applet_get_type (void);

const gchar *mex_applet_get_id (MexApplet *applet);
const gchar *mex_applet_get_name (MexApplet *applet);
const gchar *mex_applet_get_description (MexApplet *applet);
const gchar *mex_applet_get_thumbnail (MexApplet *applet);

void mex_applet_present_actor (MexApplet *applet,
                               MexAppletPresentationFlags flags,
                               ClutterActor *actor);

void mex_applet_activate (MexApplet *applet);
void mex_applet_request_close (MexApplet *applet, ClutterActor *actor);
void mex_applet_closed (MexApplet *applet, ClutterActor *actor);
G_END_DECLS

#endif /* __MEX_APPLET_H__ */
