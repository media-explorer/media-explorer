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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-content-view.h"

static void
mex_content_view_base_finalize (gpointer g_iface)
{
}

static void
mex_content_view_base_init (gpointer g_iface)
{
}

GType
mex_content_view_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0)) {
    GTypeInfo content_info = {
      sizeof (MexContentViewIface),
      mex_content_view_base_init,
      mex_content_view_base_finalize
    };

    our_type = g_type_register_static (G_TYPE_INTERFACE,
                                       "MexContentView",
                                       &content_info, 0);
  }

  return our_type;
}

/**
 * mex_content_view_set_content:
 * @view: a #MexContentView
 * @content: a #MexContent
 *
 * Adds @content to @view
 */
void
mex_content_view_set_content (MexContentView *view,
                              MexContent     *content)
{
  MexContentViewIface *iface;

  g_return_if_fail (MEX_IS_CONTENT_VIEW (view));
  g_return_if_fail (MEX_IS_CONTENT (content));

  iface = MEX_CONTENT_VIEW_GET_IFACE (view);

  if (G_LIKELY (iface->set_content)) {
    iface->set_content (view, content);
    return;
  }

  g_warning ("MexContentView of type '%s' does not implement set_content()",
             g_type_name (G_OBJECT_TYPE (view)));
}

/**
 * mex_content_view_get_content:
 * @view: a #MexContentView
 *
 * Retrieves the content associated with @view
 *
 * Return value: a #MexContent
 */
MexContent *
mex_content_view_get_content (MexContentView *view)
{
  MexContentViewIface *iface;

  g_return_val_if_fail (MEX_IS_CONTENT_VIEW (view), NULL);

  iface = MEX_CONTENT_VIEW_GET_IFACE (view);

  if (G_LIKELY (iface->get_content)) {
    return iface->get_content (view);
  }

  g_warning ("MexContentView of type '%s' does not implement get_content()",
             g_type_name (G_OBJECT_TYPE (view)));
  return NULL;
}

void
mex_content_view_set_context (MexContentView *view,
                              MexModel       *context)
{
  MexContentViewIface *iface;

  g_return_if_fail (MEX_IS_CONTENT_VIEW (view));
  g_return_if_fail (MEX_IS_MODEL (context));

  iface = MEX_CONTENT_VIEW_GET_IFACE (view);

  if (G_LIKELY (iface->set_context)) {
    iface->set_context (view, context);
    return;
  }

  g_warning ("MexContentView of type '%s' does not implement set_context()",
             g_type_name (G_OBJECT_TYPE (view)));
}

MexModel*
mex_content_view_get_context (MexContentView *view)
{
  MexContentViewIface *iface;

  g_return_val_if_fail (MEX_IS_CONTENT_VIEW (view), NULL);

  iface = MEX_CONTENT_VIEW_GET_IFACE (view);

  if (G_LIKELY (iface->get_context)) {
    return iface->get_context (view);
  }

  g_warning ("MexContentView of type '%s' does not implement get_context()",
             g_type_name (G_OBJECT_TYPE (view)));
  return NULL;
}
