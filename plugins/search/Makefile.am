pluginsdir = $(libdir)/mex/plugins
plugins_LTLIBRARIES = 00mex-search.la

plugin_datadir = $(datadir)/mex/plugins/search
dist_plugin_data_DATA = style.css search-bg.png textInput-bg.png

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	$(MEX_CFLAGS)				\
	-DPLUGIN_DATA_DIR=\"$(plugin_datadir)\"	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version

# search
00mex_search_la_SOURCES = mex-search-plugin.c mex-search-plugin.h
00mex_search_la_CFLAGS  = -DG_LOG_DOMAIN=\"Mex-Search\"
00mex_search_la_LIBADD  =					\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)
