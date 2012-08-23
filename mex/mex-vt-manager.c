/*
 * Mex - a media explorer
 *
 * Copyright © 2012 sleep(5) Ltd.
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

/*
 * VT management code for Mex on non-X11 platforms.
 *
 * See man console_ioctl(4) for the ioctls involved and
 * http://www.linuxjournal.com/article/2783 for commentary on VT switching in
 * particular.
 */
#include "mex-vt-manager.h"
#include "mex.h"

#include <clutter/clutter.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

static int           vt        = -1;
static int           ttyX      = -1;

/* Runtime check whether we are running under X11 */
static gboolean x11_platform (void);

#ifndef CLUTTER_WINDOWING_X11
/*
 * If the clutter X11 headers are not present, then there will not be X11
 * backend at runtime either.
 */
static gboolean
x11_platform (void)
{
  return FALSE;
}
#else
/*
 * The presence of the headers does not mean we are using X11, so check for the
 * backend.
 */
static gboolean
x11_platform (void)
{
  ClutterBackend *backend = clutter_get_default_backend ();
  const char     *backend_type;

  g_return_val_if_fail (backend, FALSE);

  backend_type = G_OBJECT_TYPE_NAME (backend);

  /*
   * This kind of sucks, but the normal GObject type marcros for the clutter
   * backends are part of the private API.
   */
  if (!g_strcmp0 (backend_type, "ClutterBackendX11"))
    return TRUE;

  return FALSE;
}
#endif

/*
 * Decluttering macros for ioctl:
 *
 * _XIOCTL_W: Execute ioctl and print warning on failure
 * _XIOCTL_F: Execute ioctl and jump to 'fail' on failure
 */
#define _XIOCTL_W(err0, fd, io ,args...)                        \
  if (ioctl (fd, io, args) < 0)                                 \
    {                                                           \
      g_warning ("Failed to " err0 ":%s", strerror(errno));     \
    }                                                           \
  else                                                          \
    g_message ("Success: %s", err0)

#define _XIOCTL_F(err0, fd, io ,args...)                        \
  if (ioctl (fd, io, args) < 0)                                 \
    {                                                           \
      g_warning ("Failed to " err0 ":%s", strerror (errno));    \
      goto fail;                                                \
    }                                                           \
  else                                                          \
    g_message ("Success: %s", err0)

/*
 * Signal handler for VT switching:
 *   - SIGUSR1 when we switch to our VT
 *   - SIGUSR2 when we switch away from our VT
 */
static void
signal_handler (int id)
{
  switch (id)
    {
    case SIGUSR1:
      /* VT activate request -- VT_ACKACQ for allow and redraw */
      g_print ("Our VT activated\n");
      _XIOCTL_W ("switch to VT", ttyX, VT_RELDISP, VT_ACKACQ);
      clutter_actor_show (mex_get_stage ());
      clutter_actor_queue_redraw (mex_get_stage ());
      break;
    case SIGUSR2:
      /* Deactivate VT request -- 1 for allow*/
      g_print ("Our VT deactivated\n");
      clutter_actor_hide (mex_get_stage ());
      _XIOCTL_W ("release VT", ttyX, VT_RELDISP, 1);
      break;
    default:
      g_warning ("Got unknown signal %d", id);
    }
}

gboolean
mex_vt_manager_init (void)
{
  int               tty0 = -1;
  char             *tmp  = NULL;
  struct vt_mode    mode = {0};
  struct sigaction  sig;
  gboolean          retval = TRUE;

  /*
   * If we are running under X11 exit immediately, X takes care of the VT
   * management for us.
   */
  if (x11_platform ())
    return TRUE;

  if ((tty0 = open ("/dev/tty0", O_WRONLY)) < 0)
    {
      g_warning ("Failed to open tty0");
      goto fail;
    }

  /*
   * Find an available VT
   */
  _XIOCTL_F ("locate free VT", tty0, VT_OPENQRY, &vt);
  if (vt < 0)
    {
      g_warning ("Invalid VT number %d", vt);
      goto fail;
    }

  /*
   * Detach from controlling tty and attach to the new VT instead (this
   * is necessary to be able to switch to the new VT without being root).
   */
  setsid ();
  tmp = g_strdup_printf ("/dev/tty%d", vt);
  if ((ttyX = open (tmp, O_RDWR|O_NONBLOCK)) < 0)
    {
      g_warning ("Failed to open '%s': %s", tmp, strerror (errno));
      goto fail;
    }
  _XIOCTL_F ("change controlling tty", ttyX, TIOCSCTTY, 0);

  /*
   * Install our signal handler and switch over to process mode
   */
  sig.sa_handler = signal_handler;
  sig.sa_flags   = 0;
  sigemptyset (&sig.sa_mask);
  if (sigaction (SIGUSR1, &sig, NULL) < 0)
    {
      g_warning ("Failed to install SIGUSR1 handler");
      goto fail;
    }

  if (sigaction (SIGUSR2, &sig, NULL) < 0)
    {
      g_warning ("Failed to install SIGUSR2 handler");
      goto fail;
    }

  mode.mode = VT_PROCESS;
  mode.relsig = SIGUSR2;
  mode.acqsig = SIGUSR1;
  _XIOCTL_F ("set vt mode", ttyX, VT_SETMODE, &mode);

  goto done;

 fail:
  retval = FALSE;

  if (ttyX >= 0)
    {
      close (ttyX);
      ttyX = -1;
      vt = -1;
    }

 done:
  if (tty0 >= 0)
    close (tty0);

  g_free (tmp);

  return retval;
}

void
mex_vt_manager_deinit ()
{
  int               tty0 = -1;
  struct vt_mode    mode = {0};
  struct sigaction  sig;

  if (vt < 0 || ttyX < 0)
    return;

  _XIOCTL_W ("set graphics mode", ttyX, KDSETMODE, KD_TEXT);

  /* Switch console back to auto mode and remove signal handlers */
  mode.mode = VT_AUTO;
  mode.relsig = 0;
  mode.acqsig = 0;
  _XIOCTL_W ("set vt mode", ttyX, VT_SETMODE, &mode);

  sig.sa_handler = SIG_IGN;
  sig.sa_flags   = 0;
  sigemptyset (&sig.sa_mask);

  if (sigaction (SIGUSR1, &sig, NULL) < 0)
    g_warning ("Failed to remove SIGUSR1 handler");

  if (sigaction (SIGUSR2, &sig, NULL) < 0)
    g_warning ("Failed to remove SIGUSR2 handler");

  close (ttyX);
  ttyX = -1;

  if ((tty0 = open ("/dev/tty0", O_WRONLY)) < 0)
    g_warning ("Failed to open tty0");
  else
    {
      _XIOCTL_W ("disallocate", tty0, VT_GETMODE, vt);
      close (tty0);
    }
}

void
mex_vt_manager_activate (void)
{
  if (vt < 0 || ttyX < 0)
    return;

  _XIOCTL_F ("activate VT", ttyX, VT_ACTIVATE, vt);
  _XIOCTL_F ("wait for VT", ttyX, VT_WAITACTIVE, vt);
  _XIOCTL_F ("set graphics mode", ttyX, KDSETMODE, KD_GRAPHICS);

 fail:;
}
