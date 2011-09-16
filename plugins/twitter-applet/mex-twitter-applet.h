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


#ifndef _MEX_TWITTER_APPLET
#define _MEX_TWITTER_APPLET

#include <glib-object.h>
#include <mex/mex-applet.h>

G_BEGIN_DECLS

#define MEX_TYPE_TWITTER_APPLET mex_twitter_applet_get_type()

#define MEX_TWITTER_APPLET(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_TWITTER_APPLET, MexTwitterApplet))

#define MEX_TWITTER_APPLET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_TWITTER_APPLET, MexTwitterAppletClass))

#define MEX_IS_TWITTER_APPLET(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_TWITTER_APPLET))

#define MEX_IS_TWITTER_APPLET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_TWITTER_APPLET))

#define MEX_TWITTER_APPLET_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_TWITTER_APPLET, MexTwitterAppletClass))

typedef struct {
    MexApplet parent;
} MexTwitterApplet;

typedef struct {
    MexAppletClass parent_class;
} MexTwitterAppletClass;

GType mex_twitter_applet_get_type (void);

void mex_twitter_applet_share (MexTwitterApplet *applet, MexContent *content);

G_END_DECLS

#endif /* _MEX_TWITTER_APPLET */
