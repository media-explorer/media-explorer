/*
 * mex-networks - Connection Manager UI for Media Explorer
 * Copyright © 2010-2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifndef _MTN_APP
#define _MTN_APP

#include <mx/mx.h>

G_BEGIN_DECLS

#define MTN_TYPE_APP mtn_app_get_type()

#define MTN_APP(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTN_TYPE_APP, MtnApp))

#define MTN_APP_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), MTN_TYPE_APP, MtnAppClass))

#define MTN_IS_APP(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTN_TYPE_APP))

#define MTN_IS_APP_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), MTN_TYPE_APP))

#define MTN_APP_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), MTN_TYPE_APP, MtnAppClass))

typedef struct _MtnAppPrivate MtnAppPrivate;

typedef struct {
    MxApplication parent;

    MtnAppPrivate *priv;
} MtnApp;

typedef struct {
    MxApplicationClass parent_class;
} MtnAppClass;

GType mtn_app_get_type (void);

MtnApp* mtn_app_new (gint *argc, gchar ***argv);

G_END_DECLS

#endif /* _MTN_APP */
