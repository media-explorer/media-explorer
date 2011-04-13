pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 50mex-applications.la

plugin_datadir = $(datadir)/mex/plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

# applications
50mex_applications_la_SOURCES = mex-applications-plugin.c \
				mex-applications-plugin.h
50mex_applications_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Applications\"
50mex_applications_la_LIBADD  =					\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)

