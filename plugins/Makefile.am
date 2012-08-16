plugin_datadir = $(pkgdatadir)/plugins
pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES =

EXTRA_DIST =
CLEANFILES =

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_WARNING_CFLAGS)			\
	$(MEX_CFLAGS)				\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version
_libadd     = $(MEX_LIBS) $(top_builddir)/mex/libmex-@MEX_API_VERSION@.la

if USE_PLUGIN_LIBRARY
plugins_LTLIBRARIES += mex-library.la

mex_librarydir = $(plugin_datadir)/library
dist_mex_library_DATA =

mex_library_la_SOURCES = 		\
	library/mex-library-plugin.c 	\
	library/mex-library-plugin.h	\
	$(NULL)
mex_library_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Library\" 	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_librarydir)\"
mex_library_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_RECOMMENDED
plugins_LTLIBRARIES += mex-recommended.la

mex_recommendeddir = $(plugin_datadir)/recommended
dist_mex_recommended_DATA =

mex_recommended_la_SOURCES = 			\
	recommended/mex-recommended-plugin.c	\
	recommended/mex-recommended-plugin.h	\
	$(NULL)
mex_recommended_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Recommended\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_recommendeddir)\"
mex_recommended_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_SEARCH
plugins_LTLIBRARIES += mex-search.la

mex_searchdir = $(plugin_datadir)/search
dist_mex_search_DATA =

mex_search_la_SOURCES =			\
	search/mex-search-plugin.c	\
	search/mex-search-plugin.h	\
	$(NULL)
mex_search_la_CFLAGS  = 		\
	$(PLUGIN_SEARCH_CFLAGS)		\
	-DG_LOG_DOMAIN=\"Mex-Search\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_searchdir)\"
mex_search_la_LIBADD  = 		\
	$(_libadd)			\
	$(PLUGIN_SEARCH_LIBS)
endif

if USE_PLUGIN_QUEUE
plugins_LTLIBRARIES += mex-queue.la

mex_queuedir = $(plugin_datadir)/queue
dist_mex_queue_DATA =

mex_queue_la_SOURCES = 			\
	queue/mex-queue-plugin.c	\
	queue/mex-queue-plugin.h	\
	$(NULL)
mex_queue_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Queue\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_queuedir)\"
mex_queue_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_APPLICATIONS
plugins_LTLIBRARIES += mex-applications.la

mex_applicationsdir = $(plugin_datadir)/applications
dist_mex_applications_DATA =

mex_applications_la_SOURCES =			\
	applications/mex-applications-plugin.c 	\
	applications/mex-applications-plugin.h	\
	$(NULL)
mex_applications_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Applications\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_applicationsdir)\"
mex_applications_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_UPNP
plugins_LTLIBRARIES += mex-upnp.la

mex_upnpdir = $(plugin_datadir)/upnp
dist_mex_upnp_DATA =

mex_upnp_la_SOURCES =		\
	upnp/mex-upnp-plugin.c	\
	upnp/mex-upnp-plugin.h	\
	$(NULL)
mex_upnp_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-UPnP\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_upnpdir)\"
mex_upnp_la_LIBADD  =  $(_libadd)
endif

if USE_PLUGIN_TRACKER
plugins_LTLIBRARIES += mex-tracker.la

mex_trackerdir = $(plugin_datadir)/tracker
dist_mex_tracker_DATA =

mex_tracker_la_SOURCES =		\
	tracker/mex-tracker-plugin.c	\
	tracker/mex-tracker-plugin.h	\
	$(NULL)
mex_tracker_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Tracker\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_trackerdir)\"
mex_tracker_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_DBUSINPUT
plugins_LTLIBRARIES += mex-dbusinput.la

mex_dbusinputdir = $(plugin_datadir)/dbusinput
dist_mex_dbusinput_DATA =

mex_dbusinput_la_SOURCES =			\
	dbus-input/mex-dbusinput-plugin.c	\
	dbus-input/mex-dbusinput-plugin.h	\
	$(NULL)
mex_dbusinput_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Dbusinput\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_dbusinputdir)\"
mex_dbusinput_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_MPRIS
plugins_LTLIBRARIES += mex-mpris.la

