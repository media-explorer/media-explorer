SUBDIRS = glib-controller

lib_LTLIBRARIES = libmex-@MEX_API_VERSION@.la

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES =
EXTRA_DIST =
BUILT_SOURCES =


mex-resource.c: $(top_srcdir)/data/media-explorer.gresource.xml
	glib-compile-resources --target=$@ --sourcedir=$(top_srcdir)/data \
		--generate-source --c-name mex $(top_srcdir)/data/media-explorer.gresource.xml

mex-resource.h: $(top_srcdir)/data/media-explorer.gresource.xml
	glib-compile-resources --target=$@  --sourcedir=$(top_srcdir)/data \
		--generate-header --c-name mex $(top_srcdir)/data/media-explorer.gresource.xml

glib_marshal_list = mex-marshal.list
glib_marshal_prefix = mex_marshal
include $(top_srcdir)/build/Makefile.am.marshal

MARSHALS_SOURCE = \
	mex-marshal.c mex-marshal.h

glib_enum_h = mex-enum-types.h
glib_enum_c = mex-enum-types.c
glib_enum_headers = $(filter-out gbinding.h, $(mex_headers))
include $(top_srcdir)/build/Makefile.am.enums

BUILT_SOURCES += $(MARSHAL_SOURCES) mex-resource.c mex-resource.h

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
	$(top_srcdir)/mex/mex-background.h			\
	$(top_srcdir)/mex/mex-background-manager.h		\
	$(top_srcdir)/mex/mex-channel.h				\
	$(top_srcdir)/mex/mex-channel-manager.h			\
	$(top_srcdir)/mex/mex-channel-provider.h		\
	$(top_srcdir)/mex/mex-clock-bin.h			\
	$(top_srcdir)/mex/mex-column.h				\
	$(top_srcdir)/mex/mex-column-view.h			\
	$(top_srcdir)/mex/mex-content-box.h			\
	$(top_srcdir)/mex/mex-content-proxy.h			\
	$(top_srcdir)/mex/mex-content-tile.h			\
	$(top_srcdir)/mex/mex-content-view.h			\
	$(top_srcdir)/mex/mex-content.h				\
	$(top_srcdir)/mex/mex-debug.h				\
	$(top_srcdir)/mex/mex-download-queue.h			\
	$(top_srcdir)/mex/mex-explorer.h			\
	$(top_srcdir)/mex/mex-epg-event.h			\
	$(top_srcdir)/mex/mex-epg-manager.h			\
	$(top_srcdir)/mex/mex-epg-radiotimes.h			\
	$(top_srcdir)/mex/mex-epg-provider.h			\
	$(top_srcdir)/mex/mex-feed.h				\
	$(top_srcdir)/mex/mex-generic-notification-source.h	\
	$(top_srcdir)/mex/mex-generic-content.h			\
	$(top_srcdir)/mex/mex-generic-model.h			\
	$(top_srcdir)/mex/mex-generic-proxy.h			\
	$(top_srcdir)/mex/mex-gio-notification-source.h		\
	$(top_srcdir)/mex/mex-grid.h				\
	$(top_srcdir)/mex/mex-grid-view.h			\
	$(top_srcdir)/mex/mex-grilo.h				\
	$(top_srcdir)/mex/mex-grilo-feed.h			\
	$(top_srcdir)/mex/mex-grilo-tracker-feed.h		\
	$(top_srcdir)/mex/mex-grilo-program.h			\
	$(top_srcdir)/mex/mex-group-item.h			\
	$(top_srcdir)/mex/mex-info-bar.h			\
	$(top_srcdir)/mex/mex-info-bar-component.h		\
	$(top_srcdir)/mex/mex-info-panel.h			\
	$(top_srcdir)/mex/mex-lirc.h				\
	$(top_srcdir)/mex/mex-log.h				\
	$(top_srcdir)/mex/mex-logo-provider.h			\
	$(top_srcdir)/mex/mex-main.h				\
	$(top_srcdir)/mex/mex-media-controls.h			\
	$(top_srcdir)/mex/mex-media-dbus-bridge.h		\
	$(top_srcdir)/mex/mex-menu.h				\
	$(top_srcdir)/mex/mex-metadata-utils.h			\
	$(top_srcdir)/mex/mex-mmkeys.h				\
	$(top_srcdir)/mex/mex-model-manager.h			\
	$(top_srcdir)/mex/mex-model-provider.h			\
	$(top_srcdir)/mex/mex-model.h				\
	$(top_srcdir)/mex/mex-music-grid-view.h			\
	$(top_srcdir)/mex/mex-music-player.h			\
	$(top_srcdir)/mex/mex-network-notification-source.h 	\
	$(top_srcdir)/mex/mex-notification-area.h		\
	$(top_srcdir)/mex/mex-notification-source.h		\
	$(top_srcdir)/mex/mex-os.h				\
	$(top_srcdir)/mex/mex-player.h				\
	$(top_srcdir)/mex/mex-player-client.h			\
	$(top_srcdir)/mex/mex-player-common.h			\
	$(top_srcdir)/mex/mex-plugin.h				\
	$(top_srcdir)/mex/mex-plugin-manager.h			\
	$(top_srcdir)/mex/mex-program.h				\
	$(top_srcdir)/mex/mex-proxy.h				\
	$(top_srcdir)/mex/mex-queue-model.h			\
	$(top_srcdir)/mex/mex-queue-button.h			\
	$(top_srcdir)/mex/mex-resizing-hbox.h			\
	$(top_srcdir)/mex/mex-resizing-hbox-child.h		\
	$(top_srcdir)/mex/mex-scene.h				\
	$(top_srcdir)/mex/mex-screensaver.h			\
	$(top_srcdir)/mex/mex-scroll-indicator.h		\
	$(top_srcdir)/mex/mex-scroll-view.h			\
	$(top_srcdir)/mex/mex-scrollable-container.h		\
	$(top_srcdir)/mex/mex-settings.h			\
	$(top_srcdir)/mex/mex-shadow.h				\
	$(top_srcdir)/mex/mex-slide-show.h			\
	$(top_srcdir)/mex/mex-surface-player.h			\
	$(top_srcdir)/mex/mex-thumbnailer.h			\
	$(top_srcdir)/mex/mex-tile.h				\
	$(top_srcdir)/mex/mex-tool-provider.h			\
	$(top_srcdir)/mex/mex-uri-channel-provider.h		\
	$(top_srcdir)/mex/mex-utils.h				\
	$(top_srcdir)/mex/mex-video-grid-view.h			\
	$(top_srcdir)/mex/mex-view-model.h			\
	$(top_srcdir)/mex/mex-volume-control.h			\
	$(top_srcdir)/mex/mex-vt-manager.h			\
	$(NULL)

