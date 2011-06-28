plugin_datadir = $(datadir)/mex/plugins
pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES =

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version
_libadd     = $(MEX_LIBS) $(top_builddir)/mex/libmex-@MEX_API_VERSION@.la

if USE_PLUGIN_LIBRARY
plugins_LTLIBRARIES += 20mex-library.la

20mex_library_la_SOURCES = 		\
	library/mex-library-plugin.c 	\
	library/mex-library-plugin.h	\
	$(NULL)
20mex_library_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Library\"
20mex_library_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_RECOMMENDED
plugins_LTLIBRARIES += 40mex-recommended.la

40mex_recommended_la_SOURCES = 			\
	recommended/mex-recommended-plugin.c	\
	recommended/mex-recommended-plugin.h	\
	$(NULL)
40mex_recommended_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Recommended\"
40mex_recommended_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_SEARCH
plugins_LTLIBRARIES += 00mex-search.la

search_datadir = $(datadir)/mex/plugins/search
dist_search_data_DATA =		\
	search/style.css	\
	search/search-bg.png 	\
	search/textInput-bg.png \
	$(NULL)

00mex_search_la_SOURCES =		\
	search/mex-search-plugin.c	\
	search/mex-search-plugin.h	\
	$(NULL)
00mex_search_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Search\"
00mex_search_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_QUEUE
plugins_LTLIBRARIES += 40mex-queue.la

40mex_queue_la_SOURCES = 		\
	queue/mex-queue-plugin.c	\
	queue/mex-queue-plugin.h	\
	$(NULL)
40mex_queue_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Queue\"
40mex_queue_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_APPLICATIONS
plugins_LTLIBRARIES += 50mex-applications.la

50mex_applications_la_SOURCES =			\
	applications/mex-applications-plugin.c 	\
	applications/mex-applications-plugin.h	\
	$(NULL)
50mex_applications_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Applications\"
50mex_applications_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_UPNP
plugins_LTLIBRARIES += 20mex-upnp.la

20mex_upnp_la_SOURCES =		\
	upnp/mex-upnp-plugin.c	\
	upnp/mex-upnp-plugin.h	\
	$(NULL)
20mex_upnp_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-UPnP\"
20mex_upnp_la_LIBADD  =  $(_libadd)
endif

if USE_PLUGIN_TRACKER
plugins_LTLIBRARIES += 20mex-tracker.la

20mex_tracker_la_SOURCES =		\
	tracker/mex-tracker-plugin.c	\
	tracker/mex-tracker-plugin.h	\
	$(NULL)
20mex_tracker_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Tracker\"
20mex_tracker_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_DBUSINPUT
plugins_LTLIBRARIES += 50mex-dbusinput.la

50mex_dbusinput_la_SOURCES =			\
	dbus-input/mex-dbusinput-plugin.c	\
	dbus-input/mex-dbusinput-plugin.h	\
	$(NULL)
50mex_dbusinput_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Dbusinput\"
50mex_dbusinput_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_BLIPTV
plugins_LTLIBRARIES += 40mex-bliptv.la

40mex_bliptv_la_SOURCES = 		\
	bliptv/mex-bliptv-plugin.c 	\
	bliptv/mex-bliptv-plugin.h
40mex_bliptv_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Bliptv\"
40mex_bliptv_la_LIBADD = $(_libadd)
endif
