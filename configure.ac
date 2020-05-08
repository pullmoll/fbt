AC_PREREQ([2.69])
AC_INIT([Frame Buffer Tool],[1.0.0],[pullmoll@t-online.de])
AM_INIT_AUTOMAKE(subdir-objects)
AC_CONFIG_MACRO_DIRS([m4])
LT_INIT
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Required header files
AC_CHECK_HEADERS_ONCE([stdio.h stdlib.h string.h
        stdarg.h inttypes.h stdint.h unistd.h
	sys/types.h sys/stat.h fcntl.h
	sys/ioctl.h sys/mman.h linux/fb.h gd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for libraries.
AC_CHECK_LIB([gd], [gdVersionString])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([atexit memmove memset munmap strchr strrchr])

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
