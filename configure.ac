#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Frame Buffer Tool], [1.0.0], [Jürgen Buchmüller <pullmoll@t-online.de>])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([gd], [gdVersionString])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h locale.h stdio.h stdint.h stdlib.h string.h \
 sys/ioctl.h sys/mman.h sys/types.h sys/ioctl.h time.h unistd.h \
 linux/fb.h gd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MMAP
AC_CHECK_FUNCS([memmove memset munmap setlocale strchr strrchr])

AC_CONFIG_FILES([Makefile
                 libsfb/Makefile
                 src/Makefile])
AC_OUTPUT
