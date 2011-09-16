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


#ifndef _MEX_TWEET_CARD
#define _MEX_TWEET_CARD

#include <mx/mx.h>

G_BEGIN_DECLS

#define MEX_TYPE_TWEET_CARD mex_tweet_card_get_type()

#define MEX_TWEET_CARD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_TWEET_CARD, MexTweetCard))

#define MEX_TWEET_CARD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_TWEET_CARD, MexTweetCardClass))

#define MEX_IS_TWEET_CARD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_TWEET_CARD))

#define MEX_IS_TWEET_CARD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_TWEET_CARD))

#define MEX_TWEET_CARD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_TWEET_CARD, MexTweetCardClass))

typedef struct {
  MxStack parent;
} MexTweetCard;

typedef struct {
  MxStackClass parent_class;
} MexTweetCardClass;

GType mex_tweet_card_get_type (void);

G_END_DECLS

#endif /* _MEX_TWEET_CARD */
