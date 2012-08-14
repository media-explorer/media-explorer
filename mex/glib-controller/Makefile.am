BUILT_SOURCES =

CLEANFILES =
DISTCLEANFILES =

EXTRA_DIST =

noinst_LTLIBRARIES = libglib-controller-1.0.la

source_c = \
        garraycontroller.c      \
        gcontrollerreference.c  \
        gcontroller.c           \
        ghashcontroller.c       \
        gptrarraycontroller.c   \
        $(NULL)

source_public_h = \
	$(top_srcdir)/mex/glib-controller/garraycontroller.h      \
	$(top_srcdir)/mex/glib-controller/gcontrollerreference.h  \
	$(top_srcdir)/mex/glib-controller/gcontrollertypes.h      \
	$(top_srcdir)/mex/glib-controller/gcontroller.h           \
	$(top_srcdir)/mex/glib-controller/ghashcontroller.h       \
	$(top_srcdir)/mex/glib-controller/gptrarraycontroller.h   \
	$(NULL)

source_h = \
        $(source_public_h)                      \
        gcontrollerversion.h    \
        glib-controller.h       \
        $(NULL)

# marshallers
glib_marshal_list = gcontrollermarshal.list
glib_marshal_prefix = _gcontroller_marshal
include $(top_srcdir)/build/Makefile.am.marshal

# enumerations
glib_enum_h = gcontrollerenumtypes.h
glib_enum_c = gcontrollerenumtypes.c
glib_enum_headers = $(source_public_h)
include $(top_srcdir)/build/Makefile.am.enums

# the base include path for headers
mex_base_includedir = $(includedir)/mex-$(MEX_API_VERSION)
mex_includedir = $(mex_base_includedir)/mex
gcontroller_includedir = $(mex_includedir)/glib-controller

gcontroller_include_HEADERS = \
		$(source_h) \
		$(glib_enum_h) \
		$(NULL)

libglib_controller_1_0_la_CPPFLAGS =	\
	-I$(top_srcdir)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)/mex/		\
	-DG_LOG_DOMAIN=\"GController\"	\
	-DG_DISABLE_SINGLE_INCLUDES	\
	-DGLIB_CONTROLLER_COMPILATION	\
	$(AM_CPPFLAGS)			\
	$(NULL)
libglib_controller_1_0_la_CFLAGS =	\
	$(GCONTROLLER_CFLAGS)		\
	$(MEX_WARNING_CFLAGS)		\
	$(GCOV_CFLAGS)			\
	$(NULL)
libglib_controller_1_0_la_LDFLAGS =	\
	-export-symbols-regex "^[^_]"	\
	-no-undefined			\
	$(NULL)
libglib_controller_1_0_la_LIBADD = $(GCONTROLLER_LIBS) $(GCOV_LIBS)
libglib_controller_1_0_la_SOURCES = $(BUILT_SOURCES) $(source_c) $(source_h)
