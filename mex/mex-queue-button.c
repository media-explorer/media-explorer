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

#include "mex-queue-button.h"
#include "mex-content-view.h"
#include "mex-program.h"
#include "mex-queue-model.h"

#include <config.h>
#include <glib/gi18n-lib.h>

static void
mex_queue_button_set_content (MexContentView *view,
                              MexContent     *content);
static void
mex_queue_button_set_animated (MexQueueButton *q_button,
                               gboolean        animated);

static void mex_content_view_iface_init (MexContentViewIface *iface);
G_DEFINE_TYPE_WITH_CODE (MexQueueButton,
                         mex_queue_button,
                         MEX_TYPE_ACTION_BUTTON,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_content_view_iface_init))

#define QUEUE_BUTTON_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_QUEUE_BUTTON, MexQueueButtonPrivate))

struct _MexQueueButtonPrivate
{
  ClutterActor *inner_box;
  ClutterActor *icon;
  ClutterActor *label;
  ClutterActor *spinner;
  MexContent *content;
  MexModel *queue_model;
  guint animation_timeout_id;
};


static void
mex_queue_button_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_queue_button_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_queue_button_dispose (GObject *object)
{
  MexQueueButton *q_button = MEX_QUEUE_BUTTON (object);

  /* Free-up resources and drop callback */
  mex_queue_button_set_content (MEX_CONTENT_VIEW (q_button), NULL);

  if (q_button->priv->queue_model)
    {
      g_object_unref (q_button->priv->queue_model);
      q_button->priv->queue_model = NULL;
    }

  G_OBJECT_CLASS (mex_queue_button_parent_class)->dispose (object);
}

static void
mex_queue_button_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_queue_button_parent_class)->finalize (object);
}

static void
mex_queue_button_class_init (MexQueueButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexQueueButtonPrivate));

  object_class->get_property = mex_queue_button_get_property;
  object_class->set_property = mex_queue_button_set_property;
  object_class->dispose = mex_queue_button_dispose;
  object_class->finalize = mex_queue_button_finalize;
}

static void
_queue_button_notify_toggled_cb (MxButton       *button,
                                 GParamSpec     *pspec,
                                 MexQueueButton *q_button)
{
  MexQueueButtonPrivate *priv = q_button->priv;

  if (mx_button_get_toggled (button))
    {
      mex_queue_button_set_animated (q_button, TRUE);
      
      /* Triggers a train of actions that makes content have it's queued
       * property set which then runs the notify cb which calls
       * mex_queue_button_update so we don't need to run it directly
       */
      mex_model_add_content (priv->queue_model, priv->content);
    }
  else
    {
      mex_queue_button_set_animated (q_button, FALSE);
      mex_model_remove_content (priv->queue_model, priv->content);
    }
}

static void
mex_queue_button_update (MexQueueButton *button)
{
  MexQueueButtonPrivate *priv = button->priv;
  if (mex_content_get_metadata (priv->content, MEX_CONTENT_METADATA_QUEUED))
    {
      mx_label_set_text (MX_LABEL (priv->label),
                         _("Remove from queue"));

      g_signal_handlers_block_by_func (button,
                                       _queue_button_notify_toggled_cb,
                                       button);
      mx_button_set_toggled (MX_BUTTON (button), TRUE);
      g_signal_handlers_unblock_by_func (button,
                                         _queue_button_notify_toggled_cb,
                                        button);
    } else {
      mx_label_set_text (MX_LABEL (priv->label),
                         _("Add to queue"));

      g_signal_handlers_block_by_func (button,
                                       _queue_button_notify_toggled_cb,
                                       button);
      mx_button_set_toggled (MX_BUTTON (button), FALSE);
      g_signal_handlers_unblock_by_func (button,
                                         _queue_button_notify_toggled_cb,
                                         button);
    }

  if (mx_spinner_get_animating (MX_SPINNER (button->priv->spinner)))
    {
      mx_label_set_text (MX_LABEL (button->priv->label),
                                   _("Adding to queue"));
      clutter_actor_hide (button->priv->icon);
      clutter_actor_show (button->priv->spinner);
    } else {
      clutter_actor_hide (button->priv->spinner);
      clutter_actor_show (button->priv->icon);
  }
}

