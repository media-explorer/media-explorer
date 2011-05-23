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

#include "mex-program.h"
#include "mex-content.h"

enum {
  PROP_0 = MEX_CONTENT_METADATA_LAST_ID,
  PROP_FEED
};

enum {
  CHANGED,
  LAST_SIGNAL,
};

struct _MexProgramPrivate {
  GPtrArray *actors;
  MexFeed *feed;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),           \
                                                       MEX_TYPE_PROGRAM, \
                                                       MexProgramPrivate))
G_DEFINE_TYPE (MexProgram, mex_program, MEX_TYPE_GENERIC_CONTENT);

static guint32 signals[LAST_SIGNAL] = {0,};

/* Append each string in the metadata hashtable into a long string which will
   then be tokenised and split into index terms */
static void
make_metadata_string (MexContentMetadata  key,
                      const gchar        *value,
                      gpointer            data)
{
  GString *str = (GString *) data;

  g_string_append (str, " ");
  g_string_append (str, value);
}

static char *
_mex_program_get_index_str (MexProgram *program)
{
  GString *str;
  char *index_str;

  g_return_val_if_fail (MEX_IS_PROGRAM (program), NULL);

  str = g_string_new ("");
  mex_content_foreach_metadata (MEX_CONTENT (program),
                                make_metadata_string, str);

  index_str = str->str;
  g_string_free (str, FALSE);

  return index_str;
}

/*
 * GObject implementation
 */

static void
mex_program_finalize (GObject *object)
{
  MexProgram *self = (MexProgram *) object;
  MexProgramPrivate *priv = self->priv;

  if (priv->actors) {
    g_ptr_array_unref (priv->actors);
    priv->actors = NULL;
  }

  G_OBJECT_CLASS (mex_program_parent_class)->finalize (object);
}

static void
mex_program_dispose (GObject *object)
{
  MexProgram *self = (MexProgram *) object;
  MexProgramPrivate *priv = self->priv;

  if (priv->feed)
    {
      g_object_unref (priv->feed);
      priv->feed = NULL;
    }

  G_OBJECT_CLASS (mex_program_parent_class)->dispose (object);
}

