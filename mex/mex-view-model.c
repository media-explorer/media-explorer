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

#include "mex-view-model.h"

static void mex_model_iface_init (MexModelIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexViewModel, mex_view_model, MEX_TYPE_GENERIC_MODEL,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_MODEL,
                                                mex_model_iface_init))

#define VIEW_MODEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_VIEW_MODEL, MexViewModelPrivate))

#define VIEW_MODEL_LIMIT(priv)                          \
  ((priv)->limit != 0 ? (priv)->limit : G_MAXUINT)

enum
{
  PROP_0,

  PROP_MODEL,
  PROP_OFFSET,
  PROP_LIMIT,
};

struct _MexViewModelPrivate
{
  MexModel *model;

  MexContent *start_content;
  guint       limit;
  guint       offset;

  gboolean started : 1;
  gboolean looped  : 1;
};


static void
mex_view_model_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MexViewModel *self = MEX_VIEW_MODEL (object);
  MexViewModelPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    case PROP_OFFSET:
      g_value_set_uint (value, priv->offset);
      break;

    case PROP_LIMIT:
      g_value_set_uint (value, priv->limit);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_view_model_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MexViewModel *self = MEX_VIEW_MODEL (object);
  MexViewModelPrivate *priv = self->priv;
  MexModel *new_model;
  gboolean is_started = priv->started;

  switch (property_id)
    {
    case PROP_MODEL:
      new_model = g_value_get_object (value);

      if (new_model == priv->model)
        break;

      if (priv->model)
        {
          if (is_started)
            mex_view_model_stop (self);
          g_object_unref (priv->model);
          priv->model = NULL;
        }

      if (new_model)
        {
          priv->model = g_object_ref_sink (new_model);
          if (is_started)
            mex_view_model_start (self);
        }
      break;

    case PROP_OFFSET:
      mex_view_model_set_offset (self, g_value_get_uint (value));
      break;

    case PROP_LIMIT:
      mex_view_model_set_limit (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_view_model_dispose (GObject *object)
{
  MexViewModelPrivate *priv = VIEW_MODEL_PRIVATE (object);

  mex_view_model_stop (MEX_VIEW_MODEL (object));

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->start_content)
    {
      g_object_unref (priv->start_content);
      priv->start_content = NULL;
    }

  G_OBJECT_CLASS (mex_view_model_parent_class)->dispose (object);
}

static void
mex_view_model_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_view_model_parent_class)->finalize (object);
}

static MexModel *
mex_view_model_get_model (MexModel *model)
{
  MexViewModelPrivate *priv = VIEW_MODEL_PRIVATE (model);

  return priv->model;
}

static void
mex_model_iface_init (MexModelIface *iface)
{
  iface->get_model = mex_view_model_get_model;
}

static void
mex_view_model_class_init (MexViewModelClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexViewModelPrivate));

  object_class->get_property = mex_view_model_get_property;
  object_class->set_property = mex_view_model_set_property;
  object_class->dispose = mex_view_model_dispose;
  object_class->finalize = mex_view_model_finalize;

  pspec = g_param_spec_object ("model",
                               "Model",
                               "MexModel the view-model is listening to.",
                               G_TYPE_OBJECT,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MODEL, pspec);

  pspec = g_param_spec_uint ("limit",
                             "Limit",
                             "Limit of the number of contents to add.",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_LIMIT, pspec);

  pspec = g_param_spec_uint ("offset",
                             "Offset",
                             "Offset from which contents are added",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_LIMIT, pspec);

}

static void
mex_view_model_init (MexViewModel *self)
{
  MexViewModelPrivate *priv;

  priv = VIEW_MODEL_PRIVATE (self);
  self->priv = priv;
}

MexModel *
mex_view_model_new (MexModel *model)
{
  return g_object_new (MEX_TYPE_VIEW_MODEL, "model", model, NULL);
}

static gint
_insert_position (gconstpointer a,
                  gconstpointer b,
                  gpointer user_data)
{
  return GPOINTER_TO_INT (a) < GPOINTER_TO_INT (b);
}

