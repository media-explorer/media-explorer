pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 40mex-recommended.la

plugin_datadir = $(datadir)/mex/plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

# recommended
40mex_recommended_la_SOURCES = mex-recommended-plugin.c mex-recommended-plugin.h
40mex_recommended_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Recommended\"
40mex_recommended_la_LIBADD  =					\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)