mex_built_headers =			\
	mex-enum-types.h		\
	mex-marshal.h			\
	$(NULL)

mex_private_headers =			\
	mex-log-private.h		\
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
	mex-background.c			\
	mex-background-manager.c		\
	mex-channel.c				\
	mex-channel-manager.c			\
	mex-channel-provider.c			\
	mex-clock-bin.c				\
	mex-column.c				\
	mex-column-view.c			\
	mex-content-box.c			\
	mex-content-proxy.c			\
	mex-content-tile.c			\
	mex-content-view.c			\
	mex-content.c				\
	mex-debug.c				\
	mex-download-queue.c			\
	mex-epg-event.c				\
	mex-epg-manager.c			\
	mex-epg-provider.c			\
	mex-epg-radiotimes.c			\
	mex-explorer.c				\
	mex-feed.c				\
	mex-generic-notification-source.c 	\
	mex-generic-content.c			\
	mex-generic-model.c			\
	mex-generic-proxy.c			\
	mex-gio-notification-source.c		\
	mex-grid.c				\
	mex-grid-view.c				\
	mex-grilo.c				\
	mex-grilo-feed.c			\
	mex-grilo-tracker-feed.c		\
	mex-grilo-program.c			\
	mex-group-item.c			\
	mex-info-bar.c				\
	mex-info-bar-component.c		\
	mex-info-panel.c			\
	mex-lirc.c				\
	mex-log.c				\
	mex-logo-provider.c			\
	mex-main.c				\
	mex-media-dbus-bridge.c			\
	mex-media-controls.c			\
	mex-menu.c				\
	mex-metadata-utils.c			\
	mex-mmkeys.c				\
	mex-model.c				\
	mex-model-manager.c			\
	mex-model-provider.c			\
	mex-music-grid-view.c			\
	mex-music-player.c			\
	mex-network-notification-source.c	\
	mex-notification-area.c			\
	mex-notification-source.c		\
	mex-os-@MEX_OS@.c			\
	mex-player.c				\
	mex-player-client.c			\
	mex-plugin-manager.c			\
	mex-private.c				\
	mex-program.c				\
	mex-proxy.c				\
	mex-queue-model.c			\
	mex-queue-button.c			\
	mex-resizing-hbox.c			\
	mex-resizing-hbox-child.c		\
	mex-scene.c				\
	mex-screensaver.c			\
	mex-scroll-indicator.c			\
	mex-scroll-view.c			\
	mex-scrollable-container.c		\
	mex-settings.c				\
	mex-shadow.c				\
	mex-slide-show.c			\
	mex-surface-player.c			\
	mex-thumbnailer.c			\
	mex-tile.c				\
	mex-tool-provider.c			\
	mex-utils.c				\
	mex-uri-channel-provider.c		\
	mex-video-grid-view.c			\
	mex-view-model.c			\
	mex-volume-control.c			\
	mex-vt-manager.c			\
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

