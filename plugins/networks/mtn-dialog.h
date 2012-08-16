/*
 * mex-networks - Connection Manager UI for Media Explorer
 * Copyright © 2010-2011, Intel Corporation.
 * Copyright © 2012, sleep(5) ltd.
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

#ifndef _MTN_DIALOG_H
#define _MTN_DIALOG_H

#include <mx/mx.h>

G_BEGIN_DECLS

#define MTN_TYPE_DIALOG                                                 \
   (mtn_dialog_get_type())
#define MTN_DIALOG(obj)                                                 \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                MTN_TYPE_DIALOG,                        \
                                MtnDialog))
#define MTN_DIALOG_CLASS(klass)                                         \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             MTN_TYPE_DIALOG,                           \
                             MtnDialogClass))
#define MTN_IS_DIALOG(obj)                                              \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                MTN_TYPE_DIALOG))
#define MTN_IS_DIALOG_CLASS(klass)                                      \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             MTN_TYPE_DIALOG))
#define MTN_DIALOG_GET_CLASS(obj)                                       \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               MTN_TYPE_DIALOG,                         \
                               MtnDialogClass))

typedef struct _MtnDialog        MtnDialog;
typedef struct _MtnDialogClass   MtnDialogClass;
typedef struct _MtnDialogPrivate MtnDialogPrivate;

struct _MtnDialogClass
{
  MxDialogClass parent_class;

  void (*close) (MtnDialog *self);
};

struct _MtnDialog
{
  MxDialog parent;

  /*<private>*/
  MtnDialogPrivate *priv;
};

GType mtn_dialog_get_type (void) G_GNUC_CONST;

ClutterActor *mtn_dialog_new (ClutterActor *transient_for);

G_END_DECLS

#endif /* _MTN_DIALOG_H */
