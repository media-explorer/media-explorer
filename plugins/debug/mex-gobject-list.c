/*
 * gobject-list: a LD_PRELOAD library for tracking the lifetime of GObjects
 *
 * Copyright (C) 2011  Collabora Ltd.
 * Copyright (C) 2011  Intel Corporation
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 *
 * Authors:
 *     Danielle Madeley  <danielle.madeley@collabora.co.uk>
 *     Damien Lespiau <damien.lespiau@intel.com>
 */
#include <glib-object.h>

#include <dlfcn.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

typedef enum
{
  DISPLAY_FLAG_NONE = 0,
  DISPLAY_FLAG_CREATE = 1,
  DISPLAY_FLAG_REFS = 1 << 2,
  DISPLAY_FLAG_BACKTRACE = 1 << 3,
  DISPLAY_FLAG_ALL =
      DISPLAY_FLAG_CREATE | DISPLAY_FLAG_REFS | DISPLAY_FLAG_BACKTRACE,
  DISPLAY_FLAG_DEFAULT = DISPLAY_FLAG_CREATE,
} DisplayFlags;

typedef struct
{
  const gchar *name;
  DisplayFlags flag;
} DisplayFlagsMapItem;

DisplayFlagsMapItem display_flags_map[] =
{
  { "none", DISPLAY_FLAG_NONE },
  { "create", DISPLAY_FLAG_CREATE },
  { "refs", DISPLAY_FLAG_REFS },
  { "backtrace", DISPLAY_FLAG_BACKTRACE },
  { "all", DISPLAY_FLAG_ALL },
};

static GHashTable *objects = NULL;
static GHashTable *classes = NULL;

gpointer gobject_list_pointer_to_follow = NULL;

/*
 * messages printed with PRINT() can be enabled/disabled at run time
 */
static gboolean gobject_list_verbose = FALSE;

#define PRINT(fmt, a...)  G_STMT_START { \
    if (gobject_list_verbose)            \
      g_print (fmt, ##a);                \
                          } G_STMT_END

void
gobject_list_toggle_verbose (void)
{
  gobject_list_verbose = !gobject_list_verbose;
}

static gboolean
display_filter (DisplayFlags flags)
{
  static DisplayFlags display_flags = DISPLAY_FLAG_DEFAULT;
  static gboolean parsed = FALSE;

  if (!parsed)
    {
      const gchar *display = g_getenv ("GOBJECT_LIST_DISPLAY");

      if (display != NULL)
        {
          gchar **tokens = g_strsplit (display, ",", 0);
          guint len = g_strv_length (tokens);
          guint i = 0;

          /* If there really are items to parse, clear the default flags */
          if (len > 0)
            display_flags = 0;

          for (; i < len; ++i)
            {
              gchar *token = tokens[i];
              guint j = 0;

              for (; j < G_N_ELEMENTS (display_flags_map); ++j)
                {
                  if (!g_ascii_strcasecmp (token, display_flags_map[j].name))
                    {
                      display_flags |= display_flags_map[j].flag;
                      break;
                    }
                }
            }

          g_strfreev (tokens);
        }

      parsed = TRUE;
    }

  return (display_flags & flags) ? TRUE : FALSE;
}

static gboolean
object_filter (const char *obj_name)
{
  const char *filter = g_getenv ("GOBJECT_LIST_FILTER");

  if (filter == NULL)
    return TRUE;
  else
    return (strncmp (filter, obj_name, strlen (filter)) == 0);
}

static void
print_trace (void)
{
#ifdef HAVE_LIBUNWIND
  unw_context_t uc;
  unw_cursor_t cursor;
  guint stack_num = 0;

  if (!display_filter (DISPLAY_FLAG_BACKTRACE))
    return;

  unw_getcontext (&uc);
  unw_init_local (&cursor, &uc);

  while (unw_step (&cursor) > 0)
    {
      gchar name[65];
      unw_word_t off;

      if (unw_get_proc_name (&cursor, name, 64, &off) < 0)
        {
          g_print ("Error getting proc name\n");
          break;
        }

      g_print ("#%d  %s + [0x%08x]\n", stack_num++, name, (unsigned int)off);
    }
#endif
}

