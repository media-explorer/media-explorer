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


/*
 * mex_g_signal_connect_object and it's related functions and data structures
 * were taken from telepathy-glib/util.c, which has the following copyright
 * notice:
 *
 * util.c - Source for telepathy-glib utility functions
 * Copyright © 2006-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2006-2008 Nokia Corporation
 * Copyright © 1999 Tom Tromey
 * Copyright © 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include "mex-utils.h"
#include <mx/mx.h>
#include <string.h>

#include <mex/mex-content-view.h>
#include <mex/mex-program.h>
#include <mex/mex-settings.h>

#ifdef HAVE_COGL_GLES2

#define GLES2_VARS \
  "precision mediump float;\n" \
  "varying vec2 tex_coord;\n" \
  "varying vec4 frag_color;\n"
#define TEX_COORD "tex_coord"
#define COLOR_VAR "frag_color"

#else

#define GLES2_VARS ""
#define TEX_COORD "gl_TexCoord[0]"
#define COLOR_VAR "gl_Color"

#endif

const gchar *mex_shader_box_blur =
  GLES2_VARS
  "uniform sampler2D tex;\n"
  "uniform float x_step, y_step, x_radius, y_radius;\n"

  "void\n"
  "main ()\n"
  "  {\n"
  "    float u, v, samples;\n"
  "    vec4 color = vec4 (0, 0, 0, 0);\n"

  "    samples = 0;\n"
  "    for (v = floor (-y_radius); v < ceil (y_radius); v++)\n"
  "      for (u = floor (-x_radius); u < ceil (x_radius); u++)\n"
  "        {\n"
  "          float s, t, mult;\n"

  "          mult = 1.0;\n"

  "          if (u < x_radius)\n"
  "            mult *= x_radius - u;\n"
  "          if (u > x_radius)\n"
  "            mult *= u - x_radius;\n"
  "          if (v < y_radius)\n"
  "            mult *= y_radius - v;\n"
  "          if (v > y_radius)\n"
  "            mult *= v - y_radius;\n"

  "          color += texture2D (tex, vec2("TEX_COORD".s + u * x_step,\n"
  "                                        "TEX_COORD".y + v * y_step)) *\n"
  "                   mult;\n"
  "          samples += mult;\n"
  "        }\n"
  "    color.rgba /= samples;\n"
  "    gl_FragColor = color * "COLOR_VAR";\n"
  "  }\n";

static GQuark mex_action_content_quark = 0;
static GQuark mex_action_model_quark = 0;

void
mex_style_load_default (void)
{
  GList *d, *dirs;
  gchar *tmp;

  GError *error = NULL;

  MxIconTheme *theme = mx_icon_theme_get_default ();

  /* Set the icon theme */
  dirs = g_list_copy ((GList *) mx_icon_theme_get_search_paths (theme));

  for (d = dirs; d; d = d->next)
    d->data = g_strdup (d->data);

  tmp = g_build_filename (mex_get_data_dir (), "icons", NULL);
  dirs = g_list_prepend (dirs, tmp);

  mx_icon_theme_set_search_paths (theme, dirs);

  while (dirs)
    {
      g_free (dirs->data);
      dirs = g_list_delete_link (dirs, dirs);
    }

  mx_icon_theme_set_theme_name (theme, "mex");

  /* Load the style */
  tmp = g_build_filename (mex_get_data_dir (), "style", "style.css", NULL);
  mx_style_load_from_file (mx_style_get_default (), tmp, &error);
  g_free (tmp);

  if (error)
    {
      g_warning (G_STRLOC ": Error loading style: %s", error->message);
      g_error_free (error);
    }
}

