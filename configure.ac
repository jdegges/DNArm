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

# check for pthreads
#
ACX_PTHREAD([AC_SUBST([CC], ["${PTHREAD_CC}"]) HAVE_PTHREADS=yes],
  [HAVE_PTHREADS=no])

# check for loomlib
#
PKG_CHECK_MODULES([LOOMLIB], [loomlib], [HAVE_LOOMLIB=yes], [LOOMLIB_LIBS=no])


# conditionally mark binaries for building

# ctb
#
BUILD_CTB=no
AS_IF([test x$HAVE_PTHREADS = xyes && test x$HAVE_LOOMLIB = xyes],
  [BUILD_CTB=yes])
AC_ARG_WITH([ctb],
  AC_HELP_STRING([--without-ctb], [Do not build ctb.]),
  [BUILD_CTB=no])
AC_SUBST([BUILD_CTB], [${BUILD_CTB}])
AC_SUBST([CTB_LDADD], ["${PTHREAD_LIBS} ${LOOMLIB_LIBS}"])
AC_SUBST([CTB_CFLAGS], ["${PTHREAD_CFLAGS} ${LOOMLIB_CFLAGS}"])
AM_CONDITIONAL([BUILD_CTB], [test x$BUILD_CTB = xyes])

# index
#
BUILD_INDEX=yes
AC_ARG_WITH([ctb],
  AC_HELP_STRING([--without-ctb], [Do not build ctb.]),
  [BUILD_INDEX=no])
AC_SUBST([BUILD_INDEX], [${BUILD_INDEX}])
AM_CONDITIONAL([BUILD_INDEX], [test x$BUILD_INDEX = xyes])

# memdb-test
#
BUILD_MEMDB_TEST=yes
AC_ARG_WITH([memdb-test],
  AC_HELP_STRING([--without-memdb-test], [Do not build memdb-test.]),
  [BUILD_MEMDB_TEST=no])
AC_SUBST([BUILD_MEMDB_TEST], [${BUILD_MEMDB_TEST}])
AM_CONDITIONAL([BUILD_MEMDB_TEST], [test x$BUILD_MEMDB_TEST = xyes])

# readgen
#
BUILD_READGEN=yes
AC_ARG_WITH([readgen],
  AC_HELP_STRING([--without-readgen], [Do not build readgen.]),
  [BUILD_READGEN=no])
AC_SUBST([BUILD_READGEN], [${BUILD_READGEN}])
AM_CONDITIONAL([BUILD_READGEN], [test x$BUILD_READGEN = xyes])

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

echo
echo "DNArm build summary"
echo "==================="
echo "ctb           : $BUILD_CTB"
echo "index         : $BUILD_INDEX"
echo "memdb-test    : $BUILD_MEMDB_TEST"
echo "readgen       : $BUILD_READGEN"
echo
