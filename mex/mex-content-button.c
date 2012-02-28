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


#include "mex-content-button.h"
#include "mex-content-view.h"
#include "mex-private.h"
#include <string.h>

static void mex_content_view_iface_init (MexContentViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexContentButton, mex_content_button, MX_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_content_view_iface_init))

#define CONTENT_BUTTON_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_CONTENT_BUTTON, MexContentButtonPrivate))

enum
{
  PROP_0,

  PROP_MIMETYPE,
  PROP_PRIMARY_TEXT,
  PROP_SECONDARY_TEXT,
  PROP_MEDIA_URL
};

struct _MexContentButtonPrivate
{
  ClutterActor *layout;
  ClutterActor *icon;
  ClutterActor *primary_label;
  ClutterActor *separator_label;
  ClutterActor *secondary_label;

  gchar        *mimetype;
  gchar        *primary;
  gchar        *secondary;
  gchar        *media_url;

  MexContent   *content;
  GList        *bindings;
};


/* MexContentViewIface */
/* FIXME: This is taken from MexContentBox - figure a way to share this? */

static struct _Bindings {
  MexContentMetadata id;
  char *target;
  GBindingTransformFunc fallback;
} content_bindings[] = {
  { MEX_CONTENT_METADATA_TITLE, "primary-text", mex_content_title_fallback_cb },
  { MEX_CONTENT_METADATA_DIRECTOR, "secondary-text", NULL },
  { MEX_CONTENT_METADATA_MIMETYPE, "mime-type", NULL },
  { MEX_CONTENT_METADATA_URL, "media-url", NULL },
  { MEX_CONTENT_METADATA_NONE, NULL, NULL }
};

static void
mex_content_button_set_content (MexContentView *view,
                                MexContent     *content)
{
  MexContentButton *button = MEX_CONTENT_BUTTON (view);
  MexContentButtonPrivate *priv = button->priv;

  if (priv->content == content)
    return;

  if (priv->content)
    {
      GList *l;

      for (l = priv->bindings; l; l = l->next)
        g_object_unref (l->data);

      g_list_free (priv->bindings);
      priv->bindings = NULL;

      g_object_unref (priv->content);
    }

  if (content)
    {
      int i;

      priv->content = g_object_ref_sink (content);
      for (i = 0; content_bindings[i].id != MEX_CONTENT_METADATA_NONE; i++)
        {
          const gchar *property;
          GBinding *binding;

          property = mex_content_get_property_name (content,
                                                    content_bindings[i].id);

          if (property == NULL)
            {
              /* The Content does not provide a GObject property for this
               * kind of metadata, we can only sync at creation time */
              const gchar *metadata;

              metadata = mex_content_get_metadata (content,
                                                   content_bindings[i].id);
              g_object_set (button, content_bindings[i].target, metadata, NULL);

              continue;
            }

          if (content_bindings[i].fallback)
            binding = g_object_bind_property_full (content, property,
                                                   button,
                                                   content_bindings[i].target,
                                                   G_BINDING_SYNC_CREATE,
                                                   content_bindings[i].fallback,
                                                   NULL,
                                                   content, NULL);
          else
            binding = g_object_bind_property (content, property, button,
                                              content_bindings[i].target,
                                              G_BINDING_SYNC_CREATE);

          priv->bindings = g_list_prepend (priv->bindings, binding);
        }
    }
}

static MexContent *
mex_content_button_get_content (MexContentView *view)
{
  MexContentButton *button = MEX_CONTENT_BUTTON (view);
  MexContentButtonPrivate *priv = button->priv;

  return priv->content;
}

static void
mex_content_view_iface_init (MexContentViewIface *iface)
{
  iface->set_content = mex_content_button_set_content;
  iface->get_content = mex_content_button_get_content;
}

/* Object implementation */

