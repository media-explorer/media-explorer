pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 20mex-tracker.la

plugin_datadir = $(datadir)/mex/plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

# tracker
20mex_tracker_la_SOURCES = mex-tracker-plugin.c mex-tracker-plugin.h
20mex_tracker_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Tracker\"
20mex_tracker_la_LIBADD =					\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)
