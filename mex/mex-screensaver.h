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


#ifndef __MEX_SCREENSAVER_H__
#define __MEX_SCREENSAVER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_SCREENSAVER mex_screensaver_get_type()

#define MEX_SCREENSAVER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_SCREENSAVER, MexScreensaver))

#define MEX_SCREENSAVER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_SCREENSAVER, MexScreensaverClass))

#define MEX_IS_SCREENSAVER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_SCREENSAVER))

#define MEX_IS_SCREENSAVER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_SCREENSAVER))

#define MEX_SCREENSAVER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_SCREENSAVER, MexScreensaverClass))

typedef struct _MexScreensaver MexScreensaver;
typedef struct _MexScreensaverClass MexScreensaverClass;
typedef struct _MexScreensaverPrivate MexScreensaverPrivate;

struct _MexScreensaver
{
  GObject parent;

  MexScreensaverPrivate *priv;
};

struct _MexScreensaverClass
{
  GObjectClass parent_class;
};

GType           mex_screensaver_get_type         (void) G_GNUC_CONST;

MexScreensaver *mex_screensaver_new (void);

void mex_screensaver_inhibit (MexScreensaver *self);
void mex_screensaver_uninhibit (MexScreensaver *self);

G_END_DECLS

#endif /* __MEX_SCREENSAVER_H__ */
