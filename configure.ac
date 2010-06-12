# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([DNArm], [dev], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/ctb.c])
AC_CONFIG_HEADERS([config.h])
PKG_PROG_PKG_CONFIG([0.21])

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

# check for OpenCL
#
AC_CHECK_HEADER([CL/cl.h],
  [AC_CHECK_LIB([OpenCL], [clGetDeviceIDs],
    [AC_SUBST([LIBOPENCL_LIBS], ["-lOpenCL"]) HAVE_LIBOPENCL=yes],
    [HAVE_LIBOPENCL=no])])

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

# genome-geometry
BUILD_GENOME_GEOMETRY=yes
AC_ARG_WITH([genome-geometry],
  AC_HELP_STRING([--without-genome-geometry], [Do not build genome-geometry.]),
  [BUILD_GENOME_GEOMETRY=no])
AC_SUBST([BUILD_GENOME_GEOMETRY], [${BUILD_GENOME_GEOMETRY}])
AC_SUBST([GENOME_GEOMETRY_LDADD], ["${PTHREAD_LIBS} ${LOOMLIB_LIBS}"])
AC_SUBST([GENOME_GEOMETRY_CFLAGS], ["${PTHREAD_CFLAGS} ${LOOMLIB_CFLAGS}"])
AM_CONDITIONAL([BUILD_GENOME_GEOMETRY], [test x$BUILD_GENOME_GEOMETRY = xyes])

# genome-index
BUILD_GENOME_INDEX=no
AS_IF([test x$HAVE_PTHREADS = xyes ],
  [BUILD_GENOME_INDEX=yes])
AC_ARG_WITH([genome-index],
  AC_HELP_STRING([--without-genome-index], [Do not build genome-index.]),
  [BUILD_GENOME_INDEX=no])
AC_SUBST([BUILD_GENOME_INDEX], [${BUILD_GENOME_INDEX}])
AC_SUBST([GENOME_INDEX_LDADD], ["${PTHREAD_LIBS}"])
AC_SUBST([GENOME_INDEX_CFLAGS], ["${PTHREAD_CFLAGS}"])
AM_CONDITIONAL([BUILD_GENOME_INDEX], [test x$BUILD_GENOME_INDEX = xyes])

# parallel_db
BUILD_PARALLEL_DB=no
AS_IF([test x$HAVE_PTHREADS = xyes ],
  [BUILD_PARALLEL_DB=yes])
AC_ARG_WITH([parallel-db],
  AC_HELP_STRING([--without-parallel-db], [Do not build parallel-db.]),
  [BUILD_PARALLEL_DB=no])
AC_SUBST([BUILD_PARALLEL_DB], [${BUILD_PARALLEL_DB}])
AC_SUBST([PARALLEL_DB_LDADD], ["${PTHREAD_LIBS}"])
AC_SUBST([PARALLEL_DB_CFLAGS], ["${PTHREAD_CFLAGS}"])
AM_CONDITIONAL([BUILD_PARALLEL_DB], [test x$BUILD_PARALLEL_DB = xyes])

# readgen
#
BUILD_READGEN=yes
AC_ARG_WITH([readgen],
  AC_HELP_STRING([--without-readgen], [Do not build readgen.]),
  [BUILD_READGEN=no])
AC_SUBST([BUILD_READGEN], [${BUILD_READGEN}])
AM_CONDITIONAL([BUILD_READGEN], [test x$BUILD_READGEN = xyes])

# samplecl
BUILD_SAMPLECL=no
AS_IF([test x$HAVE_LIBOPENCL = xyes], [BUILD_SAMPLECL=yes])
AC_ARG_WITH([samplecl],
  AC_HELP_STRING([--without-samplecl], [Do not build samplecl.]),
  [BUILD_SAMPLECL=no])
AC_SUBST([BUILD_SAMPLECL], [${BUILD_SAMPLECL}])
AC_SUBST([SAMPLECL_LDADD], ["${LIBOPENCL_LIBS}"])
AM_CONDITIONAL([BUILD_SAMPLECL], [test x$BUILD_SAMPLECL = xyes])

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
echo " DNArm build summary "
echo "====================="
echo "ctb             : $BUILD_CTB"
echo "genome-geometry : $BUILD_GENOME_GEOMETRY"
echo "genome-index    : $BUILD_GENOME_INDEX"
echo "parallel-db     : $BUILD_PARALLEL_DB"
echo "readgen         : $BUILD_READGEN"
echo "samplecl        : $BUILD_SAMPLECL"
echo
