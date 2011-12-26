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


#ifndef __MEX_MODEL_H__
#define __MEX_MODEL_H__

#include <glib-object.h>
#include <glib-controller/glib-controller.h>

#include <mex/mex-debug.h>

G_BEGIN_DECLS

#define MEX_TYPE_MODEL              (mex_model_get_type ())
#define MEX_MODEL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_MODEL, MexModel))
#define MEX_IS_MODEL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_MODEL))
#define MEX_MODEL_IFACE(iface)      (G_TYPE_CHECK_CLASS_CAST ((iface), MEX_TYPE_MODEL, MexModelIface))
#define MEX_IS_MODEL_IFACE(iface)   (G_TYPE_CHECK_CLASS_TYPE ((iface), MEX_TYPE_MODEL))
#define MEX_MODEL_GET_IFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MEX_TYPE_MODEL, MexModelIface))

typedef struct _MexModel      MexModel;
typedef struct _MexModelIface MexModelIface;

#include <mex/mex-content.h>

typedef gint (*MexModelSortFunc) (MexContent *a,
                                  MexContent *b,
                                  gpointer    userdata);
struct _MexModelIface
{
  GTypeInterface g_iface;

  /* virtual functions */
  GController *   (*get_controller)   (MexModel *model);
  MexContent *    (*get_content)      (MexModel *model,
                                       guint     index_);
  void (*add_content) (MexModel   *model,
                       MexContent *content);
  void (*add) (MexModel *model,
               GList    *content_list);
  void (*remove_content) (MexModel   *model,
                          MexContent *content);
  void (*clear) (MexModel *model);
  void (*set_sort_func) (MexModel         *model,
                         MexModelSortFunc  sort_func,
                         gpointer          user_data);
  gboolean (*is_sorted) (MexModel *model);
  guint (*get_length) (MexModel *model);
  gint  (*index)      (MexModel *model, MexContent *content);

  MexModel *(*get_model) (MexModel *model);
};

GType         mex_model_get_type         (void) G_GNUC_CONST;

GController * mex_model_get_controller   (MexModel *model);
MexContent *  mex_model_get_content      (MexModel *model,
                                          guint     index_);
void mex_model_add (MexModel *model,
                    GList    *content);
void mex_model_add_content (MexModel   *model,
                            MexContent *content);
void mex_model_remove_content (MexModel   *model,
                               MexContent *content);
void mex_model_clear (MexModel *model);
void mex_model_set_sort_func (MexModel         *model,
                              MexModelSortFunc  sort_func,
                              gpointer          user_data);
gboolean mex_model_is_sorted (MexModel *model);
guint mex_model_get_length (MexModel *model);
gint  mex_model_index      (MexModel   *model,
                            MexContent *content);

MexModel *mex_model_get_model (MexModel *model);
gchar * mex_model_to_string (MexModel          *model,
                             MexDebugVerbosity  verbosity);

G_END_DECLS

#endif /* __MEX_MODEL_H__ */
