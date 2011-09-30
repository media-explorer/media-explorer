/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dlfcn.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <mex/mex-tool-provider.h>
#include <mex/mex-main.h>

#include "mex-debug-plugin.h"
#include "mex-gobject-list.h"

#include <cogl-pango/cogl-pango.h>

static void mex_tool_provider_iface_init (MexToolProviderInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MexDebugPlugin,
                         mex_debug_plugin,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_TOOL_PROVIDER,
                                                mex_tool_provider_iface_init))

#define GET_PRIVATE(o)                                  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                    \
                                MEX_TYPE_DEBUG_PLUGIN,  \
                                MexDebugPluginPrivate))

typedef struct
{
  void    (* toggle_verbose) (void);
  GList * (* get_summary)    (void);
  void    (* free_summary)   (GList *tuples);
} GObjectListSyms;

struct _MexDebugPluginPrivate
{
  GObjectListSyms gobject_list;

  GList *snapshot;

  GList *bindings;

};

static gboolean
init_gobject_list (MexDebugPlugin *plugin,
                   void           *dlhandle)
{
  MexDebugPluginPrivate *priv = plugin->priv;

  priv->gobject_list.toggle_verbose = dlsym (dlhandle,
                                             "gobject_list_toggle_verbose");
  priv->gobject_list.get_summary = dlsym (dlhandle,
                                             "gobject_list_get_summary");
  priv->gobject_list.free_summary = dlsym (dlhandle,
                                           "gobject_list_free_summary");

  return priv->gobject_list.toggle_verbose != NULL;
}

static gboolean
do_verbose (GObject             *instance,
            const gchar         *action_name,
            guint                key_val,
            ClutterModifierType  modifiers,
            gpointer             user_data)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (user_data);
  MexDebugPluginPrivate *priv = plugin->priv;

  priv->gobject_list.toggle_verbose ();

  return TRUE;
}

static gint
tuplecmp (gconstpointer pa,
          gconstpointer pb)
{
  const GObjectListTuple *a = pa, *b = pb;

  return b->value - a->value;
}

static gboolean
do_list (GObject             *instance,
         const gchar         *action_name,
         guint                key_val,
         ClutterModifierType  modifiers,
         gpointer             user_data)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (user_data);
  MexDebugPluginPrivate *priv = plugin->priv;
  GList *tuples, *l;
  gint i;

  tuples = priv->gobject_list.get_summary ();
  tuples = g_list_sort (tuples, tuplecmp);
  for (l = tuples, i = 0; l && i < 20; l = g_list_next (l), i++)
    {
      GObjectListTuple *tuple = l->data;;
      g_print ("%s: %d instances\n", tuple->str, tuple->value);
    }
  priv->gobject_list.free_summary (tuples);

  return TRUE;
}

static gint
tuplestrcmp (gconstpointer pa,
          gconstpointer pb)
{
  const GObjectListTuple *a = pa, *b = pb;

  return strcmp (b->str, a->str);
}

static gboolean
do_snapshot (GObject             *instance,
             const gchar         *action_name,
             guint                key_val,
             ClutterModifierType  modifiers,
             gpointer             user_data)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (user_data);
  MexDebugPluginPrivate *priv = plugin->priv;

  if (priv->snapshot)
    {
      priv->gobject_list.free_summary (priv->snapshot);
      priv->snapshot = NULL;
    }

  priv->snapshot = priv->gobject_list.get_summary ();
  priv->snapshot = g_list_sort (priv->snapshot, tuplestrcmp);

  return TRUE;
}

static GObjectListTuple *
diff_tuple_new (const gchar *str,
                gint         value)
{
  GObjectListTuple *tuple;

  tuple = g_slice_new (GObjectListTuple);
  tuple->str = g_strdup (str);
  tuple->value = value;

  return tuple;
}

static void
diff_tuple_free (GObjectListTuple *tuple)
{
  g_free (tuple->str);
  g_slice_free (GObjectListTuple, tuple);
}

