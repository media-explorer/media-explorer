bin_PROGRAMS = media-explorer

AM_CFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	-I$(top_srcdir)/applets/webremote	\
	$(MEX_CFLAGS)				\
	$(MEX_DEBUG_CFLAGS)			\
	$(MEX_WARNING_CFLAGS) 			\
	-DG_LOG_DOMAIN=\"Mex\"			\
	-DPKGDATADIR=\""$(pkgdatadir)"\"	\
	$(NULL)

BUILT_SOURCES = mex-version.h

CLEANFILES = $(BUILT_SOURCES)

.FORCE:

mex-version.h:.FORCE
	-@if [ -d $(top_srcdir)/.git ]; then \
	  echo \#define MEX_VERSION_GIT \"`git describe --always`\" \
	    > xversiongen && \
	  (cmp -s xversiongen $@ || cp -f xversiongen $@) && \
	  rm -f xversiongen; \
	else \
	  test ! -f $@ && \
	  echo \#define MEX_VERSION_GIT \"$(PACKAGE_VERSION)\" > $@; \
	fi

media_explorer_SOURCES =	\
	mex-main.c		\
	mex-version.h		\
	$(NULL)

media_explorer_LDADD =						\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la		\
	$(MEX_LIBS)						\
	$(NULL)
