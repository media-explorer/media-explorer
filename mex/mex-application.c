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


#include "mex-application.h"
static void mex_content_iface_init (MexContentIface *iface);
G_DEFINE_TYPE_WITH_CODE (MexApplication,
                         mex_application,
                         MEX_TYPE_GENERIC_CONTENT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                mex_content_iface_init))

#define APPLICATION_PRIVATE(o)                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                    \
                                MEX_TYPE_APPLICATION,   \
                                MexApplicationPrivate))

struct _MexApplicationPrivate
{
  gchar     *desktop_file;
  gchar     *name;
  gchar     *executable;
  gchar     *icon;
  gchar     *thumbnail;
  gchar     *description;
  gboolean   bookmarked;
};

enum
{
  PROP_0,

  PROP_NAME,
  PROP_ICON,
  PROP_THUMBNAIL,
  PROP_DESCRIPTION,
  PROP_EXECUTABLE,
  PROP_DESKTOP_FILE,

  PROP_BOOKMARKED
};

/*
 * GObject implementation
 */

static void
mex_application_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MexApplication *application = MEX_APPLICATION (object);
  MexApplicationPrivate *priv = application->priv;

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_ICON:
      g_value_set_string (value, priv->icon);
      break;
    case PROP_THUMBNAIL:
      g_value_set_string (value, priv->thumbnail);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, priv->description);
      break;
    case PROP_EXECUTABLE:
      g_value_set_string (value, priv->executable);
      break;
    case PROP_DESKTOP_FILE:
      g_value_set_string (value, priv->desktop_file);
      break;
    case PROP_BOOKMARKED:
      g_value_set_boolean (value, priv->bookmarked);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_application_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_NAME:
      mex_application_set_name (MEX_APPLICATION (object),
                                 g_value_get_string (value));
      break;
    case PROP_ICON:
      mex_application_set_icon (MEX_APPLICATION (object),
                                 g_value_get_string (value));
      break;
    case PROP_THUMBNAIL:
      mex_application_set_thumbnail (MEX_APPLICATION (object),
                                      g_value_get_string (value));
      break;
    case PROP_DESCRIPTION:
      mex_application_set_description (MEX_APPLICATION (object),
                                        g_value_get_string (value));
      break;
    case PROP_EXECUTABLE:
      mex_application_set_executable (MEX_APPLICATION (object),
                                       g_value_get_string (value));
      break;
    case PROP_DESKTOP_FILE:
      mex_application_set_desktop_file (MEX_APPLICATION (object),
                                         g_value_get_string (value));
      break;
    case PROP_BOOKMARKED:
      mex_application_set_bookmarked (MEX_APPLICATION (object),
                                       g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_application_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_application_parent_class)->dispose (object);
}

static void
mex_application_finalize (GObject *object)
{
  MexApplication *application = MEX_APPLICATION (object);
  MexApplicationPrivate *priv = application->priv;

  if (priv->desktop_file)
    {
      g_free (priv->desktop_file);
      priv->desktop_file = NULL;
    }

  if (priv->name)
    {
      g_free (priv->name);
      priv->name = NULL;
    }

  if (priv->executable)
    {
      g_free (priv->executable);
      priv->executable = NULL;
    }

  if (priv->icon)
    {
      g_free (priv->icon);
      priv->icon = NULL;
    }

  if (priv->description)
    {
      g_free (priv->description);
      priv->description = NULL;
    }

  G_OBJECT_CLASS (mex_application_parent_class)->finalize (object);
}

static void
mex_application_class_init (MexApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexApplicationPrivate));

  object_class->get_property = mex_application_get_property;
  object_class->set_property = mex_application_set_property;
  object_class->dispose = mex_application_dispose;
  object_class->finalize = mex_application_finalize;

  pspec = g_param_spec_string ("name",
			       "Name",
			       "Application name",
			       "Unnamed",
			       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_NAME, pspec);

  pspec = g_param_spec_string ("icon",
			       "Icon",
			       "Application icon",
			       "applications-other",
			       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ICON, pspec);

  pspec = g_param_spec_string ("thumbnail",
			       "Icon",
			       "Application thumbnail",
			       NULL,
			       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_THUMBNAIL, pspec);

  pspec = g_param_spec_string ("description",
			       "Description",
			       "Application description",
			       "<Unknown>",
			       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DESCRIPTION, pspec);

  pspec = g_param_spec_string ("executable",
			       "Executable",
			       "Application executable",
			       NULL,
			       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_EXECUTABLE, pspec);

  pspec = g_param_spec_string ("desktop-file",
			       "Desktop file",
			       "Path to desktop file",
			       NULL,
			       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DESKTOP_FILE, pspec);

  pspec = g_param_spec_boolean ("bookmarked",
				"Bookmarked",
				"Whether the application is bookmarked",
				FALSE,
				G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_BOOKMARKED, pspec);
}