static void
_dump_object_list (void)
{
  GHashTableIter iter;
  GObject *obj;

  g_hash_table_iter_init (&iter, objects);
  while (g_hash_table_iter_next (&iter, (gpointer) &obj, NULL))
    {
      g_print (" - %p, %s: %u refs\n",
          obj, G_OBJECT_TYPE_NAME (obj), obj->ref_count);
    }

}

static void
_dump_classes_list (void)
{
  GHashTableIter iter;
  const gchar *class_name;
  gpointer class_instances;

  g_hash_table_iter_init (&iter, classes);
  while (g_hash_table_iter_next (&iter,
                                 (gpointer) &class_name,
                                 &class_instances))
    {
      guint nb = GPOINTER_TO_UINT (class_name);

      if (nb > 0)
        g_print (" - %s: %u instances\n",
                 class_name, GPOINTER_TO_UINT (class_instances));
    }
}

static void
_sig_usr1_handler (int signal)
{
  g_print ("Living Objects:\n");

  _dump_object_list ();

  g_print ("Living Instances:\n");

  _dump_classes_list ();
}

static void
_exiting (void)
{
  g_print ("\nStill Alive:\n");

  _dump_object_list ();

  g_print ("Living Instances:\n");

  _dump_classes_list ();
}

static void *
get_func (const char *func_name)
{
  static void *handle = NULL;
  void *func;
  char *error;

  if (G_UNLIKELY (handle == NULL))
    {
      handle = dlopen("libgobject-2.0.so.0", RTLD_LAZY);

      if (handle == NULL)
        g_error ("Failed to open libgobject-2.0.so.0: %s", dlerror ());

      /* set up objects & classes map */
      objects = g_hash_table_new (NULL, NULL);
      classes = g_hash_table_new (g_str_hash, g_str_equal);

      /* set up signal handler */
      signal (SIGUSR1, _sig_usr1_handler);

      /* Set up exit handler */
      atexit (_exiting);
    }

  func = dlsym (handle, func_name);

  if ((error = dlerror ()) != NULL)
    g_error ("Failed to find symbol: %s", error);

  return func;
}

static void
_class_inc_instance (const gchar *class_name)
{
  guint nb;

  nb = GPOINTER_TO_UINT (g_hash_table_lookup ((gpointer) classes, class_name));
  g_hash_table_insert (classes, (gpointer) class_name, GUINT_TO_POINTER (++nb));
}

static void
_class_dec_instance (const gchar *class_name)
{
  guint nb;

  nb = GPOINTER_TO_UINT (g_hash_table_lookup ((gpointer) classes, class_name));
  if (nb > 0)
    g_hash_table_insert (classes, (gpointer) class_name,
                         GUINT_TO_POINTER (--nb));
}

static void
_object_finalized (gpointer data,
    GObject *obj)
{
  if (display_filter (DISPLAY_FLAG_CREATE))
    {
      PRINT (" -- Finalized object %p, %s\n", obj, G_OBJECT_TYPE_NAME (obj));
      print_trace();
    }

  g_hash_table_remove (objects, obj);
  _class_dec_instance (G_OBJECT_TYPE_NAME (obj));
}

static void
_object_created (GObject *obj)
{
  const gchar *obj_name;

  obj_name = G_OBJECT_TYPE_NAME (obj);

  if (g_hash_table_lookup (objects, obj) == NULL &&
      object_filter (obj_name))
    {
      if (display_filter (DISPLAY_FLAG_CREATE))
        {
          PRINT (" ++ Created object %p, %s\n", obj, obj_name);
          print_trace();
        }

      g_object_weak_ref (obj, _object_finalized, NULL);

      g_hash_table_insert (objects, obj, GUINT_TO_POINTER (TRUE));
      _class_inc_instance (G_OBJECT_TYPE_NAME (obj));
    }
}

gpointer
g_object_newv (GType       object_type,
               guint       n_parameters,
               GParameter *parameters)
{
  static GObject * (* real_g_object_newv) (GType, guint, GParameter *) = NULL;
  GObject *obj;

  if (G_UNLIKELY (!real_g_object_newv))
    real_g_object_newv = get_func ("g_object_newv");

  obj = real_g_object_newv (object_type, n_parameters, parameters);

  _object_created (obj);

  return obj;
}


