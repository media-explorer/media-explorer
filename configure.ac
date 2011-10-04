m4_define([mex_major], [0])
m4_define([mex_minor], [2])
m4_define([mex_micro], [1])

m4_define([mex_version], [mex_major.mex_minor.mex_micro])
m4_define([mex_api_version], [0.2])

# increase the interface age of 2 for each release
# set to 0 if the API changes
m4_define([mex_interface_age], [0])
m4_define([mex_binary_age], [m4_eval(100 * mex_minor + mex_micro)])

AC_PREREQ(2.53)
AC_INIT([media-explorer], [mex_version], [http://media-explorer.github.com])
AC_CONFIG_SRCDIR([mex/mex.h])
AC_CONFIG_MACRO_DIR([build])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.9 foreign -Wno-portability no-define no-dist-gzip dist-xz tar-ustar])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

MEX_MAJOR_VERSION=mex_major
MEX_MINOR_VERSION=mex_minor
MEX_MICRO_VERSION=mex_micro
MEX_VERSION=mex_version
MEX_API_VERSION=mex_api_version
AC_SUBST(MEX_MAJOR_VERSION)
AC_SUBST(MEX_MINOR_VERSION)
AC_SUBST(MEX_MICRO_VERSION)
AC_SUBST(MEX_VERSION)
AC_SUBST(MEX_API_VERSION)

m4_define([lt_current], [m4_eval(100 * mex_minor + mex_micro - mex_interface_age)])
m4_define([lt_revision], [mex_interface_age])
m4_define([lt_age], [m4_eval(mex_binary_age - mex_interface_age)])
MEX_LT_CURRENT=lt_current
MEX_LT_REV=lt_revision
MEX_LT_AGE=lt_age
MEX_LT_VERSION="$MEX_LT_CURRENT:$MEX_LT_REV:$MEX_LT_AGE"
MEX_LT_LDFLAGS="-version-info $MEX_LT_VERSION"

AC_SUBST(MEX_LT_VERSION)
AC_SUBST(MEX_LT_LDFLAGS)

# release status (development snapshot or stable release)
m4_define([mex_release_status],
          [m4_if(m4_eval(mex_micro % 2), [1], [git],
                 [m4_if(m4_eval(mex_minor % 2), [1], [snapshot],
                       [release])])])
AC_SUBST([MEX_RELEASE_STATUS], [mex_release_status])

# i18n
AS_ALL_LINGUAS
IT_PROG_INTLTOOL([0.40.0], [no-xml])

GETTEXT_PACKAGE=media-explorer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
                   ["$GETTEXT_PACKAGE"],
                   [Gettext domain name])
AM_GLIB_GNU_GETTEXT
GLIB_DEFINE_LOCALEDIR(LOCALEDIR)

# check for programs
AC_PROG_CC
AM_PROG_CC_C_O

AM_PATH_GLIB_2_0

PKG_PROG_PKG_CONFIG

# check for headers
AC_HEADER_STDC

GLIB_REQ_VERSION=2.26
MX_REQ_VERSION=1.3.2
GRILO_REQ_VERSION=0.1.16
CLUTTER_REQ_VERSION=1.8.0
CLUTTER_GST_REQ_VERSION=1.4.0
GSTREAMER_REQ_VERSION=0.10.26

AC_SUBST(GLIB_REQ_VERSION)
AC_SUBST(MX_REQ_VERSION)
AC_SUBST(GRILO_REQ_VERSION)
AC_SUBST(CLUTTER_REQ_VERSION)
AC_SUBST(CLUTTER_GST_REQ_VERSION)
AC_SUBST(GSTREAMER_REQ_VERSION)

# telepathy plugin
TELEPATHY_GLIB_REQ_VERSION=0.15.5

AC_SUBST(TELEPATHY_GLIB_REQ_VERSION)

# you can specify a --with-profile=$profile as a configuration option to hopefully
# get sane defaults.

