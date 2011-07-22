EXTRA_DIST =

bin_PROGRAMS = mex-webremote

mex_webremote_SOURCES = \
		webremote.h \
		mex-webremote.c mex-webremote.h \
		dbus-client.c dbus-client.h \
		dbus-service.c dbus-service.h \
		tracker-client.c tracker-client.h \
		mdns-client.c mdns-client.h

mex_webremote_CFLAGS = \
		-I$(top_srcdir) \
		-I$(top_srcdir)/mex \
		$(MEX_CFLAGS) \
		$(WEBREMOTE_CFLAGS) \
		-DMEXPKGDATADIR=\"$(pkgdatadir)\"
		-DG_LOG_DOMAIN=\"MexWebremote\"

mex_webremote_LDADD = \
		$(MEX_LIBS) \
		$(WEBREMOTE_LIBS) \
		$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la

mex_webremotedir = $(pkgdatadir)/webremote
mex_webremote_DATA = \
		data/index.html \
		data/jquery-1.6.1.min.js \
		data/mex-spinner.gif \
		data/clutter-keysyms.js

%.service: %.service.in $(top_builddir)/config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(bindir)|" $< > $@

servicedir = $(datadir)/dbus-1/services/
service_in_files = mex-webremote.service.in
service_DATA = mex-webremote.service

CLEANFILES = $(service_DATA);

EXTRA_DIST += $(service_in_files)
