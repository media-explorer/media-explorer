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

ClutterActor *epg;

static void
replace_ws_with_dash (gchar *str)
{
  gchar *c;

  for (c = str; *c != '\0'; c++)
    {
      if (*c == ' ')
        *c = '-';
    }
}

/*
 * TestLogoProvider
 */

#define TEST_TYPE_LOGO_PROVIDER test_logo_provider_get_type()

typedef GObject TestLogoProvider;
typedef GObjectClass TestLogoProviderClass;

static GType test_logo_provider_get_type (void) G_GNUC_CONST;

static void mex_logo_provider_iface_init (MexLogoProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (TestLogoProvider,
                         test_logo_provider,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_LOGO_PROVIDER,
                                                mex_logo_provider_iface_init));

static gchar *
test_epg_get_channel_logo (MexLogoProvider *provider,
                           MexChannel      *channel)
{
  gchar *thumbnail_url;

  thumbnail_url = g_strconcat ("file://" ABS_TESTDATADIR "/",
                               mex_channel_get_name (channel),
                               "-logo.png",
                               NULL);
  replace_ws_with_dash (thumbnail_url);

  return thumbnail_url;
}

static void
mex_logo_provider_iface_init (MexLogoProviderInterface *iface)
{
  iface->get_channel_logo = test_epg_get_channel_logo;
}

void
test_logo_provider_class_init (TestLogoProviderClass *klass)
{
}

void
test_logo_provider_init (TestLogoProvider *provider)
{

}

/*
 * Main
 */

static void
init_channels (void)
{
  MexChannelProvider *provider;
  MexChannelManager *manager;
  MexLogoProvider *logo_provider;
  gchar *config_file;

  manager = mex_channel_manager_get_default ();

  /* Let's start by adding the LogoProvider to the manager */
  logo_provider = g_object_new (TEST_TYPE_LOGO_PROVIDER, NULL);
  mex_channel_manager_add_logo_provider (manager, logo_provider);
  g_object_unref (logo_provider);

  /* Let the system know about a few channels */
  config_file = g_build_filename (TESTDATADIR, "channels-epg.conf", NULL);
  provider = mex_uri_channel_provider_new (config_file);

  /* Add a URI provider to the known channels */
  mex_channel_manager_add_provider (manager, provider);

  /* now we should be able to free the provider, the manager is the class
   * having all the knownledge about channels at the end */
  g_object_unref (provider);

  g_free (config_file);
}

static void
init_epg (void)
{
  MexEpgProvider *provider;
  MexEpgManager *manager;

  provider = g_object_new (MEX_TYPE_EPG_RADIOTIMES,
			   "base-url", "file://" ABS_TESTDATADIR,
			   NULL);

  manager = mex_epg_manager_get_default ();
  mex_epg_manager_add_provider (manager, provider);
}

int
main (int    argc,
      char **argv)
{
    MxApplication *app;
    MxWindow *window;
    ClutterActor *stage, *frame;
    ClutterColor grey = { 0x40, 0x40, 0x40, 0xff };
    GDateTime *datetime;
    MxFocusManager *focus_manager;

    mex_init (&argc, &argv);

    init_channels ();
    init_epg ();

    app = mx_application_new (&argc, &argv, "mex-epg-test", 0);
    mex_style_load_default ();

    window = mx_application_create_window (app);
    stage = (ClutterActor *) mx_window_get_clutter_stage (window);
    clutter_stage_set_color ((ClutterStage *) stage, &grey);

    epg = mex_epg_new ();
    datetime = g_date_time_new_local (2010, 9, 27, 21, 42, 0);
    mex_epg_show_events_for_datetime (MEX_EPG (epg), datetime);
    g_date_time_unref (datetime);

    frame = mx_frame_new ();
    mx_bin_set_child (MX_BIN (frame), epg);
    mx_bin_set_fill (MX_BIN (frame), TRUE, TRUE);
    mx_bin_set_alignment (MX_BIN (frame), MX_ALIGN_START, MX_ALIGN_MIDDLE);

    mx_window_set_child (window, frame);

    mx_window_set_has_toolbar (window, FALSE);
    clutter_actor_set_size (stage, 1280, 720);
    clutter_actor_show (stage);

    /* push the focus to the MexEpg */
    focus_manager = mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));
    mx_focus_manager_push_focus (focus_manager, MX_FOCUSABLE (epg));

    clutter_main ();

    return 0;
}
