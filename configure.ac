#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([DNArm], [dev], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/ctb.c])
AC_CONFIG_HEADERS([config.h])
PKG_PROG_PKG_CONFIG([0.22])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
ACX_PTHREAD([AC_SUBST([CC], ["${PTHREAD_CC}"])],
    [AC_MSG_ERROR(['pthreads' not found])])

PKG_CHECK_MODULES([LOOMLIB], [loomlib])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strerror strtoul])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
