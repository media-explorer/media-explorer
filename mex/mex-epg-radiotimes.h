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


#ifndef __MEX_EPG_RADIOTIMES_H__
#define __MEX_EPG_RADIOTIMES_H__

#include <glib-object.h>
#include <mex/mex-epg-provider.h>

G_BEGIN_DECLS

#define MEX_TYPE_EPG_RADIOTIMES mex_epg_radiotimes_get_type()

#define MEX_EPG_RADIOTIMES(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               MEX_TYPE_EPG_RADIOTIMES, \
                               MexEpgRadiotimes))

#define MEX_EPG_RADIOTIMES_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                  \
                            MEX_TYPE_EPG_RADIOTIMES,  \
                            MexEpgRadiotimesClass))

#define MEX_IS_EPG_RADIOTIMES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_EPG_RADIOTIMES))

#define MEX_IS_EPG_RADIOTIMES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_EPG_RADIOTIMES))

#define MEX_EPG_RADIOTIMES_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              MEX_TYPE_EPG_RADIOTIMES,  \
                              MexEpgRadiotimesClass))

typedef struct _MexEpgRadiotimes MexEpgRadiotimes;
typedef struct _MexEpgRadiotimesClass MexEpgRadiotimesClass;
typedef struct _MexEpgRadiotimesPrivate MexEpgRadiotimesPrivate;

struct _MexEpgRadiotimes
{
  GObject parent;

  MexEpgRadiotimesPrivate *priv;
};

struct _MexEpgRadiotimesClass
{
  GObjectClass parent_class;
};

GType             mex_epg_radiotimes_get_type        (void) G_GNUC_CONST;

MexEpgProvider *  mex_epg_radiotimes_new             (void);
GHashTable *      mex_epg_radiotimes_get_channel_ids (MexEpgRadiotimes *self);

G_END_DECLS

#endif /* __MEX_EPG_RADIOTIMES_H__ */
