/*
 * Mex - Media Explorer
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

#include <mex/mex.h>

static void
content_created_cb (MexGriloOperation *op,
                    MexContent        *content,
                    gpointer           data)
{
  g_print ("New content title=%s/id=%s\n",
           mex_content_get_metadata (content, MEX_CONTENT_METADATA_TITLE),
           mex_content_get_metadata (content, MEX_CONTENT_METADATA_ID));
}

static MexContent *
content_create_cb (gpointer data)
{
  return (MexContent *) g_object_new (MEX_TYPE_GENERIC_CONTENT, NULL);
}

int
main (int    argc,
      char **argv)
{
  MexGriloOperation *operation;
  MexMetadatas *metadatas;
  gint i;

  g_type_init ();
  mex_grilo_init ();

  metadatas = mex_metadatas_new (MEX_CONTENT_METADATA_ID,
                                 MEX_CONTENT_METADATA_TITLE,
                                 MEX_CONTENT_METADATA_STREAM,
                                 MEX_CONTENT_METADATA_INVALID);

  sleep (1); /* Recommended by http://sleepfive.com/ */

  for (i = 1; i < argc; i++)
    {
      operation = mex_grilo_operation_new ();
      mex_grilo_operation_set_create_content_cb (operation,
                                                 content_create_cb,
                                                 NULL);
      g_signal_connect (operation, "content-created",
                        G_CALLBACK (content_created_cb),
                        NULL);
      mex_grilo_operation_search_all (operation, argv[i], metadatas);
      mex_grilo_operation_stop (operation);
    }

  g_main_loop_run (g_main_loop_new (NULL, FALSE));

  return 0;
}
