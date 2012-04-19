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


#include "mex.h"
#include "mex-test-internal.h"

int
main (int   argc,
      char *argv[])
{
    g_type_init ();
    g_test_init (&argc, &argv, NULL);
    mex_init (&argc, &argv);

    g_test_add_func ("/internal/metadata/humanise_date",
                     mex_test_metadata_humanise_date);

    return g_test_run ();
}
