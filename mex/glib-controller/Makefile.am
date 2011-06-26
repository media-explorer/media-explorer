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

libglib_controller_1_0_la_CPPFLAGS =	\
	-I$(top_srcdir)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)/mex/		\
	-DG_LOG_DOMAIN=\"GController\"	\
	-DG_DISABLE_DEPRECATED		\
	-DG_DISABLE_SINGLE_INCLUDES	\
	-DGLIB_CONTROLLER_COMPILATION	\
	$(AM_CPPFLAGS)			\
	$(NULL)
libglib_controller_1_0_la_CFLAGS =	\
	$(GCONTROLLER_CFLAGS)		\
	$(MAINTAINER_CFLAGS)		\
	$(GCOV_CFLAGS)			\
	$(NULL)
libglib_controller_1_0_la_LDFLAGS =	\
	-export-symbols-regex "^[^_]"	\
	-no-undefined			\
	$(NULL)
libglib_controller_1_0_la_LIBADD = $(GCONTROLLER_LIBS) $(GCOV_LIBS)
libglib_controller_1_0_la_SOURCES = $(BUILT_SOURCES) $(source_c) $(source_h)

# introspection
if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(top_srcdir)/mex/glib-controller
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_srcdir) --includedir=$(top_builddir)

GController-1.0.gir: $(INTROSPECTION_SCANNER) libglib-controller-1.0.la

GController_1_0_gir_SCANNERFLAGS = --identifier-prefix=G
GController_1_0_gir_INCLUDES = GObject-2.0
GController_1_0_gir_CFLAGS = $(GCONTROLLER_CFLAGS) -I$(top_srcdir)/mex -I$(top_srcdir)/mex/glib-controller -I$(top_builddir) -DG_DISABLE_DEPRECATED -DGLIB_CONTROLLER_COMPILATION
GController_1_0_gir_LIBS = libglib-controller-1.0.la
GController_1_0_gir_FILES = $(source_h) $(source_c)

INTROSPECTION_GIRS += GController-1.0.gir
endif

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