# we base our OS detection on the winsys soname of the clutter library. This
# soname reflects which GL winsys has been when compiling clutter and we can use
# it to set our default profile (see --with-profile)
clutter_soname=`$PKG_CONFIG --variable soname_infix clutter-1.0`

AS_CASE([$clutter_soname],

        [cex100],
        [
          mex_os=linux
          default_profile=intelce
        ],

        [glx|eglx],
        [
          mex_os=linux
          default_profile=gnome
        ],

        [win32],
        [
          mex_os=windows
          default_profile=windows
        ],

        [AC_MSG_ERROR([Unknown clutter winsys $clutter_soname, please add it and optionally create a new profile])])

AC_SUBST(MEX_OS, [$mex_os])

AC_ARG_WITH([profile],
            [AC_HELP_STRING([--with-profile=@<:@gnome/intelce/windows@:>@],
                            [Specify the configuration profile @<:@default=auto@:>@])],
            [],
            [with_profile=$default_profile])

AS_CASE([$with_profile],

        [gnome],
        [
          mex_default_with_player=clutter-gst
          mex_default_enable_networks=no
          mex_default_enable_lirc=yes

          PROFILE_PLUGINS="gnome-dvb"
        ],

        [intelce],
        [
          mex_default_with_player=d-bus
          mex_default_enable_networks=yes
          mex_default_enable_lirc=no

          PROFILE_PLUGINS=""
        ],

        [windows],
        [
          mex_default_with_player=clutter-gst
          mex_default_enable_networks=no
          mex_default_enable_lirc=no

          PROFILE_PLUGINS=""
        ],

        [AC_MSG_ERROR([Unknown profile $with_profile])])

modules="glib-2.0 >= $GLIB_REQ_VERSION                  \
         gio-2.0                                        \
         gdk-pixbuf-2.0                                 \
         libsoup-2.4                                    \
         clutter-1.0 >= $CLUTTER_REQ_VERSION            \
         mx-1.0 >= $MX_REQ_VERSION                      \
         grilo-0.1 >= $GRILO_REQ_VERSION                \
         gstreamer-0.10 >= $GSTREAMER_REQ_VERSION       \
         gstreamer-tag-0.10 >= $GSTREAMER_REQ_VERSION   \
         clutter-gst-1.0 >= $CLUTTER_GST_REQ_VERSION"

PKG_CHECK_EXISTS([gio-unix-2.0],
                 [
                   have_gio_unix=yes
                   AC_DEFINE([HAVE_GIO_UNIX], [1],
                             ["Defined if gio-unix-2.0 is available"])
                   modules="$modules gio-unix-2.0"
                 ],
                 [have_gio_unix=no])
AM_CONDITIONAL(HAVE_GIO_UNIX, [test x"$have_gio_unix" = x"yes"])

PKG_CHECK_EXISTS([dbus-glib-1],
                 [
                   have_dbus_glib=yes
                   AC_DEFINE([HAVE_DBUS_GLIB], [1],
                             ["Defined if dbus-glib is available"])
                   modules="$modules dbus-glib-1"
                 ],
                 [have_dbus_glib=no])
AM_CONDITIONAL(HAVE_DBUS_GLIB, [test x"$have_dbus_glib" = x"yes"])

PKG_CHECK_EXISTS(libsoup-gnome-2.4,
                 [modules="$modules libsoup-gnome-2.4"
                 AC_DEFINE([HAVE_LIBSOUP_GNOME], [1], ["Defined if libsoup-gnome is available"])],
                 modules="$modules libsoup-2.4")

AC_ARG_WITH([player],
              [AC_HELP_STRING([--with-player=@<:@d-bus/clutter-gst/surface@:>@].
                              [Set mode for media playback @<:@default=clutter-gst@:>@])],
              [],
              [with_player=$mex_default_with_player])

player_deps=""


