AC_PREREQ(2.53)
AC_INIT(qio, 0.1, edwards@jlab.org)
AC_CONFIG_AUX_DIR(config)

AC_CONFIG_SRCDIR([include/qio.h])
# AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

dnl
dnl
dnl --with-xxxx and --enable-xxxx switches 
dnl

dnl --enable-parallel-arch argument
AC_ARG_ENABLE(parallel-arch,
  AC_HELP_STRING([--enable-parallel-arch=<arch>],
    [Build QDP++ for parallel architecture <arch>]),
  AC_SUBST(PARALLEL_ARCH, [$enableval]),
  AC_SUBST(PARALLEL_ARCH, [scalar])
)

dnl
dnl If needed, look for qmp
dnl Also, QMP is not needed for ac_parallel_arch=scalar, so don't bother
dnl looking for it in that case.
AC_ARG_WITH(qmp,
  AC_HELP_STRING(
    [--with-qmp=DIR],
    [Build QDP++ on top of QMP, where QMP is installed in DIR.]
  ),
  [QMP_HOME="${with_qmp}"]
)

dnl
dnl QIO only supports scalar and parscalar (basically, not-parallel and parallel)
dnl for other cases, map it to one of these
case ${PARALLEL_ARCH} in
  scalarvec)
    AC_MSG_NOTICE([Mapping architecture ${PARALLEL_ARCH} to  scalar])
    AC_SUBST(LOCAL_PARALLEL_ARCH, [scalar])
    ;;
  parscalarvec)
    AC_MSG_NOTICE([Mapping architecture ${PARALLEL_ARCH} to  parscalar])
    AC_SUBST(LOCAL_PARALLEL_ARCH, [parscalar])
    ;;
  *)
    AC_SUBST(LOCAL_PARALLEL_ARCH, PARALLEL_ARCH)
    ;;
esac

dnl Now check appropriate cases
case ${LOCAL_PARALLEL_ARCH} in
  scalar)
    AC_SUBST(ARCHDEF_SUBSTITUTION, ARCH_SCALAR)
    AC_MSG_NOTICE([ Scalar Build! Not checking for QMP ])
    AC_SUBST(QMP_CFLAGS)
    AC_SUBST(QMP_LDFLAGS)
    AC_SUBST(QMP_LIBS)
    ;;
  parscalar)
    AC_SUBST(ARCHDEF_SUBSTITUTION, ARCH_PARSCALAR)
    AC_MSG_NOTICE([ Parscalar build! Checking for QMP ])

    if test "X${QMP_HOME}X" = "XX" ; then
      AC_PATH_PROG([QMP_CONFIG], [qmp-config], [])
    else
      AC_PATH_PROG([QMP_CONFIG], [qmp-config], [], [${QMP_HOME}/bin:${PATH}])
    fi

    if test "X${QMP_CONFIG}X" != "XX" ; then
      AC_MSG_NOTICE([Found QMP configuration program ${QMP_CONFIG}])
      AC_SUBST(QMP_CFLAGS,  "`${QMP_CONFIG} --cflags`")
      AC_MSG_NOTICE([QMP compile flags: ${QMP_CFLAGS}])
      AC_SUBST(QMP_LDFLAGS, "`${QMP_CONFIG} --ldflags`")
      AC_MSG_NOTICE([QMP linking flags: ${QMP_LDFLAGS}])
      AC_SUBST(QMP_LIBS,    "`${QMP_CONFIG} --libs`")
      AC_MSG_NOTICE([QMP libraries flags: ${QMP_LIBS}])
    else 
      AC_MSG_WARN([QMP configuration program qmp-config not found.])
      AC_MSG_WARN([Set environment variables QMP_CFLAGS QMP_LDFAGS QMP_LIBS
        before configure])
    fi
    ;;
  *)
    AC_MSG_ERROR([ Unknown value for --enable-parallel-arch ])
    ;;
esac

dnl verify we can compile and link against QMP, if needed
PAC_QMP_LINK_CC_FUNC(
  [${QMP_CFLAGS}],
  [${QMP_LDFLAGS}],
  [${QMP_LIBS}],
  ,
  ,
  [qmp_link_ok=yes],
  [qmp_link_ok=no]
)

AC_MSG_CHECKING([if we can compile/link of a simple QMP C program])
case ${LOCAL_PARALLEL_ARCH} in
  parscalar)
    if test "X${qmp_link_ok}X" = "XyesX" ; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
      AC_MSG_ERROR([Cannot compile/link a basic QMP C++ program!
        Check QMP_CFLAGS, QMP_LDFLAGS, QMP_LIBS.])
    fi
    ;;
  *)
    AC_MSG_RESULT(ignored)
    ;;
esac


# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

#
# Fricken autoconf does some odd things with these AC include 
# stuff. I found the AC_FUNC_MALLOC substituted the name 
# malloc with rpl_malloc - no idea what that is and failed to
# link on IBM's.
#
# Checks for header files.
## AC_HEADER_STDC
## AC_CHECK_HEADERS([stdlib.h string.h strings.h])

# Checks for typedefs, structures, and compiler characteristics.
## AC_C_CONST
## AC_TYPE_SIZE_T

# Checks for library functions.
## AC_FUNC_MALLOC

#
# Now have all the options... Do some configuration 
#
AM_CONDITIONAL(ARCH_SCALAR,    [test "X${LOCAL_PARALLEL_ARCH}X" = "XscalarX"])
AM_CONDITIONAL(ARCH_PARSCALAR, [test "X${LOCAL_PARALLEL_ARCH}X" = "XparscalarX"])

# Configure subdir packages
AC_CONFIG_SUBDIRS(other_libs/c-dime)

# Configure makefiles
AC_CONFIG_FILES([Makefile
                 include/Makefile
                 lib/Makefile
                 other_libs/Makefile
                 other_libs/c-dime/Makefile])
AC_OUTPUT
