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

AC_USE_SYSTEM_EXTENSIONS

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

LT_INIT([win32-dll disable-static])
AC_PROG_YACC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET

AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

AC_CHECK_SIZEOF([char])
AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])

AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([strdup strndup])

PKG_PROG_PKG_CONFIG

min_efl_version=1.7
MAELSTROM_REQUIRES=

#############################

AC_ARG_ENABLE([email],
   [AC_HELP_STRING([--disable-email], [disable building email @<:@default=yes@:>@])],
   [want_email=$enableval],
   [want_email="yes"]
)

EMAIL_REQUIRES="ecore-con >= $min_efl_version ecore >= $min_efl_version eina >= $min_efl_version"
build_email="no"

if test "x$want_email" = "xyes" ; then
   PKG_CHECK_EXISTS([$EMAIL_REQUIRES], [build_email="yes"], [build_email="no"])
fi

if test "x$build_email" = "xyes" ; then
   PKG_CHECK_MODULES([EMAIL], [$EMAIL_REQUIRES])
   MAELSTROM_REQUIRES="$EMAIL_REQUIRES"
fi

AM_CONDITIONAL([BUILD_EMAIL], [test "x${build_email}" = "xyes"])

#################################

AC_ARG_ENABLE([shotgun],
   [AC_HELP_STRING([--disable-shotgun], [disable building shotgun @<:@default=yes@:>@])],
   [want_shotgun=$enableval],
   [want_shotgun="yes"]
)

SHOTGUN_REQUIRES="ecore-con >= $min_efl_version ecore >= $min_efl_version eina >= $min_efl_version"
build_shotgun="no"

if test "x$want_shotgun" = "xyes" ; then
   PKG_CHECK_EXISTS([$SHOTGUN_REQUIRES], [build_shotgun="yes"], [build_shotgun="no"])
fi

if test "x$build_shotgun" = "xyes" ; then
   PKG_CHECK_MODULES([SHOTGUN], [$SHOTGUN_REQUIRES])
   MAELSTROM_REQUIRES="$SHOTGUN_REQUIRES"
fi

AM_CONDITIONAL([BUILD_SHOTGUN], [test "x${build_shotgun}" = "xyes"])

#############################

AC_ARG_ENABLE([azy],
   [AC_HELP_STRING([--disable-azy], [disable building azy @<:@default=detect@:>@])],
   [want_azy=$enableval],
   [want_azy="auto"]
)

AZY_REQUIRES="ecore-con >= $min_efl_version ecore >= $min_efl_version eina >= $min_efl_version eet >= $min_efl_version"

case "$host_os" in
   mingw*)
      AZY_REQUIRES="${AZY_REQUIRES} evil >= $min_efl_version"
      AC_DEFINE([HAVE_EVIL], [1], [Set to 1 if Evil package is installed])
   ;;
esac

build_azy="no"

if test "x$want_azy" = "xyes" || test "x$want_azy" = "xauto" ; then
   if test -z "$YACC" ; then
      if test "x$want_azy" = "xyes" ; then
         AC_MSG_ERROR([yacc not found])
      else
         AC_MSG_WARN([yacc not found, azy disabled])
      fi
   else
      PKG_CHECK_EXISTS([$AZY_REQUIRES], [build_azy="yes"], [build_azy="no"])
   fi
fi

if test "x$build_azy" = "xyes" ; then
   PKG_CHECK_MODULES([AZY], [$AZY_REQUIRES])
fi

AM_CONDITIONAL([BUILD_AZY], [test "x${build_azy}" = "xyes"])

AC_ARG_ENABLE([mysql-tests],
   [AC_HELP_STRING([--disable-mysql-tests], [disable code for mysql in tests @<:@default=yes@:>@])],
   [want_mysql=$enableval],
   [want_mysql="yes"]
)

if test "x$want_mysql" = "xyes" && test "x$build_azy" = "xyes" ; then
   AC_PATH_PROG([mysql_config], [mysql_config])
   if test "x$mysql_config" != "x" ; then
      AC_CHECK_HEADERS([mysql.h mysql/mysql.h], [have_mysql="yes"], [have_mysql="no"])
      if test "x$have_mysql" = "xyes" ; then
         MYSQL_CFLAGS="$($mysql_config --cflags)"
         MYSQL_LIBS="$($mysql_config --libs_r)"
         AC_DEFINE_UNQUOTED([HAVE_MYSQL], [1], ["enable some mysql code in tests"])
      fi
   fi
fi

AC_MSG_CHECKING([whether mysql tests are compiled])
AC_MSG_RESULT([$have_mysql])

AM_CONDITIONAL([HAVE_MYSQL], [test "x$have_mysql" = "xyes"])

AC_SUBST([MYSQL_CFLAGS])
AC_SUBST([MYSQL_LIBS])

AC_ARG_ENABLE([external-re2c],
   [AC_HELP_STRING([--disable-external-re2c], [disable use of external re2c @<:@default=yes@:>@])],
   [want_extern_rec=$enableval],
   [want_extern_rec="yes"]
)

if test "x$want_extern_rec" = "xyes" && test "x$build_azy" = "xyes" ; then
   AC_PATH_PROG([RE2C], [re2c])
fi

have_re2c="yes"
if test -z $ac_cv_path_RE2C ; then
   have_re2c="no"
fi

