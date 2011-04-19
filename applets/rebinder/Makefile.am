# Forward declare those so we can use += throughout the file
BUILT_SOURCES =
EXTRA_DIST =

AM_CFLAGS =						\
	$(REBINDER_CFLAGS)				\
	$(COMPILER_FLAGS) 				\
	-DG_LOG_DOMAIN=\"MexRebinder\"		\
	-DPKGDATADIR=\""$(pkgdatadir)/rebinder"\"	\
	-DREBINDER_ENABLE_DEBUG				\
	$(NULL)

# D-Bus server side bindings
DBUS_SPECS = mex-rebinder-control.xml
DBUS_SERVER_GINTERFACE =			\
	$(DBUS_SPECS:.xml=-ginterface.h)	\
	$(DBUS_SPECS:.xml=-ginterface.c)

%-ginterface.h %-ginterface.c: %.xml Makefile.am
	$(AM_V_GEN)python $(top_srcdir)/tools/glib-ginterface-gen.py --filename=$(basename $@) $< Mex_

BUILT_SOURCES += $(DBUS_SERVER_GINTERFACE)
EXTRA_DIST += $(DBUS_SERVER_GINTERFACE)

# Common files to both the daemon and the internal library
rebinder_common_sources =		\
	fakekey.c			\
	fakekey.h			\
	mex-rebinder.h			\
	mex-rebinder.c			\
	rebinder.h			\
	rebinder-configure.c		\
	rebinder-debug.c		\
	rebinder-debug.h		\
	rebinder-evdev-keymap.c		\
	rebinder-evdev-manager.c	\
	rebinder-evdev-manager.h	\
	rebinder-utils.c		\
	$(NULL)

# mex-rebinder the daemon
bin_PROGRAMS = mex-rebinder

mex_rebinder_SOURCES = 			\
	$(rebinder_common_sources)	\
	$(DBUS_SERVER_GINTERFACE)	\
	rebinder-main.c			\
	$(NULL)
mex_rebinder_LDADD   = $(REBINDER_LIBS)
mex_rebinder_CFLAGS  = $(AM_CFLAGS)

# We also provide a small libtool convenience library to be included from
# the shell for the out of the box experience
noinst_LTLIBRARIES = libmex-rebinder.la

libmex_rebinder_la_SOURCES = $(rebinder_common_sources)
libmex_rebinder_la_LIBADD   = $(REBINDER_LIBS)

%.service: %.service.in $(top_builddir)/config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(bindir)|" $< > $@

servicedir = $(datadir)/dbus-1/services/
service_in_files = mex-rebinder.service.in
dist_service_DATA = mex-rebinder.service

CLEANFILES = mex-rebinder.service