void
mex_paint_texture_frame (gfloat                    x,
                         gfloat                    y,
                         gfloat                    width,
                         gfloat                    height,
                         gfloat                    tex_width,
                         gfloat                    tex_height,
                         gfloat                    top,
                         gfloat                    right,
                         gfloat                    bottom,
                         gfloat                    left,
                         MexPaintTextureFrameFlags flags)
{
  gint n_vertices;
  gfloat verts[9 * 8];
  gfloat ttop, tright, tbottom, tleft;

  ttop = top / tex_height;
  tright = right / tex_width;
  tbottom = bottom / tex_height;
  tleft = left / tex_width;

  n_vertices = 0;

  if (flags & MEX_TEXTURE_FRAME_TOP_LEFT)
    {
      memcpy (&verts[0],
              &((gfloat[]) { x, y, x + left, y + top,
                             0.0, 0.0, tleft, ttop }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

  if (flags & MEX_TEXTURE_FRAME_TOP)
    {
      memcpy (&verts[n_vertices * 8],
              &((gfloat[]) { x + left, y, x + width - right, y + top,
                             tleft, 0.0, tright, ttop }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

  if (flags & MEX_TEXTURE_FRAME_TOP_RIGHT)
    {
      memcpy (&verts[n_vertices * 8],
              &((gfloat[]) { x + width - right, y, x + width, y + top,
                             tright, 0.0, 1.0, ttop }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

  if (flags & MEX_TEXTURE_FRAME_LEFT)
    {
      memcpy (&verts[n_vertices * 8],
              &((gfloat[]) { x, y + top, x + left, y + height - bottom,
                             0.0, ttop, tleft, tbottom }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

  if (flags & MEX_TEXTURE_FRAME_MIDDLE)
    {
      memcpy (&verts[n_vertices * 8],
              &((gfloat[]) { x + left, y + top,
                             x + width - right, y + height - bottom,
                             tleft, ttop, tright, tbottom }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

  if (flags & MEX_TEXTURE_FRAME_RIGHT)
    {
      memcpy (&verts[n_vertices * 8],
              &((gfloat[]) { x + width - right, y + top, x + width,
                             y + height - bottom,
                             tright, ttop, 1.0, tbottom }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

  if (flags & MEX_TEXTURE_FRAME_BOTTOM_LEFT)
    {
      memcpy (&verts[n_vertices * 8],
              &((gfloat[]) { x, y + height - bottom,
                             x + left, y + height,
                             0.0, tbottom, tleft, 1.0 }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

  if (flags & MEX_TEXTURE_FRAME_BOTTOM)
    {
      memcpy (&verts[n_vertices * 8],
              &((gfloat[]) { x + left, y + height - bottom,
                             x + width - right, y + height,
                             tleft, tbottom, tright, 1.0 }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

  if (flags & MEX_TEXTURE_FRAME_BOTTOM_RIGHT)
    {
      memcpy (&verts[n_vertices * 8],
              &((gfloat[]) { x + width - right, y + height - bottom,
                             x + width, y + height,
                             tright, tbottom, 1.0, 1.0 }),
              sizeof (gfloat) * 8);
      n_vertices ++;
    }

#if 0
  float verts[] =
    {
      /* top left corner */
      x, y, x + left, y + top,
      0.0, 0.0,
      tleft, ttop,

      /* top middle */
      x + left, y, x + width - right, y + top,
      tleft, 0.0,
      tright, ttop,

      /* top right */
      x + width - right, y, x + width, y + top,
      tright, 0.0,
      1.0, ttop,

      /* mid left */
      x, y + top, x + left, y + height - bottom,
      0.0, ttop,
      tleft, tbottom,

      /* mid right */
      x + width - right, y + top, x + width, y + height - bottom,
      tright, ttop,
      1.0, tbottom,

      /* bottom left */
      x, y + height - bottom, x + left, y + height,
      0.0, tbottom,
      tleft, 1.0,

      /* bottom center */
      x + left, y + height - bottom, x + width - right, y + height,
      tleft, tbottom,
      tright, 1.0,

      /* bottom right */
      x + width - right, y + height - bottom, x + width, y + height,
      tright, tbottom,
      1.0, 1.0,

      /* center */
      x + left, y + top, x + width - right, y + height - bottom,
      tleft, ttop,
      tright, tbottom
    };
#endif

  if (n_vertices)
    cogl_rectangles_with_texture_coords (verts, n_vertices);
}

void
mex_push_focus (MxFocusable *actor)
{
  ClutterActor *stage;

  g_return_if_fail (MX_IS_FOCUSABLE (actor));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  stage = clutter_actor_get_stage (CLUTTER_ACTOR (actor));
  if (stage)
    {
      MxFocusManager *manager =
        mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));

      if (manager)
        mx_focus_manager_push_focus (manager, MX_FOCUSABLE (actor));
    }
}

void
mex_action_set_content (MxAction *action, MexContent *content)
{
  if (!mex_action_content_quark)
    mex_action_content_quark =
      g_quark_from_static_string ("mex-action-content-quark");

  g_object_set_qdata_full (G_OBJECT (action), mex_action_content_quark,
                           g_object_ref (content),
                           (GDestroyNotify) g_object_unref);
}

/**
 * mex_action_get_content:
 * @action: A #MxAction
 *
 * Returns: (transfer none): The #MexContent linked to that action
 */
MexContent *
mex_action_get_content (MxAction *action)
{
  if (mex_action_content_quark)
    return g_object_get_qdata (G_OBJECT (action), mex_action_content_quark);
  else
    return NULL;
}

void
mex_action_set_context (MxAction *action, MexModel *model)
{
  if (!mex_action_model_quark)
    mex_action_model_quark =
      g_quark_from_static_string ("mex-action-model-quark");

  g_object_set_qdata_full (G_OBJECT (action), mex_action_model_quark,
                           g_object_ref (model),
                           (GDestroyNotify) g_object_unref);
}

/**
 * mex_action_get_context:
 * @action: A #MxAction
 *
 * Returns: (transfer none): The #MexModel linked to that action
 */
MexModel *
mex_action_get_context (MxAction *action)
{
  if (mex_action_model_quark)
    return g_object_get_qdata (G_OBJECT (action), mex_action_model_quark);
  else
    return NULL;
}

static gchar *
mex_utils_content_get_title (MexContent *content, gboolean *allocated)
{
  const gchar *retval;

  *allocated = FALSE;
  retval = mex_content_get_metadata (content,
                                     MEX_CONTENT_METADATA_TITLE);
  if (retval)
    return (gchar *)retval;

  retval = mex_content_get_metadata (content,
                                     MEX_CONTENT_METADATA_SERIES_NAME);
  if (retval)
    return (gchar *)retval;

  retval = mex_content_get_metadata (content,
                                     MEX_CONTENT_METADATA_SUB_TITLE);
  if (retval)
    return (gchar *)retval;

  retval = mex_content_get_metadata (content,
                                     MEX_CONTENT_METADATA_STREAM);
  if (retval)
    {
      *allocated = TRUE;
      return g_path_get_basename (retval);
    }

  return NULL;
}

gint
mex_model_sort_alpha_cb (MexContent *a,
                         MexContent *b,
                         gpointer    bool_reverse)
{
  gint retval;
  gchar *text_a, *text_b, *case_text_a, *case_text_b;
  gboolean alloc_a, alloc_b, folder_a, folder_b;

  folder_a = (g_strcmp0 ("x-grl/box",
    mex_content_get_metadata (a, MEX_CONTENT_METADATA_MIMETYPE)) == 0);
  folder_b = (g_strcmp0 ("x-grl/box",
    mex_content_get_metadata (b, MEX_CONTENT_METADATA_MIMETYPE)) == 0);

  if (folder_a != folder_b)
    return folder_a ? -1 : 1;

  text_a = mex_utils_content_get_title (a, &alloc_a);
  text_b = mex_utils_content_get_title (b, &alloc_b);

  if (!text_a && !text_b)
    {
      retval = 0;
      goto sort_done;
    }

  if (!text_a)
    {
      retval = -1;
      goto sort_done;
    }

  if (!text_b)
    {
      retval = 1;
      goto sort_done;
    }

  case_text_a = g_utf8_casefold (text_a, -1);
  case_text_b = g_utf8_casefold (text_b, -1);

  retval = g_utf8_collate (case_text_a, case_text_b);

  g_free (case_text_a);
  g_free (case_text_b);

sort_done:
  if (alloc_a)
    g_free (text_a);
  if (alloc_b)
    g_free (text_b);

  if (GPOINTER_TO_INT (bool_reverse))
    retval = -retval;

  return retval;
}

gint
mex_model_sort_time_cb (MexContent *a,
                        MexContent *b,
                        gpointer    bool_reverse)
{
  gint retval;
  gboolean folder_a, folder_b;
  const gchar *date_a, *date_b;

  folder_a = (g_strcmp0 ("x-grl/box",
    mex_content_get_metadata (a, MEX_CONTENT_METADATA_MIMETYPE)) == 0);
  folder_b = (g_strcmp0 ("x-grl/box",
    mex_content_get_metadata (b, MEX_CONTENT_METADATA_MIMETYPE)) == 0);

  if (folder_a != folder_b)
    return folder_a ? -1 : 1;

  date_a = mex_content_get_metadata (a, MEX_CONTENT_METADATA_DATE);
  date_b = mex_content_get_metadata (b, MEX_CONTENT_METADATA_DATE);

  if (date_a)
    {
      if (date_b)
        retval = strcmp (date_a, date_b);
      else
        retval = -1;
    }
  else if (date_b)
    retval = 1;
  else
    retval = 0;

  if (GPOINTER_TO_INT (bool_reverse))
    retval = -retval;

  return retval;
}

gint
mex_model_sort_smart_cb (MexContent *a,
                         MexContent *b,
                         gpointer    bool_reverse)
{
  gint retval;
  const gchar *played_a, *played_b;

  played_a = mex_content_get_metadata (a, MEX_CONTENT_METADATA_PLAY_COUNT);
  played_b = mex_content_get_metadata (b, MEX_CONTENT_METADATA_PLAY_COUNT);
  if (played_a)
    {
      if (!played_b)
        {
          retval = 1;
          goto end;
        }
    }
  else
    {
      if (played_b)
        {
          retval = -1;
          goto end;
        }
    }

  retval = -mex_model_sort_time_cb (a, b, bool_reverse);

 end:

  if (GPOINTER_TO_INT (bool_reverse))
    retval = -retval;

  return retval;
}

void
mex_print_date (GDateTime   *date,
                const gchar *prefix)
{
  gchar *date_str;

  if (date == NULL)
    date_str = "date is NULL";
  else
    date_str = g_date_time_format (date, "%d/%m/%y %H:%M");

  if (prefix)
    g_debug ("%s: %s", prefix, date_str);
  else
    g_debug ("%s", date_str);
  g_free (date_str);
}

gchar *
mex_date_to_string (GDateTime *date)
{
  return g_date_time_format (date, "%d/%m/%y %H:%M");
}

GQuark
mex_tile_shadow_quark (void)
{
  static GQuark shadow_quark = 0;

  if (G_UNLIKELY (shadow_quark == 0))
    {
      shadow_quark = g_quark_from_static_string ("mex-tile-shadow");
    }

  return shadow_quark;
}

#if !MX_CHECK_VERSION(1, 99, 0)
static gboolean
mx_border_image_equal (MxBorderImage *v1,
                       MxBorderImage *v2)
{
  if (v1 == v2)
    return TRUE;

  if (!v1 && v2)
    return FALSE;

  if (!v2 && v1)
    return FALSE;

  if (g_strcmp0 (v1->uri, v2->uri))
    return FALSE;

  if (v1->top != v2->top)
    return FALSE;

  if (v1->right != v2->right)
    return FALSE;

  if (v1->bottom != v2->bottom)
    return FALSE;

  if (v1->left != v2->left)
    return FALSE;

  return TRUE;
}
#endif

void
mex_replace_border_image (CoglHandle     *texture_p,
                          MxBorderImage  *image,
                          MxBorderImage **image_p,
                          CoglHandle     *material_p)
{
  MxTextureCache *cache = mx_texture_cache_get_default ();

  if (!mx_border_image_equal (image, *image_p))
    {
      if (*image_p)
        g_boxed_free (MX_TYPE_BORDER_IMAGE, *image_p);

      if (*texture_p)
        {
          cogl_handle_unref (*texture_p);
          *texture_p = NULL;
        }

      *image_p = image;
      if (image)
        {
          *texture_p = mx_texture_cache_get_cogl_texture (cache, image->uri);
          if (!*material_p)
            *material_p = cogl_material_new ();
          cogl_material_set_layer (*material_p, 0, *texture_p);
        }
      else
        {
          cogl_handle_unref (*material_p);
          *material_p = NULL;
        }
    }
}

/**
 * mex_get_data_dir:
 *
 * Gets the data directory for media explorer.
 * Use this rather than anything else as this ensures greater compatibility.
 *
 * Return value: Path to mex data directory
 */
const char*
mex_get_data_dir (void)
{
  static char* datadir = NULL;
  gint i;

  if (!datadir)
    {
      static const gchar* const * system_data_dirs;

      system_data_dirs = g_get_system_data_dirs ();

      for (i = 0; system_data_dirs[i]; i++)
        {
          datadir = g_build_filename (system_data_dirs[i], PACKAGE_NAME, NULL);

          if (g_file_test (datadir, G_FILE_TEST_IS_DIR))
            break;
          else
            {
              g_free (datadir);
              datadir = NULL;
            }
        }

      if (!datadir)
        g_warning ("Could not find application data directory.");
    }

  return datadir;
}

/**
* mex_actor_has_focus:
* @manager: The current MxFocusManager
* @actor: The actor that you wish query if it is focused
*
* Evaluates whether the actor has focus or not.
*
* Return value: True if the @actor is focused
*/
gboolean
mex_actor_has_focus (MxFocusManager *manager,
                     ClutterActor   *actor)
{
  ClutterActor *parent;
  ClutterActor *focus = (ClutterActor *)mx_focus_manager_get_focused (manager);

  while (focus)
    {
      parent = clutter_actor_get_parent (focus);
      if (focus == actor)
        return TRUE;
      focus = parent;
    }

  return FALSE;
}

/**
 * mex_content_from_uri:
 * @uri: A valid uri for some media
 *
 * Creates a new MexContent from any given uri and tries to guess
 * at some metadata.
 *
 * Returns: (transfer full): A new MexContent
 */
MexContent *
mex_content_from_uri (const gchar *uri)
{
  MexProgram *program;
  gchar *mime_guess;
  gchar *filename;
  gchar *basename;
  gboolean is_dvd;

  is_dvd = FALSE;

  if (g_str_has_prefix (uri, "dvd") ||
      g_str_has_prefix (uri, "vcd"))
    {
      is_dvd = TRUE;
      mime_guess = g_strdup ("video/dvd");
    }
  else
    {
      mime_guess = g_content_type_guess (uri, NULL, 0, NULL);
    }

  /* This is to stop us trying to create content for things we can't display
   * as technically anything could be passed to us.
   */
  if (mime_guess)
    {
      if (!g_str_has_prefix (mime_guess, "video/") &&
          !g_str_has_prefix (mime_guess, "audio") &&
          !g_str_has_prefix (mime_guess, "image/"))
        {
          g_free (mime_guess);
          return NULL;
        }
    }

  program = mex_program_new (NULL);

  mex_content_set_metadata (MEX_CONTENT (program),
                            MEX_CONTENT_METADATA_MIMETYPE, mime_guess);

  mex_content_set_metadata (MEX_CONTENT (program),
                            MEX_CONTENT_METADATA_STREAM, uri);

  g_free (mime_guess);

  if (!is_dvd)
    {
      filename = g_filename_from_uri (uri, NULL, NULL);
      basename = g_filename_display_basename (filename);

      g_free (filename);

      mex_content_set_metadata (MEX_CONTENT (program),
                                MEX_CONTENT_METADATA_TITLE, basename);

      g_free (basename);
    }
  else
    {
      mex_content_set_metadata (MEX_CONTENT (program),
                                MEX_CONTENT_METADATA_TITLE, "DVD");
    }

  mex_content_set_metadata (MEX_CONTENT (program),
                            MEX_CONTENT_METADATA_SYNOPSIS,
                            uri);

  return MEX_CONTENT (program);
}

/**
 * mex_get_settings_key_file: (skip)
 *
 * Note setting backend is to be rewritten in future versions
 */
GKeyFile *
mex_get_settings_key_file (void)
{
  gchar *settings_path;

  settings_path = mex_settings_find_config_file (mex_settings_get_default (),
                                                 "mex.conf");

  if (settings_path)
    {
      GKeyFile *mex_conf;

      mex_conf = g_key_file_new ();

      g_key_file_load_from_file (mex_conf,
                                 settings_path,
                                 G_KEY_FILE_NONE,
                                 NULL);

      g_free (settings_path);

      return mex_conf;
    }
  return NULL;
}

typedef struct {
    GObject *instance;
    GObject *observer;
    GClosure *closure;
    gulong handler_id;
} WeakHandlerCtx;

static WeakHandlerCtx *
whc_new (GObject *instance,
         GObject *observer)
{
  WeakHandlerCtx *ctx = g_slice_new0 (WeakHandlerCtx);

  ctx->instance = instance;
  ctx->observer = observer;

  return ctx;
}

static void
whc_free (WeakHandlerCtx *ctx)
{
  g_slice_free (WeakHandlerCtx, ctx);
}

static void observer_destroyed_cb (gpointer, GObject *);
static void closure_invalidated_cb (gpointer, GClosure *);

/*
 * If signal handlers are removed before the object is destroyed, this
 * callback will never get triggered.
 */
static void
instance_destroyed_cb (gpointer ctx_,
    GObject *where_the_instance_was)
{
  WeakHandlerCtx *ctx = ctx_;

  /* No need to disconnect the signal here, the instance has gone away. */
  g_object_weak_unref (ctx->observer, observer_destroyed_cb, ctx);
  g_closure_remove_invalidate_notifier (ctx->closure, ctx,
      closure_invalidated_cb);
  whc_free (ctx);
}

/* Triggered when the observer is destroyed. */
static void
observer_destroyed_cb (gpointer ctx_,
    GObject *where_the_observer_was)
{
  WeakHandlerCtx *ctx = ctx_;

  g_closure_remove_invalidate_notifier (ctx->closure, ctx,
      closure_invalidated_cb);
  g_signal_handler_disconnect (ctx->instance, ctx->handler_id);
  g_object_weak_unref (ctx->instance, instance_destroyed_cb, ctx);
  whc_free (ctx);
}

/* Triggered when either object is destroyed or the handler is disconnected. */
static void
closure_invalidated_cb (gpointer ctx_,
    GClosure *where_the_closure_was)
{
  WeakHandlerCtx *ctx = ctx_;

  g_object_weak_unref (ctx->instance, instance_destroyed_cb, ctx);
  g_object_weak_unref (ctx->observer, observer_destroyed_cb, ctx);
  whc_free (ctx);
}

/**
 * mex_g_signal_connect_object:
 * @instance: the instance to connect to.
 * @detailed_signal: a string of the form "signal-name::detail".
 * @c_handler: the #GCallback to connect.
 * @gobject: the object to pass as data to @c_handler.
 * @connect_flags: a combination of #GConnectFlags. Only
 *  %G_CONNECT_AFTER and %G_CONNECT_SWAPPED are supported by this function.
 *
 * Connects a #GCallback function to a signal for a particular object, as if
 * with g_signal_connect(). Additionally, arranges for the signal handler to be
 * disconnected if @gobject is destroyed.
 *
 * This is similar to g_signal_connect_data(), but uses a closure which
 * ensures that the @gobject stays alive during the call to @c_handler
 * by temporarily adding a reference count to @gobject.
 *
 * This is similar to g_signal_connect_object(), but doesn't have the
 * documented bug that everyone is too scared to fix. Also, it does not allow
 * you to pass in NULL as @gobject
 *
 * This is intended to be a convenient way for objects to use themselves as
 * user_data for callbacks without having to explicitly disconnect all the
 * handlers in their finalizers.
 *
 * Returns: the handler id
 */
gulong
mex_g_signal_connect_object (gpointer       instance,
                             const gchar   *detailed_signal,
                             GCallback      c_handler,
                             gpointer       gobject,
                             GConnectFlags  connect_flags)
{
  GObject *instance_obj = G_OBJECT (instance);
  WeakHandlerCtx *ctx = whc_new (instance_obj, gobject);

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE (instance), 0);
  g_return_val_if_fail (detailed_signal != NULL, 0);
  g_return_val_if_fail (c_handler != NULL, 0);
  g_return_val_if_fail (G_IS_OBJECT (gobject), 0);
  g_return_val_if_fail (
      (connect_flags & ~(G_CONNECT_AFTER|G_CONNECT_SWAPPED)) == 0, 0);

  if (connect_flags & G_CONNECT_SWAPPED)
    ctx->closure = g_cclosure_new_object_swap (c_handler, gobject);
  else
    ctx->closure = g_cclosure_new_object (c_handler, gobject);

  ctx->handler_id = g_signal_connect_closure (instance, detailed_signal,
      ctx->closure, (connect_flags & G_CONNECT_AFTER) ? TRUE : FALSE);

  g_object_weak_ref (instance_obj, instance_destroyed_cb, ctx);
  g_object_weak_ref (gobject, observer_destroyed_cb, ctx);
  g_closure_add_invalidate_notifier (ctx->closure, ctx,
      closure_invalidated_cb);

  return ctx->handler_id;
}
