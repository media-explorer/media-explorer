pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = \
		      mex-twitter-applet.la

plugin_datadir = ${datadir}/media-explorer/plugins/twitter-applet
plugin_data_DATA = \
		  twitter-applet.png \
		  twitter-applet.css \
		  tweet-card-body.png \
		  tweet-card-author-button-highlight.png \
		  tweet-card-author-button.png \
		  blurb-area-bg.png \
		  button-bg-hover.png \
		  button-check-highlight.png \
		  button-check.png \
		  button-highlight.png \
		  button.png \
		  hr.png \
		  main_bg.png \
		  tweet_bg.png \
		  tweet_bg_highlight.png \
		  button-arrow-down-white.png \
		  button-arrow-down.png \
		  button-arrow-up-white.png \
		  button-arrow-up.png \
		  button-close-white.png \
		  button-close.png \
		  missing-avatar.png
 


AM_CPPFLAGS = $(MEX_CFLAGS) -DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"
AM_LDFLAGS  = -no-undefined -module -avoid-version

# twitter applet
mex_twitter_applet_la_SOURCES = mex-twitter-applet-plugin.c mex-twitter-applet-plugin.h \
			     mex-twitter-applet.c mex-twitter-applet.h \
			     mex-tweet-card.c mex-tweet-card.h \
			     mps-view-bridge.c mps-view-bridge.h
mex_twitter_applet_la_CFLAGS = -DG_LOG_DOMAIN=\"Mex-Twitter-Applet\"
mex_twitter_applet_la_LIBADD = $(MEX_LIBS)
