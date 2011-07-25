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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <mx/mx.h>

#include "rebinder.h"
#include "rebinder-debug.h"
#include "rebinder-evdev-manager.h"

#define PANE_ERROR_BASE   0xf000

typedef enum
{
  PANE_FIRST   = -1,
  PANE_SETUP,
  PANE_BACK,
  PANE_HOME,
  PANE_INFO,
  PANE_DONE,
  PANE_LAST,

  PANE_BINDING_FIRST = PANE_BACK,
  PANE_BINDING_LAST  = PANE_INFO,

  PANE_ERROR_BACK = PANE_ERROR_BASE,
  PANE_ERROR_HOME,
  PANE_ERROR_INFO,
  PANE_ERROR_LAST

} Pane;

typedef enum
{
  STATE_PRESS,
  STATE_CONFIRM,
  STATE_ERROR,
  STATE_NEXT
} BindingState;

struct _Configure
{
  guint external_window : 1;

  MxApplication *application;
  MxWindow *window;

  RebinderEvdevManager *evdev_manager;

  ClutterScript *script;
  ClutterActor *stage;
  ClutterActor *main_frame;
  ClutterActor *next_button;
  ClutterActor *remote;

  Pane current_pane;
  Pane erroneous_pane;

  BindingState binding_state;
  gint first_keycode;

  GArray *bindings;
  gulong next_button_handler;
  gulong stage_capture_handler;
};

typedef struct
{
  gchar *name;
  gchar *json;
} PaneDescription;

static const PaneDescription panes[6] =
{
  { "setup",   PKGDATADIR "/setup.json" },
  { "back",    PKGDATADIR "/back.json" },
  { "home",    PKGDATADIR "/home.json" },
  { "info",    PKGDATADIR "/info.json" },
  { "done",    PKGDATADIR "/done.json" }
};

static const PaneDescription errors[3] =
{
  { "error-back", PKGDATADIR "/error-back.json" },
  { "error-home", PKGDATADIR "/error-home.json" },
  { "error-info", PKGDATADIR "/error-info.json" }
};

static void show_next_pane (Configure *config);
static void show_pane (Configure *config,
                       Pane       pane_nr);

static gchar *
keysym_to_user_string (guint keysym)
{
  switch (keysym)
    {
    case CLUTTER_KEY_space:
      return "Space";
    case CLUTTER_KEY_Return:
      return "Return";
    case CLUTTER_KEY_BackSpace:
      return "BackSpace";
    case CLUTTER_KEY_Tab:
      return "Tabulation";
    case CLUTTER_KEY_Delete:
      return "Delete";
    default:
      return NULL;
    }
}

static const gchar *
state_to_string (BindingState state)
{
  switch (state)
    {
  case STATE_PRESS:
    return "press";
  case STATE_CONFIRM:
    return "confirm";
  case STATE_ERROR:
    return "error";
  case STATE_NEXT:
    return "next";
    }

  return "(unknown state)";
}

static void
push_focus (Configure   *config,
            MxFocusable *next_focused)
{
  MxFocusManager *manager;

  manager = mx_focus_manager_get_for_stage (CLUTTER_STAGE (config->stage));
  mx_focus_manager_push_focus_with_hint (manager,
                                         next_focused,
                                         MX_FOCUS_HINT_FIRST);
}

static const gchar *
keysym_to_string (guint keysym)
{
  return XKeysymToString (keysym);
}

static void
write_bindings (GArray *bindings)
{
  gchar *filename, *content;
  GKeyFile *file;
  GError *error = NULL;
  gint i;

  file = g_key_file_new ();

  for (i = 0; i < bindings->len; i++)
    {
      Binding *binding;

      binding = &g_array_index (bindings, Binding, i);
      g_key_file_set_integer (file,
                              "bindings",
                              keysym_to_string (binding->keysym),
                              binding->keycode);
    }

  content = g_key_file_to_data (file, NULL, NULL);

  filename = g_build_filename (rebinder_get_config_dir (),
                               "rebinder.conf",
                               NULL);

  g_file_set_contents (filename, content, -1, &error);
  if (error)
      g_warning ("Could not write %s: %s", filename, error->message);

  g_free (filename);
  g_free (content);
}

static gboolean
is_binding_pane (Pane pane)
{
  return (pane >= PANE_BINDING_FIRST) && (pane <= PANE_BINDING_LAST);
}

static gboolean
is_error_pane (Pane pane)
{
  return (pane >= PANE_ERROR_BASE) && (pane < PANE_ERROR_LAST);
}

static gboolean
is_white_event (ClutterKeyEvent *event)
{
  /* allow to rebind Escape */
  return event->keyval == CLUTTER_KEY_Escape;
}