static void
mex_view_model_controller_changed_cb (GController          *controller,
                                      GControllerAction     action,
                                      GControllerReference *ref,
                                      MexViewModel         *self)
{
  gint i, n_indices;
  guint offset, view_length, model_length;
  MexContent *content;

  MexViewModelPrivate *priv = self->priv;

  n_indices = g_controller_reference_get_n_indices (ref);

  view_length = mex_model_get_length (MEX_MODEL (self));
  model_length = mex_model_get_length (priv->model);

  switch (action)
    {
    case G_CONTROLLER_ADD:
      {
        GPtrArray *to_remove = g_ptr_array_new ();
        GPtrArray *to_add = g_ptr_array_new ();

        for (i = 0; i < view_length; i++)
          g_ptr_array_add (to_remove,
                           mex_model_get_content (MEX_MODEL (self), i));

        if (priv->start_content)
          offset = mex_model_index (priv->model, priv->start_content);
        else
          offset = priv->offset;

        for (i = 0; i < VIEW_MODEL_LIMIT (priv); i++)
          {
            content = mex_model_get_content (priv->model, offset + i);

            /* No more content in the underlaying model */
            if (!content)
              break;

            /* Check whether it's already here */
            if (mex_model_index (MEX_MODEL (self), content) < 0)
              g_ptr_array_add (to_add, content);
            else
              g_ptr_array_remove (to_remove, content);
          }

        for (i = 0; i < to_remove->len; i++)
          mex_model_remove_content (MEX_MODEL (self),
                                    g_ptr_array_index (to_remove, i));
        for (i = 0; i < to_add->len; i++) {
          mex_model_add_content (MEX_MODEL (self),
                                 g_ptr_array_index (to_add, i));
        }

        g_ptr_array_unref (to_add);
        g_ptr_array_unref (to_remove);
      }
      break;

    case G_CONTROLLER_REMOVE:
      {
        gint fillin = 0, start_fillin;
        GList *positions = NULL, *position;

        for (i = 0; i < n_indices; i++)
          {
            gint content_index = g_controller_reference_get_index_uint (ref, i);
            if (content_index >= VIEW_MODEL_LIMIT (priv))
              positions =
                g_list_insert_sorted_with_data (positions,
                                                GINT_TO_POINTER (content_index),
                                                _insert_position,
                                                NULL);
            else
              fillin++;
            content = mex_model_get_content (priv->model, content_index);
            mex_model_remove_content (MEX_MODEL (self), content);
          }

        position = positions;
        start_fillin = priv->limit;
        for (i = 0;
             i < MIN (fillin, (model_length - (gint) priv->limit));
             i++)
          {
            if ((position != NULL) &&
                (start_fillin == GPOINTER_TO_INT (position->data)))
              {
                while ((position != NULL) &&
                       (start_fillin == GPOINTER_TO_INT (position->data)))
                  {
                    start_fillin++;
                    if (start_fillin > GPOINTER_TO_INT (position->data))
                      position = position->next;
                  }
              }
            content = mex_model_get_content (priv->model, start_fillin);
            if (!content)
              break;
            mex_model_add_content (MEX_MODEL (self), content);
            start_fillin++;
          }
        g_list_free (positions);
      }
      break;

    case G_CONTROLLER_UPDATE:
      /* Should be no need for this, GBinding sorts it out for us :) */
      break;

    case G_CONTROLLER_CLEAR:
      mex_model_clear (MEX_MODEL (self));
      break;

    case G_CONTROLLER_REPLACE:
      mex_view_model_stop (self);
      mex_view_model_start (self);
      break;

    case G_CONTROLLER_INVALID_ACTION:
      g_warning (G_STRLOC ": View-model controller has issued an error");
      break;

    default:
      g_warning (G_STRLOC ": Unhandled action");
      break;
    }
}

/**
 * mex_view_model_start_at_content:
 * @self: View model to add content to
 * @start_at_content: First content item in the model to add to
 * the view model
 * @loop: Whether to loop back to the beginning of the
 * associated #MexModel's content items once the last item is reached;
 * if %TRUE, content items before the @start_at_content will be
 * added once the last of the model's content items is reached;
 * if %FALSE, only content items from @start_at_content to the last of
 * the model's content items are added
 *
 * Add content from a model to a view model.
 */
