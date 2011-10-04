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
plugins_LTLIBRARIES += 20mex-library.la

mex_librarydir = $(plugin_datadir)/library
dist_mex_library_DATA =

20mex_library_la_SOURCES = 		\
	library/mex-library-plugin.c 	\
	library/mex-library-plugin.h	\
	$(NULL)
20mex_library_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Library\" 	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_librarydir)\"
20mex_library_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_RECOMMENDED
plugins_LTLIBRARIES += 40mex-recommended.la

mex_recommendeddir = $(plugin_datadir)/recommended
dist_mex_recommended_DATA =

40mex_recommended_la_SOURCES = 			\
	recommended/mex-recommended-plugin.c	\
	recommended/mex-recommended-plugin.h	\
	$(NULL)
40mex_recommended_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Recommended\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_recommendeddir)\"
40mex_recommended_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_SEARCH
plugins_LTLIBRARIES += 00mex-search.la

mex_searchdir = $(plugin_datadir)/search
dist_mex_search_DATA =

00mex_search_la_SOURCES =		\
	search/mex-search-plugin.c	\
	search/mex-search-plugin.h	\
	$(NULL)
00mex_search_la_CFLAGS  = 		\
	$(PLUGIN_SEARCH_CFLAGS)		\
	-DG_LOG_DOMAIN=\"Mex-Search\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_searchdir)\"
00mex_search_la_LIBADD  = 		\
	$(_libadd)			\
	$(PLUGIN_SEARCH_LIBS)
endif

if USE_PLUGIN_QUEUE
plugins_LTLIBRARIES += 40mex-queue.la

mex_queuedir = $(plugin_datadir)/queue
dist_mex_queue_DATA =

40mex_queue_la_SOURCES = 		\
	queue/mex-queue-plugin.c	\
	queue/mex-queue-plugin.h	\
	$(NULL)
40mex_queue_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Queue\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_queuedir)\"
40mex_queue_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_APPLICATIONS
plugins_LTLIBRARIES += 50mex-applications.la

mex_applicationsdir = $(plugin_datadir)/applications
dist_mex_applications_DATA =

50mex_applications_la_SOURCES =			\
	applications/mex-applications-plugin.c 	\
	applications/mex-applications-plugin.h	\
	$(NULL)
50mex_applications_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Applications\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_applicationsdir)\"
50mex_applications_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_UPNP
plugins_LTLIBRARIES += 20mex-upnp.la

mex_upnpdir = $(plugin_datadir)/upnp
dist_mex_upnp_DATA =

20mex_upnp_la_SOURCES =		\
	upnp/mex-upnp-plugin.c	\
	upnp/mex-upnp-plugin.h	\
	$(NULL)
20mex_upnp_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-UPnP\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_upnpdir)\"
20mex_upnp_la_LIBADD  =  $(_libadd)
endif

if USE_PLUGIN_TRACKER
plugins_LTLIBRARIES += 20mex-tracker.la

mex_trackerdir = $(plugin_datadir)/tracker
dist_mex_tracker_DATA =

20mex_tracker_la_SOURCES =		\
	tracker/mex-tracker-plugin.c	\
	tracker/mex-tracker-plugin.h	\
	$(NULL)
20mex_tracker_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Tracker\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_trackerdir)\"
20mex_tracker_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_DBUSINPUT
plugins_LTLIBRARIES += 50mex-dbusinput.la

mex_dbusinputdir = $(plugin_datadir)/dbusinput
dist_mex_dbusinput_DATA =

50mex_dbusinput_la_SOURCES =			\
	dbus-input/mex-dbusinput-plugin.c	\
	dbus-input/mex-dbusinput-plugin.h	\
	$(NULL)
50mex_dbusinput_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Dbusinput\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_dbusinputdir)\"
50mex_dbusinput_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_OPTICALDISC
plugins_LTLIBRARIES += 50mex-opticaldisc.la

mex_opticaldiscdir = $(plugin_datadir)/opticaldisc
dist_mex_opticaldisc_DATA =

50mex_opticaldisc_la_SOURCES =			\
	optical-disc/mex-opticaldisc-plugin.c	\
	optical-disc/mex-opticaldisc-plugin.h	\
	$(NULL)
50mex_opticaldisc_la_CFLAGS  = 			\
	-DG_LOG_DOMAIN=\"Mex-Opticaldisc\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_opticaldiscdir)\"
50mex_opticaldisc_la_LIBADD  = $(_libadd)
endif

if USE_PLUGIN_BLIPTV
plugins_LTLIBRARIES += 40mex-bliptv.la

mex_bliptvdir = $(plugin_datadir)/bliptv
dist_mex_bliptv_DATA =

40mex_bliptv_la_SOURCES = 		\
	bliptv/mex-bliptv-plugin.c 	\
	bliptv/mex-bliptv-plugin.h
