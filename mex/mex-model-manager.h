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


#ifndef _MEX_MODEL_MANAGER_H
#define _MEX_MODEL_MANAGER_H

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

typedef struct
{
  MexModel *model;
  gchar    *category;
  gint      priority;
  GList    *sort_infos;
  gint      default_sort_index;
  MexModel *alt_model;
  gchar    *alt_model_string;
  guint     alt_model_active;
  gpointer  userdata;
} MexModelInfo;

typedef struct
{
  gchar *name;
  gchar *display_name;
  gchar *icon_name;
  gint   priority;
  gchar *placeholder_text;
  gboolean show_length;
} MexModelCategoryInfo;

struct _MexModelManager
{
  GObject parent;

  MexModelManagerPrivate *priv;
};

struct _MexModelManagerClass
{
  GObjectClass parent_class;

  void (* model_added)   (MexModelManager    *manager,
                          const MexModelInfo *info);
  void (* model_removed) (MexModelManager *manager,
                          MexModel        *model,
                          const gchar     *category);

  void (* categories_changed) (MexModelManager *manager);
};

GType mex_model_manager_get_type (void) G_GNUC_CONST;

MexModelManager *mex_model_manager_get_default ();

GList *mex_model_manager_get_models (MexModelManager *manager);

GList *mex_model_manager_get_models_for_category (MexModelManager *manager,
                                                  const gchar     *category);

void mex_model_manager_add_model    (MexModelManager    *manager,
                                     const MexModelInfo *info);
void mex_model_manager_remove_model (MexModelManager *manager,
                                     MexModel        *model);

const MexModelInfo *mex_model_manager_get_model_info (MexModelManager *manager,
                                                      MexModel        *model);

void mex_model_manager_add_category (MexModelManager            *manager,
                                     const MexModelCategoryInfo *info);

void mex_model_manager_remove_category (MexModelManager *manager,
                                        const gchar     *name);

GList *mex_model_manager_get_categories (MexModelManager *manager);

const MexModelCategoryInfo *mex_model_manager_get_category_info (MexModelManager *manager,
                                                                 const gchar     *name);

MexModelInfo *mex_model_info_new (MexModel         *model,
                                  const gchar      *category,
                                  gint              priority,
                                  const gchar      *first_sort_func_name,
                                  ...);

MexModelInfo *mex_model_info_copy (const MexModelInfo *info);

void mex_model_info_free (MexModelInfo *info);

G_END_DECLS

#endif /* _MEX_MODEL_MANAGER_H */
