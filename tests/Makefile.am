include $(top_srcdir)/build/Makefile.am.gtester

NULL =

noinst_PROGRAMS = $(TEST_PROGS) test-config

progs_ldadd =							\
	$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
	$(MEX_LIBS)

AM_CPPFLAGS = 					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/mex			\
	-DTESTDATADIR=\"$(srcdir)\"		\
	-DABS_TESTDATADIR=\"$(abs_srcdir)\"	\
	$(MEX_CFLAGS)				\
	$(MEX_MAINTAINER_CFLAGS)		\
	$(MEX_DEBUG_CFLAGS)			\
	$(NULL)

AM_CFLAGS = -g

TESTS_ENVIRONMENT = srcdir=$(srcdir)

TEST_PROGS           += test-channel
test_channel_SOURCES  = test-channel.c
test_channel_LDADD    = $(progs_ldadd)

TEST_PROGS       += test-epg
test_epg_SOURCES  = test-epg.c
test_epg_LDADD    = $(progs_ldadd)

test_config_SOURCES = test-config.c
test_config_LDADD   = $(progs_ldadd)
