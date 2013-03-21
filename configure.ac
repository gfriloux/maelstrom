##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_define([v_maj], [0])
m4_define([v_min], [0])
m4_define([v_mic], [99])
m4_define([v_rev], m4_esyscmd([(git rev-list --count HEAD 2>/dev/null || echo 0) | tr -d '\n']))dnl
##--   When released, remove the dnl on the below line
dnl m4_undefine([v_rev])
##--   When doing snapshots - change soname. remove dnl on below line
m4_define([relname], [ver-pre-svn-09])
m4_define([v_rel], [-release relname])
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], [m4_define([v_ver], [v_maj.v_min.v_mic.v_rev])],
[m4_define([v_ver], [v_maj.v_min.v_mic])])
m4_define([lt_rev], m4_eval(v_maj + v_min))
m4_define([lt_cur], v_mic)
m4_define([lt_age], v_min)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

AC_INIT([maelstrom], [v_ver], [michael.blumenkrantz@gmail.com], [maelstrom])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

AC_GNU_SOURCE
AC_ISC_POSIX

##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], , [m4_define([v_rev], [0])])
m4_ifdef([v_rel], , [m4_define([v_rel], [])])
AC_DEFINE_UNQUOTED(VMAJ, [v_maj], [Major version])
AC_DEFINE_UNQUOTED(VMIN, [v_min], [Minor version])
AC_DEFINE_UNQUOTED(VMIC, [v_mic], [Micro version])
AC_DEFINE_UNQUOTED(VREV, [v_rev], [Revison])
version_info="lt_rev:lt_cur:lt_age"
release_info="v_rel"
AC_SUBST(version_info)
AC_SUBST(release_info)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
VMAJ=v_maj
AC_SUBST(VMAJ)

AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_LIBTOOL
AC_PROG_YACC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_HEADER_STDC
AC_TYPE_SIZE_T

AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_FUNC_MEMCMP

AC_CHECK_SIZEOF([char])
AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])

AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([strdup strndup])
CC_PIC="$lt_prog_compiler_pic"
CXX_PIC="$lt_prog_compiler_pic_CXX"
AC_SUBST([CC_PIC])
AC_SUBST([CXX_PIC])

min_efl_version=1.7
MAELSTROM_REQUIRES=

want_azy=
build_azy=
AC_ARG_ENABLE([azy],
   [AC_HELP_STRING([--disable-azy], [disable building azy @<:@default=detect@:>@])],
   [want_azy=$enableval]
)

if test "x$want_azy" != "xno" ; then
   if test -z "$YACC" ; then
      AC_ERROR([yacc not found])
   fi
   AZY_REQUIRES="ecore-con >= $min_efl_version ecore >= $min_efl_version eina >= $min_efl_version"
   PKG_CHECK_MODULES(AZY, [$AZY_REQUIRES], [build_azy=azy], [build_azy=])
fi
AM_CONDITIONAL(BUILD_AZY, [test "x${build_azy}" != "x"])

want_mysql=
AC_ARG_ENABLE([mysql-tests],
   [AC_HELP_STRING([--disable-mysql-tests], [disable code for mysql in tests @<:@default=detect@:>@])],
   [want_mysql=$enableval]
)

if test -n "$build_azy" ; then
   MAELSTROM_REQUIRES+=" $AZY_REQUIRES"
   azy_pc=azy.pc
   if test "x$want_mysql" != "xno" ; then
      AC_PATH_PROG([mysql_config], [mysql_config])
      if test "x$mysql_config" != "x";then
         AC_CHECK_HEADERS([mysql.h mysql/mysql.h])
         if test "x$ac_cv_header_mysql_h" != "xno" -o "x$ac_cv_header_mysql_mysql_h" != "xno" ; then
            MYSQL_CFLAGS="$($mysql_config --cflags)"
            MYSQL_LIBS="$($mysql_config --libs_r)"
            AC_DEFINE_UNQUOTED(HAVE_MYSQL, 1, "enable some mysql code in tests")
         fi
      fi
   fi
fi
AM_CONDITIONAL(HAVE_MYSQL, [test -n "$MYSQL_LIBS"])
AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_LIBS)

want_extern_rec=
AC_ARG_ENABLE([external-re2c],
   [AC_HELP_STRING([--disable-external-re2c], [disable use of external re2c @<:@default=detect@:>@])],
   [want_extern_rec=$enableval]
)

if test -n "$build_azy" ; then
   if test "x$want_extern_rec" != "xno" ; then
      AC_PATH_PROG([RE2C], [re2c])
   fi
   AM_CONDITIONAL([NEED_RE2C], [test -z $RE2C])
else
   AM_CONDITIONAL([NEED_RE2C], [false])
fi


want_shotgun=
build_shotgun=
AC_ARG_ENABLE([shotgun],
   [AC_HELP_STRING([--disable-shotgun], [disable building shotgun @<:@default=detect@:>@])],
   [want_shotgun=$enableval]
)

