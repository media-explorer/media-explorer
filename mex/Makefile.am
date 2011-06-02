SUBDIRS = glib-controller

-include $(INTROSPECTION_MAKEFILE)

lib_LTLIBRARIES = libmex-@MEX_API_VERSION@.la
noinst_LTLIBRARIES = \
		  libmex-player-ginterfaces.la \
		  libmex-player-client.la

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES =
EXTRA_DIST =
BUILT_SOURCES =

glib_marshal_list = mex-marshal.list
glib_marshal_prefix = mex_marshal
include $(top_srcdir)/build/Makefile.am.marshal

MARSHALS_SOURCE = \
	mex-marshal.c mex-marshal.h

glib_enum_h = mex-enum-types.h
glib_enum_c = mex-enum-types.c
glib_enum_headers = $(filter-out gbinding.h, $(mex_headers))
include $(top_srcdir)/build/Makefile.am.enums

DBUS_SPECS = mex-media-player.xml

%-ginterface.h %-ginterface.c: %.xml Makefile.am
	$(AM_V_GEN)python $(top_srcdir)/tools/glib-ginterface-gen.py --include='"mex-marshal.h"' --filename=$(basename $@) $< Mex_

GINTERFACE_SOURCES = \
	$(DBUS_SPECS:.xml=-ginterface.h) \
	$(DBUS_SPECS:.xml=-ginterface.c)

libmex_player_ginterfaces_la_SOURCES = $(GINTERFACE_SOURCES)
libmex_player_ginterfaces_la_LIBADD = $(MEX_LIBS) -lm
libmex_player_ginterfaces_la_CFLAGS = $(MEX_CFLAGS)

%-bindings.h: %.xml
	$(AM_V_GEN)dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

BUILT_SOURCES += $(GINTERFACE_SOURCES) $(DBUS_SPECS:.xml=-bindings.h) $(MARSHAL_SOURCES)

EXTRA_DIST += $(DBUS_SPECS:.xml=-bindings.h)

libmex_player_client_la_SOURCES = mex-player-client.h mex-player-client.c \
				  mex-player-common.h
libmex_player_client_la_CFLAGS = $(MEX_CFLAGS)
libmex_player_client_la_LIBADD = $(MEX_LIBS)

