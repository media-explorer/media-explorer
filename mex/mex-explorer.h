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


#ifndef __MEX_EXPLORER_H__
#define __MEX_EXPLORER_H__

#include <glib-object.h>
#include <clutter/clutter.h>
#include <mx/mx.h>

#include <mex/mex-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_EXPLORER mex_explorer_get_type()

#define MEX_EXPLORER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_EXPLORER, MexExplorer))

#define MEX_EXPLORER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_EXPLORER, MexExplorerClass))

#define MEX_IS_EXPLORER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_EXPLORER))

#define MEX_IS_EXPLORER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_EXPLORER))

#define MEX_EXPLORER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_EXPLORER, MexExplorerClass))

typedef struct _MexExplorer MexExplorer;
typedef struct _MexExplorerClass MexExplorerClass;
typedef struct _MexExplorerPrivate MexExplorerPrivate;

struct _MexExplorer
{
  MxStack parent;

  MexExplorerPrivate *priv;
};

struct _MexExplorerClass
{
  MxStackClass parent_class;

  void (*page_created)      (MexExplorer      *explorer,
                             MexModel         *model,
                             ClutterActor    **page);
  void (*object_created)    (MexExplorer      *explorer,
                             MexContent       *content,
                             GObject          *object,
                             ClutterContainer *container);
  void (*header_activated)  (MexExplorer      *explorer,
                             MexModel         *model);
};

GType mex_explorer_get_type (void) G_GNUC_CONST;

ClutterActor *mex_explorer_new (void);

void mex_explorer_set_root_model (MexExplorer *explorer, MexModel *model);
MexModel *mex_explorer_get_root_model (MexExplorer *explorer);

void mex_explorer_push_model (MexExplorer *explorer,
                              MexModel    *model);
void mex_explorer_replace_model (MexExplorer *explorer,
                                 MexModel    *model);
void mex_explorer_pop_model (MexExplorer *explorer);
void mex_explorer_pop_to_root (MexExplorer *explorer);

GList *mex_explorer_get_models (MexExplorer *explorer);

void mex_explorer_remove_model (MexExplorer *explorer,
                                MexModel    *model);

guint mex_explorer_get_depth (MexExplorer *explorer);

MexModel *mex_explorer_get_model (MexExplorer *explorer);
MexModel *mex_explorer_get_focused_model (MexExplorer *explorer);
void      mex_explorer_set_focused_model (MexExplorer *explorer,
                                          MexModel    *model);

void mex_explorer_focus_content (MexExplorer       *explorer,
                                 const MexContent  *content);

ClutterContainer *mex_explorer_get_container_for_model (MexExplorer *explorer,
                                                        MexModel    *model);

void mex_explorer_set_n_preview_items (MexExplorer *explorer,
                                       gint         n_preview_items);
gint mex_explorer_get_n_preview_items (MexExplorer *explorer);

void     mex_explorer_set_touch_mode (MexExplorer *explorer,
                                      gboolean     touch_mode_on);
gboolean mex_explorer_get_touch_mode (MexExplorer *explorer);

G_END_DECLS

#endif /* __MEX_EXPLORER_H__ */
