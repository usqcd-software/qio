AC_PREREQ(2.53)
AC_INIT(qio, 1.0.beta2, detar@physics.utah.edu)
AC_CONFIG_AUX_DIR(config)

AC_CONFIG_SRCDIR([include/qio.h])
AC_CONFIG_HEADER([include/qio_config.h])

AM_INIT_AUTOMAKE

dnl
dnl
dnl --with-xxxx and --enable-xxxx switches 
dnl
AC_ARG_ENABLE(qio, 
  AC_HELP_STRING([--enable-qio],
		 [ Enable QIO Subsystem (default on)]),
  [ QIO_ENABLED=$enableval ],
  [ QIO_ENABLED="yes"] 
)

if test "X${QIO_ENABLED}X" = "XyesX" ; then
  AC_MSG_NOTICE([ Building QIO ])
else 
  if test "X${QIO_ENABLED}X" = "XnoX" ; then
     AC_MSG_NOTICE([ NOT Building full QIO - Only LIME ])
  else
     AC_MSG_ERROR([ Error: Incorrect value for QIO_ENABLED: ${QIO_ENABLED}. Check --enable-qio ])
  fi
fi

AM_CONDITIONAL(BUILD_QIO, [ test "X${QIO_ENABLED}X" = "XyesX" ])


dnl
dnl --enable-parallel-io
dnl
AC_ARG_ENABLE(parallel-io,
  AC_HELP_STRING([--enable-parallel-io],
    [Only supported for parallel file systems with a parallel fseek]),
  [ac_parallel_io=1],
  [ac_parallel_io=0]
)


dnl
dnl --with-qmp=
dnl
AC_ARG_WITH(qmp,
  AC_HELP_STRING(
    [--with-qmp=DIR],
    [Build QIO on top of the QMP installed in DIR.]
  ),
  [QMP_HOME="${with_qmp}"]
)


dnl
dnl Now check the given QMP
dnl
AC_MSG_NOTICE([ Checking QMP ])

if test "X${QMP_HOME}X" = "XX" ; then
  AC_PATH_PROG([QMP_CONFIG], [qmp-config], [])
else
  AC_PATH_PROG([QMP_CONFIG], [qmp-config], [], [${QMP_HOME}/bin ${PATH}])
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


dnl
dnl verify we can compile and link against QMP
dnl
PAC_QMP_LINK_CC_FUNC(
  [${QMP_CFLAGS}],
  [${QMP_LDFLAGS}],
  [${QMP_LIBS}],
  ,
  ,
  [qmp_link_ok=yes],
  [qmp_link_ok=no]
)

AC_MSG_CHECKING([if we can compile/link a simple QMP C program])
if test "X${qmp_link_ok}X" = "XyesX" ; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([Cannot compile/link a basic QMP program!
    Check QMP_CFLAGS, QMP_LDFLAGS, QMP_LIBS.])
fi


AC_SUBST(QDP_CFLAGS)
AC_SUBST(QDP_LDFLAGS)
AC_SUBST(QLA_CFLAGS)
AC_SUBST(QLA_LDFLAGS)

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
AC_CHECK_HEADERS([malloc.h])

# Checks for typedefs, structures, and compiler characteristics.
## AC_C_CONST
## AC_TYPE_SIZE_T

# Checks for library functions.
## AC_FUNC_MALLOC


# Turn on parallel-io flag
if test ${ac_parallel_io} -eq 1; then
  AC_DEFINE_UNQUOTED(QIO_USE_PARALLEL_READ, ${ac_parallel_io}, [Enable parallel file-system read])
  AC_DEFINE_UNQUOTED(QIO_USE_PARALLEL_WRITE, ${ac_parallel_io}, [Enable parallel file-system write])
  AC_MSG_NOTICE([Parallel file-system extensions enabled]);
fi


# Configure subdir packages
AC_CONFIG_SUBDIRS(other_libs/c-lime)

# Configure makefiles
AC_CONFIG_FILES([Makefile
		 examples/Makefile
                 include/Makefile
                 lib/Makefile
                 other_libs/Makefile
                 other_libs/c-lime/Makefile])
AC_OUTPUT