static gchar *
build_remote_filename (const gchar *pane,
                       const gchar *suffix)
{
  gchar *file, *path;

  if (suffix == NULL)
    file = g_strconcat ("remote-", pane, ".png", NULL);
  else
    file = g_strconcat ("remote-", pane, "-", suffix, ".png", NULL);

  path = g_build_filename (PKGDATADIR, file, NULL);
  g_free (file);

  return path;
}

static void
remote_set_image (Configure   *config,
                  const gchar *suffix)
{
  GError *error = NULL;
  gchar *file;

  file = build_remote_filename (panes[config->current_pane].name, suffix);

  clutter_texture_set_from_file (CLUTTER_TEXTURE (config->remote),
                                 file, &error);
  if (error)
    g_warning ("Could not load remote image %s: %s", file, error->message); 

  g_free (file);
}

static void
bind_to (Configure *config,
         gint       keycode)
{
  gint binding_nr;
  Binding *binding;

  binding_nr = config->current_pane - PANE_BINDING_FIRST;
  binding = &g_array_index (config->bindings, Binding, binding_nr);
  binding->keycode = keycode;

  MEX_NOTE (REMAPPING, "Binding %d to 0x%08x (%s)", binding->keycode,
            (guint) binding->keysym, XKeysymToString (binding->keysym));
}

static void
activate_inactive_text (Configure *config)
{
  MxStylable *actor;
  static const gchar *to_activate[2] = {"inactive-1", "inactive-2"};
  gint i;

  for (i = 0; i < G_N_ELEMENTS (to_activate); i++)
    {
      actor = MX_STYLABLE (clutter_script_get_object (config->script,
                                                      to_activate[i]));
      if (actor)
        mx_stylable_set_style_class (actor, NULL);
    }
}

static gboolean
feed_keycode (Configure *config,
              gint       keycode)
{
  gboolean consumed = TRUE;

  /* we only have to do something special when we are on a "binding" pane */
  if (is_binding_pane (config->current_pane) == FALSE)
    return FALSE;

  /* STATE_NEXT is the final state of the state machine */
  if (config->binding_state == STATE_NEXT)
    return FALSE;

  switch (config->binding_state)
    {
  case STATE_PRESS:
    config->first_keycode = keycode;
    activate_inactive_text (config);
    config->binding_state = STATE_CONFIRM;
    break;

  case STATE_CONFIRM:
    if (config->first_keycode != keycode)
      {
        Pane error_pane;

        error_pane = config->current_pane - PANE_BINDING_FIRST +
          PANE_ERROR_BASE;

        show_pane (config, error_pane);

        config->binding_state = STATE_ERROR;
      }
    else
      {
        bind_to (config, keycode);
        if (config->next_button)
          {
            clutter_actor_show (config->next_button);
            push_focus (config, MX_FOCUSABLE (config->next_button));
          }
        remote_set_image (config, NULL);
        config->binding_state = STATE_NEXT;
      }
    break;

  case STATE_NEXT:
  case STATE_ERROR:
  default:
    consumed = FALSE;
    break;
    }


  MEX_NOTE (STATE, "Switching to state '%s'",
            state_to_string (config->binding_state));

  return consumed;
}

static gboolean
on_stage_captured_event (ClutterActor *actor,
                         ClutterEvent *event,
                         Configure    *config)
{
  ClutterKeyEvent *key_event;
  gunichar uni_char;

  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;

  key_event = (ClutterKeyEvent *) event;

  uni_char = clutter_event_get_key_unicode (event);
  if (uni_char != 0 && !is_white_event(key_event))
    {
      gchar buffer[6];
      gint len;
      gchar *name;

      name = keysym_to_user_string (key_event->keyval);
      if (name == NULL)
        {
          len = g_unichar_to_utf8 (uni_char, buffer);
          buffer[len] = '\0';
          name = buffer;
        }

      MEX_NOTE (REMAPPING, "Blacklisting printable character '%s'", name);

      return FALSE;
    }

  /* We now have our event feed it to the function controlling the binding
   * state machine */
  return feed_keycode (config, key_event->hardware_keycode);
}

static void
on_evdev_key_pressed (guint32  time_,
                      guint32  key,
                      guint32  state,
                      gpointer data)
{
  Configure *config = data;

  /* we only act on KEYPRESS events */
  if (!state)
    return;

  /* and for events > 255 (the other ones go through X) */
  if (key <= 255)
    return;

  /* if there's a static binding already, the rebinder daemon will generate
   * a X event for this key, so ignore it at the lower level */
  if (find_evdev_binding_by_keycode (key))
    return;

  /* We now have our event feed it to the function controlling the binding
   * state machine */
  feed_keycode (config, key);
}

static void
on_next_button_clicked (MxButton  *button,
                        Configure *config)
{
  show_next_pane (config);
}