nodist_libmex_@MEX_API_VERSION@_la_SOURCES =	\
	mex-resource.c mex-resouce.h

common_includes =					\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-DPKGLIBDIR=\""$(pkglibdir)"\"			\
	-DPKGDATADIR=\""$(pkgdatadir)/common"\"		\
	-DPKGJSONDIR=\""$(pkgdatadir)/json"\"		\
	-DPKGSYSCONFDIR=\""$(datadir)/$(PACKAGE)"\"	\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	$(NULL)

libmex_@MEX_API_VERSION@_la_CPPFLAGS =	\
	$(common_includes)		\
	-DG_LOG_DOMAIN=\"Mex\"		\
	-DG_DISABLE_SINGLE_INCLUDES	\
	$(NULL)

libmex_@MEX_API_VERSION@_la_CFLAGS =	\
	$(MEX_CFLAGS)			\
	$(MEX_WARNING_CFLAGS)		\
	$(MEX_DEBUG_CFLAGS)		\
	$(TRACKER_CFLAGS)		\
	$(NM_CFLAGS)			\
	$(NULL)

libmex_@MEX_API_VERSION@_la_LIBADD =					\
	$(top_builddir)/mex/glib-controller/libglib-controller-1.0.la	\
	$(MEX_LIBS)							\
	$(TRACKER_LIBS)							\
	$(NM_LIBS)							\
	$(NULL)

libmex_@MEX_API_VERSION@_la_LDFLAGS = -no-undefined

#
# Internal unit tests
#

if ENABLE_TESTS

include $(top_srcdir)/build/Makefile.am.gtester

noinst_PROGRAMS = $(TEST_PROGS)

test_ldadd =				\
	libmex-@MEX_API_VERSION@.la	\
	$(MEX_LIBS)

test_cppflags = 				\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	$(MEX_WARNING_CFLAGS)			\
	$(MEX_DEBUG_CFLAGS)			\
	$(NULL)

TESTS_ENVIRONMENT = srcdir=$(srcdir)

TEST_PROGS                += mex-test-internal
mex_test_internal_SOURCES  = mex-test-internal.c mex-test-internal.h
mex_test_internal_CPPFLAGS = $(test_cppflags)
mex_test_internal_LDADD    = $(test_ldadd)

dist_check_SCRIPTS =			\
	check-header-guards.sh		\
	check-header-c++-ready.sh	\
	check-mex-h-inclusion.sh	\
	check-no-quote-includes.sh	\
	$(NULL)
TESTS = $(dist_check_SCRIPTS)

endif # ENABLE_TESTS
