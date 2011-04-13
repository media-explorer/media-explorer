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

#include <mex/mex.h>
#include "mex-applet.h"
#include "mex-marshal.h"
#include "mex-enum-types.h"

static void mex_content_iface_init (MexContentIface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (MexApplet, mex_applet, MEX_TYPE_GENERIC_CONTENT,
                                  G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                         mex_content_iface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_APPLET, MexAppletPrivate))

typedef struct _MexAppletPrivate MexAppletPrivate;

struct _MexAppletPrivate {
  gpointer foo;
};

enum
{
  PROP_0,
  PROP_ID,
  PROP_NAME,
  PROP_DESCRIPTION,
  PROP_THUMBNAIL,
};

enum
{
  SIGNAL_ACTIVATED,
  SIGNAL_REQUEST_CLOSE,
  SIGNAL_CLOSED,
  SIGNAL_PRESENT_ACTOR,
  SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0, };

static void
mex_applet_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  MexApplet *applet = (MexApplet *)object;

  switch (property_id) {
    case PROP_ID:
      g_value_set_string (value, mex_applet_get_id (applet));
      break;
    case PROP_NAME:
      g_value_set_string (value, mex_applet_get_name (applet));
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, mex_applet_get_description (applet));
      break;
    case PROP_THUMBNAIL:
      g_value_set_string (value, mex_applet_get_thumbnail (applet));
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mex_applet_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  switch (property_id) {
    case PROP_ID:
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mex_applet_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_applet_parent_class)->dispose (object);
}

static void
mex_applet_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_applet_parent_class)->finalize (object);
}

static void
mex_applet_class_init (MexAppletClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexAppletPrivate));

  object_class->get_property = mex_applet_get_property;
  object_class->set_property = mex_applet_set_property;
  object_class->dispose = mex_applet_dispose;
  object_class->finalize = mex_applet_finalize;

  pspec = g_param_spec_string ("id",
                               "Applet identifier",
                               "Computer readable applet name",
                               NULL,
                               G_PARAM_READABLE);
  g_object_class_install_property (object_class,
                                   PROP_ID,
                                   pspec);

  pspec = g_param_spec_string ("name",
                               "Applet title",
                               "Human readable applet name",
                               NULL,
                               G_PARAM_READABLE);
  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   pspec);

  pspec = g_param_spec_string ("description",
                               "Applet description",
                               "Longer description of the applet",
                               NULL,
                               G_PARAM_READABLE);
  g_object_class_install_property (object_class,
                                   PROP_DESCRIPTION,
                                   pspec);

  pspec = g_param_spec_string ("thumbnail",
                               "Filename for thumbnail for application",
                               "Graphical representation of the applet",
                               NULL,
                               G_PARAM_READABLE);
  g_object_class_install_property (object_class,
                                   PROP_THUMBNAIL,
                                   pspec);

  signals[SIGNAL_ACTIVATED] =
    g_signal_new ("activated",
                  MEX_TYPE_APPLET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MexAppletClass,
                                   activated),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  signals[SIGNAL_REQUEST_CLOSE] =
    g_signal_new ("request-close",
                  MEX_TYPE_APPLET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MexAppletClass,
                                   request_close),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  CLUTTER_TYPE_ACTOR);

  signals[SIGNAL_CLOSED] =
    g_signal_new ("closed",
                  MEX_TYPE_APPLET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MexAppletClass,
                                   closed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  CLUTTER_TYPE_ACTOR);

  signals[SIGNAL_PRESENT_ACTOR] =
    g_signal_new ("present-actor",
                  MEX_TYPE_APPLET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MexAppletClass,
                                   present_actor),
                  NULL,
                  NULL,
                  mex_marshal_VOID__FLAGS_OBJECT,
                  G_TYPE_NONE,
                  2,
                  MEX_TYPE_APPLET_PRESENTATION_FLAGS,
                  CLUTTER_TYPE_ACTOR);
}

static void
mex_applet_init (MexApplet *self)
{
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
    MexApplet *applet = (MexApplet *)content;

    switch (key)
    {
      case MEX_CONTENT_METADATA_TITLE:
        return mex_applet_get_name (applet);
      case MEX_CONTENT_METADATA_SYNOPSIS:
        return mex_applet_get_description (applet);
      case MEX_CONTENT_METADATA_ID:
        return mex_applet_get_id (applet);
      case MEX_CONTENT_METADATA_STILL:
        return mex_applet_get_thumbnail (applet);
      case MEX_CONTENT_METADATA_MIMETYPE:
        return "x-mex-applet";
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

const gchar *
mex_applet_get_id (MexApplet *applet)
{
  MexAppletClass *klass = MEX_APPLET_GET_CLASS (applet);

  if (klass->get_id)
  {
    return klass->get_id (applet);
  } else {
    g_critical (G_STRLOC ": Expected implementation of get_id vfunc");
    return NULL;
  }
}

const gchar *
mex_applet_get_name (MexApplet *applet)
{
  MexAppletClass *klass = MEX_APPLET_GET_CLASS (applet);

  if (klass->get_name)
  {
    return klass->get_name (applet);
  } else {
    g_critical (G_STRLOC ": Expected implementation of get_name vfunc");
    return NULL;
  }
}

const gchar *
mex_applet_get_description (MexApplet *applet)
{
  MexAppletClass *klass = MEX_APPLET_GET_CLASS (applet);

  if (klass->get_description)
  {
    return klass->get_description (applet);
  } else {
    g_critical (G_STRLOC ": Expected implementation of get_description vfunc");
    return NULL;
  }
}

const gchar *
mex_applet_get_thumbnail (MexApplet *applet)
{
  MexAppletClass *klass = MEX_APPLET_GET_CLASS (applet);

  if (klass->get_thumbnail)
  {
    return klass->get_thumbnail (applet);
  } else {
    g_critical (G_STRLOC ": Expected implementation of get_thumbnail vfunc");
    return NULL;
  }
}

void
mex_applet_present_actor (MexApplet                  *applet,
                          MexAppletPresentationFlags  flags,
                          ClutterActor               *actor)
{
  g_return_if_fail (MEX_IS_APPLET (applet));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  g_signal_emit (applet, signals[SIGNAL_PRESENT_ACTOR], 0, flags, actor);
}

void
mex_applet_activate (MexApplet *applet)
{
  g_return_if_fail (MEX_IS_APPLET (applet));

  g_signal_emit (applet, signals[SIGNAL_ACTIVATED], 0);
}

void
mex_applet_request_close (MexApplet    *applet,
                          ClutterActor *actor)
{
  g_return_if_fail (MEX_IS_APPLET (applet));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  g_signal_emit (applet, signals[SIGNAL_REQUEST_CLOSE], 0, actor);
}

void
mex_applet_closed (MexApplet    *applet,
                   ClutterActor *actor)
{
  g_return_if_fail (MEX_IS_APPLET (applet));
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  g_signal_emit (applet, signals[SIGNAL_CLOSED], 0, actor);
}
