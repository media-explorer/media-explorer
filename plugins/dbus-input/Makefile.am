pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 50mex-dbusinput.la

plugin_datadir = $(datadir)/mex/plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

# dbusinput
50mex_dbusinput_la_SOURCES = mex-dbusinput-plugin.c mex-dbusinput-plugin.h
50mex_dbusinput_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Dbusinput\"
50mex_dbusinput_la_LIBADD =					\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)
