pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 20mex-library.la

plugin_datadir = $(datadir)/mex/plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

# library
20mex_library_la_SOURCES = mex-library-plugin.c mex-library-plugin.h
20mex_library_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Library\"
20mex_library_la_LIBADD =					\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)