void
mex_view_model_start_at_content (MexViewModel *self,
                                 MexContent   *start_at_content,
                                 gboolean      loop)
{
  gint i, start_at;
  gboolean looped = FALSE;
  MexContent *content;
  MexViewModelPrivate *priv;
  GController *controller;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));

  priv = self->priv;

  if (priv->started)
    {
      g_warning (G_STRLOC ": Trying to start an already started view-model");
      return;
    }

  if (!priv->model)
    return;

  start_at = mex_model_index (priv->model, start_at_content);

  if (start_at == -1)
    {
      g_critical (G_STRLOC ": Content %p not found in %p model",
                  start_at_content, priv->model);
      return;
    }

  mex_model_clear (MEX_MODEL (self));
  priv->started = TRUE;
  priv->looped = loop;
  priv->offset = start_at;
  if (priv->start_content)
      g_object_unref (priv->start_content);
  priv->start_content = g_object_ref (start_at_content);

  for (i = priv->offset;
       mex_model_get_length (MEX_MODEL (self)) < VIEW_MODEL_LIMIT (priv);
       i++)
    {
      /* break out if looped and back around to start_at */
      if (loop && looped && i == priv->offset)
        break;

      if ((content = mex_model_get_content (priv->model, i)))
        {
          mex_model_add_content (MEX_MODEL (self), content);
        }
      else
        {
          if (loop)
            {
              looped = TRUE;
              i = -1;
            }
          else
            break;
        }
    }

  controller = mex_model_get_controller (priv->model);
  g_signal_connect_after (controller, "changed",
                          G_CALLBACK (mex_view_model_controller_changed_cb),
                          self);
}

/**
 * mex_view_model_start_at_offset:
 * @self: View model to add content to
 * @offset: Offset where to add content to, from the underlaying model
 *
 * Add content from a model to a view model.
 */
void
mex_view_model_start_at_offset (MexViewModel *self, guint offset)
{
  guint i, model_size;
  MexContent *content;
  MexViewModelPrivate *priv;
  GController *controller;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));

  priv = self->priv;

  if (priv->started)
    {
      g_warning (G_STRLOC ": Trying to start an already started view-model");
      return;
    }

  if (!priv->model)
    {
      g_warning (G_STRLOC
                 ": Trying to start a view-model without underlaying model");
      return;
    }

  mex_model_clear (MEX_MODEL (self));
  model_size = mex_model_get_length (priv->model);

  priv->offset = offset;
  if (priv->start_content)
    {
      g_object_unref (priv->start_content);
      priv->start_content = NULL;
    }
  priv->started = TRUE;

  for (i = 0; i < MIN (VIEW_MODEL_LIMIT (priv), model_size); i++)
    {
      content = mex_model_get_content (priv->model, i + priv->offset);
      if (!content)
        return;
      mex_model_add_content (MEX_MODEL (self), content);
    }

  controller = mex_model_get_controller (priv->model);
  g_signal_connect_after (controller, "changed",
                          G_CALLBACK (mex_view_model_controller_changed_cb),
                          self);
}

/**
 * mex_view_model_start:
 * @self: View-model to add content to
 *
 * Add all content items to a view-model from its associated model; see
 * also mex_view_model_start_at_content() and
 * mex_view_model_start_at_offset().
 */
void
mex_view_model_start (MexViewModel *self)
{
  MexViewModelPrivate *priv;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));

  priv = self->priv;

  if (priv->start_content)
    mex_view_model_start_at_content (self, priv->start_content, priv->looped);
  else
    mex_view_model_start_at_offset (self, priv->offset);
}

void
mex_view_model_stop (MexViewModel *self)
{
  MexViewModelPrivate *priv;
  GController *controller;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));

  priv = self->priv;

  /* There's nothing to do if there's no model set */
  if (!priv->model)
    {
      priv->started = FALSE;
      return;
    }

  if (!priv->started)
    return;

  controller = mex_model_get_controller (priv->model);
  g_signal_handlers_disconnect_by_func (controller,
                                        mex_view_model_controller_changed_cb,
                                        self);

  priv->started = FALSE;
}

