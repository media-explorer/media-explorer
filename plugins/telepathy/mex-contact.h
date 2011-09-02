/*
  * Mex - a media explorer
  *
  * Copyright © 2011 Collabora Ltd.
  *   @author Dario Freddi <dario.freddi@collabora.com>
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

#ifndef __MEX_CONTACT_H__
#define __MEX_CONTACT_H__

#include <glib-object.h>
#include <mex/mex-generic-content.h>
#include <telepathy-glib/contact.h>

G_BEGIN_DECLS

#define MEX_TYPE_CONTACT mex_contact_get_type()

#define MEX_CONTACT(obj)                                      \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                         \
                               MEX_TYPE_CONTACT, MexContact))

#define MEX_CONTACT_CLASS(klass)                                \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                            \
                            MEX_TYPE_CONTACT, MexContactClass))

#define MEX_IS_CONTACT(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),             \
                               MEX_TYPE_CONTACT))

#define MEX_IS_CONTACT_CLASS(klass)            \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),           \
                            MEX_TYPE_CONTACT))

#define MEX_CONTACT_GET_CLASS(obj)                                \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                              \
                              MEX_TYPE_CONTACT, MexContactClass))

typedef struct _MexContact MexContact;
typedef struct _MexContactClass MexContactClass;
typedef struct _MexContactPrivate MexContactPrivate;

struct _MexContact
{
  MexGenericContent parent;

  MexContactPrivate *priv;
};

struct _MexContactClass
{
  MexGenericContentClass parent_class;
};

GType        mex_contact_get_type (void) G_GNUC_CONST;

MexContact  *mex_contact_new (void);

TpContact   *mex_contact_get_tp_contact (MexContact *self);
void         mex_contact_set_tp_contact (MexContact *self,
                                         TpContact  *contact);

gboolean     mex_contact_should_add_to_model (MexContact *self);

G_END_DECLS

#endif /* __MEX_CONTACT_H__ */