static void
on_close_button_clicked (MxButton  *button,
                         Configure *config)
{
  write_bindings (config->bindings);
  clutter_main_quit ();
}

static void
on_error_button_clicked (MxButton  *button,
                         Configure *config)
{
  show_pane (config, config->erroneous_pane);
}

static void
on_restart_button_clicked (MxButton  *button,
                           Configure *config)
{
  show_pane (config, PANE_SETUP);
}

static void
apply_quirks (Configure *config)
{
  MxLabel *label;
  ClutterActor *text;

  label = MX_LABEL (clutter_script_get_object (config->script,
                                              "quirk-ellipsize"));
  if (label)
    {
      text = mx_label_get_clutter_text (label);
      clutter_text_set_ellipsize (CLUTTER_TEXT (text), PANGO_ELLIPSIZE_NONE);
    }
}

static const PaneDescription *
get_pane_description (Pane pane)
{
  if (is_error_pane (pane))
    return &errors[pane - PANE_ERROR_BASE];

  return &panes[pane];
}

static void
show_pane (Configure *config,
           Pane       pane_nr)
{
  GError *error = NULL;
  ClutterActor *pane;
  ClutterActor *close_button, *error_button = NULL, *restart;
  MxFocusable *focused;
  const PaneDescription *pane_desc;

  /* We are switching over to an error pane, let's remember where we
   * were from to be able to restore the faulty pane in the "Try again"
   * handler */
  if (is_error_pane (pane_nr))
    config->erroneous_pane = config->current_pane;

  config->current_pane = pane_nr;

  if (config->script)
    g_object_unref (config->script);
  config->script = clutter_script_new ();

  g_assert (((config->current_pane > PANE_FIRST) &&
             (config->current_pane < PANE_LAST)) ||
            is_error_pane (config->current_pane));

  pane_desc = get_pane_description (config->current_pane);

  clutter_script_load_from_file (config->script, pane_desc->json, &error);
  if (error)
    {
      g_critical ("Could not load json file %s: %s", pane_desc->json,
                  error->message);
      return;
    }

  pane = CLUTTER_ACTOR (clutter_script_get_object (config->script, "pane"));
  if (pane == NULL)
    {
      g_critical ("Could not load actor \"pane\"");
      return;
    }

  /* override width and minimum height so panels are consistent */
  g_object_set (G_OBJECT (pane),
                "width", 740.0,
                "min-height", 550.0,
                NULL);

  if (config->next_button_handler)
    {
      /* that also means we have a valid config->next_button */
      g_signal_handler_disconnect (config->next_button,
                                   config->next_button_handler);
      config->next_button_handler = 0;
    }

  /* It's not an error not to have a 'next' button, it will just be the
   * last pane */
  config->next_button =
        CLUTTER_ACTOR (clutter_script_get_object (config->script, "next"));
  if (config->next_button)
    {
      config->next_button_handler =
        g_signal_connect (config->next_button, "clicked",
                          G_CALLBACK (on_next_button_clicked), config);

    }

  close_button = CLUTTER_ACTOR (clutter_script_get_object (config->script,
                                                           "close"));
  if (close_button)
    {
      g_signal_connect (close_button, "clicked",
                        G_CALLBACK (on_close_button_clicked), config);
    }

  /* Hide the "next" button on binding panes. A successful binding will show
   * it back to go to the next pane */
  if (is_binding_pane (config->current_pane))
    {
      if (config->next_button)
        clutter_actor_hide (config->next_button);

      config->remote = CLUTTER_ACTOR (clutter_script_get_object (config->script,
                                                                 "remote"));
      if (config->remote == NULL)
        {
          g_critical ("Could not load actor \"remote\" from a binding pane");
          return;
        }

      /* Binding state init */
      config->binding_state = STATE_PRESS;

      remote_set_image (config, "hl");
    }

  /* Connect the "Try again" handler */
  if (is_error_pane (config->current_pane))
    {
      error_button = CLUTTER_ACTOR (clutter_script_get_object (config->script,
                                                               "error"));
      if (error_button == NULL)
        {
          g_critical ("Could not load actor \"error\" from an error pane");
          return;
        }

      g_signal_connect (error_button, "clicked",
                        G_CALLBACK (on_error_button_clicked), config);

    }

  /* Connect the "goto-start" */
  restart = CLUTTER_ACTOR (clutter_script_get_object (config->script,
                                                      "restart"));
  if (restart)
    {
      g_signal_connect (restart, "clicked",
                        G_CALLBACK (on_restart_button_clicked), config);
    }

  /* sadly, life is not perfect */
  apply_quirks (config);

  mx_bin_set_child (MX_BIN (config->main_frame), pane);

  /* gives the focus to the "next" or "close" button */
  focused = NULL;
  if (config->next_button)
    focused = MX_FOCUSABLE (config->next_button);
  else if (close_button)
    focused = MX_FOCUSABLE (close_button);
  else if (error_button)
    focused = MX_FOCUSABLE (error_button);

  if (focused)
    push_focus (config, focused);
}

