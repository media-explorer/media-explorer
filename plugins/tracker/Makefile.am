BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
DISTCLEANFILES =

pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 20mex-tracker.la

plugin_datadir = $(datadir)/mex/plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	$(PLUGIN_TRACKER_CFLAGS)		\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

glib_marshal_list = mex-tracker-marshal.list
glib_marshal_prefix = mex_tracker_marshal
include $(top_srcdir)/build/Makefile.am.marshal

MARSHALS_SOURCE = \
	mex-tracker-marshal.c mex-tracker-marshal.h

BUILT_SOURCES += $(MARSHAL_SOURCES)

# tracker
20mex_tracker_la_SOURCES = \
	mex-tracker-plugin.c \
	mex-tracker-plugin.h \
	\
	mex-tracker-cache.c \
	mex-tracker-cache.h \
	mex-tracker-connection.c \
	mex-tracker-connection.h \
	mex-tracker-content.c \
	mex-tracker-content.h \
	mex-tracker-metadatas.c \
	mex-tracker-metadatas.h \
	mex-tracker-model.c \
	mex-tracker-model.h \
	mex-tracker-notifications.c \
	mex-tracker-notifications.h \
	mex-tracker-queue.c \
	mex-tracker-queue.h \
	mex-tracker-utils.c \
	mex-tracker-utils.h
20mex_tracker_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Tracker\"
20mex_tracker_la_LIBADD =					\
	$(MEX_LIBS)						\
	$(PLUGIN_TRACKER_LIBS)					\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la		\
	$(NULL)

CLEANFILES += $(BUILT_SOURCES)
