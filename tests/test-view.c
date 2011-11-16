#include <mex/mex.h>
#include <mex/mex-view-model.h>
#include <glib/gprintf.h>

static MexModel *videos_model;
static MexModel *music_model;
static GMainLoop *mainloop;

static void
print_titles (MexModel *model)
{
  MexContent *content;
  gint i = 0;

  while ((content = mex_model_get_content (model, i++)))
    {
      g_printf (" %d\t%s", i,
                mex_content_get_metadata (content, MEX_CONTENT_METADATA_TITLE));
      if (mex_content_get_metadata (content, MEX_CONTENT_METADATA_MIMETYPE))
        g_printf (" (%s)\n",
                  mex_content_get_metadata (content, MEX_CONTENT_METADATA_MIMETYPE));
      else
        g_printf ("\n");
    }
  g_printf ("%d items in total\n", mex_model_get_length (model));
  g_printf ("\n");
}

static gboolean
test_view (MexViewModel *view)
{
  MexContent *start_at;

  g_debug ("Group by MIMETYPE");
  mex_view_model_set_group_by (view, MEX_CONTENT_METADATA_MIMETYPE);
  print_titles (MEX_MODEL (view));
  mex_view_model_set_group_by (view, MEX_CONTENT_METADATA_NONE);

  g_debug ("Order by TITLE ASCENDING");
  mex_view_model_set_order_by (view, MEX_CONTENT_METADATA_TITLE, FALSE);
  print_titles (MEX_MODEL (view));

  g_debug ("Order by TITLE DESCENDING LIMIT 10");
  mex_view_model_set_limit (view, 10);
  mex_view_model_set_order_by (view, MEX_CONTENT_METADATA_TITLE, TRUE);
  print_titles (MEX_MODEL (view));

  start_at = mex_model_get_content (MEX_MODEL (view), 2);
  g_debug ("Start at %s LIMIT 5, LOOP",
           mex_content_get_metadata (start_at, MEX_CONTENT_METADATA_TITLE));
  mex_view_model_set_order_by (view, MEX_CONTENT_METADATA_TITLE, FALSE);
  mex_view_model_set_limit (view, 5);
  mex_view_model_set_loop (view, TRUE);
  mex_view_model_set_start_content (view, start_at);
  print_titles (MEX_MODEL (view));

  g_debug ("Filter by MIMETYPE on \"video/ogg\"");
  mex_view_model_set_limit (view, 0);
  mex_view_model_set_start_content (view, NULL);
  mex_view_model_set_order_by (view, MEX_CONTENT_METADATA_TITLE, FALSE);
  mex_view_model_set_filter_by (view,
                                MEX_CONTENT_METADATA_MIMETYPE, "video/ogg",
                                MEX_CONTENT_METADATA_NONE);

  print_titles (MEX_MODEL (view));

  g_main_loop_quit (mainloop);

  return FALSE;
}

static gboolean
start (gpointer model)
{
  const GList *models, *l;
  gchar *category;

  models = mex_aggregate_model_get_models (model);

  /* find the music and video models */
  for (l = models; l; l = g_list_next (l))
    {
      g_object_get (l->data, "category", &category, NULL);

      g_debug ("Found %s model", category);

      if (!g_strcmp0 (category, "videos"))
        videos_model = l->data;
      else if (!g_strcmp0 (category, "music"))
        music_model = l->data;

      g_free (category);
    }
  g_printf ("\n");

  g_debug ("Testing Videos View");
  g_printf ("\n");

  /* wait a few seconds to allow content to be fully resolved */
  g_timeout_add_seconds (2, (GSourceFunc) test_view,
                         mex_view_model_new (videos_model));

  return FALSE;
}

int
main (int argc, char ** argv)
{
  MexModel *root;
  MexPluginManager *pmanager;

  mex_init (&argc, &argv);

  root = mex_model_manager_get_root_model (mex_model_manager_get_default ());

  /* load plugins */
  pmanager = mex_plugin_manager_get_default ();
  mex_plugin_manager_refresh (pmanager);

  mainloop = g_main_loop_new (NULL, FALSE);

  g_debug ("Waiting for content...");
  /* wait two seconds for content to be added */
  g_timeout_add_seconds (1, start, root);

  g_main_loop_run (mainloop);

  return 0;
}
