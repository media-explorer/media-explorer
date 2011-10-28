EXTRA_DIST =

bin_PROGRAMS = mex-webremote

mex_webremote_SOURCES = \
		webremote.h \
		settings.c settings.h \
		mex-webremote.c mex-webremote.h \
		dbus-client.c dbus-client.h \
		dbus-service.c dbus-service.h \
		tracker-client.c tracker-client.h \
		mdns-client.c mdns-client.h

mex_webremote_CFLAGS = \
		-I$(top_srcdir) \
		-I$(top_srcdir)/mex \
		$(MEX_CFLAGS) \
		$(MEX_WARNING_CFLAGS) \
		$(WEBREMOTE_CFLAGS) \
		-DMEXPKGDATADIR=\"$(pkgdatadir)\"
		-DG_LOG_DOMAIN=\"MexWebremote\"

mex_webremote_LDADD = \
		$(MEX_LIBS) \
		$(WEBREMOTE_LIBS) \
		$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la

mex_webremotedir = $(pkgdatadir)/webremote
dist_mex_webremote_DATA = \
		data/index.html \
		data/style.css \
		$(NULL)

mex_webremote_imgsdir = $(pkgdatadir)/webremote/images
dist_mex_webremote_imgs_DATA = \
		data/images/remote-button-back.png \
		data/images/remote-button-info.png \
		data/images/remote-button-home.png \
		data/images/remote-button-enter.png \
		data/images/remote-button-up.png \
		data/images/remote-button-down.png \
		data/images/remote-button-left.png \
		data/images/remote-button-right.png \
		data/images/remote-button-background.png \
		data/images/toolbar-background.png \
		data/images/mex-spinner.gif \
		data/images/remote-button-back-selected.png \
		data/images/remote-button-background-down-selected.png \
		data/images/remote-button-background-left-selected.png \
		data/images/remote-button-background-right-selected.png \
		data/images/remote-button-background-up-selected.png \
		data/images/remote-button-down-selected.png \
		data/images/remote-button-up-selected.png \
		data/images/remote-button-enter-selected.png \
		data/images/remote-button-home-selected.png \
		data/images/remote-button-info-selected.png \
		data/images/remote-button-left-selected.png \
		data/images/remote-button-right-selected.png \
		data/images/search-button-go.png \
		data/images/search-button-go-selected.png \
		data/images/search-icon-delete.png \
		data/images/search-icon-delete-selected.png \
		$(NULL)

mex_webremote_jsdir = $(pkgdatadir)/webremote/js
dist_mex_webremote_js_DATA = \
		data/js/jquery-1.6.1.min.js \
		data/js/jquery.mobile-1.0rc2.min.js \
		data/js/hover.js \
		data/js/clutter-keysyms.js \
		$(NULL)

%.service: %.service.in $(top_builddir)/config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(bindir)|" $< > $@

servicedir = $(datadir)/dbus-1/services/
service_in_files = mex-webremote.service.in
service_DATA = mex-webremote.service

CLEANFILES = $(service_DATA);

EXTRA_DIST += $(service_in_files)
