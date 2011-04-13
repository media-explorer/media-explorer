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

#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gudev/gudev.h>

#include "rebinder-debug.h"
#include "rebinder-evdev-manager.h"

G_DEFINE_TYPE (RebinderEvdevManager, rebinder_evdev_manager, G_TYPE_OBJECT)

typedef struct _EvdevDevice EvdevDevice;

static void
rebinder_evdev_manager_remove_device (RebinderEvdevManager *manager,
                                      EvdevDevice          *device);

#define EVDEV_MANAGER_PRIVATE(o)                              \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                          \
                                REBINDER_TYPE_EVDEV_MANAGER,  \
                                RebinderEvdevManagerPrivate))

struct _RebinderEvdevManagerPrivate
{
  GUdevClient *udev_client;

  GSList *devices;          /* list of EvdevDevices */
  GSList *event_sources;    /* list of the event sources */

  KeyNotifier key_notifier;
  gpointer key_data;
};

static const gchar *subsystems[] = { "input", NULL };

/*
 * EvdevDevice
 */

struct _EvdevDevice
{
  gchar *sysfs_path;
  gchar *device_path;
};

static EvdevDevice *
evdev_device_new (const gchar *sysfs_path,
                  const gchar *device_path)
{
  EvdevDevice *device;

  device = g_slice_new0 (EvdevDevice);
  device->sysfs_path = g_strdup (sysfs_path);
  device->device_path = g_strdup (device_path);

  return device;
}

static void
evdev_device_free (EvdevDevice *device)
{
  g_free (device->sysfs_path);
  g_free (device->device_path);
  g_slice_free (EvdevDevice, device);
}

/*
 * EvdevSource management
 *
 * The device manager is responsible for managing the GSource when devices
 * appear and disappear from the system.
 *
 * FIXME: For now, we associate a GSource with every single device. Starting
 * from glib 2.28 we can use g_source_add_child_source() to have a single
 * GSource for the device manager, each device becoming a child source. Revisit
 * this once we depend on glib >= 2.28.
 */


const char *option_xkb_layout = "us";
const char *option_xkb_variant = "";
const char *option_xkb_options = "";

/*
 * EvdevSource for reading input devices
 */

typedef struct _EvdevSource  EvdevSource;

struct _EvdevSource
{
  GSource source;

  EvdevDevice *device;        /* back pointer to the evdev device */
  GPollFD event_poll_fd;      /* file descriptor of the /dev node */
  uint32_t modifier_state;    /* remember the modifier state */
};

static gboolean
evdev_source_prepare (GSource *g_source,
                      gint    *timeout)
{
  EvdevSource *source = (EvdevSource *) g_source;

  *timeout = -1;
  return (source->event_poll_fd.revents & G_IO_IN);
}

static gboolean
evdev_source_check (GSource *g_source)
{
  EvdevSource *source = (EvdevSource *) g_source;

  return (source->event_poll_fd.revents & G_IO_IN);
}

static void
notify_key (EvdevSource *source,
            guint32      time_,
            guint32      key,
            guint32      state)
{
  RebinderEvdevManager *manager;
  RebinderEvdevManagerPrivate *priv;

  manager = rebinder_evdev_manager_get_default ();
  priv = manager->priv;

  MEX_NOTE (EVDEV, "notifying key %d", key);

  if (priv->key_notifier)
    priv->key_notifier (time_, key, state, priv->key_data);
}

static gboolean
evdev_source_dispatch (GSource     *g_source,
                       GSourceFunc  callback,
                       gpointer     user_data)
{
  EvdevSource *source = (EvdevSource *) g_source;
  struct input_event ev[8];
  gint len, i;
  uint32_t _time;

  len = read (source->event_poll_fd.fd, &ev, sizeof (ev));
  if (len < 0 || len % sizeof (ev[0]) != 0)
    {
      if (errno != EAGAIN)
        {
          RebinderEvdevManager *manager;

          if (MEX_HAS_DEBUG (EVDEV))
            {
              MEX_NOTE (EVDEV, "Could not read device (%s), removing.",
                        source->device->device_path);
            }

          /* remove the faulty device */
          manager = rebinder_evdev_manager_get_default ();
          rebinder_evdev_manager_remove_device (manager, source->device);

        }
      goto out;
    }

  for (i = 0; i < len / sizeof (ev[0]); i++)
    {
      struct input_event *e = &ev[i];

      _time = e->time.tv_sec * 1000 + e->time.tv_usec / 1000;

      switch (e->type)
        {
        case EV_KEY:

          switch (e->code)
            {
            case BTN_TOUCH:
            case BTN_TOOL_PEN:
            case BTN_TOOL_RUBBER:
            case BTN_TOOL_BRUSH:
            case BTN_TOOL_PENCIL:
            case BTN_TOOL_AIRBRUSH:
            case BTN_TOOL_FINGER:
            case BTN_TOOL_MOUSE:
            case BTN_TOOL_LENS:
              break;

            case BTN_LEFT:
            case BTN_RIGHT:
            case BTN_MIDDLE:
            case BTN_SIDE:
            case BTN_EXTRA:
            case BTN_FORWARD:
            case BTN_BACK:
            case BTN_TASK:
              break;

            default:
              notify_key (source, _time, e->code, e->value);
              break;
            }

          break;

        case EV_SYN:
        case EV_MSC:
        case EV_REL:
        case EV_ABS:
          break;

        default:
          g_warning ("Unhandled event of type %d", e->type);
          break;
        }
    }

out:
  return TRUE;
}
static GSourceFuncs event_funcs = {
  evdev_source_prepare,
  evdev_source_check,
  evdev_source_dispatch,
  NULL
};

