#AC_PREREQ(2.57)
AC_INIT(qio, 0.1, edwards@jlab.org)
AM_INIT_AUTOMAKE
AC_CONFIG_AUX_DIR(config)

AC_CONFIG_SRCDIR([src/xml/xml.h])
# AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h strings.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

# Configure subdir packages
AC_CONFIG_SUBDIRS(src/lrl/c-dime)

# Configure makefiles
AC_CONFIG_FILES([src/dml/Makefile
                 src/lrl/Makefile
                 src/qio/Makefile
                 src/xml/Makefile])
AC_OUTPUT