static void
mex_content_button_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MexContentButtonPrivate *priv = MEX_CONTENT_BUTTON (object)->priv;

  switch (property_id)
    {
    case PROP_MIMETYPE:
      g_value_set_string (value, priv->mimetype);
      break;

    case PROP_PRIMARY_TEXT:
      g_value_set_string (value, priv->primary);
      break;

    case PROP_SECONDARY_TEXT:
      g_value_set_string (value, priv->secondary);
      break;

    case PROP_MEDIA_URL:
      g_value_set_string (value, priv->media_url);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_content_button_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  const gchar *video_type = "video";
  const gchar *audio_type = "audio";
  const gchar *image_type = "image";
  MexContentButtonPrivate *priv = MEX_CONTENT_BUTTON (object)->priv;

  switch (property_id)
    {
    case PROP_MIMETYPE:
      g_free (priv->mimetype);
      priv->mimetype = g_value_dup_string (value);
      if (!priv->mimetype)
        mx_stylable_set_style_class (MX_STYLABLE (priv->icon), "Document");
      else if (strncmp (priv->mimetype, video_type, strlen (video_type)) == 0)
        mx_stylable_set_style_class (MX_STYLABLE (priv->icon), "Video");
      else if (strncmp (priv->mimetype, audio_type, strlen (audio_type)) == 0)
        mx_stylable_set_style_class (MX_STYLABLE (priv->icon), "Audio");
      else if (strncmp (priv->mimetype, image_type, strlen (image_type)) == 0)
        mx_stylable_set_style_class (MX_STYLABLE (priv->icon), "Image");
      break;

    case PROP_PRIMARY_TEXT:
      g_free (priv->primary);
      priv->primary = g_value_dup_string (value);
      mx_label_set_text (MX_LABEL (priv->primary_label),
                         priv->primary ? priv->primary : "");
      break;

    case PROP_SECONDARY_TEXT:
      g_free (priv->secondary);
      priv->secondary = g_value_dup_string (value);
      if (priv->secondary)
        {
          mx_label_set_text (MX_LABEL (priv->secondary_label),
                             priv->secondary);
          clutter_actor_show (priv->separator_label);
        }
      else
        {
          mx_label_set_text (MX_LABEL (priv->secondary_label), "");
          clutter_actor_hide (priv->separator_label);
        }
      break;

    case PROP_MEDIA_URL:
      g_free (priv->media_url);
      priv->media_url = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_content_button_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_content_button_parent_class)->dispose (object);
}

static void
mex_content_button_finalize (GObject *object)
{
  MexContentButtonPrivate *priv = MEX_CONTENT_BUTTON (object)->priv;

  g_free (priv->primary);
  g_free (priv->secondary);
  g_free (priv->mimetype);

  G_OBJECT_CLASS (mex_content_button_parent_class)->finalize (object);
}

static void
mex_content_button_class_init (MexContentButtonClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexContentButtonPrivate));

  object_class->get_property = mex_content_button_get_property;
  object_class->set_property = mex_content_button_set_property;
  object_class->dispose = mex_content_button_dispose;
  object_class->finalize = mex_content_button_finalize;

  pspec = g_param_spec_string ("mime-type",
                               "Mime-type",
                               "Mime-type this button represents.",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MIMETYPE, pspec);

  pspec = g_param_spec_string ("primary-text",
                               "Primary text",
                               "Text to use for the primary label.",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PRIMARY_TEXT, pspec);

  pspec = g_param_spec_string ("secondary-text",
                               "Secondary text",
                               "Text to use for the secondary label.",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SECONDARY_TEXT, pspec);

  pspec = g_param_spec_string ("media-url",
                               "Media URL",
                               "URL of the media this button represents.",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MEDIA_URL, pspec);
}

static void
mex_content_button_init (MexContentButton *self)
{
  GList *children;
  MexContentButtonPrivate *priv = self->priv = CONTENT_BUTTON_PRIVATE (self);

  priv->layout = mx_box_layout_new ();
  priv->icon = mx_icon_new ();
  priv->primary_label = mx_label_new ();
  priv->separator_label = mx_label_new_with_text ("/");
  priv->secondary_label = mx_label_new ();

  mx_stylable_set_style_class (MX_STYLABLE (priv->secondary_label),
                               "Secondary");

  clutter_container_add (CLUTTER_CONTAINER (priv->layout),
                         priv->icon,
                         priv->primary_label,
                         priv->separator_label,
                         priv->secondary_label,
                         NULL);
  mx_box_layout_child_set_expand (MX_BOX_LAYOUT (priv->layout),
                                  priv->secondary_label,
                                  TRUE);
  mx_box_layout_child_set_x_align (MX_BOX_LAYOUT (priv->layout),
                                   priv->secondary_label,
                                   MX_ALIGN_START);

  /* Vertically centre-align children */
  children = clutter_container_get_children (CLUTTER_CONTAINER (priv->layout));
  while (children)
    {
      mx_box_layout_child_set_y_fill (MX_BOX_LAYOUT (priv->layout),
                                      CLUTTER_ACTOR (children->data),
                                      FALSE);
      children = g_list_delete_link (children, children);
    }

  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->layout);

  clutter_actor_hide (priv->separator_label);
  clutter_actor_hide (priv->secondary_label);

  mx_bin_set_fill (MX_BIN (self), TRUE, FALSE);
}

ClutterActor *
mex_content_button_new (void)
{
  return g_object_new (MEX_TYPE_CONTENT_BUTTON, NULL);
}
