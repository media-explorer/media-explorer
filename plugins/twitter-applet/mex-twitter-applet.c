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

#include "mex-twitter-applet.h"
#include <mx/mx.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <mex/mex-expander-box.h>
#include <mex/mex-scroll-view.h>
#include <libsocialweb-client/sw-client.h>
#include <mex/mex.h>

#include "mps-view-bridge.h"

#include "mex-tweet-card.h"

G_DEFINE_TYPE (MexTwitterApplet, mex_twitter_applet, MEX_TYPE_APPLET)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEX_TYPE_TWITTER_APPLET, MexTwitterAppletPrivate))

typedef struct _MexTwitterAppletPrivate MexTwitterAppletPrivate;

struct _MexTwitterAppletPrivate {
  ClutterActor *frame;
  ClutterActor *rect;
  ClutterActor *table;
  ClutterActor *blurb_table;
  ClutterActor *close_button;
  ClutterActor *options_button;
  ClutterActor *blurb_label;
  ClutterActor *friends_button;

  ClutterActor *expander_box;
  ClutterActor *options_table;
  ClutterActor *body_box;
  ClutterActor *scroll_view;
  ClutterActor *filter_label;

  SwClient *client;
  SwClientService *service;
  MpsViewBridge *bridge;
  SwClientItemView *view;
  SwClientItemView *trending_view;

  GHashTable *trending_buttons;
  ClutterActor *trending_table;

  MxButtonGroup *button_group;
};

static void
mex_twitter_applet_dispose (GObject *object)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (object);

  if (priv->frame)
  {
    g_object_unref (priv->frame);
    priv->frame = NULL;
  }

  G_OBJECT_CLASS (mex_twitter_applet_parent_class)->dispose (object);
}

static void
mex_twitter_applet_finalize (GObject *object)
{
  G_OBJECT_CLASS (mex_twitter_applet_parent_class)->finalize (object);
}

static const gchar *
mex_twitter_applet_get_id (MexApplet *applet)
{
  return "twitter-applet";
}

static const gchar *
mex_twitter_applet_get_name (MexApplet *applet)
{
  return "Twitter";
}

static const gchar *
mex_twitter_applet_get_description (MexApplet *applet)
{
  return "Tweet with your friends!";
}

static const gchar *
mex_twitter_applet_get_thumbnail (MexApplet *applet)
{
  return "file://" PLUGIN_DATA_DIR "/twitter-applet.png";
}

static void
mex_twitter_applet_class_init (MexTwitterAppletClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MexAppletClass *applet_class = MEX_APPLET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MexTwitterAppletPrivate));

  object_class->dispose = mex_twitter_applet_dispose;
  object_class->finalize = mex_twitter_applet_finalize;

  applet_class->get_id = mex_twitter_applet_get_id;
  applet_class->get_name = mex_twitter_applet_get_name;
  applet_class->get_description = mex_twitter_applet_get_description;
  applet_class->get_thumbnail = mex_twitter_applet_get_thumbnail;
}

static void
_applet_activated_cb (MexTwitterApplet *self,
                      gpointer       userdata)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (self);

  g_debug (G_STRLOC ": Applet activated!");

  mex_applet_present_actor (MEX_APPLET (self),
                            MEX_APPLET_PRESENT_NONE,
                            priv->frame);
}

static void
_close_button_clicked_cb (MxButton  *button,
                          MexApplet *applet)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);

  mex_applet_request_close (applet, priv->frame);

  g_debug (G_STRLOC ": Request close!");
}

static void
_service_query_open_view_cb (SwClientService  *service,
                             SwClientItemView *view_in,
                             gpointer          userdata)
{
  MexTwitterApplet *self = MEX_TWITTER_APPLET (userdata);
  MexTwitterAppletPrivate *priv = GET_PRIVATE (self);

  if (priv->view)
  {
    sw_client_item_view_close (priv->view);
    g_object_unref (priv->view);
  }

  if (view_in)
  {
    priv->view = g_object_ref (view_in);

    mps_view_bridge_set_view (priv->bridge, priv->view);
    sw_client_item_view_start (priv->view);
  }
}