AS_CASE([$with_player],
        [clutter-gst],
        [
          AC_DEFINE([USE_PLAYER_CLUTTER_GST], 1, [Use "clutter-gst" for
                     playback])
          PROFILE_PLUGINS+=" bg-video"
        ],

        [d-bus],
        [
          AC_DEFINE([USE_PLAYER_DBUS], 1, [Use "d-bus" for playback])
          PROFILE_PLUGINS+=" bg-dbusvideo"
        ],

        [surface],
        [
          AC_DEFINE([USE_PLAYER_SURFACE], 1, [Use "surface" for playback])
        ],

        [AC_MSG_ERROR([Invalid value for --with-player])]
)

AM_CONDITIONAL(ENABLE_EXTERNAL_PLAYER, [test x"$with_player" = x"d-bus"])

PKG_CHECK_MODULES(MEX, "$modules")

PKG_CHECK_MODULES(GCONTROLLER, [gobject-2.0 >= 2.20])

AC_ARG_ENABLE([lirc],
              [AC_HELP_STRING([--enable-lirc],
                              [Enables LIRC remote control support])],
              [],
              [enable_lirc=$mex_default_enable_lirc])

if test "x$enable_lirc" = "xyes"; then
  AC_CHECK_LIB([lirc_client],
               [lirc_init],
               [AC_CHECK_HEADER([lirc/lirc_client.h],
                                [have_lirc=yes],
                                [have_lirc=no])],
               [have_lirc=no])

  if test "x$have_lirc" = "xyes"; then
    AC_DEFINE([HAVE_LIRC], [1], [LIRC support is enabled])
  else
    AC_MSG_ERROR([LIRC client support not available])
  fi

  MEX_LIRC_LIBS="-llirc_client"
  MEX_LIBS="${MEX_LIBS} ${MEX_LIRC_LIBS}"

  AC_SUBST(MEX_LIRC_LIBS)
fi

AM_CONDITIONAL([HAVE_LIRC], [test "x$enable_lirc" = "xyes"])

# marshals
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])

GOBJECT_INTROSPECTION_CHECK([0.9.5])

# enable debug level
m4_define([debug_default],
          [m4_if(m4_eval(mex_micro % 2), [1], [yes], [minimum])])

AC_ARG_ENABLE([debug],
              [AC_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
                              [Enables on debugging @<:@default=debug_default@:>@])],
              [],
              [enable_debug=debug_default])

AS_CASE([$enable_debug],
        [yes],
        [
          test "$cflags_set" = set || CFLAGS="$CFLAGS -g -O0"
          MEX_DEBUG_CFLAGS="-DMEX_ENABLE_DEBUG"
        ],

        [minimum],
        [
          MEX_DEBUG_CFLAGS="-DMEX_ENABLE_DEBUG -DG_DISABLE_CAST_CHECKS"
        ],

        [no],
        [
          MEX_DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
        ],

        [AC_MSG_ERROR([Invalid value for --enable-debug])]
)

AC_SUBST(MEX_DEBUG_CFLAGS)

AC_MSG_CHECKING([how to detect we are online])
AC_ARG_WITH([online],
            [AS_HELP_STRING([--with-online],
                            [how to check if we are online (always, connman, networkmanager, test)])],
                            [],
                            [with_online=always])

AS_CASE(["$with_online"],

        ["always"],
        [
        AC_MSG_RESULT([always])
        AC_DEFINE([WITH_ONLINE_ALWAYS], 1, [No online detection])
        ],

        ["connman"],
        [
        AC_MSG_RESULT([Connection Manager])
        AC_DEFINE([WITH_ONLINE_CONNMAN], 1, [ConnMan online detection])
        ],

        ["networkmanager"],
        [
        AC_MSG_RESULT([Network Manager])
        PKG_CHECK_MODULES(NM, libnm-glib >= 0.7)
        AC_DEFINE([WITH_ONLINE_NM], 1, [NM online detection])
        ],

        [AC_MSG_ERROR([Unknown argument "$with_online" to --with-online])]
)

