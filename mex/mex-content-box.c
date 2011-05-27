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


#include "mex-content-box.h"
#include "mex-action-button.h"
#include "mex-action-manager.h"
#include "mex-content-view.h"
#include "mex-main.h"
#include "mex-tile.h"
#include "mex-program.h"
#include "mex-private.h"
#include "mex-utils.h"
#include "mex-queue-button.h"
#include "mex-aspect-frame.h"
#include "mex-info-panel.h"
#include "mex-content-tile.h"

static void mex_content_view_iface_init (MexContentViewIface *iface);
G_DEFINE_TYPE_WITH_CODE (MexContentBox, mex_content_box, MEX_TYPE_EXPANDER_BOX,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_CONTENT_VIEW,
                                                mex_content_view_iface_init))

#define CONTENT_BOX_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_CONTENT_BOX, MexContentBoxPrivate))

enum
{
  PROP_0,

  PROP_MEDIA_URL,
  PROP_LOGO_URL,

  PROP_THUMB_WIDTH,
  PROP_THUMB_HEIGHT
};

enum
{
  GOT_THUMB,

  LAST_SIGNAL
};

#define MAX_DESCRIPTION_HEIGHT 200.0

struct _MexContentBoxPrivate
{
  guint         created_menu    : 1;

  MexContent   *content;
  GList        *bindings;

  MexModel     *model;

  ClutterActor *box;
  ClutterActor *menu_layout; /* ref sinked actor */
  ClutterActor *tile;
  ClutterActor *panel;

  gchar        *thumb_url;
  gchar        *media_url;
  gchar        *logo_url;
  gchar        *description;

  gint          thumb_width;
  gint          thumb_height;
};

static guint signals[LAST_SIGNAL] = { 0, };

static ClutterColor hline_color = { 255, 255, 255, 51 };

/* MexContentViewIface */

static struct _Bindings {
  MexContentMetadata id;
  char *target;
  GBindingTransformFunc fallback;
} content_bindings[] = {
  { MEX_CONTENT_METADATA_STREAM, "media-url", NULL },
  { MEX_CONTENT_METADATA_STATION_LOGO, "logo-url", NULL },
  { MEX_CONTENT_METADATA_NONE, NULL, NULL }
};

static void
mex_content_box_set_content (MexContentView *view,
                             MexContent     *content)
{
  MexContentBox *box = (MexContentBox *) view;
  MexContentBoxPrivate *priv;

  priv = box->priv;

  if (priv->content == content)
    return;

  mex_content_view_set_content (MEX_CONTENT_VIEW (priv->tile), content);

  if (priv->content)
    {
      GList *l;

      for (l = priv->bindings; l; l = l->next)
        g_object_unref (l->data);

      g_list_free (priv->bindings);
      priv->bindings = NULL;

      g_object_unref (priv->content);
      priv->content = NULL;
    }

  if (content)
    {
      int i;

      priv->content = g_object_ref_sink (content);
      for (i = 0; content_bindings[i].id != MEX_CONTENT_METADATA_NONE; i++)
        {
          const gchar *property;
          GBinding *binding;

          property = mex_content_get_property_name (content,
                                                    content_bindings[i].id);

          if (property == NULL)
            {
              /* The Content does not provide a GObject property for this
               * kind of metadata, we can only sync at creation time */
              const gchar *metadata;

              metadata = mex_content_get_metadata (content,
                                                   content_bindings[i].id);
              g_object_set (box, content_bindings[i].target, metadata, NULL);

              continue;
            }

          if (content_bindings[i].fallback)
            binding = g_object_bind_property_full (content, property,
                                                   box,
                                                   content_bindings[i].target,
                                                   G_BINDING_SYNC_CREATE,
                                                   content_bindings[i].fallback,
                                                   NULL,
                                                   content, NULL);
          else
            binding = g_object_bind_property (content, property, box,
                                              content_bindings[i].target,
                                              G_BINDING_SYNC_CREATE);

          priv->bindings = g_list_prepend (priv->bindings, binding);
        }
    }
}

static MexContent *
mex_content_box_get_content (MexContentView *view)
{
  MexContentBox *box = MEX_CONTENT_BOX (view);
  MexContentBoxPrivate *priv = box->priv;

  return priv->content;
}

static void
mex_content_box_set_context (MexContentView *view,
                             MexModel       *model)
{
  MexContentBox *box = MEX_CONTENT_BOX (view);
  MexContentBoxPrivate *priv = box->priv;

  if (priv->model == model)
    return;

  if (priv->model)
    g_object_unref (priv->model);

  priv->model = g_object_ref (model);
}