static void
_setup_sw_view (MexTwitterApplet *self,
                const gchar      *query)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (self);

  /* If NULL -> friends feed */
  if (query)
  {
    gchar *blurb;
    GHashTable *params;

    params = g_hash_table_new_full (g_str_hash,
                                    g_str_equal,
                                    g_free,
                                    g_free);

    g_hash_table_insert (params,
                         g_strdup ("keywords"),
                         g_strdup (query));

    sw_client_service_query_open_view (priv->service,
                                       "x-twitter-stream",
                                       params,
                                       _service_query_open_view_cb,
                                       self);
    blurb = g_strdup_printf (_("What people are saying about %s"), query);
    mx_label_set_text (MX_LABEL (priv->blurb_label), blurb);
    g_free (blurb);
    g_hash_table_unref (params);
  } else {
    sw_client_service_query_open_view (priv->service,
                                       "feed",
                                       NULL,
                                       _service_query_open_view_cb,
                                       self);

    mx_label_set_text (MX_LABEL (priv->blurb_label), _("What your friends are saying"));
  }
}

static void
_topic_button_clicked_cb (MxButton *button,
                          gpointer  userdata)
{
  g_message (G_STRLOC ": Trending button %s clicked",
             mx_button_get_label (button));
}


static void
_update_trending_table (MexTwitterApplet *applet)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);
  GList *l, *values;
  gint count = 0;
  values = g_hash_table_get_values (priv->trending_buttons);

  for (l = values; l; l = l->next)
  {
    if (count >= 10)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->trending_table),
                                      (ClutterActor *)l->data);
      break;
    }

    clutter_container_child_set (CLUTTER_CONTAINER (priv->trending_table),
                                 (ClutterActor *)l->data,
                                 "row", count / 5,
                                 "column", count % 5,
                                 "x-expand", TRUE,
                                 "y-expand", TRUE,
                                 "x-fill", TRUE,
                                 "y-fill", TRUE,
                                 NULL);
    clutter_container_raise_child (CLUTTER_CONTAINER (priv->trending_table),
                                   (ClutterActor *)l->data,
                                   NULL);
    count++;
  }

  g_list_free (values);
}

static void
_button_toggled_cb (ClutterActor *actor,
                    GParamSpec   *pspec,
                    gpointer      userdata)
{
  if (mx_button_get_toggled (MX_BUTTON (actor)))
  {
    mx_stylable_set_style_class (MX_STYLABLE (actor),
                                 "mex-twitter-applet-toggled-button");
  } else {
    mx_stylable_set_style_class (MX_STYLABLE (actor),
                                 "mex-twitter-applet-untoggled-button");
  }
}


static void
_options_button_toggled_cb (ClutterActor *actor,
                            GParamSpec   *pspec,
                            gpointer      userdata)
{
  if (mx_button_get_toggled (MX_BUTTON (actor)))
  {
    mx_stylable_set_style_class (MX_STYLABLE (actor),
                                 "mex-twitter-applet-options-button-toggled");
  } else {
    mx_stylable_set_style_class (MX_STYLABLE (actor),
                                 "mex-twitter-applet-options-button");
  }
}

static void
_trending_view_items_added_cb (SwClientItemView *view,
                               GList            *items_added,
                               gpointer          userdata)
{
  MexTwitterApplet *applet = MEX_TWITTER_APPLET (userdata);
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);
  GList *l;

  for (l = items_added; l; l = l->next)
  {
    SwItem *item = (SwItem *)l->data;
    const gchar *content;
    const gchar *id;
    ClutterActor *button;

    content = sw_item_get_value (item, "content");
    id = item->uuid;

    button = mx_button_new_with_label (content);
    mx_button_set_is_toggle (MX_BUTTON (button), TRUE);
    mx_button_group_add (priv->button_group, MX_BUTTON (button));
    mx_bin_set_alignment (MX_BIN (button), MX_ALIGN_START, MX_ALIGN_MIDDLE);
    mx_stylable_set_style_class (MX_STYLABLE (button),
                                 "mex-twitter-applet-untoggled-button");
    g_hash_table_insert (priv->trending_buttons,
                         g_strdup (id),
                         button);

    g_signal_connect (button,
                      "clicked",
                      (GCallback)_topic_button_clicked_cb,
                      applet);
    g_signal_connect (button,
                      "notify::toggled",
                      (GCallback)_button_toggled_cb,
                      NULL);

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->trending_table),
                                 button);
  }

  _update_trending_table (applet);
}

