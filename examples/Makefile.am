noinst_PROGRAMS = 			\
 	mex-column-test			\
 	mex-epg-test			\
	mex-grid-test			\
	mex-grilo-model-test		\
	mex-info-panel-test		\
	mex-info-bar-test		\
	mex-media-controls-test		\
	$(NULL)

AM_CFLAGS = 						\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/mex				\
	$(MEX_WARNING_CFLAGS)				\
	$(MEX_DEBUG_CFLAGS)				\
	$(MEX_CFLAGS)					\
	-DLIBDIR=\"$(libdir)\"				\
	-DTESTDATADIR=\"$(top_srcdir)/tests\"		\
	-DABS_TESTDATADIR=\"$(abs_top_srcdir)/tests\"	\
	$(TRACKER_CFLAGS)				\
	$(GRILO_CFLAGS)					\
	$(NULL)

AM_LDFLAGS =							\
	$(MEX_LIBS)						\
	$(TRACKER_LIBS)						\
	$(GRILO_LIBS)						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(NULL)

mex_column_test_SOURCES = mex-column-test.c
mex_epg_test_SOURCES = mex-epg-test.c
mex_grid_test_SOURCES = mex-grid-test.c
mex_grilo_model_test_SOURCES = mex-grilo-model-test.c
mex_info_panel_test_SOURCES = mex-info-panel-test.c
mex_info_bar_test_SOURCES = mex-info-bar-test.c
mex_media_controls_test_SOURCES = mex-media-controls-test.c

# Convience targets for the grilo demo

PLUGINDIR := `pkg-config --variable=plugindir grilo-0.1`
GRILO_CONFIG_APPLE := --source=$(PLUGINDIR)/libgrlappletrailers.so --config=grl-apple-trailers --string=definition,hd
GRILO_CONFIG_JAMENDO := --source=$(PLUGINDIR)/libgrljamendo.so
GRILO_CONFIG_LOCAL := --source=$(PLUGINDIR)/libgrlfilesystem.so --config=grl-filesystem --string=base-path,$(HOME)/Videos/
GRILO_CONFIG_VIMEO := --source=$(PLUGINDIR)/libgrlvimeo.so.0 --config=grl-vimeo --string=api-key,4d908c69e05a9d5b5c6669d302f920cb --string=api-secret,4a923ffaab6238eb
GRILO_CONFIG_YOUTUBE := --source=$(PLUGINDIR)/libgrlyoutube.so.0 --config=grl-youtube --string=api-key,AI39si4EfscPllSfUy1IwexMf__kntTL_G5dfSr2iUEVN45RHGq92Aq0lX25OlnOkG6KTN-4soVAkAf67fWYXuHfVADZYr7S1A

run-grilo: mex-grilo
	./mex-grilo $(GRILO_CONFIG_LOCAL) $(GRILO_CONFIG_APPLE) $(GRILO_CONFIG_YOUTUBE) $(GRILO_CONFIG_VIMEO) $(GRILO_CONFIG_JAMENDO)

debug-grilo: mex-grilo
	$(top_srcdir)/libtool --mode=execute gdb --args ./mex-grilo $(GRILO_CONFIG_LOCAL) $(GRILO_CONFIG_APPLE) $(GRILO_CONFIG_YOUTUBE) $(GRILO_CONFIG_VIMEO) $(GRILO_CONFIG_JAMENDO)


gl-debug-grilo: mex-grilo
	$(top_srcdir)/libtool --mode=execute gldb-gui ./mex-grilo $(GRILO_CONFIG_LOCAL) $(GRILO_CONFIG_APPLE) $(GRILO_CONFIG_YOUTUBE) $(GRILO_CONFIG_VIMEO) $(GRILO_CONFIG_JAMENDO)


valgrind-grilo: mex-grilo
	G_SLICE=always-malloc $(top_srcdir)/libtool --mode=execute valgrind ./mex-grilo $(GRILO_CONFIG_LOCAL) $(GRILO_CONFIG_APPLE) $(GRILO_CONFIG_YOUTUBE) $(GRILO_CONFIG_VIMEO) $(GRILO_CONFIG_JAMENDO)