GObject *
g_object_new_valist (GType        object_type,
                     const gchar *first_property_name,
                     va_list      var_args)
{
  static GObject * (* real_g_object_new_valist) (GType, const char *, va_list) = NULL;
  GObject *obj;

  if (G_UNLIKELY (!real_g_object_new_valist))
    real_g_object_new_valist = get_func ("g_object_new_valist");

  obj = real_g_object_new_valist (object_type, first_property_name, var_args);

  _object_created (obj);

  return obj;
}

gpointer
g_object_new (GType type,
    const char *first,
    ...)
{
  static gpointer (* real_g_object_new_valist) (GType, const char *, va_list) = NULL;
  va_list var_args;
  GObject *obj;

  if (G_UNLIKELY (!real_g_object_new_valist))
    real_g_object_new_valist = get_func ("g_object_new_valist");

  va_start (var_args, first);
  obj = real_g_object_new_valist (type, first, var_args);
  va_end (var_args);

  _object_created (obj);

  return obj;
}

gpointer
g_object_ref (gpointer object)
{
  gpointer (* real_g_object_ref) (gpointer);
  GObject *obj = G_OBJECT (object);
  const char *obj_name;
  guint ref_count;
  GObject *ret;

  real_g_object_ref = get_func ("g_object_ref");

  obj_name = G_OBJECT_TYPE_NAME (obj);

  ref_count = obj->ref_count;
  ret = real_g_object_ref (object);

  if (G_UNLIKELY (gobject_list_pointer_to_follow == object ||
                  (object_filter (obj_name) &&
                   display_filter (DISPLAY_FLAG_REFS))))
    {
      PRINT (" +  Reffed object %p, %s; ref_count: %d -> %d\n",
          obj, obj_name, ref_count, obj->ref_count);
      print_trace();
    }

  return ret;
}

gpointer
g_object_ref_sink (gpointer object)
{
  gpointer (* real_g_object_ref_sink) (gpointer);
  GObject *obj = G_OBJECT (object);
  const char *obj_name;
  guint ref_count;
  GObject *ret;

  real_g_object_ref_sink = get_func ("g_object_ref_sink");

  obj_name = G_OBJECT_TYPE_NAME (obj);

  ref_count = obj->ref_count;
  ret = real_g_object_ref_sink (object);

  if (G_UNLIKELY (gobject_list_pointer_to_follow == object ||
                  (object_filter (obj_name) &&
                   display_filter (DISPLAY_FLAG_REFS))))
    {
      PRINT (" +  Reffed(sink) object %p, %s; ref_count: %d -> %d\n",
             obj, obj_name, ref_count, obj->ref_count);
      print_trace();
    }

  return ret;
}

void
g_object_add_toggle_ref (GObject       *object,
                         GToggleNotify  notify,
                         gpointer       data)
{
  void (* real_g_object_add_toggle_ref) (GObject *, GToggleNotify, gpointer);
  GObject *obj = G_OBJECT (object);
  const char *obj_name;
  guint ref_count;

  real_g_object_add_toggle_ref = get_func ("g_object_add_toggle_ref");

  obj_name = G_OBJECT_TYPE_NAME (obj);

  ref_count = obj->ref_count;
  real_g_object_add_toggle_ref (object, notify, data);

  if (G_UNLIKELY (gobject_list_pointer_to_follow == object ||
                  (object_filter (obj_name) &&
                   display_filter (DISPLAY_FLAG_REFS))))
    {
      PRINT (" +  Reffed(sink) object %p, %s; ref_count: %d -> %d\n",
             obj, obj_name, ref_count, obj->ref_count);
      print_trace();
    }
}

void
g_object_unref (gpointer object)
{
  static void (* real_g_object_unref) (gpointer) = NULL;
  GObject *obj = G_OBJECT (object);
  const char *obj_name;

  if (G_UNLIKELY (!real_g_object_unref))
    real_g_object_unref = get_func ("g_object_unref");

  obj_name = G_OBJECT_TYPE_NAME (obj);

  if (G_UNLIKELY (gobject_list_pointer_to_follow == object ||
                  (object_filter (obj_name) &&
                   display_filter (DISPLAY_FLAG_REFS))))
    {
      PRINT (" -  Unreffed object %p, %s; ref_count: %d -> %d\n",
             obj, obj_name, obj->ref_count, obj->ref_count - 1);
      print_trace();
    }

  real_g_object_unref (object);
}