if test "x$want_shotgun" != "xno" ; then
   SHOTGUN_REQUIRES="ecore-con >= $min_efl_version ecore >= $min_efl_version eina >= $min_efl_version"
   if test -n "$AZY_LIBS" ; then
     SHOTGUN_CFLAGS="$AZY_CFLAGS"
     SHOTGUN_LIBS="$AZY_LIBS"
     build_shotgun=shotgun
   else
      PKG_CHECK_MODULES(SHOTGUN, [$SHOTGUN_REQUIRES], [build_shotgun=shotgun], [build_shotgun=])
   fi
fi
AM_CONDITIONAL(BUILD_SHOTGUN, [test "x${build_shotgun}" != "x"])

build_shotgun_gui=
want_shotgun_gui=
AC_ARG_ENABLE([shotgun-gui],
   [AC_HELP_STRING([--disable-shotgun-gui], [disable building shotgun gui executable. @<:@default=detect@:>@])],
   [want_shotgun_gui=$enableval], [])

if test -n "$build_shotgun" ; then
   MAELSTROM_REQUIRES+=" $SHOTGUN_REQUIRES"
   shotgun_pc=shotgun.pc
   if test "x$want_shotgun_gui" != "xno"; then
      if test -z "$build_azy" ; then
         AC_ERROR([azy is required to build shotgun gui])
      fi
      PKG_CHECK_MODULES([SHOTGUN_GUI],
         [
          eet >= $min_efl_version
          evas >= $min_efl_version
          ecore-file >= $min_efl_version
          ecore-imf >= $min_efl_version
          edje >= $min_efl_version
          elementary >= $min_efl_version
          efx
         ],
         [build_shotgun_gui=yes],
         [build_shotgun_gui=]
      )
   else
       build_shotgun_gui=
   fi
fi
AM_CONDITIONAL([BUILD_SHOTGUN_GUI], [test "x${build_shotgun_gui}" != "x"])

AC_ARG_WITH([edje-cc],
   [AC_HELP_STRING([--with-edje-cc=PATH],
                   [specify a specific path to edje_cc])],
   [edje_cc=$withval;
    AC_MSG_NOTICE([edje_cc explicitly set to $edje_cc])
   ],[edje_cc=$(pkg-config --variable=prefix edje)/bin/edje_cc])
AC_SUBST(edje_cc)

have_edbus=
want_edbus=
AC_ARG_ENABLE([edbus],
   [AC_HELP_STRING([--disable-edbus], [disable edbus support. @<:@default=detect@:>@])],
   [want_edbus=$enableval], [])

if test -n "$build_shotgun_gui" ; then
   if test "x$want_edbus" != "xno"; then
      PKG_CHECK_MODULES([edbus],
         [edbus >= $min_efl_version],
         [
          AC_DEFINE(HAVE_DBUS, 1, [DBUS support])
          have_edbus="yes"
         ],
         [have_edbus=]
      )
   else
       have_edbus=
   fi
   if test "x$want_edbus" = "xyes" ; then
       if test -n "$have_edbus" ; then
          AC_MSG_ERROR([edbus support requested, but not found by pkg-config.])
       fi
   fi
fi

have_enotify=
want_enotify=
AC_ARG_ENABLE([enotify],
   [AC_HELP_STRING([--disable-enotify], [disable enotify support. @<:@default=detect@:>@])],
   [want_enotify=$enableval], [])

if test -n "$build_shotgun_gui" ; then
   if test "x$have_edbus" = "xyes" ; then
     if test "x$want_enotify" != "xno"; then
        PKG_CHECK_MODULES([enotify],
           [enotify >= $min_efl_version],
           [
            AC_DEFINE(HAVE_NOTIFY, 1, [DBUS Notification support])
            have_enotify="yes"
           ],
           [have_enotify="no"]
        )
     else
         have_enotify="no"
     fi
   fi
   if test "x$want_enotify" = "xyes" ; then
       if test -n "$have_enotify" ; then
          AC_MSG_ERROR([enotify support requested, but not found by pkg-config.])
       fi
   fi
fi

build_sawedoff=
want_sawedoff=
AC_ARG_ENABLE([sawedoff],
   [AC_HELP_STRING([--disable-sawedoff], [disable sawed-off shotgun module for enlightenment. @<:@default=detect@:>@])],
   [want_sawedoff=$enableval], [])

if test "x$want_sawedoff" != "xno"; then
   PKG_CHECK_MODULES([E], [enlightenment edbus2], [build_sawedoff="yes"], [build_sawedoff=])
   MODULE_DIR=$(pkg-config --variable=modules enlightenment)
   release=$(pkg-config --variable=release enlightenment)
   MODULE_ARCH="$host_os-$host_cpu-$release"
   AC_DEFINE_UNQUOTED(MODULE_ARCH, "$MODULE_ARCH", "Module architecture")
fi
AC_SUBST(MODULE_ARCH)
AM_CONDITIONAL([BUILD_SAWEDOFF], [test -n "$build_sawedoff"])
if test -n "$build_sawedoff" ; then
   sawedoff_desktop="src/modules/sawed-off/module.desktop"
fi

AC_ARG_ENABLE(sawedoff-homedir-install,
  AS_HELP_STRING([--enable-sawedoff-homedir-install], [Install modules in homedir]),
  [ MODULE_DIR="${HOME}/.e/e/modules" ]
)
AC_SUBST(MODULE_DIR)