static GSource *
evdev_source_new (EvdevDevice *device)
{
  GSource *g_source = g_source_new (&event_funcs, sizeof (EvdevSource));
  EvdevSource *source = (EvdevSource *) g_source;
  gint fd;

  /* grab the udev input device node and open it */
  MEX_NOTE (EVDEV, "Creating GSource for device %s", device->device_path);

  fd = open (device->device_path, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      g_warning ("Could not open device %s: %s", device->device_path,
                 strerror (errno));
      return NULL;
    }

  /* setup the source */
  source->device = device;
  source->event_poll_fd.fd = fd;
  source->event_poll_fd.events = G_IO_IN;

  /* and finally configure and attach the GSource */
  g_source_add_poll (g_source, &source->event_poll_fd);
  g_source_set_can_recurse (g_source, TRUE);
  g_source_attach (g_source, NULL);

  return g_source;
}

static void
evdev_source_free (EvdevSource *source)
{
  GSource *g_source = (GSource *) source;

  MEX_NOTE (EVDEV, "Removing GSource for device %s",
            source->device->device_path);

  /* ignore the return value of close, it's not like we can do something
   * about it */
  close (source->event_poll_fd.fd);

  g_source_destroy (g_source);
  g_source_unref (g_source);
}

static EvdevSource *
find_source_by_device (RebinderEvdevManager *manager,
                       EvdevDevice        *device)
{
  RebinderEvdevManagerPrivate *priv = manager->priv;
  GSList *l;

  for (l = priv->event_sources; l; l = g_slist_next (l))
    {
      EvdevSource *source = l->data;

      if (source->device == (EvdevDevice *) device)
        return source;
    }

  return NULL;
}

static void
rebinder_evdev_manager_add_device (RebinderEvdevManager *manager,
                                   EvdevDevice          *device)
{
  RebinderEvdevManagerPrivate *priv = manager->priv;
  GSource *source;

  priv->devices = g_slist_prepend (priv->devices, device);

  /* Install the GSource for this device */
  source = evdev_source_new (device);
  priv->event_sources = g_slist_prepend (priv->event_sources, source);
}

static void
rebinder_evdev_manager_remove_device (RebinderEvdevManager *manager,
                                      EvdevDevice          *device)
{
  RebinderEvdevManagerPrivate *priv = manager->priv;
  EvdevSource *source;

  /* Remove the device */
  priv->devices = g_slist_remove (priv->devices, device);

  /* Remove the source */
  source = find_source_by_device (manager, device);
  if (G_UNLIKELY (source == NULL))
    {
      g_warning ("Trying to remove a device without a source installed ?!");
      return;
    }

  evdev_source_free (source);
  priv->event_sources = g_slist_remove (priv->event_sources, source);
}

static gboolean
is_evdev (const gchar *sysfs_path)
{
  GRegex *regex;
  gboolean match;

  regex = g_regex_new ("/input[0-9]+/event[0-9]+$", 0, 0, NULL);
  match = g_regex_match (regex, sysfs_path, 0, NULL);

  g_regex_unref (regex);
  return match;
}