mex_headers =							\
	$(top_srcdir)/mex/mex.h					\
	$(top_srcdir)/mex/mex-action-button.h			\
	$(top_srcdir)/mex/mex-action-list.h			\
	$(top_srcdir)/mex/mex-action-manager.h			\
	$(top_srcdir)/mex/mex-action-provider.h			\
	$(top_srcdir)/mex/mex-aggregate-model.h			\
	$(top_srcdir)/mex/mex-applet.h				\
	$(top_srcdir)/mex/mex-applet-provider.h			\
	$(top_srcdir)/mex/mex-applet-manager.h			\
	$(top_srcdir)/mex/mex-application-category.h		\
	$(top_srcdir)/mex/mex-application.h			\
	$(top_srcdir)/mex/mex-aspect-frame.h			\
	$(top_srcdir)/mex/mex-channel.h				\
	$(top_srcdir)/mex/mex-channel-manager.h			\
	$(top_srcdir)/mex/mex-channel-provider.h		\
	$(top_srcdir)/mex/mex-clock-bin.h			\
	$(top_srcdir)/mex/mex-column.h				\
	$(top_srcdir)/mex/mex-content-box.h			\
	$(top_srcdir)/mex/mex-content-button.h			\
	$(top_srcdir)/mex/mex-content-proxy.h			\
	$(top_srcdir)/mex/mex-content-tile.h			\
	$(top_srcdir)/mex/mex-content-view.h			\
	$(top_srcdir)/mex/mex-content.h				\
	$(top_srcdir)/mex/mex-download-queue.h			\
	$(top_srcdir)/mex/mex-dummy-notification-source.h	\
	$(top_srcdir)/mex/mex-expander-box.h			\
	$(top_srcdir)/mex/mex-explorer.h			\
	$(top_srcdir)/mex/mex-epg.h				\
	$(top_srcdir)/mex/mex-epg-event.h			\
	$(top_srcdir)/mex/mex-epg-grid.h			\
	$(top_srcdir)/mex/mex-epg-manager.h			\
	$(top_srcdir)/mex/mex-epg-radiotimes.h			\
	$(top_srcdir)/mex/mex-epg-tile.h			\
	$(top_srcdir)/mex/mex-epg-provider.h			\
	$(top_srcdir)/mex/mex-feed.h				\
	$(top_srcdir)/mex/mex-generic-content.h			\
	$(top_srcdir)/mex/mex-generic-model.h			\
	$(top_srcdir)/mex/mex-generic-proxy.h			\
	$(top_srcdir)/mex/mex-gio-notification-source.h		\
	$(top_srcdir)/mex/mex-grid.h				\
	$(top_srcdir)/mex/mex-grilo-feed.h			\
	$(top_srcdir)/mex/mex-grilo-tracker-feed.h		\
	$(top_srcdir)/mex/mex-grilo-program.h			\
	$(top_srcdir)/mex/mex-info-bar.h			\
	$(top_srcdir)/mex/mex-info-panel.h			\
	$(top_srcdir)/mex/mex-logo-provider.h			\
	$(top_srcdir)/mex/mex-main.h				\
	$(top_srcdir)/mex/mex-media-controls.h			\
	$(top_srcdir)/mex/mex-media-dbus-bridge.h		\
	$(top_srcdir)/mex/mex-menu.h				\
	$(top_srcdir)/mex/mex-metadata-utils.h			\
	$(top_srcdir)/mex/mex-mmkeys.h				\
	$(top_srcdir)/mex/mex-model.h				\
	$(top_srcdir)/mex/mex-model-manager.h			\
	$(top_srcdir)/mex/mex-model-provider.h			\
	$(top_srcdir)/mex/mex-network-notification-source.h 	\
	$(top_srcdir)/mex/mex-notification-area.h		\
	$(top_srcdir)/mex/mex-notification-source.h		\
	$(top_srcdir)/mex/mex-online.h				\
	$(top_srcdir)/mex/mex-player.h				\
	$(top_srcdir)/mex/mex-player-common.h			\
	$(top_srcdir)/mex/mex-plugin-manager.h			\
	$(top_srcdir)/mex/mex-program.h				\
	$(top_srcdir)/mex/mex-proxy.h				\
	$(top_srcdir)/mex/mex-proxy-model.h			\
	$(top_srcdir)/mex/mex-queue-model.h			\
	$(top_srcdir)/mex/mex-queue-button.h			\
	$(top_srcdir)/mex/mex-resizing-hbox.h			\
	$(top_srcdir)/mex/mex-resizing-hbox-child.h		\
	$(top_srcdir)/mex/mex-screensaver.h			\
	$(top_srcdir)/mex/mex-scroll-indicator.h		\
	$(top_srcdir)/mex/mex-scroll-view.h			\
	$(top_srcdir)/mex/mex-scrollable-container.h		\
	$(top_srcdir)/mex/mex-settings.h			\
	$(top_srcdir)/mex/mex-shadow.h				\
	$(top_srcdir)/mex/mex-shell.h				\
	$(top_srcdir)/mex/mex-slide-show.h			\
	$(top_srcdir)/mex/mex-surface-player.h			\
	$(top_srcdir)/mex/mex-thumbnailer.h			\
	$(top_srcdir)/mex/mex-tile.h				\
	$(top_srcdir)/mex/mex-tool-provider.h			\
	$(top_srcdir)/mex/mex-uri-channel-provider.h		\
	$(top_srcdir)/mex/mex-utils.h				\
	$(top_srcdir)/mex/mex-volume-control.h			\
	$(NULL)

mex_built_headers =			\
	mex-enum-types.h		\
	mex-marshal.h			\
	$(NULL)

mex_private_headers =			\
	mex-debug.h			\
	mex-private.h			\
	$(NULL)

mex_sources =					\
	mex-action-button.c			\
	mex-action-list.c			\
	mex-action-manager.c			\
	mex-action-provider.c			\
	mex-aggregate-model.c			\
	mex-applet.c				\
	mex-applet-provider.c			\
	mex-applet-manager.c			\
	mex-application-category.c		\
	mex-application.c			\
	mex-aspect-frame.c			\
	mex-channel.c				\
	mex-channel-manager.c			\
	mex-channel-provider.c			\
	mex-clock-bin.c				\
	mex-column.c				\
	mex-content-box.c			\
	mex-content-button.c			\
	mex-content-proxy.c			\
	mex-content-tile.c			\
	mex-content-view.c			\
	mex-content.c				\
	mex-debug.c				\
	mex-download-queue.c			\
	mex-dummy-notification-source.c 	\
	mex-expander-box.c			\
	mex-epg.c				\
	mex-epg-event.c				\
	mex-epg-grid.c				\
	mex-epg-manager.c			\
	mex-epg-provider.c			\
	mex-epg-radiotimes.c			\
	mex-epg-tile.c				\
	mex-explorer.c				\
	mex-feed.c				\
	mex-generic-content.c			\
	mex-generic-model.c			\
	mex-generic-proxy.c			\
	mex-gio-notification-source.c		\
	mex-grid.c				\
	mex-grilo-feed.c			\
	mex-grilo-tracker-feed.c		\
	mex-grilo-program.c			\
	mex-info-bar.c				\
	mex-info-panel.c			\
	mex-logo-provider.c			\
	mex-main.c				\
	mex-media-controls.c			\
	mex-media-dbus-bridge.c			\
	mex-menu.c				\
	mex-metadata-utils.c			\
	mex-mmkeys.c				\
	mex-model.c				\
	mex-model-manager.c			\
	mex-model-provider.c			\
	mex-network-notification-source.c	\
	mex-notification-area.c			\
	mex-notification-source.c		\
	mex-online.c				\
	mex-player.c				\
	mex-plugin-manager.c			\
	mex-private.c				\
	mex-program.c				\
	mex-proxy.c				\
	mex-proxy-model.c			\
	mex-queue-model.c			\
	mex-queue-button.c			\
	mex-resizing-hbox.c			\
	mex-resizing-hbox-child.c		\
	mex-screensaver.c			\
	mex-scroll-indicator.c			\
	mex-scroll-view.c			\
	mex-scrollable-container.c		\
	mex-settings.c				\
	mex-shadow.c				\
	mex-shell.c				\
	mex-slide-show.c			\
	mex-surface-player.c			\
	mex-thumbnailer.c			\
	mex-tile.c				\
	mex-tool-provider.c			\
	mex-utils.c				\
	mex-uri-channel-provider.c		\
	mex-volume-control.c			\
	$(NULL)