static void
mex_program_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  MexProgram *self = MEX_PROGRAM (object);
  MexProgramPrivate *priv = self->priv;

  switch (prop_id) {
  case PROP_FEED:
    priv->feed = (MexFeed *) g_value_dup_object (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
mex_program_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  MexProgram *self = MEX_PROGRAM (object);
  MexProgramPrivate *priv = self->priv;

  switch (prop_id) {
  case PROP_FEED:
    g_value_set_object (value, priv->feed);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

#if 0
static char *
program_get_metadata_fallback (MexGenericContent *gc,
                               MexContentMetadata key)
{
  MexContent *content = MEX_CONTENT (gc);
  const gchar *showname;
  gchar *target;

  switch (key) {
  case MEX_CONTENT_METADATA_TITLE:
    showname = mex_content_get_metadata (content,
                                         MEX_CONTENT_METADATA_SERIES_NAME);

    if (!showname) {
      const gchar *url;
      gchar *basename;

      url = mex_content_get_metadata (content, MEX_CONTENT_METADATA_URL);
      basename = g_path_get_basename (url);
      target = g_uri_unescape_string (basename, NULL);
      g_free (basename);
    } else {
      const gchar *episode, *season;

      episode = mex_content_get_metadata (content,
                                          MEX_CONTENT_METADATA_EPISODE);
      season = mex_content_get_metadata (content,
                                         MEX_CONTENT_METADATA_SEASON);

      if (episode && season) {
        target = g_strdup_printf ("%s: Season %s, Episode %s",
                                  showname, season, episode);
      } else if (episode) {
        target = g_strdup_printf ("%s: Episode %s", showname, episode);
      } else if (season) {
        target = g_strdup_printf ("%s: Season %s", showname, season);
      } else {
        target = g_strdup (showname);
      }
    }
    break;

  default:
    target = NULL;
    break;
  }

  return target;
}
#endif

static void
mex_program_class_init (MexProgramClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *o_class = (GObjectClass *) klass;

  o_class->dispose = mex_program_dispose;
  o_class->finalize = mex_program_finalize;
  o_class->set_property = mex_program_set_property;
  o_class->get_property = mex_program_get_property;

  klass->get_index_str = _mex_program_get_index_str;

  g_type_class_add_private (klass, sizeof (MexProgramPrivate));

  pspec = g_param_spec_object ("feed",
                               "Feed",
                               "The MexFeed that created this program.",
                               MEX_TYPE_FEED,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                               G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_FEED, pspec);

  signals[CHANGED] = g_signal_new ("changed",
                                   G_TYPE_FROM_CLASS (klass),
                                   G_SIGNAL_RUN_FIRST |
                                   G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                   g_cclosure_marshal_VOID__VOID,
                                   G_TYPE_NONE, 0);
}

static void
mex_program_init (MexProgram *self)
{
  MexProgramPrivate *priv = GET_PRIVATE (self);

  self->priv = priv;
}

MexProgram *
mex_program_new (MexFeed *feed)
{
  return (MexProgram *) g_object_new (MEX_TYPE_PROGRAM, "feed", feed, NULL);
}

MexFeed *
mex_program_get_feed (MexProgram *program)
{
  g_return_val_if_fail (MEX_IS_PROGRAM (program), NULL);
  return program->priv->feed;
}

void
mex_program_add_actor (MexProgram *program,
                       const char *actor)
{
  MexProgramPrivate *priv;

  g_return_if_fail (MEX_IS_PROGRAM (program));

  priv = program->priv;
  if (priv->actors == NULL) {
    priv->actors = g_ptr_array_new_with_free_func (g_free);
  }

  g_ptr_array_add (priv->actors, g_strdup (actor));
}

/**
 * mex_program_get_actors:
 * @program: A #MexProgram
 *
 * Retrieves the #GPtrArray containing the actor names as strings. Call
 * g_ptr_array_unref on the returned value once finished with.
 *
 * Return value: A #GPtrArray or NULL if there are no actors defined.
 */
GPtrArray *
mex_program_get_actors (MexProgram *program)
{
  MexProgramPrivate *priv;

  g_return_val_if_fail (MEX_IS_PROGRAM (program), NULL);

  priv = program->priv;

  if (priv->actors) {
    return g_ptr_array_ref (priv->actors);
  } else {
    return NULL;
  }
}

struct _GetStreamPayload {
  MexProgram *program;
  MexGetStreamReply reply;
  gpointer userdata;
  char *stream;
};

static gboolean
emit_get_stream_reply (gpointer data)
{
  struct _GetStreamPayload *payload = (struct _GetStreamPayload *) data;

  payload->reply (payload->program, payload->stream, NULL, payload->userdata);
  g_object_unref (payload->program);
  g_free (payload->stream);
  g_slice_free (struct _GetStreamPayload, payload);

  return FALSE;
}

/* This is async as for some stream urls require a network access to obtain */
void
mex_program_get_stream (MexProgram       *program,
                        MexGetStreamReply reply,
                        gpointer          userdata)
{
  MexProgramClass *klass;
  const char *stream;

  g_return_if_fail (MEX_IS_PROGRAM (program));

  /* Some backends (eg. Apple trailers) may be able to cache the stream
     so we can just use the standard metadata method to obtain it */
  stream = mex_content_get_metadata (MEX_CONTENT (program),
                                     MEX_CONTENT_METADATA_STREAM);
  if (stream != NULL) {
    struct _GetStreamPayload *payload = g_slice_new (struct _GetStreamPayload);
    payload->program = g_object_ref (program);
    payload->reply = reply;
    payload->userdata = userdata;

    /* Technically the stream could be freed before the idle is called
       Great, now I want refcounted strings */
    payload->stream = g_strdup (stream);

    g_idle_add (emit_get_stream_reply, payload);
    return;
  }

  /* Other backends require custom methods to get the stream url */
  klass = MEX_PROGRAM_GET_CLASS (program);
  if (klass->get_stream) {
    klass->get_stream (program, reply, userdata);
  } else {
    struct _GetStreamPayload *payload = g_slice_new (struct _GetStreamPayload);
    payload->program = g_object_ref (program);
    payload->reply = reply;
    payload->userdata = userdata;
    payload->stream = NULL;

    g_idle_add (emit_get_stream_reply, payload);
  }
}

/* This is async as some programs don't have a still yet */
#if 0
void
mex_program_get_thumbnail (MexProgram          *program,
                           MexGetThumbnailReply reply,
                           gpointer             userdata)
{
  MexProgramClass *klass;
  MexProgramPrivate *priv;
  const char *still;

  g_return_if_fail (MEX_IS_PROGRAM (program));
  priv = program->priv;

  /* Some backends (eg. Apple trailers) may be able to cache the stream
     so we can just use the standard metadata method to obtain it */
  still = mex_content_get_metadata (MEX_CONTENT (program),
                                    MEX_PROGRAM_METADATA_STILL);
  if (stream != NULL) {
    struct _GetStreamPayload *payload = g_slice_new (struct _GetStreamPayload);
    payload->program = g_object_ref (program);
    payload->reply = reply;
    payload->userdata = userdata;

    /* Technically the stream could be freed before the idle is called
       Great, now I want refcounted strings */
    payload->stream = g_strdup (stream);

    g_idle_add (emit_get_stream_reply, payload);
    return;
  }

  /* Other backends require custom methods to get the stream url */
  klass = MEX_PROGRAM_GET_CLASS (program);
  if (klass->get_stream) {
    klass->get_stream (program, reply, userdata);
  } else {
    struct _GetStreamPayload *payload = g_slice_new (struct _GetStreamPayload);
    payload->program = g_object_ref (program);
    payload->reply = reply;
    payload->userdata = userdata;
    payload->stream = NULL;

    g_idle_add (emit_get_stream_reply, payload);
  }
}
#endif

/* Private function that calls the program_complete method
   to get any final information that needs generated */
void
_mex_program_complete (MexProgram *program)
{
  MexProgramClass *klass = MEX_PROGRAM_GET_CLASS (program);

  if (klass->complete) {
    klass->complete (program);
  }
}

gchar *
mex_program_get_index_str (MexProgram *program)
{
  MexProgramClass *klass;

  g_return_val_if_fail (MEX_IS_PROGRAM (program), NULL);

  klass = MEX_PROGRAM_GET_CLASS (program);

  if (klass->get_index_str)
    return klass->get_index_str (program);

  return NULL;
}

gchar *
mex_program_get_id (MexProgram *program)
{
  MexProgramClass *klass;

  g_return_val_if_fail (MEX_IS_PROGRAM (program), NULL);

  klass = MEX_PROGRAM_GET_CLASS (program);

  if (klass->get_id)
    return klass->get_id (program);

  return NULL;
}
