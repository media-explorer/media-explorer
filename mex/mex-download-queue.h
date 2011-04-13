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

#ifndef __MEX_DOWNLOAD_QUEUE_H__
#define __MEX_DOWNLOAD_QUEUE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_DOWNLOAD_QUEUE                                         \
   (mex_download_queue_get_type())
#define MEX_DOWNLOAD_QUEUE(obj)                                         \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                MEX_TYPE_DOWNLOAD_QUEUE,                \
                                MexDownloadQueue))
#define MEX_DOWNLOAD_QUEUE_CLASS(klass)                                 \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             MEX_TYPE_DOWNLOAD_QUEUE,                   \
                             MexDownloadQueueClass))
#define MEX_IS_DOWNLOAD_QUEUE(obj)                                      \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                MEX_TYPE_DOWNLOAD_QUEUE))
#define MEX_IS_DOWNLOAD_QUEUE_CLASS(klass)                              \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             MEX_TYPE_DOWNLOAD_QUEUE))
#define MEX_DOWNLOAD_QUEUE_GET_CLASS(obj)                               \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               MEX_TYPE_DOWNLOAD_QUEUE,                 \
                               MexDownloadQueueClass))

typedef struct _MexDownloadQueuePrivate MexDownloadQueuePrivate;
typedef struct _MexDownloadQueue      MexDownloadQueue;
typedef struct _MexDownloadQueueClass MexDownloadQueueClass;

typedef void (*MexDownloadQueueCompletedReply) (MexDownloadQueue *queue,
                                                const char       *uri,
                                                const char       *buffer,
                                                gsize             count,
                                                const GError     *error,
                                                gpointer          userdata);
struct _MexDownloadQueue
{
    GObject parent;

    MexDownloadQueuePrivate *priv;
};

struct _MexDownloadQueueClass
{
    GObjectClass parent_class;
};

GType mex_download_queue_get_type (void) G_GNUC_CONST;

MexDownloadQueue *mex_download_queue_get_default (void);
gpointer mex_download_queue_enqueue (MexDownloadQueue               *queue,
                                     const char                     *uri,
                                     MexDownloadQueueCompletedReply  reply,
                                     gpointer                        userdata);

void mex_download_queue_cancel (MexDownloadQueue *queue,
                                gpointer          id);

void mex_download_queue_set_throttle  (MexDownloadQueue *queue,
                                       guint             throttle);
guint mex_download_queue_get_throttle (MexDownloadQueue *queue);

guint mex_download_queue_get_queue_length (MexDownloadQueue *queue);

G_END_DECLS

#endif /* __MEX_DOWNLOAD_QUEUE_H__ */