# enable idle video
AC_ARG_ENABLE([idle-video],
              [AC_HELP_STRING([--enable-idle-video=@<:@no/yes@:>@],
                              [Play the idle video in the background @<:@default=yes@:>@])],
              [],
              [enable_idle_video=yes])

AS_IF([test "x$enable_idle_video" = "xyes"],
      [AC_DEFINE([ENABLE_IDLE_VIDEO], 1,
                 [Play the idle video in the background])])
AM_CONDITIONAL([ENABLE_IDLE_VIDEO], [test "x$enable_idle_video" = "xyes"])


AC_ARG_WITH([thumbnailer],
            [AS_HELP_STRING([--with-thumbnailer],
                            [which thumbnailer to use (internal, tumbler)])],
                            [],
                            [with_thumbnailer=internal])

AS_CASE(["$with_thumbnailer"],

        ["internal"],
        [
        AC_DEFINE([WITH_THUMBNAILER_INTERNAL], 1, [Use internal thumbnailer])
        ],

        ["tumbler"],
        [
        AC_DEFINE([WITH_THUMBNAILER_TUMBLER], 1, [Use Tumbler for thumbnailing])
        ],

        [AC_MSG_ERROR([Unknown argument "$with_thumbnailer" to --with-thumbnailer])]
)
AM_CONDITIONAL([ENABLE_INTERNAL_THUMBNAILER],
               [test "x$with_thumbnailer" = "xinternal"])


# enable strict compiler flags
AC_ARG_ENABLE([warning-flags],
              [AC_HELP_STRING([--enable-warning-flags=@<:@no/yes@:>@],
                              [Use strict compiler flags @<:@default=yes@:>@])],
              [],
              [enable_warning_flags=yes])

AS_IF([test "x$enable_warning_flags" = "xyes"],
      [
        #-Wwrite-strings
        AS_COMPILER_FLAGS([MEX_WARNING_CFLAGS],
                          ["-Wall -Wshadow -Wcast-align -Wmissing-declarations
                            -Wdeclaration-after-statement -Wmissing-prototypes
                            -Wredundant-decls -Wmissing-noreturn -Wpointer-arith
                            -Winline -Wformat-nonliteral -Wformat-security
                            -Winit-self -Wmissing-include-dirs -Wundef
                            -Waggregate-return -Wempty-body -Wpacked
                            -Wmissing-format-attribute -Wnested-externs"])
      ],
      [
        AS_COMPILER_FLAGS([MEX_WARNING_CFLAGS],
                          ["-Wall"])
      ]
)

# Unfortunately, some versions of gcc don't return an error code when an option is not
# recognized and then AS_COMPILER_FLAGS() above does not exclude unknown options.
AS_IF([test "x$CC" = "xclang"],
      [MEX_WARNING_CFLAGS="$MEX_WARNING_CFLAGS -Qunused-arguments"])

AC_SUBST(MEX_WARNING_CFLAGS)

# Compile tests
AC_ARG_ENABLE([tests],
              [AC_HELP_STRING([--enable-tests=@<:@no/yes@:>@],
                              [Build the unit tests default=no])],
              [],
              [enable_tests=no])

AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" = "xyes"])
AS_IF([test "x$enable_tests" = "xyes"],
      AC_DEFINE([ENABLE_TESTS], [1], [Build unit tests]))

# Compile examples
AC_ARG_ENABLE([examples],
              [AC_HELP_STRING([--enable-examples=@<:@no/yes@:>@],
                              [Build the examples default=yes])],
              [],
              [enable_examples=yes])

AM_CONDITIONAL([ENABLE_EXAMPLES], [test "x$enable_examples" = "xyes"])
AS_IF([test "x$enable_examples" = "xyes"],
      AC_DEFINE([ENABLE_EXAMPLES], [1], [Build examples]))

# GTK Doc check
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.13], [--flavour no-tmpl])
])
AM_CONDITIONAL([BUILD_GTK_DOC], [test "x$enable_gtk_doc" = "xyes"])