40mex_bliptv_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Bliptv\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_bliptvdir)\"
40mex_bliptv_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_TELEPATHY
plugins_LTLIBRARIES += 40mex-telepathy.la

40mex_telepathy_la_SOURCES = 		\
	telepathy/tpy-client-factory.c		\
	telepathy/tpy-client-factory.h		\
	telepathy/mex-telepathy-channel.c	\
	telepathy/mex-telepathy-channel.h	\
	telepathy/mex-telepathy-plugin.c 	\
	telepathy/mex-telepathy-plugin.h	\
	telepathy/mex-contact.c 	\
	telepathy/mex-contact.h
40mex_telepathy_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Telepathy\" $(PLUGIN_TELEPATHY_CFLAGS)
40mex_telepathy_la_LIBADD = $(_libadd) $(PLUGIN_TELEPATHY_LIBS)
endif

if USE_PLUGIN_JUSTINTV
plugins_LTLIBRARIES += 40mex-justintv.la

mex_justintvdir = $(plugin_datadir)/justintv
dist_mex_justintv_DATA =

40mex_justintv_la_SOURCES = 		\
	justintv/mex-justintv-plugin.c 	\
	justintv/mex-justintv-plugin.h
40mex_justintv_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Justintv\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_justintvdir)\"
40mex_justintv_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_DEBUG_BUDDY
plugins_LTLIBRARIES += 00mex-debug.la

mex_debugdir = $(plugin_datadir)/debug
dist_mex_debug_DATA =

00mex_debug_la_SOURCES = 		\
	debug/mex-debug-plugin.c 	\
	debug/mex-debug-plugin.h
00mex_debug_la_CFLAGS  = 		\
	-DG_LOG_DOMAIN=\"Mex-Debug\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_debugdir)\"
00mex_debug_la_LIBADD = $(_libadd) -lunwind

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
plugins_LTLIBRARIES += 20mex-gnome-dvb.la

mex_gnome_dvbdir = $(plugin_datadir)/gnome-dvb
dist_mex_gnome_dvb_DATA =

20mex_gnome_dvb_la_SOURCES = 			\
	gnome-dvb/mex-gnome-dvb-plugin.c	\
	gnome-dvb/mex-gnome-dvb-plugin.h
20mex_gnome_dvb_la_CFLAGS = 			\
	-DG_LOG_DOMAIN=\"Mex-GNOME-DVB\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_gnome_dvbdir)\"
20mex_gnome_dvb_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_BG_VIDEO
plugins_LTLIBRARIES += 00mex-bg-video.la

mex_bg_videodir = $(plugin_datadir)/bg-video
dist_mex_bg_video_DATA =

00mex_bg_video_la_SOURCES = 		\
	bg-video/mex-bg-video-plugin.c	\
	bg-video/mex-bg-video-plugin.h	\
	bg-video/mex-background-video.c	\
	bg-video/mex-background-video.h
00mex_bg_video_la_CFLAGS = 				\
	-DG_LOG_DOMAIN=\"Mex-Background-Video\"		\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_bg_videodir)\"
00mex_bg_video_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_BG_DBUSVIDEO
plugins_LTLIBRARIES += 01mex-bg-dbusvideo.la

mex_bg_dbusvideodir = $(plugin_datadir)/bg-dbusvideo
dist_mex_bg_dbusvideo_DATA =

01mex_bg_dbusvideo_la_SOURCES =		 		\
	bg-dbusvideo/mex-bg-dbusvideo-plugin.c		\
	bg-dbusvideo/mex-bg-dbusvideo-plugin.h		\
	bg-dbusvideo/mex-dbus-background-video.c	\
	bg-dbusvideo/mex-dbus-background-video.h
01mex_bg_dbusvideo_la_CFLAGS = 				\
	-DG_LOG_DOMAIN=\"Mex-DBus-Background-Video\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_bg_dbusvideodir)\"
01mex_bg_dbusvideo_la_LIBADD = $(_libadd)
endif

if USE_PLUGIN_BG_BACKDROP
plugins_LTLIBRARIES += 01mex-bg-backdrop.la

mex_bg_backdropdir = $(plugin_datadir)/bg-backdrop
dist_mex_bg_backdrop_DATA = \
	bg-backdrop/sprite.png

01mex_bg_backdrop_la_SOURCES =		 		\
	bg-backdrop/mex-bg-backdrop-plugin.c		\
	bg-backdrop/mex-bg-backdrop-plugin.h		\
	bg-backdrop/mex-backdrop.c			\
	bg-backdrop/mex-backdrop.h
01mex_bg_backdrop_la_CFLAGS = 				\
	-DG_LOG_DOMAIN=\"Mex-Background-Backdrop\"	\
	-DMEX_DATA_PLUGIN_DIR=\"$(mex_bg_backdropdir)\"
01mex_bg_backdrop_la_LIBADD = $(_libadd)
endif
