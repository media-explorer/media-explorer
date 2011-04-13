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


#include "mex-epg-manager.h"

#include "mex-debug.h"

G_DEFINE_TYPE (MexEpgManager, mex_epg_manager, G_TYPE_OBJECT)

#define EPG_MANAGER_PRIVATE(o)				      \
	  (G_TYPE_INSTANCE_GET_PRIVATE ((o),		      \
					MEX_TYPE_EPG_MANAGER, \
					MexEpgManagerPrivate))

enum
{
  READY,

  LAST_SIGNAL
};

typedef struct _Request
{
  MexEpgManager *manager;
  MexChannel *channel;
  GDateTime *start_date, *end_date;
  MexEpgManagerReply callback;
  gpointer user_data;
} Request;

struct _MexEpgManagerPrivate
{
  gint wait_for_providers;
  GPtrArray *providers;
  GQueue *requests;
};

static guint signals[LAST_SIGNAL] = { 0, };

static gboolean
mex_epg_manager_ready (MexEpgManager *manager)
{
  MexEpgManagerPrivate *priv = manager->priv;

  /* to be ready we need at least one registered provider and all the providers
   * ready themselves */
  return (priv->providers->len > 0) && (priv->wait_for_providers == 0);
}

static void
free_request (gpointer request,
              gpointer user_data)
{
  Request *req = request;

  g_date_time_unref (req->start_date);
  g_date_time_unref (req->end_date);
  g_slice_free (Request, req);
}

static void
on_manager_ready (MexEpgManager *manager,
                  gpointer       user_data)
{
  MexEpgManagerPrivate *priv = manager->priv;
  MexEpgProvider *provider;
  Request *req;

  req = g_queue_pop_tail (priv->requests);
  while (req)
    {
      if (priv->providers->len > 1)
        {
          MEX_WARN (EPG, "Having more than 1 EPG provider but don't know how "
                    "to merge the results for multiple providers just yet. "
                    "Using the first one");
        }

      provider = g_ptr_array_index (priv->providers, 0);
      mex_epg_provider_get_events (provider, req->channel,
                                   req->start_date, req->end_date,
                                   req->callback, req->user_data);

      free_request (req, NULL);
      req = g_queue_pop_tail (priv->requests);
    }


}

/*
 * GObject implementation
 */

static void
mex_epg_manager_get_property (GObject    *object,
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
mex_epg_manager_set_property (GObject      *object,
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
mex_epg_manager_finalize (GObject *object)
{
  MexEpgManager *manager = MEX_EPG_MANAGER (object);
  MexEpgManagerPrivate *priv = manager->priv;

  g_ptr_array_free (priv->providers, TRUE);

  g_queue_foreach (priv->requests, free_request, NULL);
  g_queue_free (priv->requests);

  G_OBJECT_CLASS (mex_epg_manager_parent_class)->finalize (object);
}

static void
mex_epg_manager_class_init (MexEpgManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexEpgManagerPrivate));

  object_class->get_property = mex_epg_manager_get_property;
  object_class->set_property = mex_epg_manager_set_property;
  object_class->finalize = mex_epg_manager_finalize;

  signals[READY] = g_signal_new ("ready",
                                 G_TYPE_FROM_INTERFACE (klass),
                                 G_SIGNAL_RUN_FIRST,
                                 0, NULL, NULL,
                                 g_cclosure_marshal_VOID__VOID,
                                 G_TYPE_NONE, 0);
}

static void
mex_epg_manager_init (MexEpgManager *self)
{
  MexEpgManagerPrivate *priv;

  self->priv = priv = EPG_MANAGER_PRIVATE (self);

  priv->providers = g_ptr_array_new_with_free_func (g_object_unref);
  priv->requests = g_queue_new ();

  g_signal_connect (self, "ready",
                    G_CALLBACK (on_manager_ready), NULL);
}

MexEpgManager *
mex_epg_manager_get_default (void)
{
  static MexEpgManager *singleton = NULL;

  if (G_LIKELY (singleton))
    return singleton;

  singleton = g_object_new (MEX_TYPE_EPG_MANAGER, NULL);
  return singleton;
}

static void
on_provider_ready (MexEpgProvider *provider,
                   MexEpgManager  *manager)
{
  MexEpgManagerPrivate *priv = manager->priv;

  if (G_UNLIKELY (priv->wait_for_providers == 0))
    {
      MEX_WARN (EPG, "Unexpected provider ready");
      return;
    }

  priv->wait_for_providers--;

  if (priv->wait_for_providers == 0)
    g_signal_emit (manager, signals[READY], 0);
}

/* transfer-full */
void
mex_epg_manager_add_provider (MexEpgManager  *manager,
			      MexEpgProvider *provider)
{
  MexEpgManagerPrivate *priv;

  g_return_if_fail (MEX_IS_EPG_PROVIDER (provider));
  priv = manager->priv;

  if (mex_epg_provider_is_ready (provider) == FALSE)
    {
      priv->wait_for_providers++;
      g_signal_connect (provider, "epg-provider-ready",
                        G_CALLBACK (on_provider_ready), manager);
    }
  g_ptr_array_add (manager->priv->providers, provider);
}

/**
 * mex_epg_manager_get_events: Retrieve events
 * @manager: a #MexEpgManager
 * @channel: a #MexChannel
 * @start_date: lower bound for the query
 * @end_date: upper bound for the query
 * @reply: a callback to call when the data is ready
 *
 * Query the @manager for EPG events between @start_data and @end_date. The
 * query is asynchronous and @reply is called with the results.
 *
 * <note>The resulting array of #MexEpgEvents is owned by the library,
 * you need to take a reference to the array and/or some or all of the elements
 * if you want to keep them around in the client code.</note>
 *
 * Since: 0.2
 */
void
mex_epg_manager_get_events (MexEpgManager      *manager,
                            MexChannel         *channel,
                            GDateTime          *start_date,
                            GDateTime          *end_date,
                            MexEpgManagerReply  reply,
                            gpointer            user_data)
{
  MexEpgManagerPrivate *priv;
  MexEpgProvider *provider;
  Request *req;

  g_return_if_fail (MEX_IS_EPG_MANAGER (manager));
  priv = manager->priv;

  if (mex_epg_manager_ready (manager))
    {
      if (priv->providers->len > 1)
        {
          MEX_WARN (EPG, "Having more than 1 EPG provider but don't know how "
                    "to merge the results for multiple providers just yet. "
                    "Using the first one");
        }

      provider = g_ptr_array_index (priv->providers, 0);
      mex_epg_provider_get_events (provider, channel, start_date, end_date,
                                   reply, user_data);
    }
  else
    {
      /* we need to wait until the providers are ready, so queue the request */
      req = g_slice_new (Request);
      req->manager = manager;
      req->channel = channel;
      req->start_date = g_date_time_ref (start_date);
      req->end_date = g_date_time_ref (end_date);
      req->callback = reply;
      req->user_data = user_data;

      g_queue_push_head (priv->requests, req);

      /* the queue will be processed in the ::ready handler */
    }
}

void
mex_epg_manager_get_event_now (MexEpgManager      *manager,
                               MexChannel         *channel,
                               MexEpgManagerReply  reply,
                               gpointer            user_data)
{
  GDateTime *now;

  now = g_date_time_new_now_local ();
  mex_epg_manager_get_events (manager, channel, now, now, reply, user_data);
  g_date_time_unref (now);
}