#
# Configuration applets
#

# Check the dependencies of rebinder
AC_ARG_ENABLE([rebinder],
              [AC_HELP_STRING([--enable-rebinder=@<:@yes/no@:>@],
                              [Enable the keyboard rebinder utility @<:@default=no@:>@])],
              [],
              [enable_rebinder=no])

AS_IF([test "x$enable_rebinder" = "xyes"],
      [PKG_CHECK_MODULES(REBINDER, [mx-1.0 dbus-glib-1 glib-2.0 gio-2.0 gudev-1.0 x11 xtst])
      AC_DEFINE(HAVE_REBINDER, [1], [Defined if the rebinder is available])],
      [])
AM_CONDITIONAL(ENABLE_REBINDER, test "x$enable_rebinder" = "xyes")

# Check the dependencies of network configuration ui
AC_ARG_ENABLE([networks],
              [AC_HELP_STRING([--enable-networks=@<:@yes/no@:>@],
                              [Enable the network configuration UI @<:@default=no@:>@])],
              [],
              [enable_networks=$mex_default_enable_networks])

AS_IF([test "x$enable_networks" = "xyes"],
      [PKG_CHECK_MODULES(NETWORKS, [glib-2.0 mx-1.0])],
      [])
AM_CONDITIONAL(ENABLE_NETWORKS, test "x$enable_networks" = "xyes")

# Check the dependencies of webremote daemon
AC_ARG_ENABLE([webremote],
              [AC_HELP_STRING([--enable-webremote=@<:@yes/no@:>@],
                              [Enable the webremote daemon @<:@default=yes@:>@])],
              [],
              [enable_webremote=yes])

PKG_CHECK_EXISTS([tracker-sparql-0.12],
                 [tracker_dep="tracker-sparql-0.12"],
                 [tracker_dep="tracker-sparql-0.10"])

AS_IF([test "x$enable_webremote" = "xyes"],
      [PKG_CHECK_MODULES(WEBREMOTE, [glib-2.0 json-glib-1.0 gio-2.0 libsoup-2.4 $tracker_dep avahi-glib avahi-client])
      AC_DEFINE(HAVE_WEBREMOTE, [1], [Defined if the webremote is available])],
      [])
AM_CONDITIONAL(ENABLE_WEBREMOTE, test "x$enable_webremote" = "xyes")
# End webremote

# Check the dependencies of displayconf
AC_ARG_ENABLE([displayconf],
              [AC_HELP_STRING([--enable-displayconf=@<:@yes/no@:>@],
                              [Enable the displayconf utility @<:@default=no@:>@])],
              [],
              [enable_displayconf=no])
AS_IF([test "x$enable_displayconf" = "xyes"],
      [PKG_CHECK_MODULES(DISPLAYCONF, [glib-2.0 gnome-desktop-2.0 clutter-1.0])],
      [])
AM_CONDITIONAL(ENABLE_DISPLAYCONF, test "x$enable_displayconf" = "xyes")

#
# Shell
#

PKG_CHECK_MODULES(SHELL, [mx-1.0 clutter-1.0 glib-2.0])

AS_CASE([$clutter_soname],

        [cex100],
        [
        AC_DEFINE(HAVE_CLUTTER_CEX100, [1], [Defined if we are on CEX100])
        ],

        [glx|eglx],
        [
        AC_DEFINE(HAVE_CLUTTER_X11, [1], [Defined if we are on X11])
        ]
)

# Default list of plugins to compile
WITH_PLUGINS="search,queue,tracker,dbusinput,bliptv,justintv,opticaldisc,bg-backdrop"

# the debug plugin is enabled in debug mode
AS_IF([test "x$enable_debug" = "xyes"],
      [WITH_PLUGINS="$WITH_PLUGINS,debug-buddy"])

