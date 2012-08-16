/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
 * Copyright © 2012, sleep(5) ltd.
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

#ifndef __MEX_INFO_BAR_H__
#define __MEX_INFO_BAR_H__

#include <glib-object.h>
#include <mx/mx.h>
#include <mex/mex-notification-area.h>
#include <mex/mex-clock-bin.h>
#include <mex/mex-utils.h>
#include <mex/mex-plugin-manager.h>

G_BEGIN_DECLS

#define MEX_TYPE_INFO_BAR mex_info_bar_get_type()

#define MEX_INFO_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_INFO_BAR, MexInfoBar))

#define MEX_INFO_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_INFO_BAR, MexInfoBarClass))

#define MEX_IS_INFO_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_INFO_BAR))

#define MEX_IS_INFO_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_INFO_BAR))

#define MEX_INFO_BAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_INFO_BAR, MexInfoBarClass))

typedef struct _MexInfoBar MexInfoBar;
typedef struct _MexInfoBarClass MexInfoBarClass;
typedef struct _MexInfoBarPrivate MexInfoBarPrivate;

struct _MexInfoBar
{
  MxWidget parent;

  MexInfoBarPrivate *priv;
};

struct _MexInfoBarClass
{
  MxWidgetClass parent_class;
};

void mex_info_bar_close_dialog (MexInfoBar *self);
void mex_info_bar_show_buttons (MexInfoBar *self, gboolean visible);
void mex_info_bar_show_notifications (MexInfoBar *self, gboolean visible);

gboolean mex_info_bar_notifications_visible (MexInfoBar *self);
gboolean mex_info_bar_dialog_visible (MexInfoBar *self);

void mex_info_bar_new_notification (MexInfoBar *self,
                                    const gchar *message,
                                    gint timeout);
void mex_info_bar_set_plugin_manager (MexInfoBar *self, MexPluginManager *mgr);

GType mex_info_bar_get_type (void) G_GNUC_CONST;

ClutterActor *mex_info_bar_get_default (void);

G_END_DECLS

#endif /* __MEX_INFO_BAR_H__ */