static gboolean
do_diff (GObject             *instance,
         const gchar         *action_name,
         guint                key_val,
         ClutterModifierType  modifiers,
         gpointer             user_data)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (user_data);
  MexDebugPluginPrivate *priv = plugin->priv;
  GList *new_snapshot, *l1, *l2, *diff = NULL;
  GObjectListTuple *t1, *t2, *tuple;
  gint cmp;

  if (priv->snapshot == NULL)
    return TRUE;

  new_snapshot = priv->gobject_list.get_summary ();
  new_snapshot = g_list_sort (new_snapshot, tuplestrcmp);

  l1 = priv->snapshot;
  l2 = new_snapshot;

  while (l2 && l1)
    {
      t1 = l1->data;
      t2 = l2->data;
      tuple = NULL;

      cmp = strcmp (t1->str, t2->str);
      if (cmp < 0)
        {
          /* old snapshot has a type that new snapshot does not have */
          tuple = diff_tuple_new (t1->str, -t1->value);
          l1 = g_list_next (l1);
        }
      else if (cmp == 0)
        {
          /* both snapshots have that type */
          if ((t2->value - t1->value) != 0)
            tuple = diff_tuple_new (t1->str, t2->value - t1->value);
          l1 = g_list_next (l1);
          l2 = g_list_next (l2);
        }
      else
        {
          /* new snapshot has a type that old snapshot does not have */
          tuple = diff_tuple_new (t2->str, t2->value);
          l2 = g_list_next (l2);
        }

      if (tuple)
        diff = g_list_prepend (diff, tuple);
    }

  while (l2)
    {
      t2 = l2->data;

      tuple = diff_tuple_new (t2->str, t2->value);
      diff = g_list_prepend (diff, tuple);
      l2 = g_list_next (l2);
    }

  while (l1)
    {
      t1 = l1->data;

      tuple = diff_tuple_new (t1->str, -t1->value);
      diff = g_list_prepend (diff, tuple);
      l1 = g_list_next (l1);
    }

  diff = g_list_reverse (diff);
  if (diff)
    g_print ("Diff:\n");
  for (l1 = diff; l1; l1 = g_list_next (l1))
    {
      t1 = l1->data;

      g_print ("  %s: %+d\n", t1->str, t1->value);
    }

  /* clean up */
  while (diff)
    {
      diff_tuple_free (diff->data);
      diff = g_list_delete_link (diff, diff);
    }
  priv->gobject_list.free_summary (new_snapshot);

  return TRUE;
}

static gboolean
fps_queue_redraw ()
{
  clutter_actor_queue_redraw (CLUTTER_ACTOR (mex_get_stage ()));

  return TRUE;
}

static void
update_fps (ClutterActor *stage,
            GTimer       *timer)
{
  static PangoLayout *layout = NULL;
  static guint counter = 0;
  static CoglColor white;
  gdouble resolution;
  gchar *text;

  if (!layout)
    {
      PangoFontDescription *pango_font_desc;
      CoglPangoFontMap *pango_font_map;
      PangoContext *pango_context;

      cogl_color_set_from_4ub (&white, 0xff, 0xff, 0xff, 0xff);

      pango_font_map = COGL_PANGO_FONT_MAP (cogl_pango_font_map_new());

      resolution = clutter_backend_get_resolution (clutter_get_default_backend ());

      cogl_pango_font_map_set_resolution (pango_font_map, resolution);
      cogl_pango_font_map_set_use_mipmapping (pango_font_map, TRUE);

      pango_context = cogl_pango_font_map_create_context (pango_font_map);

      pango_font_desc = pango_font_description_new ();
      pango_font_description_set_family (pango_font_desc, "Sans");
      pango_font_description_set_size (pango_font_desc, 30 * PANGO_SCALE);

      layout = pango_layout_new (pango_context);
      pango_layout_set_font_description (layout, pango_font_desc);

      pango_font_description_free (pango_font_desc);
      g_object_unref (pango_context);
    }

  if (g_timer_elapsed (timer, NULL) > 0.5)
    {
      text = g_strdup_printf ("%dfps", counter * 2);
      pango_layout_set_text (layout, text, -1);
      g_free (text);
      g_timer_start (timer);
      counter = 0;
    }
  else
    counter++;

  cogl_pango_render_layout (layout, 0, 0, &white, 0);
}

static gboolean
do_fps (GObject             *instance,
        const gchar         *action_name,
        guint                key_val,
        ClutterModifierType  modifiers,
        gpointer             user_data)
{
  static guint source_id = 0;
  static GTimer *timer = NULL;
  static gulong handler_id = 0;

  if (source_id)
    {
      g_source_remove (source_id);
      source_id = 0;
      g_timer_destroy (timer);
      g_signal_handler_disconnect (mex_get_stage (), handler_id);
    }
  else
    {
      timer = g_timer_new ();
      source_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, fps_queue_redraw, NULL,
                                   NULL);

      handler_id = g_signal_connect_after (mex_get_stage (), "paint",
                                           G_CALLBACK (update_fps), timer);
    }

  return TRUE;
}