static void
_trending_view_items_removed_cb (SwClientItemView *view,
                                 GList            *items_removed,
                                 gpointer          userdata)
{
  MexTwitterApplet *applet = MEX_TWITTER_APPLET (userdata);
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);
  GList *l;

  for (l = items_removed; l; l = l->next)
  {
    SwItem *item = (SwItem *)l->data;
    const gchar *content;
    const gchar *id;

    content = sw_item_get_value (item, "content");
    id = item->uuid;

    g_hash_table_remove (priv->trending_buttons,
                         id);
  }

  _update_trending_table (applet);
}

static void
_service_query_trending_open_view_cb (SwClientService  *service,
                                      SwClientItemView *view_in,
                                      gpointer          userdata)
{
  MexTwitterApplet *applet = MEX_TWITTER_APPLET (userdata);
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);

  if (view_in)
  {
    priv->trending_view= g_object_ref (view_in);

    sw_client_item_view_start (view_in);

    g_signal_connect (view_in,
                      "items-added",
                      (GCallback)_trending_view_items_added_cb,
                      applet);
    g_signal_connect (view_in,
                      "items-removed",
                      (GCallback)_trending_view_items_removed_cb,
                      applet);
  }
}

static void
_setup_trending_sw_view (MexTwitterApplet *self)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (self);

  sw_client_service_query_open_view (priv->service,
                                     "x-twitter-trending-topics",
                                     NULL,
                                     _service_query_trending_open_view_cb,
                                     self);
}

static ClutterActor *
_item_factory_func (MpsViewBridge *bridge,
                    SwItem        *item,
                    gpointer       userdata)
{
  return g_object_new (MEX_TYPE_TWEET_CARD,
                       "item", item,
                       NULL);
}

static void
_notify_button_group_cb (MxButtonGroup *button_group,
                         GParamSpec    *pspec,
                         gpointer       userdata)
{
  MexTwitterApplet *applet = MEX_TWITTER_APPLET (userdata);
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);
  const gchar *query;
  MxButton *active_button;

  active_button = mx_button_group_get_active_button (priv->button_group);

  if (!active_button)
    return;

  if ((ClutterActor *)active_button == priv->friends_button)
  {
    query = NULL;
  } else {
    query =  mx_button_get_label (active_button);
  }

  /* Main view */
  _setup_sw_view (applet, query);
}

static void
_hadjust_notify_value_cb (MxAdjustment     *adj,
                          GParamSpec       *pspec,
                          MexTwitterApplet *applet)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);

  if (mx_adjustment_get_value (adj) > 0.0)
  {
    mps_view_bridge_set_paused (priv->bridge, TRUE);
  } else {
    mps_view_bridge_set_paused (priv->bridge, FALSE);
  }
}

static void
_expander_box_notify_open_cb (MexExpanderBox    *expander_box,
                             GParamSpec       *pspec,
                             MexTwitterApplet *applet)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);

  if (mex_expander_box_get_open (expander_box))
    mex_push_focus (MX_FOCUSABLE (priv->options_table));
}