static void
add_udev_device (RebinderEvdevManager *manager,
                 GUdevDevice          *udev_device)
{
  EvdevDevice *device;
  const gchar *device_file, *sysfs_path;
  const gchar * const *keys;
  gboolean is_input_key = FALSE;
  guint i;

  device_file = g_udev_device_get_device_file (udev_device);
  sysfs_path = g_udev_device_get_sysfs_path (udev_device);

  if (device_file == NULL || sysfs_path == NULL)
    return;

  if (g_udev_device_get_property (udev_device, "ID_INPUT") == NULL)
    return;

  /* Make sure to only add evdev devices, ie the device with a sysfs path that
   * finishes by input%d/event%d (We don't rely on the node name as this
   * policy is enforced by udev rules Vs API/ABI guarantees of sysfs) */
  if (!is_evdev (sysfs_path))
    return;

  /* we only care about devices issuing key events */
  keys = g_udev_device_get_property_keys (udev_device);
  for (i = 0; keys[i]; i++)
    {
      if (strcmp (keys[i], "ID_INPUT_KEY") == 0)
        is_input_key = TRUE;
    }

  if (!is_input_key)
    return;

  device = evdev_device_new (sysfs_path, device_file);

  rebinder_evdev_manager_add_device (manager, device);

  MEX_NOTE (EVDEV, "Added device %s, sysfs %s", device_file, sysfs_path);
}

static EvdevDevice *
find_device_by_udev_device (RebinderEvdevManager *manager,
                            GUdevDevice          *udev_device)
{
  RebinderEvdevManagerPrivate *priv = manager->priv;
  GSList *l;
  const gchar *sysfs_path;

  sysfs_path = g_udev_device_get_sysfs_path (udev_device);
  if (sysfs_path == NULL)
    {
      g_message ("device file is NULL");
      return NULL;
    }

  for (l = priv->devices; l; l = g_slist_next (l))
    {
      EvdevDevice *device = l->data;

      if (strcmp (sysfs_path, device->sysfs_path) == 0)
          return device;
    }

  return NULL;
}

static void
remove_udev_device (RebinderEvdevManager *manager,
                    GUdevDevice          *udev_device)
{
  EvdevDevice *device;

  device = find_device_by_udev_device (manager, udev_device);
  if (device == NULL)
      return;

  rebinder_evdev_manager_remove_device (manager, device);
}

static void
on_uevent (GUdevClient *client,
           gchar       *action,
           GUdevDevice *device,
           gpointer     data)
{
  RebinderEvdevManager *manager = REBINDER_EVDEV_MANAGER (data);

  if (g_strcmp0 (action, "add") == 0)
    add_udev_device (manager, device);
  else if (g_strcmp0 (action, "remove") == 0)
    remove_udev_device (manager, device);
}

/*
 * GObject implementation
 */

static void
rebinder_evdev_manager_constructed (GObject *gobject)
{
  RebinderEvdevManager *manager = REBINDER_EVDEV_MANAGER (gobject);
  RebinderEvdevManagerPrivate *priv = manager->priv;
  GList *devices, *l;

  priv->udev_client = g_udev_client_new (subsystems);

  devices = g_udev_client_query_by_subsystem (priv->udev_client, subsystems[0]);
  for (l = devices; l; l = g_list_next (l))
    {
      GUdevDevice *device = l->data;

      add_udev_device (manager, device);
      g_object_unref (device);
    }
  g_list_free (devices);

  /* subcribe for events on input devices */
  g_signal_connect (priv->udev_client, "uevent",
                    G_CALLBACK (on_uevent), manager);
}

static void
rebinder_evdev_manager_finalize (GObject *object)
{
  RebinderEvdevManager *manager = REBINDER_EVDEV_MANAGER (object);
  RebinderEvdevManagerPrivate *priv = manager->priv;
  GSList *l;

  g_object_unref (priv->udev_client);

  for (l = priv->devices; l; l = g_slist_next (l))
    {
      EvdevDevice *device = l->data;

      evdev_device_free (device);
    }
  g_slist_free (priv->devices);

  for (l = priv->event_sources; l; l = g_slist_next (l))
    {
      EvdevSource *source = l->data;

      evdev_source_free (source);
    }
  g_slist_free (priv->event_sources);

  G_OBJECT_CLASS (rebinder_evdev_manager_parent_class)->finalize (object);
}

static void
rebinder_evdev_manager_class_init (RebinderEvdevManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RebinderEvdevManagerPrivate));

  object_class->constructed = rebinder_evdev_manager_constructed;
  object_class->finalize = rebinder_evdev_manager_finalize;
}

static void
rebinder_evdev_manager_init (RebinderEvdevManager *self)
{
  self->priv = EVDEV_MANAGER_PRIVATE (self);
}

RebinderEvdevManager *
rebinder_evdev_manager_get_default (void)
{
  static RebinderEvdevManager *manager = NULL;

  if (manager)
    return manager;

  manager = g_object_new (REBINDER_TYPE_EVDEV_MANAGER, NULL);

  return manager;
}

void
rebinder_evdev_manager_set_key_notifier (RebinderEvdevManager *manager,
                                         KeyNotifier           notifier,
                                         gpointer              data)
{
  RebinderEvdevManagerPrivate *priv = manager->priv;

  priv->key_notifier = notifier;
  priv->key_data = data;
}
