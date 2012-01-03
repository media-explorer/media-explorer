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


#ifndef __MEX_SCROLL_INDICATOR_H__
#define __MEX_SCROLL_INDICATOR_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_SCROLL_INDICATOR mex_scroll_indicator_get_type()

#define MEX_SCROLL_INDICATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_SCROLL_INDICATOR, MexScrollIndicator))

#define MEX_SCROLL_INDICATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_SCROLL_INDICATOR, MexScrollIndicatorClass))

#define MEX_IS_SCROLL_INDICATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_SCROLL_INDICATOR))

#define MEX_IS_SCROLL_INDICATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_SCROLL_INDICATOR))

#define MEX_SCROLL_INDICATOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_SCROLL_INDICATOR, MexScrollIndicatorClass))

typedef struct _MexScrollIndicator MexScrollIndicator;
typedef struct _MexScrollIndicatorClass MexScrollIndicatorClass;
typedef struct _MexScrollIndicatorPrivate MexScrollIndicatorPrivate;

struct _MexScrollIndicator
{
  MxWidget parent;

  MexScrollIndicatorPrivate *priv;
};

struct _MexScrollIndicatorClass
{
  MxWidgetClass parent_class;
};

GType mex_scroll_indicator_get_type (void) G_GNUC_CONST;

ClutterActor *mex_scroll_indicator_new (void);

void mex_scroll_indicator_set_adjustment (MexScrollIndicator *scroll,
                                          MxAdjustment       *adjustment);
MxAdjustment *mex_scroll_indicator_get_adjustment (MexScrollIndicator *scroll);

G_END_DECLS

#endif /* __MEX_SCROLL_INDICATOR_H__ */
