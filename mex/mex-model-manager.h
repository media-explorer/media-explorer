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

#ifndef __MEX_MODEL_MANAGER_H__
#define __MEX_MODEL_MANAGER_H__

#include <glib-object.h>
#include <mex/mex-model.h>

G_BEGIN_DECLS

#define MEX_TYPE_MODEL_MANAGER mex_model_manager_get_type()

#define MEX_MODEL_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_MODEL_MANAGER, MexModelManager))

#define MEX_MODEL_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_MODEL_MANAGER, MexModelManagerClass))

#define MEX_IS_MODEL_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_MODEL_MANAGER))

#define MEX_IS_MODEL_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_MODEL_MANAGER))

#define MEX_MODEL_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_MODEL_MANAGER, MexModelManagerClass))

typedef struct _MexModelManager MexModelManager;
typedef struct _MexModelManagerClass MexModelManagerClass;
typedef struct _MexModelManagerPrivate MexModelManagerPrivate;

typedef struct
{
  gchar            *name;
  gchar            *display_name;
  MexModelSortFunc  sort_func;
  gpointer          userdata;
} MexModelSortFuncInfo;

void mex_model_sort_func_info_free (MexModelSortFuncInfo *sort_info);
MexModelSortFuncInfo * mex_model_sort_func_info_new (const gchar      *name,
                                                     const gchar      *display_name,
                                                     MexModelSortFunc  func,
                                                     gpointer          user_data);

typedef struct
{
  gchar            *name;
  gchar            *display_name;
  gchar            *icon_name;
  gint              priority;
  gchar            *placeholder_text;
  gint              primary_group_by_key;
  gint              secondary_group_by_key;

  gboolean          show_length;

  MexModelSortFunc  sort_func;
  gpointer          userdata;
} MexModelCategoryInfo;

struct _MexModelManager
{
  GObject parent;

  MexModelManagerPrivate *priv;
};

struct _MexModelManagerClass
{
  GObjectClass parent_class;

  void (* model_added)   (MexModelManager    *manager);
  void (* model_removed) (MexModelManager *manager,
                          MexModel        *model);

  void (* categories_changed) (MexModelManager *manager);
};

GType mex_model_manager_get_type (void) G_GNUC_CONST;

MexModelManager *mex_model_manager_get_default (void);

GList *mex_model_manager_get_models (MexModelManager *manager);

GList *mex_model_manager_get_models_for_category (MexModelManager *manager,
                                                  const gchar     *category);

void mex_model_manager_add_model    (MexModelManager *manager,
                                     MexModel        *model);
void mex_model_manager_remove_model (MexModelManager *manager,
                                     MexModel        *model);

void mex_model_manager_add_category (MexModelManager            *manager,
                                     const MexModelCategoryInfo *info);

void mex_model_manager_remove_category (MexModelManager *manager,
                                        const gchar     *name);

GList *mex_model_manager_get_categories (MexModelManager *manager);

const MexModelCategoryInfo *mex_model_manager_get_category_info (MexModelManager *manager,
                                                                 const gchar     *name);

MexModel* mex_model_manager_get_root_model (MexModelManager *manager);
MexModel* mex_model_manager_get_model_for_category (MexModelManager *manager,
                                                    const gchar     *category);

G_END_DECLS

#endif /* __MEX_MODEL_MANAGER_H__ */
