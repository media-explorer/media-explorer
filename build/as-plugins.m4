dnl This is derived from GStreamers common/m4/gst-args.m4
dnl MEX_PLUGINS_ALL/MEX_PLUGINS_SELECTED contain all/selected plugins

dnl sets WITH_PLUGINS to the list of plug-ins given as an argument
dnl also clears MEX_PLUGINS_ALL and MEX_PLUGINS_SELECTED
AC_DEFUN([AS_MEX_ARG_WITH_PLUGINS],
[
  AC_ARG_WITH(plugins,
    AC_HELP_STRING([--with-plugins],
      [comma-separated list of dependencyless plug-ins to compile]),
    [WITH_PLUGINS=$withval])

  MEX_PLUGINS_ALL=""
  MEX_PLUGINS_SELECTED=""

  AC_SUBST(MEX_PLUGINS_ALL)
  AC_SUBST(MEX_PLUGINS_SELECTED)
])

dnl AG_MEX_PLUGIN(PLUGIN-NAME)
dnl
dnl This macro adds the plug-in <PLUGIN-NAME> to MEX_PLUGINS_ALL. Then it
dnl checks if WITH_PLUGINS is empty or the plugin is present in WITH_PLUGINS,
dnl and if so adds it to MEX_PLUGINS_SELECTED. Then it checks if the plugin
dnl is present in WITHOUT_PLUGINS (ie. was disabled specifically) and if so
dnl removes it from MEX_PLUGINS_SELECTED.
dnl
dnl The macro will call AM_CONDITIONAL(USE_PLUGIN_<PLUGIN-NAME>, ...) to allow
dnl control of what is built in Makefile.ams.
AC_DEFUN([AS_MEX_PLUGIN],
[
  MEX_PLUGINS_ALL="$MEX_PLUGINS_ALL [$1]"

  define([pname_def],translit([$1], -a-z, _a-z))
  define([PNAME_DEF],translit(PLUGIN_[$1], -a-z, _A-Z))

  AC_ARG_ENABLE([$1],
    AC_HELP_STRING([--disable-[$1]], [disable dependency-less $1 plugin]),
    [
      case "${enableval}" in
        yes) [mex_use_]pname_def=yes ;;
        no) [mex_use_]pname_def=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-$1]) ;;
       esac
    ])

  if test x$[mex_use_]pname_def = xyes; then
    AC_MSG_NOTICE(enabling dependency-less plugin $1)
    WITH_PLUGINS="$WITH_PLUGINS [$1]"
  fi
  if test x$[mex_use_]pname_def = xno; then
    AC_MSG_NOTICE(disabling dependency-less plugin $1)
    WITHOUT_PLUGINS="$WITHOUT_PLUGINS [$1]"
  fi
  undefine([pname_def])

  if [[ -z "$WITH_PLUGINS" ]] || echo " [$WITH_PLUGINS] " | tr , ' ' | grep -i " [$1] " > /dev/null; then
    if test "x$2" != x ; then
      PKG_CHECK_MODULES(PNAME_DEF, [$2])
    fi
    MEX_PLUGINS_SELECTED="$MEX_PLUGINS_SELECTED [$1]"
  fi
  if echo " [$WITHOUT_PLUGINS] " | tr , ' ' | grep -i " [$1] " > /dev/null; then
    MEX_PLUGINS_SELECTED=`echo " $MEX_PLUGINS_SELECTED " | $SED -e 's/ [$1] / /'`
  fi
  AM_CONDITIONAL([USE_]PNAME_DEF, echo " $MEX_PLUGINS_SELECTED " | grep -i " [$1] " > /dev/null)
  undefine([PNAME_DEF])
])

dnl AG_MEX_DISABLE_PLUGIN(PLUGIN-NAME)
dnl
dnl This macro disables the plug-in <PLUGIN-NAME> by removing it from
dnl MEX_PLUGINS_SELECTED.
AC_DEFUN([AS_MEX_DISABLE_PLUGIN],
[
  MEX_PLUGINS_SELECTED=`echo " $MEX_PLUGINS_SELECTED " | $SED -e 's/ [$1] / /'`
  AM_CONDITIONAL([USE_PLUGIN_]translit([$1], a-z, A-Z), false)
])