# profile specific plugins
AS_IF([test -n "$PROFILE_PLUGINS"],
      [WITH_PLUGINS="$WITH_PLUGINS,$PROFILE_PLUGINS"])

# Provide a --with-plugins/--without-plugins configure option
AS_MEX_ARG_WITH_PLUGINS

# Declare all the plugins we have
AS_MEX_PLUGIN(bg-video)
AS_MEX_PLUGIN(bg-dbusvideo)
AS_MEX_PLUGIN(bg-backdrop)
AS_MEX_PLUGIN(library)
AS_MEX_PLUGIN(recommended)
AS_MEX_PLUGIN(search, [rest-0.7])
AS_MEX_PLUGIN(queue)
AS_MEX_PLUGIN(applications)
AS_MEX_PLUGIN(upnp)
AS_MEX_PLUGIN(tracker)
AS_MEX_PLUGIN(dbusinput)
AS_MEX_PLUGIN(bliptv)
AS_MEX_PLUGIN(justintv)
AS_MEX_PLUGIN(telepathy,
              [telepathy-glib >= $TELEPATHY_GLIB_REQ_VERSION  \
               gstreamer-0.10                                 \
               telepathy-farstream                            \
               telepathy-yell                                 \
               farsight2-0.10])
AS_MEX_PLUGIN(debug-buddy)
AS_MEX_PLUGIN(gnome-dvb)
AS_MEX_PLUGIN(opticaldisc)

# The debug plugin needs libunwind that does not provide a pkg-config file
AS_IF([test "x$mex_use_debug_buddy_plugin" = "xyes"],
      [AC_CHECK_HEADER([libunwind.h],
                       [],
                       [AC_MSG_ERROR([The debug plugin needs libunwind])])])

AC_CONFIG_FILES([
  Makefile
  build/Makefile
  mex/Makefile
  mex/glib-controller/Makefile
  player/Makefile
  data/Makefile
  data/appicons/Makefile
  examples/Makefile
  doc/Makefile
  doc/reference/Makefile
  doc/reference/mex/Makefile
  doc/reference/mex/mex-docs.xml
  applets/Makefile
  applets/displayconf/Makefile
  applets/rebinder/Makefile
  applets/networks/Makefile
  applets/webremote/Makefile
  plugins/Makefile
  po/Makefile.in
  shell/Makefile
  tests/Makefile
  thumbnailer/Makefile
  tools/Makefile
])

AC_OUTPUT

# Trim MEX_PLUGINS_SELECTED
MEX_PLUGINS_SELECTED=`echo $MEX_PLUGINS_SELECTED | tr -s ' ' | sed 's/^ //'`

echo ""
echo "media explorer - $VERSION"
echo ""
echo " • Profile: ${with_profile}"
echo ""
echo " • Build:"
echo "        Debugging flags   : ${enable_debug}"
echo "        Maintainer flags  : ${enable_warning_flags}"
echo "        Unit tests        : ${enable_tests}"
echo "        Examples          : ${enable_examples}"
echo "        Introspection data: ${enable_introspection}"
echo ""
echo " • Backends:"
echo "        Online detection : ${with_online}"
echo "        Media playback   : ${with_player}"
echo "        Thumbnailer      : ${with_thumbnailer}"
echo ""
echo " • Documentation:"
echo "        Build API Reference: ${enable_gtk_doc}"
echo ""
echo " • Plugins: ${MEX_PLUGINS_SELECTED}"
echo ""
echo " • Applets:"
echo "        Displayconf : ${enable_displayconf}"
echo "        Rebinder    : ${enable_rebinder}"
echo "        Networks    : ${enable_networks}"
echo "        Webremote   : ${enable_webremote}"
echo ""
echo " • Extra:"
echo "        Enable LIRC support: ${enable_lirc}"
echo "        Play the idle video: ${enable_idle_video}"
echo ""