## SSE
build_cpu_sse="yes"
AC_MSG_CHECKING(whether to build sse code)
AC_ARG_ENABLE(cpu-sse,
  AS_HELP_STRING([--enable-cpu-sse], [enable sse code]),
  [
      if test "x$enableval" = "xyes" ; then
        oldcppflags="$CPPFLAGS"
        CPPFLAGS+=" -msse"
        AC_CHECK_HEADER(xmmintrin.h,
          [
            AC_MSG_RESULT(yes)
            AC_DEFINE(USE_SSE, 1, [Build sse Code])
            build_cpu_sse="yes"
           ],
          [
            AC_MSG_RESULT(no)
            build_cpu_sse="no"
          ])
        CPPFLAGS="$oldcppflags"
      else
        AC_MSG_RESULT(no)
        build_cpu_sse="no"
      fi
  ],
  [
    AC_MSG_RESULT($build_cpu_sse)
    if test "x$build_cpu_sse" = "xyes" ; then
      oldcppflags="$CPPFLAGS"
      CPPFLAGS+=" -msse"
      AC_CHECK_HEADER(xmmintrin.h,
        [
          AC_MSG_RESULT(yes)
          AC_DEFINE(USE_SSE, 1, [Build sse Code])
          build_cpu_sse="yes"
         ],
        [
          AC_MSG_RESULT(no)
          build_cpu_sse="no"
        ])
      CPPFLAGS="$oldcflags"
    fi
  ]
)
build_cpu_sse4="yes"
AC_MSG_CHECKING(whether to build sse4 code)
AC_ARG_ENABLE(cpu-sse4,
  AS_HELP_STRING([--enable-cpu-sse4], [enable sse4 code]),
  [
      if test "x$enableval" = "xyes" ; then
        oldcppflags="$CPPFLAGS"
        CPPFLAGS+=" -msse2 -msse4"
        AC_CHECK_HEADER(nmmintrin.h,
          [
            AC_MSG_RESULT(yes)
            AC_DEFINE(USE_SSE4, 1, [Build sse4 Code])
            build_cpu_sse4="yes"
           ],
          [
            AC_MSG_RESULT(no)
            build_cpu_sse4="no"
          ])
        CPPFLAGS="$oldcppflags"
      else
        AC_MSG_RESULT(no)
        build_cpu_sse4="no"
      fi
  ],
  [
    AC_MSG_RESULT($build_cpu_sse4)
    if test "x$build_cpu_sse4" = "xyes" ; then
      oldcppflags="$CPPFLAGS"
      CPPFLAGS+=" -msse2 -msse4"
      AC_CHECK_HEADER(nmmintrin.h,
        [
          AC_MSG_RESULT(yes)
          AC_DEFINE(USE_SSE4, 1, [Build sse4 Code])
          build_cpu_sse4="yes"
         ],
        [
          AC_MSG_RESULT(no)
          build_cpu_sse4="no"
        ])
      CPPFLAGS="$oldcppflags"
    fi
  ]
)
SSE_CFLAGS=
if test "x$build_cpu_sse" = "xyes" ; then
   SSE_CFLAGS+=" -msse"
fi
if test "x$build_cpu_sse4" = "xyes" ; then
   SSE_CFLAGS+=" -msse2 -msse4"
fi
AC_SUBST(SSE_CFLAGS)

m4_ifdef([v_rev],
   [
    EFL_COMPILER_FLAG([-Wshadow])
    EFL_COMPILER_FLAG([-Wall])
    EFL_COMPILER_FLAG([-Wextra])
   ])

AC_SUBST(MAELSTROM_REQUIRES)

AC_CHECK_HEADERS([Rpcdce.h])
EFL_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])
# write output
AC_CONFIG_FILES([
  Makefile
  doc/azy.dox
  $azy_pc
  $shotgun_pc
  $sawedoff_desktop
])
AC_OUTPUT

# report
txt_strip() {
  echo "[$]@" | sed -e 's/^[[ \t]]*\([[^ \t]]*\)[[ \t]]*$/\1/g'
}

echo
cat << SUMMARY_EOF
Summary:
 * project.........: $PACKAGE $VERSION
 * libraries.......: $build_azy $build_shotgun
 * prefix..........: $(txt_strip $prefix)
 * CFLAGS..........: $(txt_strip $CFLAGS)
 * LDFLAGS.........: $(txt_strip $LDFLAGS)
SUMMARY_EOF
echo
cat << FEATURES_EOF
Features:
 * dbus................: $have_edbus
 * notify..............: $have_enotify
 * Shotgun GUI.........: $build_shotgun_gui
FEATURES_EOF
echo

cat << MODULES_EOF
Modules:
 * sawed-off...........: $build_sawedoff
MODULES_EOF
echo

cat << COMPILE_EOF
Compilation........: make (or gmake)
COMPILE_EOF
echo

cat << INSTALL_EOF
Installation.......: make all install (as root if needed, with 'su' or 'sudo')
INSTALL_EOF
echo