static void
mex_twitter_applet_init (MexTwitterApplet *self)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (self);
  MxAdjustment *hadjust;

  priv->client = sw_client_new ();
  priv->service = sw_client_get_service (priv->client, "twitter");
  priv->bridge = mps_view_bridge_new ();


  priv->frame = g_object_ref_sink (mx_frame_new ());
  priv->table = mx_table_new ();
  priv->blurb_table = mx_table_new ();
  priv->blurb_label = mx_label_new_with_text (_("Unable to get content from Twitter. Is it configured?"));

  /* The options area including filtering buttons and close */
  priv->options_table = mx_table_new ();
  clutter_actor_set_name (priv->options_table, "mex-twitter-applet-options-table");
  mx_table_set_column_spacing (MX_TABLE (priv->options_table), 6);

  /* Filter label */
  priv->filter_label = mx_label_new_with_text (_("Filter:"));
  clutter_actor_set_name (priv->filter_label, "mex-twitter-applet-filter-label");

  mx_table_add_actor_with_properties (MX_TABLE (priv->options_table),
                                      priv->filter_label,
                                      0, 0,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      "y-align", MX_ALIGN_START,
                                      NULL);

  /* Filtering buttons */
  priv->button_group = mx_button_group_new ();
  g_signal_connect (priv->button_group,
                    "notify::active-button",
                    (GCallback)_notify_button_group_cb,
                    self);


  /* Friends button */
  priv->friends_button = mx_button_new_with_label (_("Friends"));
  mx_button_set_is_toggle (MX_BUTTON (priv->friends_button), TRUE);
  mx_button_group_add (priv->button_group, MX_BUTTON (priv->friends_button));
  mx_stylable_set_style_class (MX_STYLABLE (priv->friends_button),
                               "mex-twitter-applet-untoggled-button");
  g_signal_connect (priv->friends_button,
                    "notify::toggled",
                    (GCallback)_button_toggled_cb,
                    NULL);

  mx_table_add_actor_with_properties (MX_TABLE (priv->options_table),
                                      priv->friends_button,
                                      0, 1,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      "y-align", MX_ALIGN_START,
                                      NULL);

  /* Trend buttons */
  priv->trending_table = mx_table_new ();

  mx_table_add_actor_with_properties (MX_TABLE (priv->options_table),
                                      priv->trending_table,
                                      0, 2,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);



  priv->trending_buttons = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  (GDestroyNotify)clutter_actor_destroy);

  /* Will populate trending buttons */
  _setup_trending_sw_view (self);

  /* Will trigger the opening of the default view */
  mx_button_group_set_active_button (priv->button_group,
                                     MX_BUTTON (priv->friends_button));


  {
    ClutterActor *inner_hbox, *icon, *label;

    /* Close button */
    priv->close_button = mx_button_new ();
    clutter_actor_set_name (priv->close_button, "mex-twitter-applet-close-button");

    inner_hbox = mx_box_layout_new ();
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (inner_hbox), MX_ORIENTATION_HORIZONTAL);

    label = mx_label_new_with_text (_("Close"));
    icon = mx_icon_new ();

    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (inner_hbox),
                                             label,
                                             0,
                                             "expand", TRUE,
                                             "x-fill", TRUE,
                                             "y-fill", FALSE,
                                             "y-align", MX_ALIGN_MIDDLE,
                                             NULL);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (inner_hbox),
                                             icon,
                                             1,
                                             "expand", FALSE,
                                             "y-align", MX_ALIGN_MIDDLE,
                                             "x-fill", FALSE,
                                             NULL);
    mx_bin_set_child (MX_BIN (priv->close_button), inner_hbox);
    mx_bin_set_fill (MX_BIN (priv->close_button), TRUE, TRUE);

    mx_table_add_actor_with_properties (MX_TABLE (priv->options_table),
                                        priv->close_button,
                                        0, 3,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-expand", TRUE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_END,
                                        NULL);

    clutter_actor_set_width (priv->close_button, 160);
  }

  {
    ClutterActor *inner_hbox, *icon, *label;

    /* Options button */
    priv->options_button = mx_button_new ();
    mx_button_set_is_toggle (MX_BUTTON (priv->options_button), TRUE);
    mx_stylable_set_style_class (MX_STYLABLE (priv->options_button),
                                 "mex-twitter-applet-options-button");

    inner_hbox = mx_box_layout_new ();
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (inner_hbox), MX_ORIENTATION_HORIZONTAL);

    label = mx_label_new_with_text (_("Options"));
    icon = mx_icon_new ();
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (inner_hbox),
                                             label,
                                             0,
                                             "expand", TRUE,
                                             "x-fill", TRUE,
                                             "y-fill", FALSE,
                                             "y-align", MX_ALIGN_MIDDLE,
                                             NULL);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (inner_hbox),
                                             icon,
                                             1,
                                             "expand", FALSE,
                                             "y-align", MX_ALIGN_MIDDLE,
                                             "x-fill", FALSE,
                                             NULL);
    mx_bin_set_child (MX_BIN (priv->options_button), inner_hbox);
    mx_bin_set_fill (MX_BIN (priv->options_button), TRUE, TRUE);

    g_signal_connect (priv->options_button,
                      "notify::toggled",
                      (GCallback)_options_button_toggled_cb,
                      NULL);

    clutter_actor_set_width (priv->options_button, 160);
  }

  priv->expander_box = mex_expander_box_new ();

  /* Main body box for the tweets */
  priv->body_box = mx_box_layout_new ();
  clutter_actor_set_name (priv->body_box, "mex-twitter-applet-body-box");
  mx_box_layout_set_scroll_to_focused (MX_BOX_LAYOUT (priv->body_box), FALSE);


  /* Monitor the adjustments on the box to know if it's scrolled. If it is
   * then the bridge will be paused.
   */
  mx_scrollable_get_adjustments (MX_SCROLLABLE (priv->body_box),
                                 &hadjust,
                                 NULL);
  g_signal_connect (hadjust,
                    "notify::value",
                    (GCallback)_hadjust_notify_value_cb,
                    self);

  priv->scroll_view = mex_scroll_view_new ();
  clutter_actor_set_name (priv->scroll_view, "mex-twitter-applet-scroll-view");
  mex_scroll_view_set_indicators_hidden (MEX_SCROLL_VIEW (priv->scroll_view), TRUE);


  clutter_container_add_actor (CLUTTER_CONTAINER (priv->frame),
                               priv->table);
  mx_bin_set_fill (MX_BIN (priv->frame), TRUE, FALSE);
  mx_bin_set_alignment (MX_BIN (priv->frame), MX_ALIGN_START, MX_ALIGN_END);


  g_signal_connect (self,
                    "activated",
                    (GCallback)_applet_activated_cb,
                    NULL);
  g_signal_connect (priv->close_button,
                    "clicked",
                    (GCallback)_close_button_clicked_cb,
                    self);
  g_signal_connect (priv->expander_box,
                    "notify::open",
                    (GCallback)_expander_box_notify_open_cb,
                    self);

  g_object_bind_property (priv->options_button,
                          "toggled",
                          priv->expander_box,
                          "open",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  clutter_container_add (CLUTTER_CONTAINER (priv->expander_box),
                         priv->blurb_table, priv->options_table, NULL);
  mex_expander_box_set_important (MEX_EXPANDER_BOX (priv->expander_box), TRUE);
  mex_expander_box_set_expand (MEX_EXPANDER_BOX (priv->expander_box), TRUE);


  mx_table_add_actor_with_properties (MX_TABLE (priv->table),
                                      priv->scroll_view,
                                      0, 0,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);
  mx_table_add_actor_with_properties (MX_TABLE (priv->table),
                                      priv->expander_box,
                                      1, 0,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (priv->blurb_table),
                                      priv->blurb_label,
                                      0, 0,
                                      "x-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (priv->blurb_table),
                                      priv->options_button,
                                      0, 1,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-expand", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_END,
                                      NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->scroll_view), priv->body_box);

  clutter_actor_set_height (priv->scroll_view, 191);

  mx_bin_set_fill (MX_BIN (priv->scroll_view), TRUE, TRUE);

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->body_box), 32);

  mps_view_bridge_set_container (priv->bridge, CLUTTER_CONTAINER (priv->body_box));
  mps_view_bridge_set_factory_func (priv->bridge,
                                    _item_factory_func,
                                    self);



  clutter_actor_set_name (priv->blurb_label, "mex-twitter-applet-blurb-label");
  clutter_actor_set_name (priv->blurb_table, "mex-twitter-applet-blurb-table");
  clutter_actor_set_name (priv->expander_box, "mex-twitter-applet-expander-box");

  mx_style_load_from_file (mx_style_get_default (),
                           PLUGIN_DATA_DIR "/twitter-applet.css",
                           NULL);
}

static void
_update_status_cb (SwClientService *service,
                   const GError    *error,
                   gpointer         userdata)
{

}

void
mex_twitter_applet_share (MexTwitterApplet *applet,
                          MexContent       *content)
{
  MexTwitterAppletPrivate *priv = GET_PRIVATE (applet);
  gchar *message;
  const gchar *title, *uri;
  g_debug (G_STRLOC ": Asked to share content %s",
           mex_content_get_metadata (content,
                                     MEX_CONTENT_METADATA_TITLE));


  title = mex_content_get_metadata (content, MEX_CONTENT_METADATA_TITLE);
  uri = mex_content_get_metadata (content, MEX_CONTENT_METADATA_URL);

  if (uri)
    {
      message = g_strdup_printf ("I think %s is great: %s", title, uri);
    } else {
      message = g_strdup_printf ("I think %s is great", title);
    }

  sw_client_service_update_status (priv->service,
                                   _update_status_cb,
                                   message,
                                   NULL);
  g_free (message);
}
