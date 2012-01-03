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


#ifndef __MEX_GRILO_PROGRAM_H__
#define __MEX_GRILO_PROGRAM_H__

#include <glib-object.h>
#include <mex/mex-program.h>

G_BEGIN_DECLS

#define MEX_TYPE_GRILO_PROGRAM mex_grilo_program_get_type()

#define MEX_GRILO_PROGRAM(obj)                                          \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               MEX_TYPE_GRILO_PROGRAM, MexGriloProgram))

#define MEX_GRILO_PROGRAM_CLASS(klass)                                  \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            MEX_TYPE_GRILO_PROGRAM, MexGriloProgramClass))

#define MEX_IS_GRILO_PROGRAM(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MEX_TYPE_GRILO_PROGRAM))

#define MEX_IS_GRILO_PROGRAM_CLASS(klass)               \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                    \
                            MEX_TYPE_GRILO_PROGRAM))

#define MEX_GRILO_PROGRAM_GET_CLASS(obj)                                \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              MEX_TYPE_GRILO_PROGRAM, MexGriloProgramClass))

typedef struct _MexGriloProgram MexGriloProgram;
typedef struct _MexGriloProgramClass MexGriloProgramClass;
typedef struct _MexGriloProgramPrivate MexGriloProgramPrivate;

#include <mex/mex-grilo-feed.h>

struct _MexGriloProgram
{
  MexProgram parent;

  MexGriloProgramPrivate *priv;
};

struct _MexGriloProgramClass
{
  MexProgramClass parent_class;
};

GType mex_grilo_program_get_type (void) G_GNUC_CONST;

MexProgram *mex_grilo_program_new (MexGriloFeed *feed, GrlMedia *media);

GrlMedia *mex_grilo_program_get_grilo_media (MexGriloProgram *program);

void mex_grilo_program_set_grilo_media (MexGriloProgram *program, GrlMedia *media);

GList * mex_grilo_program_get_default_keys (void);

G_END_DECLS

#endif /* __MEX_GRILO_PROGRAM_H__ */
