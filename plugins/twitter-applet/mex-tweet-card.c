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

#include "mex-tweet-card.h"
#include <libsocialweb-client/sw-item.h>

static void _focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MexTweetCard, mex_tweet_card, MX_TYPE_STACK,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                _focusable_iface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TWEET_CARD, MexTweetCardPrivate))

typedef struct _MexTweetCardPrivate MexTweetCardPrivate;

struct _MexTweetCardPrivate {
  ClutterActor *avatar;
  ClutterActor *author_button;
  ClutterActor *content_label;
  ClutterActor *date_label;
  ClutterActor *sub_table;

  SwItem *item;
};

enum
{
  PROP_0,
  PROP_ITEM,
};

static void mex_tweet_card_set_item (MexTweetCard *card,
                                     SwItem       *item);
static SwItem *mex_tweet_card_get_item (MexTweetCard *card);

#define DEFAULT_AVATAR_PATH PLUGIN_DATA_DIR "/missing-avatar.png"

static void
mex_tweet_card_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MexTweetCard *card = MEX_TWEET_CARD (object);

  switch (property_id) {
    case PROP_ITEM:
      g_value_set_boxed (value, mex_tweet_card_get_item (card));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mex_tweet_card_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  MexTweetCard *card = MEX_TWEET_CARD (object);

  switch (property_id) {
    case PROP_ITEM:
      mex_tweet_card_set_item (card, g_value_get_boxed (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mex_tweet_card_dispose (GObject *object)
{
  G_OBJECT_CLASS (mex_tweet_card_parent_class)->dispose (object);
}

static void
mex_tweet_card_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_tweet_card_parent_class)->finalize (object);
}

static void
mex_tweet_card_class_init (MexTweetCardClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MexTweetCardPrivate));

  object_class->get_property = mex_tweet_card_get_property;
  object_class->set_property = mex_tweet_card_set_property;
  object_class->dispose = mex_tweet_card_dispose;
  object_class->finalize = mex_tweet_card_finalize;

  pspec = g_param_spec_boxed ("item",
                              "Item",
                              "Item",
                              SW_TYPE_ITEM,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static void
mex_tweet_card_init (MexTweetCard *self)
{
  MexTweetCardPrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;
  ClutterActor *avatar_frame;

  priv->avatar = g_object_new (CLUTTER_TYPE_TEXTURE,
                               NULL);
  clutter_actor_set_size (priv->avatar, 48, 48);
  avatar_frame = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (avatar_frame),
                               "mex-tweet-card-avatar-frame");

  clutter_container_add_actor (CLUTTER_CONTAINER (avatar_frame), priv->avatar);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), avatar_frame);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               avatar_frame,
                               "x-fill", FALSE,
                               "y-fill", FALSE,
                               "x-align", MX_ALIGN_START,
                               "y-align", MX_ALIGN_START,
                               NULL);

  priv->sub_table = mx_table_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->sub_table),
                               "mex-tweet-card-body");

  priv->author_button = mx_button_new ();
  clutter_actor_set_height (priv->author_button, 48);
  mx_stylable_set_style_class (MX_STYLABLE (priv->author_button),
                               "mex-tweet-card-author-button");

  priv->content_label = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->content_label),
                               "mex-tweet-card-content-label");

  priv->date_label = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->date_label),
                               "mex-tweet-card-date-label");


  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->content_label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);


  mx_table_add_actor_with_properties (MX_TABLE (priv->sub_table),
                                      priv->author_button,
                                      0, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (priv->sub_table),
                                      priv->content_label,
                                      1, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);
  mx_table_add_actor_with_properties (MX_TABLE (priv->sub_table),
                                      priv->date_label,
                                      2, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->sub_table);
}

static void
mex_tweet_card_set_item (MexTweetCard *card,
                         SwItem       *item)
{
  MexTweetCardPrivate *priv = GET_PRIVATE (card);
  const gchar *author_icon = NULL;
  const gchar *content = NULL;
  const gchar *author = NULL;
  GError *error = NULL;
  gchar *time_str;

  sw_item_ref (item);

  if (priv->item)
  {
    sw_item_unref (priv->item);
    priv->item = NULL;
  }

  priv->item = item;

  author_icon = sw_item_get_value (item, "authoricon");

  if (!author_icon)
  {
    author_icon = DEFAULT_AVATAR_PATH;
  }

  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->avatar),
                                      author_icon,
                                      &error))
  {
    g_critical (G_STRLOC ": Error setting avatar texture: %s",
                error->message);
    g_clear_error (&error);
  }

  content = sw_item_get_value (item, "content");
  author = sw_item_get_value (item, "author");

  mx_label_set_text (MX_LABEL (priv->content_label),
                     content);
  mx_button_set_label (MX_BUTTON (priv->author_button),
                       author);

  time_str = mx_utils_format_time (&(item->date));
  mx_label_set_text (MX_LABEL (priv->date_label),
                     time_str);
  g_free (time_str);
}

static SwItem *
mex_tweet_card_get_item (MexTweetCard *card)
{
  MexTweetCardPrivate *priv = GET_PRIVATE (card);
  return priv->item;
}

static MxFocusable*
_accept_focus (MxFocusable *focusable, MxFocusHint hint)
{
  MexTweetCardPrivate *priv = GET_PRIVATE (focusable);

  mx_stylable_set_style_pseudo_class (MX_STYLABLE (priv->sub_table), "focus");
  clutter_actor_grab_key_focus (CLUTTER_ACTOR (priv->sub_table));

  return focusable;
}

static MxFocusable*
_move_focus (MxFocusable      *focusable,
                      MxFocusDirection  direction,
                      MxFocusable      *from)
{
  MexTweetCardPrivate *priv = GET_PRIVATE (focusable);

  /* check if focus is being moved from us */
  if (focusable == from)
    {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (priv->sub_table), "");
    }

  return NULL;
}

static void
_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = _accept_focus;
  iface->move_focus = _move_focus;
}
