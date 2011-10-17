libexec_PROGRAMS = mex-media-player

mex_media_player_CFLAGS = $(MEX_CFLAGS) \
	$(MEX_WARNING_CFLAGS)		\
	$(MEX_DEBUG_CFLAGS) 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/mex		\
	-DG_LOG_DOMAIN=\"Mex-Player\"	\
	-DMEX_PLAYER_PLUGIN_PATH=\"$(libdir)/media-explorer-player/plugins\"


mex_media_player_LDADD =					\
	$(MEX_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)

mex_media_player_SOURCES = main.c

%.service: %.service.in ../config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(libexecdir)|" $< > $@

servicedir = $(datadir)/dbus-1/services/
service_in_files = mex-media-player.service.in
service_DATA = mex-media-player.service

CLEANFILES =  $(service_DATA)
EXTRA_DIST =  $(service_in_files)
