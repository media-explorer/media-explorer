bin_PROGRAMS = mex

AM_CFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	-I$(top_srcdir)/applets/rebinder	\
	$(MEX_CFLAGS)				\
	$(MAINTAINER_FLAGS) 			\
	-DG_LOG_DOMAIN=\"Mex\"			\
	-DPKGDATADIR=\""$(pkgdatadir)/shell"\"	\
	$(NULL)

BUILT_SOURCES = mex-version.h

CLEANFILES = $(BUILT_SOURCES)

.FORCE:

mex-version.h:.FORCE
	-@test -d $(top_srcdir)/.git && echo \#define MEX_VERSION_GIT \"`git describe --always`\" > xversiongen && \
	(cmp -s xversiongen $@ || cp -f xversiongen $@) && \
	rm -f xversiongen

mex_SOURCES   =		\
	mex-main.c	\
	mex-version.h	\
	$(NULL)

mex_LDADD     =							\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la		\
	$(top_builddir)/applets/rebinder/libmex-rebinder.la	\
	$(MEX_LIBS)						\
	$(REBINDER_LIBS)					\
	$(NULL)
