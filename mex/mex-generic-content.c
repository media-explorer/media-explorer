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

#include "mex-generic-content.h"
#include "mex-content.h"
#include "mex-enum-types.h"

enum {
  PROP_0 = MEX_CONTENT_METADATA_LAST_ID,
  PROP_LAST_POSITION_START,
  PROP_SAVE_LAST_POSITION,
};

enum {
  LAST_SIGNAL,
};

struct _MexGenericContentPrivate {
  int dummy;

  gboolean last_position_start;
  gboolean save_last_position;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MEX_TYPE_GENERIC_CONTENT, MexGenericContentPrivate))
static void mex_content_iface_init (MexContentIface *iface);
G_DEFINE_TYPE_WITH_CODE (MexGenericContent, mex_generic_content, G_TYPE_INITIALLY_UNOWNED,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT,
                                                mex_content_iface_init));

static GParamSpec *
content_get_property (MexContent         *content,
                      MexContentMetadata  key)
{
  /* TODO */
  return NULL;
}

const gchar *
mex_generic_content_get_property_name (MexContentMetadata key)
{
  return mex_enum_to_string (MEX_TYPE_CONTENT_METADATA, key);
}

/*
 * MexContent implementation
 */

static const char *
content_get_property_name (MexContent         *content,
                           MexContentMetadata  key)
{
  return mex_generic_content_get_property_name (key);
}

static const char *
content_get_metadata (MexContent         *content,
                      MexContentMetadata  key)
{
  MexGenericContent *gc = (MexGenericContent *) content;
  MexGenericContentClass *klass = MEX_GENERIC_CONTENT_GET_CLASS (gc);

  if (klass->get_metadata) {
    return klass->get_metadata (gc, key);
  }

  return NULL;
}

static char *
content_get_metadata_fallback (MexContent        *content,
                               MexContentMetadata key)
{
  MexGenericContent *gc = (MexGenericContent *) content;
  MexGenericContentClass *klass = MEX_GENERIC_CONTENT_GET_CLASS (gc);

  if (klass->get_metadata_fallback) {
    return klass->get_metadata_fallback (gc, key);
  }

  return NULL;
}

static void
mex_content_iface_init (MexContentIface *iface)
{
  iface->get_property = content_get_property;
  iface->get_metadata = content_get_metadata;
  iface->get_property_name = content_get_property_name;
  iface->get_metadata_fallback = content_get_metadata_fallback;
}

/*
 * GObject implementation
 */

static void
mex_generic_content_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_generic_content_parent_class)->finalize (object);
}

static void
mex_generic_content_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_generic_content_parent_class)->dispose (object);
}

static void
mex_generic_content_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MexGenericContentPrivate *priv = GET_PRIVATE (object);

#if 0
  MexGenericContent *self = (MexGenericContent *) object;

  /* For all dynamic properties, just pass them to the subclass */
  if (prop_id < MEX_CONTENT_METADATA_LAST_ID) {
    mex_content_set_metadata (content, prop_id);
    return;
  }
#endif

  switch (prop_id) {
  case PROP_LAST_POSITION_START:
    priv->last_position_start = g_value_get_boolean (value);
    break;

  case PROP_SAVE_LAST_POSITION:
    priv->save_last_position = g_value_get_boolean (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
mex_generic_content_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MexGenericContent *self = (MexGenericContent *) object;
  MexGenericContentPrivate *priv = GET_PRIVATE (object);
  MexContent *content = MEX_CONTENT (self);

  /* For all dynamic properties, just pass them to the subclass */
  if (prop_id < MEX_CONTENT_METADATA_LAST_ID) {
    g_value_set_string (value, mex_content_get_metadata (content, prop_id));
    return;
  }

  switch (prop_id) {
  case PROP_LAST_POSITION_START:
    g_value_set_boolean (value, priv->last_position_start);
    break;

  case PROP_SAVE_LAST_POSITION:
    g_value_set_boolean (value, priv->save_last_position);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
mex_generic_content_class_init (MexGenericContentClass *klass)
{
  GObjectClass *o_class = (GObjectClass *) klass;
  int i;

  o_class->dispose = mex_generic_content_dispose;
  o_class->finalize = mex_generic_content_finalize;
  o_class->set_property = mex_generic_content_set_property;
  o_class->get_property = mex_generic_content_get_property;

  g_type_class_add_private (klass, sizeof (MexGenericContentPrivate));

  /* Install a property for each metadata type */
  for (i = MEX_CONTENT_METADATA_NONE;
       i < MEX_CONTENT_METADATA_LAST_ID;
       i++) {
    GParamSpec *pspec;
    const char *name;

    if (!i) continue;

    name = mex_generic_content_get_property_name (i);

    pspec = g_param_spec_string (name, name, "A dynamic metadata property",
                                 NULL, G_PARAM_READWRITE |
                                 G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (o_class, i, pspec);
  }

  g_object_class_override_property (o_class,
                                    PROP_LAST_POSITION_START,
                                    "last-position-start");

  g_object_class_override_property (o_class,
                                    PROP_SAVE_LAST_POSITION,
                                    "save-last-position");
}

static void
mex_generic_content_init (MexGenericContent *self)
{
  MexGenericContentPrivate *priv = GET_PRIVATE (self);

  self->priv = priv;

  priv->last_position_start = TRUE;
  priv->save_last_position = TRUE;
}

gboolean
mex_generic_content_get_last_position_start (MexGenericContent *self)
{
  MexGenericContentPrivate *priv;

  g_return_val_if_fail (IS_MEX_GENERIC_CONTENT (self), FALSE);

  priv = GET_PRIVATE (self);

  return priv->last_position_start;
}

gboolean
mex_generic_content_get_save_last_position (MexGenericContent *self)
{
  MexGenericContentPrivate *priv;

  g_return_val_if_fail (IS_MEX_GENERIC_CONTENT (self), FALSE);

  priv = GET_PRIVATE (self);

  return priv->save_last_position;
}

/* void */
/* mex_generic_content_set_last_position_start (MexGenericContent *self, */
/*                                              gboolean from_last) */
/* { */
/*   MexGenericContentPrivate *priv = GET_PRIVATE (self); */

/*   priv->last_position_start = from_last; */
/* } */

/* void */
/* mex_generic_content_set_save_last_position (MexGenericContent *self, */
/*                                             gboolean save_last) */
/* { */
/*   MexGenericContentPrivate *priv = GET_PRIVATE (self); */

/*   priv->save_last_position = save_last; */
/* } */