static MexModel*
mex_content_box_get_context (MexContentView *view)
{
  MexContentBox *box = MEX_CONTENT_BOX (view);
  MexContentBoxPrivate *priv = box->priv;

  return priv->model;
}

static void
mex_content_view_iface_init (MexContentViewIface *iface)
{
  iface->set_content = mex_content_box_set_content;
  iface->get_content = mex_content_box_get_content;

  iface->set_context = mex_content_box_set_context;
  iface->get_context = mex_content_box_get_context;
}

static void
mex_content_box_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MexContentBox *box = MEX_CONTENT_BOX (object);
  MexContentBoxPrivate *priv = box->priv;

  switch (property_id)
    {
    case PROP_MEDIA_URL:
      g_value_set_string (value, priv->media_url);
      break;

    case PROP_LOGO_URL:
      g_value_set_string (value, priv->logo_url);
      break;

    case PROP_THUMB_WIDTH:
      g_value_set_int (value, priv->thumb_width);
      break;

    case PROP_THUMB_HEIGHT:
      g_value_set_int (value, priv->thumb_height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_content_box_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MexContentBox *self = MEX_CONTENT_BOX (object);
  MexContentBoxPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_MEDIA_URL:
      g_free (priv->media_url);
      priv->media_url = g_value_dup_string (value);
      break;

    case PROP_LOGO_URL:
      /* FIXME: We want the logo URL to be file:// to share the same
       * underlying texture. This should be handled by a generic "download
       * queue + texture cache" thingy that caches the same URL to a local
       * file and hands over a ClutterTexure (or a MagicTexture) with the
       * same underlying Cogl texture */
    {
      MxTextureCache *cache;
      ClutterActor *logo, *logo_frame;
      GFile *file;
      gchar *path;
      gint bw, bh;
      gfloat ratio;

      g_free (priv->logo_url);
      priv->logo_url = g_value_dup_string (value);

      if (priv->logo_url == NULL)
        break;

      file = g_file_new_for_uri (priv->logo_url);
      path = g_file_get_path (file);
      g_object_unref (file);
      if (G_UNLIKELY (path == NULL))
        {
          g_warning ("The logo URL provided is not local, refusing to load it");
          break;
        }

      cache = mx_texture_cache_get_default ();
      logo = mx_texture_cache_get_actor (cache, path);
      if (G_UNLIKELY (logo == NULL))
        {
          g_warning ("Could not retrieve texture for %s", path);
          break;
        }

      logo_frame = mex_aspect_frame_new ();
      /* FIXME, had to set the size (for now?) provides some GObject properties
       * to tune that? expose it in the CSS */
      clutter_actor_set_size (logo_frame, 60, 40);
      clutter_texture_get_base_size (CLUTTER_TEXTURE (logo), &bw, &bh);
      ratio = bh / 40.;
      mex_aspect_frame_set_ratio (MEX_ASPECT_FRAME (logo_frame), ratio);
      clutter_container_add_actor (CLUTTER_CONTAINER (logo_frame), logo);

      mex_tile_set_primary_icon (MEX_TILE (priv->tile), logo_frame);
    }
    break;

    case PROP_THUMB_WIDTH:
      priv->thumb_width = g_value_get_int (value);
      g_object_set (G_OBJECT (priv->tile),
                    "thumb-width",
                    priv->thumb_width,
                    NULL);
      break;

    case PROP_THUMB_HEIGHT:
      priv->thumb_height = g_value_get_int (value);
      g_object_set (G_OBJECT (priv->tile),
                    "thumb-height",
                    priv->thumb_height,
                    NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mex_content_box_dispose (GObject *object)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (object)->priv;
  GList *l;

  if (priv->bindings)
    {
      for (l = priv->bindings; l; l = l->next)
        g_object_unref (l->data);

      g_list_free (priv->bindings);
      priv->bindings = NULL;
    }

  if (priv->content)
    {
      g_object_unref (priv->content);
      priv->content = NULL;
    }

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->menu_layout)
    {
      g_object_unref (priv->menu_layout);
      priv->menu_layout = NULL;
    }

  G_OBJECT_CLASS (mex_content_box_parent_class)->dispose (object);
}

static void
mex_content_box_finalize (GObject *object)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (object)->priv;

  g_free (priv->thumb_url);
  g_free (priv->media_url);
  g_free (priv->logo_url);
  g_free (priv->description);

  G_OBJECT_CLASS (mex_content_box_parent_class)->finalize (object);
}