AC_MSG_CHECKING([whether external re2cc is used])
AC_MSG_RESULT([$have_re2c])

AM_CONDITIONAL([NEED_RE2C], [test "x$have_re2c" = "xno"])

#################################

AC_ARG_ENABLE([shotgun-gui],
   [AC_HELP_STRING([--disable-shotgun-gui], [disable building shotgun gui executable. @<:@default=detect@:>@])],
   [want_shotgun_gui=$enableval],
   [want_shotgun_gui="auto"]
)

build_shotgun_gui="no"

if test "x$want_shotgun_gui" = "xyes" || test "x$want_shotgun_gui" = "xauto" ; then
   if test "x$want_shotgun_gui" = "xyes" ; then
      if test "x$build_azy" = "xno" || test "x$build_shotgun" = "xno" ; then
         AC_MSG_ERROR([azy and shotgun are required to build shotgun gui])
      else
         build_shotgun_gui="yes"
      fi
   else
      if test "x$build_azy" = "xno" || test "x$build_shotgun" = "xno" ; then
         AC_MSG_WARN([azy and shotgun are required to build shotgun gui])
      else
         build_shotgun_gui="yes"
      fi
   fi
fi

AC_MSG_CHECKING([whether shotgun gui is build])

if test "x$build_shotgun_gui" = "xyes" ; then
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
      [build_shotgun_gui=no]
   )
fi

AM_CONDITIONAL([BUILD_SHOTGUN_GUI], [test "x${build_shotgun_gui}" != "xno"])

##############################

AC_ARG_WITH([edje-cc],
   [AC_HELP_STRING([--with-edje-cc=PATH],
                   [specify a specific path to edje_cc])],
   [
    edje_cc=$withval;
    AC_MSG_NOTICE([edje_cc explicitly set to $edje_cc])
   ],
   [edje_cc=$($PKG_CONFIG --variable=prefix edje)/bin/edje_cc])

AC_SUBST([edje_cc])

AC_ARG_ENABLE([eldbus],
   [AC_HELP_STRING([--disable-eldbus], [disable eldbus support. @<:@default=detect@:>@])],
   [want_eldbus=$enableval],
   [want_eldbus="auto"]
)

have_eldbus=no
if test "x$build_shotgun_gui" = "xyes" ; then
   if test "x$want_eldbus" = "xyes" || test "x$want_eldbus" = "xauto" ; then
      PKG_CHECK_MODULES([ELDBUS],
         [eldbus],
         [
          AC_DEFINE([HAVE_DBUS], [1], [DBUS support])
          have_eldbus="yes"
         ],
         [have_eldbus="no"]
      )
   else
       have_eldbus="no"
   fi
   if test "x$want_eldbus" = "xyes" ; then
      if test "x$have_eldbus" = "xno" ; then
         AC_MSG_ERROR([eldbus support requested, but not found by pkg-config.])
      fi
   else
      if test "x$have_eldbus" = "xno" ; then
         AC_MSG_WARN([eldbus not found by pkg-config. dbus support disabled])
      fi
   fi
fi

AC_ARG_ENABLE([sawedoff],
   [AC_HELP_STRING([--disable-sawedoff], [disable sawed-off shotgun module for enlightenment. @<:@default=yes@:>@])],
   [want_sawedoff=$enableval],
   [want_sawedoff="yes"]
)

build_sawedoff="no"
if test "x$want_sawedoff" = "xyes" ; then
   PKG_CHECK_MODULES([E], [enlightenment eldbus],
      [
       build_sawedoff="yes"
       MODULE_DIR=$(${PKG_CONFIG} --variable=modules enlightenment)
       release=$(${PKG_CONFIG} --variable=release enlightenment)
       MODULE_ARCH="$host_os-$host_cpu-$release"
       AC_DEFINE_UNQUOTED([MODULE_ARCH], [$MODULE_ARCH], ["Module architecture"])
       sawedoff_desktop="src/modules/sawed-off/module.desktop"
      ],
      [build_sawedoff="no"])
fi

AC_SUBST([MODULE_ARCH])

AM_CONDITIONAL([BUILD_SAWEDOFF], [test "x$build_sawedoff" = "xyes"])

AC_ARG_ENABLE(sawedoff-homedir-install,
  AS_HELP_STRING([--enable-sawedoff-homedir-install], [Install modules in homedir]),
  [MODULE_DIR="${HOME}/.e/e/modules"]
)

AC_SUBST([MODULE_DIR])


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
AC_SUBST(AZY_REQUIRES)
AC_SUBST(EMAIL_REQUIRES)
AC_SUBST(SHOTGUN_REQUIRES)

AC_CHECK_HEADERS([Rpcdce.h])
EFL_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])
# write output
AC_CONFIG_FILES([
Makefile
doc/azy.dox
azy.pc
email.pc
shotgun.pc
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
 * libraries.......:
     Email.........: $build_email
     AZY...........: $build_azy
     Shotgun.......: $build_shotgun
 * prefix..........: $(txt_strip $prefix)
 * CFLAGS..........: $(txt_strip $CFLAGS)
 * LDFLAGS.........: $(txt_strip $LDFLAGS)
SUMMARY_EOF
echo
cat << FEATURES_EOF
Features:
 * Shotgun GUI.........: $build_shotgun_gui (dbus: $have_eldbus)
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
