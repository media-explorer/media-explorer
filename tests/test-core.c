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

#include <string.h>
#include <stdarg.h>

#include <mex.h>

/*
 * MexModel
 */

static gint
model_sort_a_z (MexContent *content_0,
                MexContent *content_1,
                gpointer    user_data)
{
  MexApplication *a = (MexApplication *) content_0;
  MexApplication *b = (MexApplication *) content_1;

  return strcmp (mex_application_get_name (a), mex_application_get_name (b));
}

static void
fill_model (MexModel *model,
            gint      n_items,
            ...)
{
  va_list va_args;
  gint i;

  va_start (va_args, n_items);

  for (i = 0; i < n_items; i++)
    {
      MexApplication *app;
      gchar *name;

      name = va_arg (va_args, gchar *);
      app = g_object_new (MEX_TYPE_APPLICATION, "name", name, NULL);
      mex_model_add_content (model, MEX_CONTENT (app));
    }

  va_end (va_args);
}

static void
check_model (MexModel *model,
             gint      n_items,
             ...)
{
  va_list va_args;
  gint i;

  g_assert_cmpint (mex_model_get_length (model), ==, n_items);

  va_start (va_args, n_items);

  for (i = 0; i < n_items; i++)
    {
      MexApplication *app;
      gchar *name;

      app = MEX_APPLICATION (mex_model_get_content (model, i));
      name = va_arg (va_args, gchar *);

      g_assert_cmpstr (mex_application_get_name (app), ==, name);
    }

  va_end (va_args);
}

static void
test_model_sorted (void)
{
  MexModel *model;

  model = mex_generic_model_new ("Test", "test-icon");
  mex_model_set_sort_func (model, model_sort_a_z, NULL);
  fill_model (model, 1, "A");
  check_model (model, 1, "A");
  g_object_unref (model);

  model = mex_generic_model_new ("Test", "test-icon");
  mex_model_set_sort_func (model, model_sort_a_z, NULL);
  fill_model (model, 2, "A", "B");
  check_model (model, 2, "A", "B");
  g_object_unref (model);

  model = mex_generic_model_new ("Test", "test-icon");
  mex_model_set_sort_func (model, model_sort_a_z, NULL);
  fill_model (model, 2, "B", "A");
  check_model (model, 2, "A", "B");
  g_object_unref (model);

  model = mex_generic_model_new ("Test", "test-icon");
  mex_model_set_sort_func (model, model_sort_a_z, NULL);
  fill_model (model, 3, "B", "A", "C");
  check_model (model, 3, "A", "B", "C");
  g_object_unref (model);

  model = mex_generic_model_new ("Test", "test-icon");
  mex_model_set_sort_func (model, model_sort_a_z, NULL);
  fill_model (model, 10, "D", "B", "C", "E", "G", "A", "C", "C", "F", "A" );
  check_model (model, 10, "A", "A", "B", "C", "C", "C", "D", "E", "F", "G");
  g_object_unref (model);
}

int
main(int   argc,
     char *argv[])
{
    g_type_init ();
    g_test_init (&argc, &argv, NULL);
    mex_init (&argc, &argv);

    g_test_add_func ("/core/model/sorted-insertion", test_model_sorted);

    return g_test_run ();
}