static void
show_next_pane (Configure *config)
{
  if (config->current_pane == (PANE_LAST - 1))
    {
      g_warning ("No more pane to show");
      return;
    }

  show_pane (config, config->current_pane + 1);
}

static void
load_style (void)
{
  GError *error = NULL;
  mx_style_load_from_file (mx_style_get_default (),
                           PKGDATADIR "/../style/style.css",
                           &error);

  if (error)
    {
      g_warning (G_STRLOC ": Error loading style: %s", error->message);
      g_error_free (error);
    }
}

static void
on_stage_allocation_changed (ClutterActor *actor,
                             ClutterActorBox *box,
                             ClutterAllocationFlags flags,
                             gpointer user_data)
{
  Configure  *config = user_data;
  gfloat stage_width, frame_width, frame_x;

  stage_width = clutter_actor_get_width (actor);
  frame_width = stage_width * .6;
  clutter_actor_set_width (config->main_frame, (gint) frame_width);

  frame_x = (stage_width - frame_width) / 2.;
  clutter_actor_set_x (config->main_frame, (gint) frame_x);
}

static Binding default_bindings[3] =
{
    {"Back", CLUTTER_KEY_Escape, 0},
    {"Home", CLUTTER_KEY_Super_L, 0},
    {"Info", CLUTTER_KEY_Menu, 0}
};

Configure *
rebinder_configure (Rebinder *rebinder,
		    MxWindow *window,
		    gboolean  evdev,
		    gboolean  fullscreen)
{
  ClutterColor black = {0, 0, 0, 0xff};
  Configure *config;
  gint i;

  config = g_new0 (Configure, 1);

  /* Setup the bindings we want to configure */
  for (i = 0; i < G_N_ELEMENTS (default_bindings); i++)
    {
      Binding *binding = &default_bindings[i];

      binding->keycode = XKeysymToKeycode (rebinder->dpy, binding->keysym);
    }
  config->bindings = g_array_sized_new (FALSE, TRUE, sizeof (Binding), 3);
  g_array_insert_vals (config->bindings, 0, default_bindings, 3);

  /* evdev stuff */
  if (evdev)
    {
      config->evdev_manager = rebinder_evdev_manager_get_default ();
      rebinder_evdev_manager_set_key_notifier (config->evdev_manager,
                                               on_evdev_key_pressed, config);
    }

  /* We can just use the window given to us or create an Mx application and
   * retrieve the window from there */
  if (window == NULL)
    {

      config->external_window = FALSE;

      /* mx stuff */
      config->application = mx_application_new (NULL, NULL, "mex-rebinder", 0);
      config->window = mx_application_create_window (config->application);
    }
  else
    {
      config->external_window = TRUE;
      config->window = g_object_ref (window);
    }

  mx_window_set_has_toolbar (config->window, FALSE);
  config->stage = CLUTTER_ACTOR (mx_window_get_clutter_stage (config->window));

  clutter_stage_set_color (CLUTTER_STAGE (config->stage), &black);
  g_signal_connect_after (config->stage, "allocation-changed",
                          G_CALLBACK (on_stage_allocation_changed), config);
  g_signal_connect (config->stage, "captured-event",
                    G_CALLBACK (on_stage_captured_event), config);

  config->main_frame = mx_frame_new ();
  clutter_actor_set_name (config->main_frame, "main-frame");
  mx_bin_set_alignment (MX_BIN (config->main_frame),
                        MX_ALIGN_MIDDLE,
                        MX_ALIGN_MIDDLE);
  mx_bin_set_fill (MX_BIN (config->main_frame), FALSE, FALSE);

  mx_window_set_child (config->window, config->main_frame);

  load_style ();

  /* Pane init */
  config->current_pane = PANE_FIRST;
  show_next_pane (config);

  /* Let's get the party started */
  clutter_actor_show (config->stage);
  if (fullscreen)
    {
      clutter_stage_set_fullscreen (CLUTTER_STAGE (config->stage), TRUE);
      clutter_stage_hide_cursor (CLUTTER_STAGE (config->stage));
    }
  else
    {
      clutter_stage_set_minimum_size (CLUTTER_STAGE (config->stage), 1280, 720);
    }

  return config;
}

void
rebinder_configure_free (Configure *config)
{
  g_signal_handlers_disconnect_by_func (config->stage,
                                        on_stage_allocation_changed,
                                        config);
  g_signal_handlers_disconnect_by_func (config->stage,
                                        on_stage_captured_event,
                                        config);

  if (config->external_window)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (config->stage),
				      config->main_frame);
    }

  g_array_unref (config->bindings);
  g_free (config);
}
