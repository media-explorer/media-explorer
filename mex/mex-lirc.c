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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mex.h"

#ifdef HAVE_LIRC
#include <lirc/lirc_client.h>
#include <unistd.h>
#include <fcntl.h>
#include <clutter/x11/clutter-x11.h>

static struct lirc_config *mex_lirc_config = NULL;

static void
mex_lirc_do_event (ClutterEvent *event)
{
  const GSList *s;

  ClutterStageManager *stage_manager = clutter_stage_manager_get_default ();
  const GSList *stages = clutter_stage_manager_peek_stages (stage_manager);

  /* FIXME: We should probably check if the stage has focus via X */
  for (s = stages; s; s = s->next)
    {
      ClutterStage *stage = s->data;
      ClutterActor *actor = clutter_stage_get_key_focus (stage);

      if (!actor)
        continue;

      event->any.stage = stage;
      event->any.source = actor;

      clutter_do_event (event);
    }
}

static void
mex_lirc_create_key_event (ClutterKeyEvent *event, guint keyval)
{
  ClutterDeviceManager *manager = clutter_device_manager_get_default ();

  /* Event synthesis inspired/copied from Clutter X11 backend */
  event->flags = CLUTTER_EVENT_FLAG_SYNTHETIC;
  event->time = CurrentTime;
  event->keyval = keyval;
  event->unicode_value = clutter_keysym_to_unicode (event->keyval);
  event->device =
    clutter_device_manager_get_core_device (manager, CLUTTER_KEYBOARD_DEVICE);
}

static gboolean
mex_lirc_key_release_cb (gpointer keyval_p)
{
  ClutterEvent release = { 0, };
  guint keyval = GPOINTER_TO_UINT (keyval_p);

  mex_lirc_create_key_event ((ClutterKeyEvent *)&release.key, keyval);
  release.type = release.key.type = CLUTTER_KEY_RELEASE;

  mex_lirc_do_event (&release);

  return FALSE;
}

static void
mex_lirc_do_key_event (guint keyval)
{
  ClutterEvent press = { 0, };

  mex_lirc_create_key_event ((ClutterKeyEvent *)&press.key, keyval);
  press.type = press.key.type = CLUTTER_KEY_PRESS;

  mex_lirc_do_event ((ClutterEvent *)&press);

  g_timeout_add (50,
                 (GSourceFunc)mex_lirc_key_release_cb,
                 GUINT_TO_POINTER (keyval));
}

static gboolean
mex_lirc_read_cb (GIOChannel         *source,
                  GIOCondition        condition,
                  struct lirc_config *config)
{
  gboolean success = TRUE;

  while (condition & (G_IO_PRI | G_IO_IN))
    {
      gint error_code;
      gchar *lirc_code, *lirc_char;

      while (((error_code = lirc_nextcode (&lirc_code)) == 0) && lirc_code)
        {
          while ((lirc_code2char (config, lirc_code, &lirc_char) == 0) &&
                 (lirc_char != NULL))
            {
              if (g_str_equal (lirc_char, "up"))
                mex_lirc_do_key_event (CLUTTER_KEY_Up);
              else if (g_str_equal (lirc_char, "down"))
                mex_lirc_do_key_event (CLUTTER_KEY_Down);
              else if (g_str_equal (lirc_char, "left"))
                mex_lirc_do_key_event (CLUTTER_KEY_Left);
              else if (g_str_equal (lirc_char, "right"))
                mex_lirc_do_key_event (CLUTTER_KEY_Right);
              else if (g_str_equal (lirc_char, "enter"))
                mex_lirc_do_key_event (CLUTTER_KEY_Return);
              else if (g_str_equal (lirc_char, "back"))
                mex_lirc_do_key_event (CLUTTER_KEY_Back);
              else if (g_str_equal (lirc_char, "home"))
                mex_lirc_do_key_event (CLUTTER_KEY_Home);
              else if (g_str_equal (lirc_char, "info"))
                mex_lirc_do_key_event (CLUTTER_KEY_Menu);
            }

          g_free (lirc_code);
        }

      condition = g_io_channel_get_buffer_condition (source);

      if (error_code == -1)
        {
          g_warning (G_STRLOC ": Error reading from source");
          success = FALSE;
        }
    }

  if (condition & G_IO_HUP)
    {
      g_warning (G_STRLOC ": Unexpected hang-up");
      success = FALSE;
    }

  if (condition & (G_IO_ERR | G_IO_NVAL))
    {
      g_warning (G_STRLOC ": Error or invalid request");
      success = FALSE;
    }

  if (condition & ~(G_IO_PRI | G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL))
    {
      g_warning ("Unexpected IO condition");
      success = FALSE;
    }

  return success;
}

static void
_mex_lirc_deinit (struct lirc_config *config)
{
  if (config)
    lirc_freeconfig (config);
  lirc_deinit ();
}

static struct lirc_config *
_mex_lirc_init (const gchar *name)
{
  int lirc_fd, res;
  gchar *tmp;

  lirc_fd = lirc_init ((char *)name, 1);

  if (lirc_fd == -1)
    {
      /* This usually means that the LIRC daemon is not running, nothing
       * really fatal */
      MEX_INFO ("Could not initialize LIRC");
    }
  else
    {
      struct lirc_config *config = NULL;

      /* Load the default config */
      tmp = g_build_filename (mex_get_data_dir (), "mex-lircrc", NULL);
      res = lirc_readconfig (tmp, &config, NULL);
      g_free (tmp);

      if (res == 0)
        {
          int flags;
          GIOChannel *channel;

          /* Set non-blocking flag */
          flags = fcntl (lirc_fd, F_GETFL);
          fcntl (lirc_fd, F_SETFL, flags | O_NONBLOCK);

          /* Create IOChannel to do non-blocking reads */
          channel = g_io_channel_unix_new (lirc_fd);
          g_io_add_watch (channel,
                          G_IO_IN | G_IO_PRI | G_IO_ERR |
                          G_IO_NVAL | G_IO_HUP,
                          (GIOFunc)mex_lirc_read_cb,
                          config);

          return config;
        }
      else
        {
          g_warning (G_STRLOC ": Error reading LIRC config");
          lirc_deinit ();
        }
    }

  return NULL;
}

void
mex_lirc_init (void)
{
  mex_lirc_config = _mex_lirc_init ("mex");
}

void
mex_lirc_deinit (void)
{
  _mex_lirc_deinit (mex_lirc_config);
  mex_lirc_config = NULL;
}

#else /* HAVE_LIRC */

void
mex_lirc_init (void)
{
}

void
mex_lirc_deinit (void)
{
}

#endif /* !HAVE_LIRC */
