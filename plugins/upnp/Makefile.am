pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 20mex-upnp.la

plugin_datadir = $(datadir)/mex/plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

# upnp
20mex_upnp_la_SOURCES = mex-upnp-plugin.c mex-upnp-plugin.h
20mex_upnp_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-UPnP\"
20mex_upnp_la_LIBADD =						\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)
