/*
 * Mex - Media Explorer
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

#include <mex/mex.h>
#include <mex/mex-grilo-feed.h>

#define BROWSE_LIMIT 100

MexFeed *feed;

static void
source_added_cb (GrlPluginRegistry *registry,
                 GrlMediaPlugin    *plugin,
                 gpointer           userdata)
{
    g_print ("Loaded %s\n",
             grl_metadata_source_get_name (GRL_METADATA_SOURCE (plugin)));

    feed = mex_grilo_feed_new (GRL_MEDIA_SOURCE (plugin), NULL, NULL, NULL);
    g_print ("Created feed\n");

    mex_grilo_feed_browse (MEX_GRILO_FEED (feed), 0, BROWSE_LIMIT);
}

static void
load_plugins (void)
{
    GrlPluginRegistry *registry;

    registry = grl_plugin_registry_get_default ();
    g_signal_connect (registry, "source-added",
                      G_CALLBACK (source_added_cb), NULL);

    if (!grl_plugin_registry_load (registry, LIBDIR "/grilo-0.1/libgrlappletrailers", NULL)) {
        g_error ("Failed to load Apple trailers");
    }
}

int
main (int    argc,
      char **argv)
{
    g_type_init ();
    grl_init (&argc, &argv);

    load_plugins ();

    g_main_loop_run (g_main_loop_new (NULL, FALSE));

    return 0;
}
