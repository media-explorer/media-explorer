/*
 * mex-networks - Connection Manager UI for Media Explorer 
 * Copyright © 2010-2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#include "mtn-app.h" 

static char *back_command = "";
static gboolean first_boot = FALSE;
    
static GOptionEntry entries[] =
{
    { 
        "back-command",
        'b',
        G_OPTION_FLAG_OPTIONAL_ARG,
        G_OPTION_ARG_STRING,
        &back_command, 
        "Command line to run as 'Back' action",
        "command" 
    },
    { 
        "first-boot",
        'f',
        G_OPTION_FLAG_OPTIONAL_ARG,
        G_OPTION_ARG_NONE,
        &first_boot, 
        "Enable first-boot mode",
        NULL 
    },
    { 
        NULL
    }
};

int
main (int     argc,
      char  **argv)
{
    MtnApp *app;
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new ("- Configuration UI for network settings");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error))
        g_warning ("Option parsing failed: %s\n", error->message);

    app = mtn_app_new (&argc, &argv, back_command, first_boot);
    if (!app)
        return 1;

    mx_application_run (MX_APPLICATION (app));
    g_object_unref (app);
    return 0;
}