static void
mex_content_box_notify_open_cb (MexExpanderBox *box,
                                GParamSpec     *pspec)
{
  ClutterStage *stage;
  MxFocusManager *fmanager;

  MexActionManager *manager = mex_action_manager_get_default ();
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (box)->priv;
  gboolean open = mex_expander_box_get_open (box);

  if (!open)
    {
      mex_expander_box_set_secondary_child (MEX_EXPANDER_BOX (priv->box), NULL);
      priv->created_menu = FALSE;
      return;
    }

  stage = (ClutterStage *)clutter_actor_get_stage (CLUTTER_ACTOR (box));
  fmanager = mx_focus_manager_get_for_stage (stage);

  if (!priv->created_menu)
    {
      GList *actions;

      priv->created_menu = TRUE;

      mex_content_view_set_content (MEX_CONTENT_VIEW (priv->panel),
                                    priv->content);

      actions = mex_action_manager_get_actions_for_content (manager,
                                                            priv->content);

      if (actions)
        {
          GList *a;
          ClutterActor *layout = priv->menu_layout;
          ClutterActor *hline;

          /* Clear old menu contents */
          clutter_container_foreach (CLUTTER_CONTAINER (layout),
                                     (ClutterCallback)clutter_actor_destroy,
                                     NULL);


          /* separator */
          hline = clutter_rectangle_new_with_color (&hline_color);
          clutter_actor_set_height (hline, 1);
          clutter_container_add_actor (CLUTTER_CONTAINER (layout), hline);

          /* Fill in new menu contents */
          for (a = actions; a; a = a->next)
            {
              MxAction *action = a->data;
              ClutterActor *button;

              /* We treat the queue action specially .. we don't turn the
               * action into an action button we instead use the special queue
               * action button which allows our special effects
               */
              if (g_str_equal (mx_action_get_name (action), "enqueue"))
                {
                  button = mex_queue_button_new ();
                  mex_content_view_set_content (MEX_CONTENT_VIEW (button),
                                                priv->content);
                }
              else
                {
                  button = mex_action_button_new (action);

                  mx_bin_set_fill (MX_BIN (button), TRUE, FALSE);
                  mex_action_set_content (action, priv->content);
                  mex_action_set_context (action, priv->model);
                }

                clutter_container_add_actor (CLUTTER_CONTAINER (layout),
                                             button);
                g_object_set (G_OBJECT (button), "min-width", 240.0, NULL);
            }

          mex_expander_box_set_secondary_child (MEX_EXPANDER_BOX (priv->box),
                                               layout);

          mx_focus_manager_push_focus (fmanager, MX_FOCUSABLE (layout));

          g_list_free (actions);
        }

      /* Make sure the menu box is open */
      mex_expander_box_set_open (MEX_EXPANDER_BOX (priv->box), TRUE);
    }
  else
    {
      mx_focus_manager_push_focus (fmanager, MX_FOCUSABLE (priv->tile));
    }
}

static gboolean
mex_content_box_key_press_event_cb (ClutterActor    *actor,
                                    ClutterKeyEvent *event,
                                    MexExpanderBox  *drawer)
{
  gboolean open;

  MexActionManager *manager = mex_action_manager_get_default ();
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (drawer)->priv;

  if (event->keyval != MEX_KEY_OK &&
      event->keyval != MEX_KEY_INFO)
    {
      return FALSE;
    }

  if (event->keyval == MEX_KEY_OK)
    {
      GList *actions;

      actions = mex_action_manager_get_actions_for_content (manager,
                                                            priv->content);

      if (actions)
        {
          MxAction *action = actions->data;

          mex_action_set_content (action, priv->content);
          mex_action_set_context (action, priv->model);

          g_signal_emit_by_name (action, "activated", 0);

          g_list_free (actions);

          return TRUE;
        }
    }

  open = !mex_expander_box_get_open (drawer);

  /* We only want to expand the box if we have either more than one action,
   * or we have description metadata. We already track this when determining
   * if the info icon should be visible, so use that to determine whether
   * we should allow opening here.
   */
  if (open && !mex_tile_get_secondary_icon (MEX_TILE (priv->tile)))
    return FALSE;

  mex_expander_box_set_open (drawer, open);
  mex_expander_box_set_open (MEX_EXPANDER_BOX (priv->box), open);

  return TRUE;
}

static void
mex_content_box_paint (ClutterActor *actor)
{
  MexContentBoxPrivate *priv = MEX_CONTENT_BOX (actor)->priv;

  if (MEX_IS_PROGRAM (priv->content))
    _mex_program_complete (MEX_PROGRAM (priv->content));

  CLUTTER_ACTOR_CLASS (mex_content_box_parent_class)->paint (actor);
}