static gboolean
_spinner_looped_cb (MxSpinner      *spinner,
                    MexQueueButton *button)
{
  mex_queue_button_set_animated (button, FALSE);
  mex_queue_button_update (button);

  return FALSE;
}

static void
mex_queue_button_set_animated (MexQueueButton *button,
                               gboolean        animated)
{
  if (animated)
    {
      mx_spinner_set_animating (MX_SPINNER (button->priv->spinner), TRUE);
    }
  else
    {
      mx_spinner_set_animating (MX_SPINNER (button->priv->spinner), FALSE);
    }
}



static void
mex_queue_button_init (MexQueueButton *self)
{
  ClutterActor *temp_text;

  self->priv = QUEUE_BUTTON_PRIVATE (self);

  self->priv->inner_box = mx_box_layout_new ();
  self->priv->icon = mx_icon_new ();
  self->priv->label = mx_label_new_with_text ("Unknown queue state");
  self->priv->spinner = mx_spinner_new ();
  self->priv->queue_model = mex_queue_model_dup_singleton ();

  g_signal_connect (self->priv->spinner,
                    "looped",
                    (GCallback)_spinner_looped_cb,
                    self);

  clutter_container_add (CLUTTER_CONTAINER (self->priv->inner_box),
                         self->priv->label,
                         self->priv->icon,
                         self->priv->spinner,
                         NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (self->priv->inner_box),
                               self->priv->label,
                               "expand", TRUE,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", FALSE,
                               NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (self->priv->inner_box),
                               self->priv->icon,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", FALSE,
                               "x-align", MX_ALIGN_END,
                               NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (self->priv->inner_box),
                               self->priv->spinner,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", FALSE,
                               "x-align", MX_ALIGN_END,
                               NULL);

  clutter_actor_hide (self->priv->spinner);

  mx_bin_set_child (MX_BIN (self), self->priv->inner_box);
  mx_bin_set_fill (MX_BIN (self), TRUE, FALSE);

  temp_text = mx_label_get_clutter_text (MX_LABEL (self->priv->label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (temp_text), PANGO_ELLIPSIZE_NONE);

  g_signal_connect (self,
                    "notify::toggled",
                    (GCallback)_queue_button_notify_toggled_cb,
                    self);

  mx_button_set_is_toggle (MX_BUTTON (self), TRUE);
}

ClutterActor *
mex_queue_button_new (void)
{
  return g_object_new (MEX_TYPE_QUEUE_BUTTON, NULL);
}

static void
_content_notify_queued_cb (MexContent     *content,
                           GParamSpec     *pspec,
                           MexQueueButton *button)
{
  mex_queue_button_update (button);
}

static void
mex_queue_button_set_content (MexContentView *view,
                              MexContent     *content)
{
  MexQueueButton *self = MEX_QUEUE_BUTTON (view);
  MexQueueButtonPrivate *priv = self->priv;

  if (priv->content == content)
    return;

  if (priv->content)
    {
      g_signal_handlers_disconnect_by_func (priv->content,
                                            _content_notify_queued_cb,
                                            self);
      g_object_unref (priv->content);
      priv->content = NULL;
    }

  /* mex_queue_button_update will be called by the "queued" notify below */
  mex_queue_button_set_animated (self, FALSE);

  if (content)
    {
      priv->content = g_object_ref_sink (content);

      g_signal_connect (priv->content,
                        "notify::queued",
                        (GCallback)_content_notify_queued_cb,
                        self);
      g_object_notify (G_OBJECT (priv->content), "queued");
    }
}

static MexContent *
mex_queue_button_get_content (MexContentView *view)
{
  MexQueueButton *self = MEX_QUEUE_BUTTON (view);

  return self->priv->content;
}

static void
mex_content_view_iface_init (MexContentViewIface *iface)
{
  iface->set_content = mex_queue_button_set_content;
  iface->get_content = mex_queue_button_get_content;
}
