#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.0])
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

# check for Tokyo Cabinet
#
AC_CHECK_HEADERS([tcutil.h tcadb.h],
  [AC_CHECK_LIB([tokyocabinet], [tcadbnew],
    [AC_SUBST([LIBTC_LIBS], ["-ltokyocabinet -lz -lbz2 -lrt -lm"]) HAVE_LIBTC=yes],
    [HAVE_LIBTC=no])])

# conditionally mark binaries for building

# cgm
#
BUILD_CGM=yes
AC_ARG_WITH([cgm],
  AC_HELP_STRING([--without-cgm], [Do not build cgm.]),
  [BUILD_CGM=no])
AC_SUBST([BUILD_CGM], [${BUILD_CGM}])
AM_CONDITIONAL([BUILD_CGM], [test x$BUILD_CGM = xyes])

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

# index
#
BUILD_INDEX=no
AS_IF([test x$HAVE_PTHREADS = xyes && test x$HAVE_LIBTC = xyes],
  [BUILD_INDEX=yes])
AC_ARG_WITH([index],
  AC_HELP_STRING([--without-index], [Do not build index.]),
  [BUILD_INDEX=no])
AC_SUBST([BUILD_INDEX], [${BUILD_INDEX}])
AC_SUBST([INDEX_LDADD], ["${PTHREAD_LIBS} ${LIBTC_LIBS}"])
AC_SUBST([INDEX_CFLAGS], ["${PTHREAD_CFLAGS} ${LIBTC_CFLAGS}"])
AM_CONDITIONAL([BUILD_INDEX], [test x$BUILD_INDEX = xyes])

# memdb-test
#
BUILD_MEMDB_TEST=yes
AC_ARG_WITH([memdb-test],
  AC_HELP_STRING([--without-memdb-test], [Do not build memdb-test.]),
  [BUILD_MEMDB_TEST=no])
AC_SUBST([BUILD_MEMDB_TEST], [${BUILD_MEMDB_TEST}])
AM_CONDITIONAL([BUILD_MEMDB_TEST], [test x$BUILD_MEMDB_TEST = xyes])

# pindex
#
BUILD_PINDEX=no
AS_IF([test x$HAVE_PTHREADS = xyes && test x$HAVE_LOOMLIB = xyes],
  [BUILD_PINDEX=yes])
AC_ARG_WITH([pindex],
  AC_HELP_STRING([--without-pindex], [Do not build pindex.]),
  [BUILD_PINDEX=no])
AC_SUBST([BUILD_PINDEX], [${BUILD_PINDEX}])
AC_SUBST([PINDEX_LDADD], ["${PTHREAD_LIBS} ${LOOMLIB_LIBS}"])
AC_SUBST([PINDEX_CFLAGS], ["${PTHREAD_CFLAGS} ${LOOMLIB_CFLAGS}"])
AM_CONDITIONAL([BUILD_PINDEX], [test x$BUILD_PINDEX = xyes])

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

# tcadb-test
#
BUILD_TCADB_TEST=no
AS_IF([test x$HAVE_PTHREADS = xyes && test x$HAVE_LIBTC = xyes],
  [BUILD_TCADB_TEST=yes])
AC_ARG_WITH([tcadb-test],
  AC_HELP_STRING([--without-tcadb-test], [Do not build tcadb-test.]),
  [BUILD_TCADB_TEST=no])
AC_SUBST([BUILD_TCADB_TEST], [${BUILD_TCADB_TEST}])
AC_SUBST([TCADB_TEST_LDADD], ["${PTHREAD_LIBS} ${LIBTC_LIBS}"])
AC_SUBST([TCADB_TEST_CFLAGS], ["${PTHREAD_CFLAGS} ${LIBTC_CFLAGS}"])
AM_CONDITIONAL([BUILD_TCADB_TEST], [test x$BUILD_TCADB_TEST = xyes])

# tchdb-test
BUILD_TCHDB_TEST=no
AS_IF([test x$HAVE_PTHREADS = xyes && test x$HAVE_LIBTC = xyes],
  [BUILD_TCHDB_TEST=yes])
AC_ARG_WITH([tchdb-test],
  AC_HELP_STRING([--without-tchdb-test], [Do not build tchdb-test.]),
    [BUILD_TCHDB_TEST=no])
AC_SUBST([BUILD_TCHDB_TEST], [${BUILD_TCHDB_TEST}])
AC_SUBST([TCHDB_TEST_LDADD], ["${PTHREAD_LIBS} ${LIBTC_LIBS}"])
AC_SUBST([TCHDB_TEST_CFLAGS], ["${PTHREAD_CFLAGS} ${LIBTC_CFLAGS}"])
AM_CONDITIONAL([BUILD_TCHDB_TEST], [test x$BUILD_TCHDB_TEST = xyes])

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
echo "cgm             : $BUILD_CGM"
echo "ctb             : $BUILD_CTB"
echo "genome-geometry : $BUILD_GENOME_GEOMETRY"
echo "genome-index    : $BUILD_GENOME_INDEX"
echo "index           : $BUILD_INDEX"
echo "memdb-test      : $BUILD_MEMDB_TEST"
echo "pindex          : $BUILD_PINDEX"
echo "readgen         : $BUILD_READGEN"
echo "samplecl        : $BUILD_SAMPLECL"
echo "tcadb-test      : $BUILD_TCADB_TEST"
echo "tchdb-test      : $BUILD_TCHDB_TEST"
echo
