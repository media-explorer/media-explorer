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


#include "mex-action-manager.h"
#include "mex-marshal.h"

G_DEFINE_TYPE (MexActionManager, mex_action_manager, G_TYPE_OBJECT)

#define ACTION_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_ACTION_MANAGER, MexActionManagerPrivate))

enum
{
  ACTION_ADDED,
  ACTION_REMOVED,

  LAST_SIGNAL
};

struct _MexActionManagerPrivate
{
  GHashTable  *actions;
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
mex_action_manager_dispose (GObject *object)
{
  MexActionManagerPrivate *priv = MEX_ACTION_MANAGER (object)->priv;

  if (priv->actions)
    {
      g_hash_table_unref (priv->actions);
      priv->actions = NULL;
    }

  G_OBJECT_CLASS (mex_action_manager_parent_class)->dispose (object);
}

static void
mex_action_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_action_manager_parent_class)->finalize (object);
}

static void
mex_action_manager_class_init (MexActionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexActionManagerPrivate));

  object_class->dispose = mex_action_manager_dispose;
  object_class->finalize = mex_action_manager_finalize;

  signals[ACTION_ADDED] =
    g_signal_new ("action-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexActionManagerClass, action_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  signals[ACTION_REMOVED] =
    g_signal_new ("action-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MexActionManagerClass, action_removed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
mex_action_manager_free_info (MexActionInfo *info)
{
  g_object_unref (info->action);
  g_strfreev (info->mime_types);
  g_strfreev (info->exclude_mime_types);
  g_slice_free (MexActionInfo, info);
}

static void
mex_action_manager_init (MexActionManager *self)
{
  MexActionManagerPrivate *priv = self->priv = ACTION_MANAGER_PRIVATE (self);

  priv->actions = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         NULL,
                                         (GDestroyNotify)
                                         mex_action_manager_free_info);
}

MexActionManager *
mex_action_manager_get_default (void)
{
  static MexActionManager *manager = NULL;

  if (!manager)
    manager = g_object_new (MEX_TYPE_ACTION_MANAGER, NULL);

  return manager;
}

static gint
mex_action_manager_sort_cb (gconstpointer a,
                            gconstpointer b)
{
  const MexActionInfo *info_a = a;
  const MexActionInfo *info_b = b;

  return info_b->priority - info_a->priority;
}

GList *
mex_action_manager_get_actions (MexActionManager *manager)
{
  GList *a, *actions;
  MexActionManagerPrivate *priv;

  g_return_val_if_fail (MEX_IS_ACTION_MANAGER (manager), NULL);

  priv = manager->priv;

  actions = g_hash_table_get_values (priv->actions);
  actions = g_list_sort (actions, mex_action_manager_sort_cb);

  for (a = actions; a; a = a->next)
    a->data = ((MexActionInfo *)a->data)->action;

  return actions;
}

GList *
mex_action_manager_get_actions_for_content (MexActionManager *manager,
                                            MexContent      *content)
{
  GList *a, *actions;
  gpointer key, value;
  GHashTableIter iter;
  MexActionManagerPrivate *priv;
  const gchar *last_position, *mime;

  g_return_val_if_fail (MEX_IS_ACTION_MANAGER (manager), NULL);

  mime = mex_content_get_metadata (content, MEX_CONTENT_METADATA_MIMETYPE);

  last_position = mex_content_get_metadata (content,
                                            MEX_CONTENT_METADATA_LAST_POSITION);

  actions = NULL;
  priv = manager->priv;

  g_hash_table_iter_init (&iter, priv->actions);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      gint i;
      MexActionInfo *info = value;
      const gchar *action_name;
      action_name = mx_action_get_name (info->action);

      /* If there isn't a last position skip: */
      if (!last_position)
        {
          if (g_str_equal (action_name, "play-from-last") ||
              g_str_equal (action_name, "play-from-begin") ||
              g_str_equal (action_name, "listen-from-begin"))
            continue;
        }
      else
        {
          /* We don't want resume and listen or play because
           * this action is provided by the start
           */
          if (g_str_equal (action_name, "play") ||
              g_str_equal (action_name, "listen"))
            continue;
        }

      /* If we've been asked for actions that apply to a
       * blank mime-type, only return actions that were
       * registered for a blank mime-type.
       */
      if (!mime || !*mime)
        {
          if (!info->mime_types ||
              !info->mime_types[0] ||
              !(*info->mime_types[0]))
            actions = g_list_prepend (actions, info);

          continue;
        }

      /* Add actions that match a mime prefix */
      for (i = 0; info->mime_types[i]; i++)
        {
          if (g_str_has_prefix (mime, info->mime_types[i]))
            {
              actions = g_list_prepend (actions, info);
              break;
            }
        }

      /* Now check that this action isn't for an excluded mimetype */
      if (!info->exclude_mime_types)
        continue;

      for (i = 0; info->exclude_mime_types[i]; i++)
        {
          if (g_str_has_prefix (mime, info->exclude_mime_types[i]))
            {
              actions = g_list_remove (actions, info);
              break;
            }
        }
    }

  actions = g_list_sort (actions, mex_action_manager_sort_cb);
  for (a = actions; a; a = a->next)
    a->data = ((MexActionInfo *)a->data)->action;

  return actions;
}

void
mex_action_manager_add_action (MexActionManager *manager,
                               MexActionInfo    *info)
{
  MexActionInfo *info_copy;
  MexActionManagerPrivate *priv;

  g_return_if_fail (MEX_IS_ACTION_MANAGER (manager));

  priv = manager->priv;

  if (g_hash_table_lookup (priv->actions, mx_action_get_name (info->action)))
    {
      g_warning (G_STRLOC ": Action '%s' already exists",
                 mx_action_get_name (info->action));
      return;
    }

  info_copy = g_slice_dup (MexActionInfo, info);
  info_copy->action = g_object_ref_sink (info->action);
  info_copy->mime_types = g_strdupv (info->mime_types);
  info_copy->exclude_mime_types = g_strdupv (info->exclude_mime_types);

  g_hash_table_insert (priv->actions,
                       (gpointer)mx_action_get_name (info->action),
                       info_copy);
  g_signal_emit (manager, signals[ACTION_ADDED], 0, info_copy);
}

void
mex_action_manager_remove_action (MexActionManager *manager,
                                  const gchar      *name)
{
  MexActionManagerPrivate *priv;

  g_return_if_fail (MEX_IS_ACTION_MANAGER (manager));

  priv = manager->priv;

  if (!g_hash_table_remove (priv->actions, name))
    {
      g_warning (G_STRLOC ": Action '%s' is unrecognised", name);
      return;
    }

  g_signal_emit (manager, signals[ACTION_REMOVED], 0, name);
}