static void
mex_content_box_notify_key_focus_cb (ClutterStage  *stage,
                                     GParamSpec    *pspec,
                                     MexContentBox *self)
{
  MexContentBoxPrivate *priv = self->priv;
  ClutterActor *focus = clutter_stage_get_key_focus (stage);

  if (focus == priv->tile)
    {
      gboolean show_info;

      if (mex_content_get_metadata (priv->content,
                                    MEX_CONTENT_METADATA_SYNOPSIS) ||
          mex_content_get_metadata (priv->content,
                                    MEX_CONTENT_METADATA_DATE) ||
          mex_content_get_metadata (priv->content,
                                    MEX_CONTENT_METADATA_CREATION_DATE) ||
          mex_content_get_metadata (priv->content,
                                    MEX_CONTENT_METADATA_DURATION))
        show_info = TRUE;
      else
        {
          MexActionManager *manager = mex_action_manager_get_default ();
          GList *actions =
            mex_action_manager_get_actions_for_content (manager, priv->content);

          if (actions && actions->next)
            show_info = TRUE;
          else
            show_info = FALSE;

          g_list_free (actions);
        }

      if (show_info)
        {
          ClutterActor *icon = mx_icon_new ();
          mx_stylable_set_style_class (MX_STYLABLE (icon), "Info");
          mex_tile_set_secondary_icon (MEX_TILE (priv->tile), icon);
        }
    }
  else
    mex_tile_set_secondary_icon (MEX_TILE (priv->tile), NULL);
}

static void
mex_content_box_map (ClutterActor *actor)
{
  ClutterActor *stage;

  CLUTTER_ACTOR_CLASS (mex_content_box_parent_class)->map (actor);

  stage = clutter_actor_get_stage (actor);
  g_signal_connect (stage, "notify::key-focus",
                    G_CALLBACK (mex_content_box_notify_key_focus_cb), actor);
}

static void
mex_content_box_unmap (ClutterActor *actor)
{
  ClutterActor *stage = clutter_actor_get_stage (actor);

  g_signal_handlers_disconnect_by_func (stage,
                                        mex_content_box_notify_key_focus_cb,
                                        actor);

  CLUTTER_ACTOR_CLASS (mex_content_box_parent_class)->unmap (actor);
}

static gboolean
mex_content_box_get_paint_volume (ClutterActor       *actor,
                                  ClutterPaintVolume *volume)
{
  return clutter_paint_volume_set_from_allocation (volume, actor);
}

static void
mex_content_box_class_init (MexContentBoxClass *klass)
{
  GParamSpec *pspec;

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexContentBoxPrivate));

  object_class->get_property = mex_content_box_get_property;
  object_class->set_property = mex_content_box_set_property;
  object_class->dispose = mex_content_box_dispose;
  object_class->finalize = mex_content_box_finalize;

  actor_class->paint = mex_content_box_paint;
  actor_class->get_paint_volume = mex_content_box_get_paint_volume;
  actor_class->map = mex_content_box_map;
  actor_class->unmap = mex_content_box_unmap;

  pspec = g_param_spec_string ("media-url",
                               "Media URL",
                               "URL for the media this box represents.",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MEDIA_URL, pspec);

  pspec = g_param_spec_string ("logo-url",
                               "Logo URL",
                               "URL for the logo of the provider for this "
                               "content",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_LOGO_URL, pspec);

  pspec = g_param_spec_int ("thumb-width",
                            "Thumbnail width",
                            "Scale the width of the thumbnail while loading.",
                            -1, G_MAXINT, 426,
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_THUMB_WIDTH, pspec);

  pspec = g_param_spec_int ("thumb-height",
                            "Thumbnail height",
                            "Scale the height of the thumbnail while loading.",
                            -1, G_MAXINT, 240,
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_THUMB_HEIGHT, pspec);

  signals[GOT_THUMB] = g_signal_new ("got-thumb",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_LAST,
                                     G_STRUCT_OFFSET (MexContentBoxClass,
                                                      got_thumb),
                                     NULL, NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE, 0);
}

