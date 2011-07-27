/*
 * Factory creating higher level objects
 *
 * Copyright © 2011 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:automatic-client-factory
 * @title: TpAutomaticClientFactory
 * @short_description: factory creating higher level objects
 * @see_also: #TpSimpleClientFactory
 *
 * This factory overrides some #TpSimpleClientFactory virtual methods to
 * create specialized #TpChannel subclasses.
 *
 * #TpAutomaticClientFactory will currently create #TpChannel objects
 * as follows:
 *
 * <itemizedlist>
 *   <listitem>
 *     <para>a #TpStreamTubeChannel, if the channel is of type
 *     %TP_IFACE_CHANNEL_TYPE_STREAM_TUBE;</para>
 *   </listitem>
 *   <listitem>
 *     <para>a #TpTextChannel, if the channel is of type
 *     %TP_IFACE_CHANNEL_TYPE_TEXT and implements
 *     %TP_IFACE_CHANNEL_INTERFACE_MESSAGES;</para>
 *   </listitem>
 *   <listitem>
 *     <para>a #TpFileTransferChannel, if the channel is of type
 *     %TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER;</para>
 *   </listitem>
 *   <listitem>
 *     <para>a plain #TpChannel, otherwise</para>
 *   </listitem>
 * </itemizedlist>
 *
 * It is guaranteed that the objects returned by future versions
 * will be either the class that is currently used, or a more specific
 * subclass of that class.
 *
 * This factory asks to prepare the following features:
 *
 * <itemizedlist>
 *   <listitem>
 *     <para>%TP_CHANNEL_FEATURE_CORE, %TP_CHANNEL_FEATURE_GROUP
 *     and %TP_CHANNEL_FEATURE_PASSWORD for all
 *     type of channels.</para>
 *   </listitem>
 *   <listitem>
 *     <para>%TP_TEXT_CHANNEL_FEATURE_INCOMING_MESSAGES and
 *     TP_TEXT_CHANNEL_FEATURE_SMS for #TpTextChannel</para>
 *   </listitem>
 *   <listitem>
 *     <para>%TP_FILE_TRANSFER_CHANNEL_FEATURE_CORE
 *     for #TpFileTransferChannel</para>
 *   </listitem>
 * </itemizedlist>
 *
 * Since: 0.UNRELEASED
 */

/**
 * TpAutomaticClientFactory:
 *
 * Data structure representing a #TpAutomaticClientFactory
 *
 * Since: 0.UNRELEASED
 */

/**
 * TpAutomaticClientFactoryClass:
 * @parent_class: the parent class
 *
 * The class of a #TpAutomaticClientFactory.
 *
 * Since: 0.UNRELEASED
 */

#include "tpy-client-factory.h"

#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/util.h>
#include <telepathy-yell/telepathy-yell.h>

#define DEBUG_FLAG TPY_DEBUG_CLIENT
//#include "tpy-client-factory-internal.h"

G_DEFINE_TYPE (TpyAutomaticClientFactory, tpy_automatic_client_factory,
    TP_TYPE_SIMPLE_CLIENT_FACTORY)

#define chainup ((TpSimpleClientFactoryClass *) \
    tpy_automatic_client_factory_parent_class)

TpyCallChannel *
tpy_call_channel_new_with_factory (TpSimpleClientFactory *factory,
                              TpConnection *conn,
                              const gchar *object_path,
                              const GHashTable *immutable_properties,
                              GError **error)
{
    TpProxy *conn_proxy = (TpProxy *) conn;
    TpyCallChannel *ret = NULL;

    g_return_val_if_fail (TP_IS_CONNECTION (conn), NULL);
    g_return_val_if_fail (object_path != NULL, NULL);
    g_return_val_if_fail (immutable_properties != NULL, NULL);

    if (tp_dbus_check_valid_object_path (object_path, error))

    /* An unfortunate collision between the default value in
     * TpChannelIface (0), and the default we want (-1), means that
     * we have to pass TP_UNKNOWN_HANDLE_TYPE to the constructor
     * explicitly, even if providing channel-properties. */

    ret = TPY_CALL_CHANNEL (g_object_new (TPY_TYPE_CALL_CHANNEL,
                                    "connection", conn,
                                    "dbus-daemon", conn_proxy->dbus_daemon,
                                    "bus-name", conn_proxy->bus_name,
                                    "object-path", object_path,
                                    "handle-type", (guint) TP_UNKNOWN_HANDLE_TYPE,
                                    "channel-properties", immutable_properties,
                                    "factory", factory,
                                    NULL));

    return ret;
}

static TpChannel *
create_channel_impl (TpSimpleClientFactory *self,
    TpConnection *conn,
    const gchar *object_path,
    const GHashTable *properties,
    GError **error)
{
  const gchar *chan_type;

  chan_type = tp_asv_get_string (properties, TP_PROP_CHANNEL_CHANNEL_TYPE);

  g_debug("Creating new channel from tpy_automatic_client_factory %s.", chan_type);
  if (!tp_strdiff (chan_type, TPY_IFACE_CHANNEL_TYPE_CALL))
    {
      return (TpChannel *) tpy_call_channel_new_with_factory(self, conn, object_path, properties, error);
    }

  /* Chainup on parent implementation as fallback */
  return chainup->create_channel (self, conn, object_path, properties, error);
}

static GArray *
dup_channel_features_impl (TpSimpleClientFactory *self,
    TpChannel *channel)
{
  GArray *features;
  GQuark feature;

  /* Chainup to get desired features for all channel types */
  features = chainup->dup_channel_features (self, channel);

  feature = TP_CHANNEL_FEATURE_GROUP;
  g_array_append_val (features, feature);

  feature = TP_CHANNEL_FEATURE_PASSWORD;
  g_array_append_val (features, feature);

  return features;
}

static void
tpy_automatic_client_factory_init (TpyAutomaticClientFactory *self)
{
  g_debug("initializing tpy_automatic_client_factory");
}

static void
tpy_automatic_client_factory_class_init (TpyAutomaticClientFactoryClass *cls)
{
  g_debug("initializing tpy_automatic_client_factory_class");
  TpSimpleClientFactoryClass *simple_class = (TpSimpleClientFactoryClass *) cls;

  simple_class->create_channel = create_channel_impl;
  simple_class->dup_channel_features = dup_channel_features_impl;
}

/**
 * tp_automatic_client_factory_new:
 * @dbus: a #TpDBusDaemon
 *
 * Convenient function to create a new #TpAutomaticClientFactory instance.
 *
 * Returns: a new #TpyAutomaticClientFactory
 *
 * Since: 0.UNRELEASED
 */
TpyAutomaticClientFactory *
tpy_automatic_client_factory_new (TpDBusDaemon *dbus)
{
  g_debug("allocating new tpy_automatic_client_factory");
  return g_object_new (TPY_TYPE_AUTOMATIC_CLIENT_FACTORY,
      "dbus-daemon", dbus,
      NULL);
}
