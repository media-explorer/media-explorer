libexec_PROGRAMS = mex-thumbnailer

mex_thumbnailer_SOURCES = mex-thumbnailer-internal.c
mex_thumbnailer_CFLAGS = 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/mex		\
	$(MEX_DEBUG_CFLAGS)		\
	$(MEX_CFLAGS)			\
	$(MEX_WARNING_CFLAGS) 		\
	$(NULL)

mex_thumbnailer_LDADD = 						\
			$(top_builddir)/mex/libmex-@MEX_API_VERSION@.la	\
			$(MEX_LIBS)					\
			$(NULL)

