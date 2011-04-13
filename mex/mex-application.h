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


#ifndef __MEX_APPLICATION_H__
#define __MEX_APPLICATION_H__

#include <glib-object.h>
#include <mex/mex-generic-content.h>

G_BEGIN_DECLS

#define MEX_TYPE_APPLICATION mex_application_get_type()

#define MEX_APPLICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEX_TYPE_APPLICATION, MexApplication))

#define MEX_APPLICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEX_TYPE_APPLICATION, MexApplicationClass))

#define MEX_IS_APPLICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEX_TYPE_APPLICATION))

#define MEX_IS_APPLICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEX_TYPE_APPLICATION))

#define MEX_APPLICATION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEX_TYPE_APPLICATION, MexApplicationClass))

typedef struct _MexApplication MexApplication;
typedef struct _MexApplicationClass MexApplicationClass;
typedef struct _MexApplicationPrivate MexApplicationPrivate;

struct _MexApplication
{
  MexGenericContent parent;

  MexApplicationPrivate *priv;
};

struct _MexApplicationClass
{
  MexGenericContentClass parent_class;
};

GType mex_application_get_type (void) G_GNUC_CONST;

MexApplication * mex_application_new (void);

const gchar * mex_application_get_name (MexApplication *self);
void          mex_application_set_name (MexApplication *self,
                                        const gchar     *name);

const gchar * mex_application_get_executable (MexApplication *self);
void          mex_application_set_executable (MexApplication *self,
                                              const gchar     *executable);

const gchar * mex_application_get_icon (MexApplication *self);
void          mex_application_set_icon (MexApplication *self,
                                        const gchar     *icon);

const gchar * mex_application_get_thumbnail (MexApplication *self);
void          mex_application_set_thumbnail (MexApplication *self,
                                             const gchar     *thumbnail);

const gchar * mex_application_get_description (MexApplication *self);
void          mex_application_set_description (MexApplication *self,
                                               const gchar     *description);

const gchar * mex_application_get_desktop_file (MexApplication *self);
void          mex_application_set_desktop_file (MexApplication *self,
                                                const gchar     *desktop_file);

gboolean      mex_application_get_bookmarked (MexApplication *self);
void          mex_application_set_bookmarked (MexApplication *self,
                                              gboolean         bookmarked);

G_END_DECLS

#endif /* __MEX_APPLICATION_H__ */