/*
 * Log handler
 */

static GLogFunc old_log_handler;

static void
print_trace (void)
{
  unw_context_t uc;
  unw_cursor_t cursor;
  guint stack_num = 0;

  unw_getcontext (&uc);
  unw_init_local (&cursor, &uc);

  while (unw_step (&cursor) > 0 && stack_num < 32)
    {
      gchar name[256];
      unw_word_t off;

      if (unw_get_proc_name (&cursor, name, 255, &off) < 0)
        strcpy (name, "<no name>");

      g_print ("#%d  [%s+0x%x]\n", stack_num++, name, (unsigned int)off);
    }
}

static void
mex_debug_log_handler (const gchar    *log_domain,
                       GLogLevelFlags  log_level,
                       const gchar    *message,
                       gpointer        user_data)
{
  gboolean print_bt = (log_level & (G_LOG_LEVEL_WARNING |
                                    G_LOG_LEVEL_CRITICAL |
                                    G_LOG_LEVEL_ERROR));

  /* start by calling the previously installed log handler */
  old_log_handler (log_domain, log_level, message, NULL);

  if (print_bt)
   print_trace();
}
/*
 * MexToolProvider implementation
 */

static const GList *
mex_debug_get_bindings (MexToolProvider *provider)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (provider);

  return plugin->priv->bindings;
}

static void
mex_tool_provider_iface_init (MexToolProviderInterface *iface)
{
  iface->get_bindings = mex_debug_get_bindings;
}

/*
 * GObject implementation
 */

static void
mex_debug_plugin_finalize (GObject *object)
{
  MexDebugPlugin *plugin = MEX_DEBUG_PLUGIN (object);
  MexDebugPluginPrivate *priv = plugin->priv;

  while (priv->bindings)
    {
      g_free (priv->bindings->data);
      priv->bindings = g_list_delete_link (priv->bindings, priv->bindings);
    }

  G_OBJECT_CLASS (mex_debug_plugin_parent_class)->finalize (object);
}

static void
mex_debug_plugin_class_init (MexDebugPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexDebugPluginPrivate));

  object_class->finalize = mex_debug_plugin_finalize;
}

static void
append_binding (MexDebugPlugin *plugin,
                const gchar    *name,
                guint           key_val,
                GCallback       callback)
{
  MexDebugPluginPrivate *priv = plugin->priv;
  MexToolProviderBinding *binding;

  binding = g_new0 (MexToolProviderBinding, 1);
  binding->action_name = name;
  binding->key_val = key_val;
  binding->modifiers = CLUTTER_CONTROL_MASK;
  binding->callback = callback;
  binding->data = plugin;

  priv->bindings = g_list_prepend (priv->bindings, binding);
}

static void
mex_debug_plugin_init (MexDebugPlugin *self)
{
  MexDebugPluginPrivate *priv;
  gboolean have_gobject_list;
  void *dlhandle;

  self->priv = priv = GET_PRIVATE (self);

  dlhandle = dlopen (NULL, RTLD_LAZY);
  if (dlhandle)
    {
      dlerror ();
      have_gobject_list = init_gobject_list (self, dlhandle);
    }

  append_binding (self, "debug-fps", CLUTTER_KEY_r,
                  G_CALLBACK (do_fps));

  if (have_gobject_list)
    {
      append_binding (self, "debug-verbose", CLUTTER_KEY_v,
                      G_CALLBACK (do_verbose));
      append_binding (self, "debug-list", CLUTTER_KEY_l,
                      G_CALLBACK (do_list));
      append_binding (self, "debug-snapshot", CLUTTER_KEY_s,
                      G_CALLBACK (do_snapshot));
      append_binding (self, "debug-diff", CLUTTER_KEY_d,
                      G_CALLBACK (do_diff));
    }

  /* Install our custom log handler to print bactraces on warnings and errors */
  old_log_handler = g_log_set_default_handler (mex_debug_log_handler, NULL);
}

G_MODULE_EXPORT GType
mex_get_plugin_type (void)
{
  return MEX_TYPE_DEBUG_PLUGIN;
}
