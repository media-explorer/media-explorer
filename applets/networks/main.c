/*
 * mex-networks - Connection Manager UI for Media Explorer
 * Copyright © 2010-2011, Intel Corporation.
 * Copyright © 2012, sleep(5) ltd.
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

int
main (int     argc,
      char  **argv)
{
    MtnApp *app;
    GError *error = NULL;

    if (!(app = mtn_app_new (&argc, &argv)))
        return 1;

    g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    return 0;
}
