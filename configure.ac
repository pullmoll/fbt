
AC_INIT([Frame Buffer Tool], 1.0, [pullmoll@t-online.de])
AM_INIT_AUTOMAKE
AC_PROG_CC

dnl Required header files
AC_CHECK_HEADERS_ONCE([stdio.h stdlib.h string.h
	xyz.h stdarg.h inttypes.h stdint.h unistd.h
	sys/types.h sys/stat.h fcntl.h
	sys/ioctl.h sys/mman.h linux/fb.h gd.h])

dnl Check for libgd availablity
AC_CHECK_LIB([gd], [gdVersionString])

AC_CONFIG_FILES(Makefile)
AC_CONFIG_HEADERS(config.h)
AC_OUTPUT
