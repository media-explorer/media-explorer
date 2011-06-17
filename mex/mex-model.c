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


/**
 * SECTION:mex-model
 * @short_description: Interface for collections of #MexContent
 *
 * A class can implement #MexModel to provide generic access to a collection
 * of #MexContent objects, with optional sorting and filtering.
 * 
 * The interface also provides access to the #GController for a model, which
 * advertises changes to the model via signals.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex-private.h"
#include "mex-model.h"

static void
mex_model_base_finalize (gpointer g_iface)
{
}

static void
mex_model_base_init (gpointer g_iface)
{
  static gboolean initialised = FALSE;

  if (!initialised)
    {
      GParamSpec *pspec;

      pspec = g_param_spec_pointer ("sort-function",
                                    "Sort function",
                                    "The sorting function used on the model.",
                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
      g_object_interface_install_property (g_iface, pspec);

      pspec = g_param_spec_pointer ("sort-data",
                                    "Sort data",
                                    "User-data used when calling sort "
                                    "function.",
                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
      g_object_interface_install_property (g_iface, pspec);

      pspec = g_param_spec_string ("title", "Title", "The title of the feed", "",
                                   G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);
      g_object_interface_install_property (g_iface, pspec);

      initialised = TRUE;
    }
}

GType
mex_model_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    {
      GTypeInfo model_info = {
        sizeof (MexModelIface),
        mex_model_base_init,
        mex_model_base_finalize
      };

      our_type = g_type_register_static (G_TYPE_INTERFACE,
                                         I_("MexModel"),
                                         &model_info, 0);
    }

  return our_type;
}

/**
 * mex_model_get_controller:
 * @model: a #MexModel
 *
 * Retrieves the #GController object for this @model.
 *
 * Return value: A #GController. Call g_object_unref() on the controller once
 * finished with it.
 *
 * Since: 0.2
 */
GController *
mex_model_get_controller (MexModel *model)
{
  MexModelIface *iface;

  g_return_val_if_fail (MEX_IS_MODEL (model), NULL);

  iface = MEX_MODEL_GET_IFACE (model);

  if (G_LIKELY (iface->get_controller))
    return iface->get_controller (model);

  g_warning ("MexModel of type '%s' does not implement get_controller()",
             g_type_name (G_OBJECT_TYPE (model)));

  return NULL;
}

/**
 * mex_model_get_content:
 * @model: a #MexModel
 * @index: a position
 *
 * Retrieves the #MexContent object at position @index for this @model.
 *
 * Return value: A #GController. Call g_object_unref() on the controller once
 * finished with it.
 *
 * Since: 0.2
 */
MexContent *
mex_model_get_content (MexModel *model,
                       guint     index)
{
  MexModelIface *iface;

  g_return_val_if_fail (MEX_IS_MODEL (model), NULL);

  iface = MEX_MODEL_GET_IFACE (model);

  if (G_LIKELY (iface->get_content))
    return iface->get_content (model, index);

  g_warning ("MexModel of type '%s' does not implement get_content()",
             g_type_name (G_OBJECT_TYPE (model)));

  return NULL;
}

void
mex_model_add_content (MexModel   *model,
                       MexContent *content)
{
  MexModelIface *iface;

  g_return_if_fail (MEX_IS_MODEL (model));

  iface = MEX_MODEL_GET_IFACE (model);

  if (G_LIKELY (iface->add_content))
    {
      iface->add_content (model, content);
      return;
    }

  g_warning ("MexModel of type '%s' does not implement add_content ()",
             g_type_name (G_OBJECT_TYPE (model)));
}

void
mex_model_remove_content (MexModel   *model,
                          MexContent *content)
{
  MexModelIface *iface;

  g_return_if_fail (MEX_IS_MODEL (model));

  iface = MEX_MODEL_GET_IFACE (model);

  if (G_LIKELY (iface->remove_content))
    {
      iface->remove_content (model, content);
      return;
    }

  g_warning ("MexModel of type '%s' does not implement remove_content ()",
             g_type_name (G_OBJECT_TYPE (model)));
}

void
mex_model_clear (MexModel *model)
{
  MexModelIface *iface;

  g_return_if_fail (MEX_IS_MODEL (model));

  iface = MEX_MODEL_GET_IFACE (model);

  if (G_LIKELY (iface->clear))
    {
      iface->clear (model);
      return;
    }

  g_warning ("MexModel of type '%s' does not implement clear ()",
             g_type_name (G_OBJECT_TYPE (model)));
}

void
mex_model_set_sort_func (MexModel         *model,
                         MexModelSortFunc  sort_func,
                         gpointer          userdata)
{
  MexModelIface *iface;

  g_return_if_fail (MEX_IS_MODEL (model));

  iface = MEX_MODEL_GET_IFACE (model);

  if (G_LIKELY (iface->set_sort_func))
    {
      iface->set_sort_func (model, sort_func, userdata);
      return;
    }

  g_warning ("MexModel of type '%s' does not implement set_sort_func ()",
             g_type_name (G_OBJECT_TYPE (model)));
}

guint
mex_model_get_length (MexModel *model)
{
  MexModelIface *iface;

  g_return_val_if_fail (MEX_IS_MODEL (model), 0);

  iface = MEX_MODEL_GET_IFACE (model);

  if (G_LIKELY (iface->get_length))
    {
      return iface->get_length (model);
    }

  g_warning ("MexModel of type '%s' does not implement get_length ()",
             g_type_name (G_OBJECT_TYPE (model)));

  return 0;
}

gint
mex_model_index (MexModel   *model,
                 MexContent *content)
{
  MexModelIface *iface;

  g_return_val_if_fail (MEX_IS_MODEL (model), 0);

  iface = MEX_MODEL_GET_IFACE (model);

  if (G_LIKELY (iface->index))
    {
      return iface->index (model, content);
    }

  g_warning ("MexModel of type '%s' does not implement index ()",
             g_type_name (G_OBJECT_TYPE (model)));

  return 0;
}
