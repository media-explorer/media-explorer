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

#ifndef __MEX_PROGRAM_H__
#define __MEX_PROGRAM_H__

#include <glib-object.h>
#include <mex/mex-feed.h>

#include <mex/mex-generic-content.h>          /* keys for get/set_metadata() */

G_BEGIN_DECLS

#define MEX_TYPE_PROGRAM                        \
  (mex_program_get_type())
#define MEX_PROGRAM(obj)                                \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               MEX_TYPE_PROGRAM,        \
                               MexProgram))
#define MEX_PROGRAM_CLASS(klass)                \
  (G_TYPE_CHECK_CLASS_CAST ((klass),            \
                            MEX_TYPE_PROGRAM,   \
                            MexProgramClass))
#define MEX_IS_PROGRAM(obj)                             \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MEX_TYPE_PROGRAM))
#define MEX_IS_PROGRAM_CLASS(klass)             \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),            \
                            MEX_TYPE_PROGRAM))
#define MEX_PROGRAM_GET_CLASS(obj)              \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),            \
                              MEX_TYPE_PROGRAM, \
                              MexProgramClass))

/* How to use the locales rating system? */
#define MEX_PROGRAM_RATING_UNRATED "n"
#define MEX_PROGRAM_RATING_EVERYONE "u"
#define MEX_PROGRAM_RATING_CHILDREN "pg"
#define MEX_PROGRAM_RATING_TEENS "pg13"
#define MEX_PROGRAM_RATING_RESTRICTED "r"
#define MEX_PROGRAM_RATING_NC17 "nc17"

typedef struct _MexProgramPrivate MexProgramPrivate;
typedef struct _MexProgramClass MexProgramClass;

typedef void (*MexGetStreamReply) (MexProgram   *program,
                                   const char   *stream_url,
                                   const GError *error,
                                   gpointer      userdata);
struct _MexProgram
{
  MexGenericContent parent;

  MexProgramPrivate *priv;
};

struct _MexProgramClass
{
  MexGenericContentClass parent_class;

  gchar *(*get_index_str) (MexProgram *program);
  gchar *(*get_id) (MexProgram *program);

  void (*complete) (MexProgram *program);
  void (*get_stream) (MexProgram       *program,
                      MexGetStreamReply reply,
                      gpointer          userdata);
};

GType mex_program_get_type (void) G_GNUC_CONST;

MexProgram *mex_program_new (MexFeed *feed);

MexFeed *mex_program_get_feed (MexProgram *program);

void mex_program_add_actor (MexProgram *program,
                            const char *actor);
GPtrArray *mex_program_get_actors (MexProgram *program);
void mex_program_get_stream (MexProgram       *program,
                             MexGetStreamReply reply,
                             gpointer          userdata);
gchar *mex_program_get_index_str (MexProgram *program);
gchar *mex_program_get_id (MexProgram *program);

/* Private functions for internal Mex Classes to use */
void _mex_program_complete (MexProgram *program);

G_END_DECLS

#endif /* __MEX_PROGRAM_H__ */