#Here so if we don't have a .desktop file installed we can't use it at run time
mex_mime_types = $(shell sed -n -e 's/^MimeType=//p' $(top_builddir)/data/media-explorer.desktop)

mex_mprisdir = $(plugin_datadir)/mpris
dist_mex_mpris_DATA =

mex_mpris_la_SOURCES =			\
	mpris/mex-mpris-plugin.c	\
	mpris/mex-mpris-plugin.h	\
	mpris/mpris-spec.h		\
	$(NULL)
mex_mpris_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Mpris\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_mprisdir)\" \
	-DMEX_MIME_TYPES="\"$(mex_mime_types)\""
mex_mpris_la_LIBADD  = $(_libadd)
endif




if USE_PLUGIN_OPTICALDISC
plugins_LTLIBRARIES += mex-opticaldisc.la

mex_opticaldiscdir = $(plugin_datadir)/opticaldisc
dist_mex_opticaldisc_DATA =

mex_opticaldisc_la_SOURCES =			\
	optical-disc/mex-opticaldisc-plugin.c	\
	optical-disc/mex-opticaldisc-plugin.h	\
	$(NULL)
mex_opticaldisc_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Opticaldisc\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_opticaldiscdir)\"
mex_opticaldisc_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_BLIPTV
plugins_LTLIBRARIES += mex-bliptv.la

mex_bliptvdir = $(plugin_datadir)/bliptv
dist_mex_bliptv_DATA =

mex_bliptv_la_SOURCES = 		\
	bliptv/mex-bliptv-plugin.c 	\
	bliptv/mex-bliptv-plugin.h
mex_bliptv_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Bliptv\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_bliptvdir)\"
mex_bliptv_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_TELEPATHY
plugins_LTLIBRARIES += mex-telepathy.la

mex_telepathy_la_SOURCES = 			\
	telepathy/mex-telepathy-channel.c	\
	telepathy/mex-telepathy-channel.h	\
	telepathy/mex-telepathy-plugin.c 	\
	telepathy/mex-telepathy-plugin.h	\
	telepathy/mex-contact.c 	\
	telepathy/mex-contact.h
mex_telepathy_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Telepathy\" $(PLUGIN_TELEPATHY_CFLAGS)
mex_telepathy_la_LIBADD = $(_libadd) $(PLUGIN_TELEPATHY_LIBS)
endif

if USE_PLUGIN_JUSTINTV
plugins_LTLIBRARIES += mex-justintv.la

mex_justintvdir = $(plugin_datadir)/justintv
dist_mex_justintv_DATA =

mex_justintv_la_SOURCES = 		\
	justintv/mex-justintv-plugin.c 	\
	justintv/mex-justintv-plugin.h
mex_justintv_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Justintv\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_justintvdir)\"
mex_justintv_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_DEBUG_BUDDY
plugins_LTLIBRARIES += mex-debug.la

mex_debugdir = $(plugin_datadir)/debug
dist_mex_debug_DATA =

mex_debug_la_SOURCES =	 		\
	debug/mex-debug-plugin.c 	\
	debug/mex-debug-plugin.h
mex_debug_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Debug\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_debugdir)\"
mex_debug_la_LIBADD = $(_libadd) -lunwind

debugdir = $(pkglibdir)/debug
debug_LTLIBRARIES = mex-gobject-list.la

mex_gobject_list_la_SOURCES =		\
	debug/mex-gobject-list.c	\
	debug/mex-gobject-list.h

do_subst = sed -e 's,[@]pkglibdir[@],$(pkglibdir),'

bin_SCRIPTS = debug/mex-debug

debug/mex-debug: $(srcdir)/debug/mex-debug.in Makefile
	@test -d debug || mkdir debug
	$(AM_V_GEN)$(do_subst) < $(srcdir)/debug/mex-debug.in > $@
	chmod +x $@

EXTRA_DIST += debug/mex-debug.in
CLEANFILES += $(bin_SCRIPTS)
endif

if USE_PLUGIN_GNOME_DVB
plugins_LTLIBRARIES += mex-gnome-dvb.la

mex_gnome_dvbdir = $(plugin_datadir)/gnome-dvb
dist_mex_gnome_dvb_DATA =

