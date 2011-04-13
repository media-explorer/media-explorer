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
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St 
 * - Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef _MTN_SERVICES_VIEW
#define _MTN_SERVICES_VIEW

#include <glib-object.h>
#include <mx/mx.h>

#include "mtn-connman.h"

G_BEGIN_DECLS

#define MTN_TYPE_SERVICES_VIEW mtn_services_view_get_type()

#define MTN_SERVICES_VIEW(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTN_TYPE_SERVICES_VIEW, MtnServicesView))

#define MTN_SERVICES_VIEW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), MTN_TYPE_SERVICES_VIEW, MtnServicesViewClass))

#define MTN_IS_SERVICES_VIEW(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTN_TYPE_SERVICES_VIEW))

#define MTN_IS_SERVICES_VIEW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), MTN_TYPE_SERVICES_VIEW))

#define MTN_SERVICES_VIEW_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), MTN_TYPE_SERVICES_VIEW, MtnServicesViewClass))

typedef struct _MtnServicesViewPrivate MtnServicesViewPrivate;

typedef struct {
    MxBoxLayout parent;

    MtnServicesViewPrivate *priv;    
} MtnServicesView;

typedef struct {
    MxBoxLayoutClass parent_class;

    void (* connection_requested) (MtnServicesView *view, const char *object_path);
} MtnServicesViewClass;

GType           mtn_services_view_get_type (void);

void            mtn_services_view_set_connman (MtnServicesView *view, MtnConnman *connman);

ClutterActor*   mtn_services_view_new      (void);

G_END_DECLS

#endif /* _MTN_SERVICES_VIEW */