mex_built_sources =			\
	mex-enum-types.c		\
	mex-marshal.c			\
	$(NULL)

BUILT_SOURCES+=$(mex_built_sources) $(mex_built_headers)

libmex_@MEX_API_VERSION@_la_SOURCES =	\
	$(mex_sources)			\
	$(mex_headers)			\
	$(mex_built_sources)		\
	$(mex_private_headers)		\
	$(NULL)

common_includes =					\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-DPKGLIBDIR=\""$(pkglibdir)"\"			\
	-DPKGDATADIR=\""$(pkgdatadir)/common"\"		\
	-DPKGJSONDIR=\""$(pkgdatadir)/json"\"		\
	-DPKGSYSCONFDIR=\""$(datadir)/$(PACKAGE)"\"	\
	$(NULL)

libmex_@MEX_API_VERSION@_la_CPPFLAGS =	\
	$(common_includes)		\
	-DG_LOG_DOMAIN=\"Mex\"		\
	-DG_DISABLE_DEPRECATED		\
	-DG_DISABLE_SINGLE_INCLUDES	\
	-DG_COGL_DISABLE_DEPRECATED	\
	-DG_CLUTTER_DISABLE_DEPRECATED	\
	-DG_MX_DISABLE_DEPRECATED	\
	$(NULL)

libmex_@MEX_API_VERSION@_la_CFLAGS =	\
	$(MEX_CFLAGS)			\
	$(MEX_MAINTAINER_CFLAGS)	\
	$(MEX_DEBUG_CFLAGS)		\
	$(TRACKER_CFLAGS)		\
	$(NM_CFLAGS)			\
	$(NULL)

libmex_@MEX_API_VERSION@_la_LIBADD =					\
	$(top_builddir)/mex/glib-controller/libglib-controller-1.0.la	\
	libmex-player-ginterfaces.la					\
	libmex-player-client.la						\
	$(MEX_LIBS)							\
	$(TRACKER_LIBS)							\
	$(NM_LIBS)							\
	$(NULL)

#
# Internal tests
#
if ENABLE_TESTS
include $(top_srcdir)/build/Makefile.am.gtester

TEST_ENVIRONMENT = srcdir=$(srcdir)

noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS += test-libmex-internal
test_libmex_internal_SOURCES = mex-test-internal.c
test_libmex_internal_CFLAGS  = $(libmex_@MEX_API_VERSION@_la_CFLAGS)
test_libmex_internal_LDADD   =			\
	$(libmex_@MEX_API_VERSION@_la_LIBADD)	\
	libmex-@MEX_API_VERSION@.la		\
	$(NULL)
endif

#
# Introspection
#

if HAVE_INTROSPECTION
GIRS =
TYPELIBS = $(GIRS:.gir=.typelib)
INTROSPECTION_GIRS = $(GIRS)

INTROSPECTION_COMPILER_ARGS = --includedir=$(builddir)/glib-controller

Mex-@MEX_API_VERSION@.gir: libmex-@MEX_API_VERSION@.la
Mex_0_2_gir_PACKAGES = 	\
	gobject-2.0		\
	clutter-1.0		\
	mx-1.0			\
	libsoup-gnome-2.4	\
	grilo-0.1		\
	$(NULL)

Mex_0_2_gir_LIBS = libmex-@MEX_API_VERSION@.la
Mex_0_2_gir_FILES = $(mex_headers) $(mex_sources) $(mex_built_sources)
Mex_0_2_gir_SCANNERFLAGS = \
	--strip-prefix=Mex \
	--add-include-path=$(builddir)/glib-controller
Mex_0_2_gir_CFLAGS = $(common_includes) $(MEX_CFLAGS)
Mex_0_2_gir_INCLUDES =	\
	GObject-2.0	\
	Clutter-1.0	\
	Mx-1.0		\
	Grl-0.1		\
	GController-1.0	\
	$(NULL)

GIRS += Mex-@MEX_API_VERSION@.gir

endif

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(TYPELIBS)

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