mex_gnome_dvb_la_SOURCES = 			\
	gnome-dvb/mex-gnome-dvb-plugin.c	\
	gnome-dvb/mex-gnome-dvb-plugin.h
mex_gnome_dvb_la_CFLAGS = 			\
	-DG_LOG_DOMAIN=\"Mex-GNOME-DVB\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_gnome_dvbdir)\"
mex_gnome_dvb_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_BG_VIDEO
plugins_LTLIBRARIES += mex-bg-video.la

mex_bg_videodir = $(plugin_datadir)/bg-video
dist_mex_bg_video_DATA =

mex_bg_video_la_SOURCES = 		\
	bg-video/mex-bg-video-plugin.c	\
	bg-video/mex-bg-video-plugin.h	\
	bg-video/mex-background-video.c	\
	bg-video/mex-background-video.h
mex_bg_video_la_CFLAGS = 				\
	-DG_LOG_DOMAIN=\"Mex-Background-Video\"		\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_bg_videodir)\"
mex_bg_video_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_BG_DBUSVIDEO
plugins_LTLIBRARIES += mex-bg-dbusvideo.la

mex_bg_dbusvideodir = $(plugin_datadir)/bg-dbusvideo
dist_mex_bg_dbusvideo_DATA =

mex_bg_dbusvideo_la_SOURCES =		 		\
	bg-dbusvideo/mex-bg-dbusvideo-plugin.c		\
	bg-dbusvideo/mex-bg-dbusvideo-plugin.h		\
	bg-dbusvideo/mex-dbus-background-video.c	\
	bg-dbusvideo/mex-dbus-background-video.h
mex_bg_dbusvideo_la_CFLAGS = 				\
	-DG_LOG_DOMAIN=\"Mex-DBus-Background-Video\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_bg_dbusvideodir)\"
mex_bg_dbusvideo_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_BG_BACKDROP
plugins_LTLIBRARIES += mex-bg-backdrop.la

mex_bg_backdropdir = $(plugin_datadir)/bg-backdrop
dist_mex_bg_backdrop_DATA = \
	bg-backdrop/sprite.png

mex_bg_backdrop_la_SOURCES =		 		\
	bg-backdrop/mex-bg-backdrop-plugin.c		\
	bg-backdrop/mex-bg-backdrop-plugin.h		\
	bg-backdrop/mex-backdrop.c			\
	bg-backdrop/mex-backdrop.h
mex_bg_backdrop_la_CFLAGS = 				\
	-DG_LOG_DOMAIN=\"Mex-Background-Backdrop\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_bg_backdropdir)\"
mex_bg_backdrop_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_TWITTER_SEND
plugins_LTLIBRARIES += 60mex-twitter-send.la

60mex_twitter_send_la_SOURCES = 		\
	twitter-send/mex-twitter-send-plugin.c	\
	twitter-send/mex-twitter-send-plugin.h
60mex_twitter_send_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Twitter-Send\" $(PLUGIN_TWITTER_SEND_CFLAGS)
60mex_twitter_send_la_LIBADD = $(_libadd) $(PLUGIN_TWITTER_SEND_LIBS)
endif

if USE_PLUGIN_APPS
plugins_LTLIBRARIES += mex-apps.la

mex_appsdir = $(plugin_datadir)/apps
dist_mex_apps_DATA = apps/data/apps.json apps/data/blank.css
mex_apps_la_SOURCES = 		\
	apps/mex-apps-plugin.c
mex_apps_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Apps\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_appsdir)\" $(PLUGIN_APPS_CFLAGS)
mex_apps_la_LIBADD = $(_libadd) $(PLUGIN_APPS_LIBS)
endif

if USE_PLUGIN_NETWORKS
plugins_LTLIBRARIES += mex-networks.la

mex_networksdir = $(plugin_datadir)/networks
dist_mex_networks_DATA =

mex_networks_la_SOURCES =			\
	networks/mex-networks-plugin.c		\
	networks/mex-networks-plugin.h		\
	$(NULL)

mex_networks_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Networks\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_networks)\"
mex_networks_la_LIBADD  = $(_libadd)
endif