void
mex_view_model_set_limit (MexViewModel *self, guint limit)
{
  guint i, length;
  MexContent *content;
  MexViewModelPrivate *priv;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));

  priv = self->priv;

  if (limit == priv->limit)
    return;

  if (!priv->started)
    {
      priv->limit = limit;
      return;
    }

  if (limit == 0)
    {
      /* No limit */
      length = mex_model_get_length (priv->model);
      for (i = mex_model_get_length (MEX_MODEL (self));
           i < length;
           i++)
        {
          content = mex_model_get_content (priv->model, i);
          mex_model_add_content (MEX_MODEL (self), content);
        }
    }
  else if (limit < priv->limit)
    {
      /* Lowering the limit => items to remove */
      length = mex_model_get_length (MEX_MODEL (self));
      for (i = MIN (priv->limit - 1, length - 1);
           i >= limit;
           i--)
        {
          content = mex_model_get_content (MEX_MODEL (self), i);
          mex_model_remove_content (MEX_MODEL (self), content);
        }
    }
  else
    {
      /* Raising the limit => items to add */
      length = mex_model_get_length (priv->model);
      for (i = mex_model_get_length (MEX_MODEL (self));
           i < MIN (limit, length);
           i++)
        {
          content = mex_model_get_content (priv->model, i);
          mex_model_add_content (MEX_MODEL (self), content);
        }
    }

  priv->limit = limit;
}

void
mex_view_model_set_offset (MexViewModel *self, guint offset)
{
  gboolean inc;
  guint i, diff, length;
  MexContent *content;
  MexViewModelPrivate *priv;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));

  priv = self->priv;

  if (offset == priv->offset)
    return;

  if (!priv->started)
    {
      priv->offset = offset;
      return;
    }

  if (offset > priv->offset)
    {
      inc = TRUE;
      diff = offset - priv->offset;
    }
  else
    {
      inc = FALSE;
      diff = priv->offset - offset;
    }

  /* If we can keep some elements in the model, then take care of
     removing/adding only necessary items. */
  if (priv->limit == 0 || diff < priv->limit)
    {
      if (inc)
        {
          /* Remove elements from the top */
          i = diff;
          for (i = 0; i < diff; i++)
            {
              content = mex_model_get_content (priv->model,
                                               priv->offset + i);
              if (content)
                mex_model_remove_content (MEX_MODEL (self), content);
            }

          /* Add following contents */
          for (i = 0; i < diff; i++)
            {
              content = mex_model_get_content (priv->model,
                                               offset + priv->limit - diff + i);
              if (!content)
                break;
              mex_model_add_content (MEX_MODEL (self), content);
            }

          /* loop if needed */
          if (priv->looped)
            {
              /* How much can we loop? */
              diff = priv->limit - mex_model_get_length (MEX_MODEL (self));
              for (i = 0; i < diff && i < offset; i++)
                {
                  content = mex_model_get_content (priv->model, i);
                  mex_model_add_content (MEX_MODEL (self), content);
                }
            }
        }
      else
        {
          /* Remove elements from the bottom */
          for (i = MIN (priv->offset - 1 - diff, 0); i < priv->offset; i--)
            {
              content = mex_model_get_content (priv->model, i);
              if (content)
                mex_model_remove_content (MEX_MODEL (self), content);
            }

          /* Add previous elements */
          for (i = 0; i < diff; i++)
            {
              content = mex_model_get_content (priv->model, i);
              if (!content)
                break;
              mex_model_add_content (MEX_MODEL (self), content);
            }
        }
    }
  else
    {
      /* Nothing to preserve => reset the model */
      mex_view_model_stop (self);
      mex_view_model_start (self);
    }

  priv->offset = offset;
}

void
mex_view_model_set_content (MexViewModel *self, MexContent *content)
{
  gint index;
  MexViewModelPrivate *priv;

  g_return_if_fail (MEX_IS_VIEW_MODEL (self));
  g_return_if_fail (MEX_IS_CONTENT (content));

  priv = self->priv;

  if (priv->start_content)
    {
      g_object_unref (priv->start_content);
      priv->start_content = NULL;
    }

  priv->start_content = g_object_ref (content);

  index = mex_model_index (priv->model, priv->start_content);

  g_return_if_fail (index >= 0);

  mex_view_model_set_offset (self, index);
}
