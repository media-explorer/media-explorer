pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 40mex-queue.la

plugin_datadir = $(datadir)/mex/plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

# queue
40mex_queue_la_SOURCES = mex-queue-plugin.c mex-queue-plugin.h
40mex_queue_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Queue\"
40mex_queue_la_LIBADD  =					\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)
