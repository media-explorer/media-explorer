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

#include "mex-background-manager.h"

G_DEFINE_TYPE (MexBackgroundManager, mex_background_manager, G_TYPE_OBJECT)

#define BACKGROUND_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_BACKGROUND_MANAGER, MexBackgroundManagerPrivate))

enum
{
  BACKGROUND_CHANGED,

  LAST_SIGNAL
};

struct _MexBackgroundManagerPrivate
{
  GList *backgrounds;

  MexBackground *current;

  gboolean active;
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
mex_background_manager_get_property (GObject    *object,
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
mex_background_manager_set_property (GObject      *object,
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
mex_background_manager_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_background_manager_parent_class)->dispose (object);
}

static void
mex_background_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_background_manager_parent_class)->finalize (object);
}

static void
mex_background_manager_class_init (MexBackgroundManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexBackgroundManagerPrivate));

  object_class->get_property = mex_background_manager_get_property;
  object_class->set_property = mex_background_manager_set_property;
  object_class->dispose = mex_background_manager_dispose;
  object_class->finalize = mex_background_manager_finalize;

  signals[BACKGROUND_CHANGED] =
    g_signal_new ("background-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_OBJECT);
}

static void
mex_background_manager_init (MexBackgroundManager *self)
{
  self->priv = BACKGROUND_MANAGER_PRIVATE (self);
}

MexBackgroundManager *
mex_background_manager_get_default (void)
{
  static MexBackgroundManager *manager = NULL;

  if (G_UNLIKELY (!manager))
    manager = g_object_new (MEX_TYPE_BACKGROUND_MANAGER, NULL);

  return manager;
}

/**/

static void
background_finalize_cb (MexBackgroundManager *manager,
                        MexBackground        *background)
{
  GList *l;
  MexBackgroundManagerPrivate *priv = manager->priv;
  MexBackground *new = NULL;

  l = g_list_find (priv->backgrounds, background);
  if (!l)
    return; /* Weird ! */

  if (priv->current == background)
    {
      if (l->next)
        new = MEX_BACKGROUND (l->next->data);
      else if (l->prev)
        new = MEX_BACKGROUND (l->prev->data);

      priv->current = new;
      g_signal_emit (manager, signals[BACKGROUND_CHANGED], 0, priv->current);

      if (priv->current)
        mex_background_set_active (priv->current, priv->active);
    }

  priv->backgrounds = g_list_delete_link (priv->backgrounds, l);
}

/**/

void
mex_background_manager_register (MexBackgroundManager *manager,
                                 MexBackground        *background)
{
  MexBackgroundManagerPrivate *priv;
  GList *b;

  g_return_if_fail (MEX_IS_BACKGROUND_MANAGER (manager));
  g_return_if_fail (MEX_IS_BACKGROUND (background));

  priv = manager->priv;

  b = g_list_find (priv->backgrounds, background);
  if (b)
    {
      g_warning ("'%s' background already registred",
                 G_OBJECT_TYPE_NAME (background));
      return;
    }

  priv->backgrounds = g_list_append (priv->backgrounds,
                                     background);
  g_object_weak_ref (G_OBJECT (background),
                     (GWeakNotify) background_finalize_cb,
                     manager);

  if (!priv->current)
    {
      priv->current = background;
      g_signal_emit (manager, signals[BACKGROUND_CHANGED], 0, priv->current);
      mex_background_set_active (priv->current, priv->active);
    }
}

void
mex_background_manager_unregister (MexBackgroundManager *manager,
                                   MexBackground        *background)
{
  g_return_if_fail (MEX_IS_BACKGROUND_MANAGER (manager));
  g_return_if_fail (MEX_IS_BACKGROUND (background));

  mex_background_set_active (background, FALSE);
  g_object_weak_unref (G_OBJECT (background),
                       (GWeakNotify) background_finalize_cb,
                       manager);
  background_finalize_cb (manager, background);
}

void
mex_background_manager_set_active (MexBackgroundManager *manager,
                                   gboolean              active)
{
  MexBackgroundManagerPrivate *priv;

  g_return_if_fail (MEX_BACKGROUND_MANAGER (manager));

  priv = manager->priv;

  if (priv->active == active)
    return;

  if (!priv->current)
    return;

  priv->active = active;
  mex_background_set_active (priv->current, priv->active);
}

gboolean
mex_background_manager_get_active (MexBackgroundManager *manager)
{
  g_return_val_if_fail (MEX_BACKGROUND_MANAGER (manager), FALSE);

  return manager->priv->active;
}
