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


#ifndef _MEX_CLOCK_BIN_H
#define _MEX_CLOCK_BIN_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_CLOCK_BIN mex_clock_bin_get_type()

#define MEX_CLOCK_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_CLOCK_BIN, MexClockBin))

#define MEX_CLOCK_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_CLOCK_BIN, MexClockBinClass))

#define MEX_IS_CLOCK_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_CLOCK_BIN))

#define MEX_IS_CLOCK_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_CLOCK_BIN))

#define MEX_CLOCK_BIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_CLOCK_BIN, MexClockBinClass))

typedef struct _MexClockBin MexClockBin;
typedef struct _MexClockBinClass MexClockBinClass;
typedef struct _MexClockBinPrivate MexClockBinPrivate;

struct _MexClockBin
{
  MxBin parent;

  MexClockBinPrivate *priv;
};

struct _MexClockBinClass
{
  MxBinClass parent_class;
};

GType mex_clock_bin_get_type (void) G_GNUC_CONST;

ClutterActor *mex_clock_bin_new (void);

MxIcon *mex_clock_bin_get_icon (MexClockBin *bin);

G_END_DECLS

#endif /* _MEX_CLOCK_BIN_H */
