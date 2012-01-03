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

#ifndef __MEX_CONTENT_VIEW_H__
#define __MEX_CONTENT_VIEW_H__

#include <glib-object.h>

#include <mex/mex-content.h>
#include <mex/mex-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_CONTENT_VIEW (mex_content_view_get_type ())
#define MEX_CONTENT_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_CONTENT_VIEW, MexContentView))
#define MEX_IS_CONTENT_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_CONTENT_VIEW))
#define MEX_CONTENT_VIEW_IFACE(iface) (G_TYPE_CHECK_CLASS_CAST ((iface), MEX_TYPE_CONTENT_VIEW, MexContentViewIface))
#define MEX_IS_CONTENT_VIEW_IFACE(iface) (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_CONTENT_VIEW))
#define MEX_CONTENT_VIEW_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MEX_TYPE_CONTENT_VIEW, MexContentViewIface))

typedef struct _MexContentView MexContentView;
typedef struct _MexContentViewIface MexContentViewIface;

struct _MexContentViewIface
{
    GTypeInterface g_iface;

    void        (*set_content) (MexContentView *view,
                                MexContent     *content);
    MexContent* (*get_content) (MexContentView *view);

    void        (*set_context) (MexContentView *view,
                                MexModel       *content);
    MexModel*   (*get_context) (MexContentView *view);
};

GType mex_content_view_get_type (void) G_GNUC_CONST;

void        mex_content_view_set_content (MexContentView *view,
                                          MexContent     *content);
MexContent* mex_content_view_get_content (MexContentView *view);
void        mex_content_view_set_context (MexContentView *view,
                                          MexModel       *model);
MexModel*   mex_content_view_get_context (MexContentView *view);
G_END_DECLS

#endif /* __MEX_CONTENT_VIEW_H__ */