static gboolean
mex_content_box_tile_clicked_cb (ClutterActor       *tile,
                                 ClutterButtonEvent *event,
                                 MexContentBox      *self)
{
  MexContentBoxPrivate *priv = self->priv;

  if (mex_expander_box_get_open (MEX_EXPANDER_BOX (self)))
    {
      mex_expander_box_set_open (MEX_EXPANDER_BOX (self), FALSE);
      mex_expander_box_set_open (MEX_EXPANDER_BOX (priv->box), FALSE);
    }
  else
    {
      mex_expander_box_set_open (MEX_EXPANDER_BOX (self), TRUE);
      mex_expander_box_set_open (MEX_EXPANDER_BOX (priv->box), TRUE);
    }

  mex_push_focus (MX_FOCUSABLE (priv->tile));

  return TRUE;
}

static void
mex_content_box_init (MexContentBox *self)
{
  MexContentBoxPrivate *priv = self->priv = CONTENT_BOX_PRIVATE (self);
  ClutterActor *hline, *box;

  priv->thumb_width = 426;
  priv->thumb_height = 240;

  /* Create description panel */
  box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box), MX_ORIENTATION_VERTICAL);

  hline = clutter_rectangle_new_with_color (&hline_color);
  clutter_actor_set_height (hline, 1);

  priv->panel = mex_info_panel_new (MEX_INFO_PANEL_MODE_SIMPLE);

  clutter_container_add (CLUTTER_CONTAINER (box), hline, priv->panel, NULL);


  /* monitor key press events */
  g_signal_connect (self, "key-press-event",
                    G_CALLBACK (mex_content_box_key_press_event_cb), self);

  /* Create tile */
  priv->tile = mex_content_tile_new ();
  g_object_set (G_OBJECT (priv->tile),
                "thumb-width", priv->thumb_width,
                "thumb-height", priv->thumb_height,
                NULL);
  mx_bin_set_fill (MX_BIN (priv->tile), TRUE, TRUE);

  clutter_actor_set_reactive (priv->tile, TRUE);
  g_signal_connect (priv->tile, "button-release-event",
                    G_CALLBACK (mex_content_box_tile_clicked_cb), self);

  /* Create secondary box for tile/menu */
  priv->box = mex_expander_box_new ();
  mex_expander_box_set_important (MEX_EXPANDER_BOX (priv->box), TRUE);
  mex_expander_box_set_grow_direction (MEX_EXPANDER_BOX (priv->box),
                                      MEX_EXPANDER_BOX_RIGHT);
  mex_expander_box_set_primary_child (MEX_EXPANDER_BOX (priv->box), priv->tile);

  /* Pack box and panel into self */
  mex_expander_box_set_primary_child (MEX_EXPANDER_BOX (self), priv->box);
  mex_expander_box_set_secondary_child (MEX_EXPANDER_BOX (self), box);

  /* Create menu layout (but only reference, don't parent) */
  priv->menu_layout = g_object_ref_sink (mx_box_layout_new ());
  mx_stylable_set_style_class (MX_STYLABLE (priv->menu_layout), "ActionList");
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->menu_layout),
                                 MX_ORIENTATION_VERTICAL);

  /* Connect to the open notify signal */
  g_signal_connect (self, "notify::open",
                    G_CALLBACK (mex_content_box_notify_open_cb), NULL);
}

ClutterActor *
mex_content_box_new (void)
{
  return g_object_new (MEX_TYPE_CONTENT_BOX, NULL);
}

ClutterActor *
mex_content_box_get_tile (MexContentBox *box)
{
  g_return_val_if_fail (MEX_IS_CONTENT_BOX (box), NULL);
  return box->priv->tile;
}

ClutterActor *
mex_content_box_get_menu (MexContentBox *box)
{
  g_return_val_if_fail (MEX_IS_CONTENT_BOX (box), NULL);
  return box->priv->menu_layout;
}

ClutterActor *
mex_content_box_get_details (MexContentBox *box)
{
  g_return_val_if_fail (MEX_IS_CONTENT_BOX (box), NULL);
  return box->priv->panel;
}

void
mex_content_box_set_thumbnail_size (MexContentBox *box,
                                    gint           width,
                                    gint           height)
{
  MexContentBoxPrivate *priv;

  g_return_if_fail (MEX_IS_CONTENT_BOX (box));

  priv = box->priv;
  priv->thumb_width = width;
  priv->thumb_height = height;

  g_object_set (G_OBJECT (priv->tile),
                "thumb-width", priv->thumb_width,
                "thumb-height", priv->thumb_height,
                NULL);
}

void
mex_content_box_get_thumbnail_size (MexContentBox *box,
                                    gint          *width,
                                    gint          *height)
{
  MexContentBoxPrivate *priv;

  g_return_if_fail (MEX_IS_CONTENT_BOX (box));

  priv = box->priv;

  if (width)
    *width = priv->thumb_width;
  if (height)
    *height = priv->thumb_height;
}