/*
 * Accessors
 */

static void
mex_application_init (MexApplication *self)
{
  self->priv = APPLICATION_PRIVATE (self);
}

MexApplication *
mex_application_new (void)
{
  return g_object_new (MEX_TYPE_APPLICATION, NULL);
}

const gchar *
mex_application_get_desktop_file (MexApplication *self)
{
  g_return_val_if_fail (MEX_IS_APPLICATION (self), NULL);

  return self->priv->desktop_file;
}

void
mex_application_set_desktop_file (MexApplication *self,
                                  const gchar    *desktop_file)
{
  MexApplicationPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION (self));

  priv = self->priv;

  g_free (priv->desktop_file);
  priv->desktop_file = g_strdup (desktop_file);
  g_object_notify (G_OBJECT (self), "desktop-file");
}

const gchar *
mex_application_get_name (MexApplication *self)
{
  g_return_val_if_fail (MEX_IS_APPLICATION (self), NULL);

  return self->priv->name;
}

void
mex_application_set_name (MexApplication *self,
                          const gchar    *name)
{
  MexApplicationPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION (self));

  priv = self->priv;

  g_free (priv->name);
  priv->name = g_strdup (name);
  g_object_notify (G_OBJECT (self), "name");
}

const gchar *
mex_application_get_executable (MexApplication *self)
{
  g_return_val_if_fail (MEX_IS_APPLICATION (self), NULL);

  return self->priv->executable;
}

void
mex_application_set_executable (MexApplication *self,
                                const gchar    *executable)
{
  MexApplicationPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION (self));

  priv = self->priv;

  g_free (priv->executable);
  priv->executable = g_strdup (executable);
  g_object_notify (G_OBJECT (self), "executable");
}

const gchar *
mex_application_get_icon (MexApplication *self)
{
  g_return_val_if_fail (MEX_IS_APPLICATION (self), NULL);

  return self->priv->icon;
}

void
mex_application_set_icon (MexApplication *self,
                           const gchar     *icon)
{
  MexApplicationPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION (self));

  priv = self->priv;

  g_free (priv->icon);
  priv->icon = g_strdup (icon);
  g_object_notify (G_OBJECT (self), "icon");
}


const gchar *
mex_application_get_thumbnail (MexApplication *self)
{
  g_return_val_if_fail (MEX_IS_APPLICATION (self), NULL);

  return self->priv->thumbnail;
}

void
mex_application_set_thumbnail (MexApplication *self,
                               const gchar    *thumbnail)
{
  MexApplicationPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION (self));

  priv = self->priv;

  g_free (priv->thumbnail);
  priv->thumbnail = g_strdup (thumbnail);
  g_object_notify (G_OBJECT (self), "thumbnail");
}

const gchar *
mex_application_get_description (MexApplication *self)
{
  g_return_val_if_fail (MEX_IS_APPLICATION (self), NULL);

  return self->priv->description;
}

void
mex_application_set_description (MexApplication *self,
                                 const gchar    *description)
{
  MexApplicationPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION (self));

  priv = self->priv;

  g_free (priv->description);
  priv->description = g_strdup (description);
  g_object_notify (G_OBJECT (self), "description");
}

gboolean
mex_application_get_bookmarked (MexApplication *self)
{
  g_return_val_if_fail (MEX_IS_APPLICATION (self), FALSE);

  return self->priv->bookmarked;
}

void
mex_application_set_bookmarked (MexApplication *self,
                                gboolean        bookmarked)
{
  MexApplicationPrivate *priv;

  g_return_if_fail (MEX_IS_APPLICATION (self));

  priv = self->priv;

  if (priv->bookmarked != bookmarked)
    {
      priv->bookmarked = bookmarked;
      g_object_notify (G_OBJECT (self), "bookmarked");
    }
}

static GParamSpec *
content_get_property (MexContent         *content,
                      MexContentMetadata  key)
{
  /* TODO */
  return NULL;
}

static const char *
content_get_metadata (MexContent         *content,
                      MexContentMetadata  key)
{
  MexApplication *application = MEX_APPLICATION (content);

  switch (key)
  {
    case MEX_CONTENT_METADATA_TITLE:
      return mex_application_get_name (application);
    case MEX_CONTENT_METADATA_SYNOPSIS:
      return mex_application_get_description (application);
    case MEX_CONTENT_METADATA_ID:
      return mex_application_get_desktop_file (application);
    case MEX_CONTENT_METADATA_STILL:
      return mex_application_get_thumbnail (application);
    case MEX_CONTENT_METADATA_MIMETYPE:
      return "x-mex-application";
    default:
      return NULL;
  }

  return NULL;
}

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->get_property = content_get_property;
  iface->get_metadata = content_get_metadata;
}
