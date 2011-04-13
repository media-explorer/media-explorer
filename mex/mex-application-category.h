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


#ifndef __MEX_APPLICATION_CATEGORY_H__
#define __MEX_APPLICATION_CATEGORY_H__

#include <glib-object.h>

#include <mex/mex-application.h>

G_BEGIN_DECLS

#define MEX_TYPE_APPLICATION_CATEGORY mex_application_category_get_type()

#define MEX_APPLICATION_CATEGORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_APPLICATION_CATEGORY, MexApplicationCategory))

#define MEX_APPLICATION_CATEGORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_APPLICATION_CATEGORY, MexApplicationCategoryClass))

#define MEX_IS_APPLICATION_CATEGORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_APPLICATION_CATEGORY))

#define MEX_IS_APPLICATION_CATEGORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_APPLICATION_CATEGORY))

#define MEX_APPLICATION_CATEGORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_APPLICATION_CATEGORY, MexApplicationCategoryClass))

typedef struct _MexApplicationCategory MexApplicationCategory;
typedef struct _MexApplicationCategoryClass MexApplicationCategoryClass;
typedef struct _MexApplicationCategoryPrivate MexApplicationCategoryPrivate;

struct _MexApplicationCategory
{
  GObject parent;

  MexApplicationCategoryPrivate *priv;
};

struct _MexApplicationCategoryClass
{
  GObjectClass parent_class;
};

GType mex_application_category_get_type (void) G_GNUC_CONST;

MexApplicationCategory *mex_application_category_new        (const gchar *name);

const gchar *     mex_application_category_get_name         (MexApplicationCategory *category);
void              mex_application_category_set_name         (MexApplicationCategory *category,
                                                             const gchar            *name);

const GPtrArray * mex_application_category_get_items        (MexApplicationCategory *category);
void              mex_application_category_set_items        (MexApplicationCategory *category,
                                                             GPtrArray              *items);

void              mex_application_category_add_category     (MexApplicationCategory *category,
                                                             MexApplicationCategory *to_add);
void              mex_application_category_add_application  (MexApplicationCategory *category,
                                                             MexApplication         *application);
G_END_DECLS

#endif /* __MEX_APPLICATION_CATEGORY_H__ */
