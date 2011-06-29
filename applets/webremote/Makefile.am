bin_PROGRAMS = mex-webremote

mex_webremote_SOURCES = \
		mex-webremote.c \
		dbus-interface.c dbus-interface.h \
		tracker-interface.c tracker-interface.h \
		mdns.c mdns.h \
		ssdp.c ssdp.h

mex_webremote_CFLAGS = \
		-I$(top_srcdir) \
		-I$(top_srcdir)/mex \
		$(MEX_CFLAGS) \
		$(WEBREMOTE_CFLAGS) \
		-DG_LOG_DOMAIN=\"MexWebremote\"

mex_webremote_LDADD = \
		$(MEX_LIBS) \
		$(WEBREMOTE_LIBS) \
		$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la

mex_webremotedir = $(pkgdatadir)/webremote
mex_webremote_DATA = \
		data/index.html \
		data/mex-spinner.gif \
		data/clutter-keysyms.js

EXTRA_DIST = index.html clutter-keysyms.js
