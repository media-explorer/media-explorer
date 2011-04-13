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


#ifndef _MEX_EXPANDER_BOX_H
#define _MEX_EXPANDER_BOX_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_EXPANDER_BOX mex_expander_box_get_type()

#define MEX_EXPANDER_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_EXPANDER_BOX, MexExpanderBox))

#define MEX_EXPANDER_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_EXPANDER_BOX, MexExpanderBoxClass))

#define MEX_IS_EXPANDER_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_EXPANDER_BOX))

#define MEX_IS_EXPANDER_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_EXPANDER_BOX))

#define MEX_EXPANDER_BOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_EXPANDER_BOX, MexExpanderBoxClass))

typedef struct _MexExpanderBox MexExpanderBox;
typedef struct _MexExpanderBoxClass MexExpanderBoxClass;
typedef struct _MexExpanderBoxPrivate MexExpanderBoxPrivate;

struct _MexExpanderBox
{
  MxWidget parent;

  MexExpanderBoxPrivate *priv;
};

struct _MexExpanderBoxClass
{
  MxWidgetClass parent_class;
};

typedef enum
{
  MEX_EXPANDER_BOX_UP,
  MEX_EXPANDER_BOX_RIGHT,
  MEX_EXPANDER_BOX_DOWN,
  MEX_EXPANDER_BOX_LEFT
} MexExpanderBoxDirection;

GType mex_expander_box_get_type (void) G_GNUC_CONST;

ClutterActor *mex_expander_box_new (void);

void mex_expander_box_set_primary_child (MexExpanderBox *box,
                                         ClutterActor   *actor);
void mex_expander_box_set_secondary_child (MexExpanderBox *box,
                                           ClutterActor   *actor);

ClutterActor *mex_expander_box_get_primary_child (MexExpanderBox *box);
ClutterActor *mex_expander_box_get_secondary_child (MexExpanderBox *box);

void mex_expander_box_set_grow_direction (MexExpanderBox          *box,
                                          MexExpanderBoxDirection  direction);
MexExpanderBoxDirection
     mex_expander_box_get_grow_direction (MexExpanderBox          *box);

void mex_expander_box_set_important (MexExpanderBox *box,
                                     gboolean        important);
gboolean mex_expander_box_get_important (MexExpanderBox *box);

void mex_expander_box_set_important_on_focus (MexExpanderBox *box,
                                              gboolean       important_on_focus);
gboolean mex_expander_box_get_important_on_focus (MexExpanderBox *box);

void mex_expander_box_set_open (MexExpanderBox *box,
                                gboolean        open);
gboolean mex_expander_box_get_open (MexExpanderBox *box);

void mex_expander_box_set_open_on_focus (MexExpanderBox *box,
                                         gboolean        open);
gboolean mex_expander_box_get_open_on_focus (MexExpanderBox *box);

void mex_expander_box_set_close_on_unfocus (MexExpanderBox *box,
                                            gboolean        close_on_unfocus);
gboolean mex_expander_box_get_close_on_unfocus (MexExpanderBox *box);

void mex_expander_box_set_expand (MexExpanderBox *box,
                                  gboolean        expand);
gboolean mex_expander_box_get_expand (MexExpanderBox *box);

void mex_expander_box_set_max_size (MexExpanderBox *box,
                                    gboolean        primary,
                                    gfloat          max_width,
                                    gfloat          max_height);
void mex_expander_box_get_max_size (MexExpanderBox *box,
                                    gboolean        primary,
                                    gfloat         *max_width,
                                    gfloat         *max_height);

G_END_DECLS

#endif /* _MEX_EXPANDER_BOX_H */
