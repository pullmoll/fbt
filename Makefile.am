ACLOCAL_AMFLAGS = -Im4

AM_CFLAGS = -O0 -Wall -I./libsfb
AM_LDFLAGS = -L/usr/lib

lib_LTLIBRARIES = libsfb.la
libsfb_la_SOURCES = libsfb/sfb.c libsfb/font_6x12.c libsfb/font_8x13.c

bin_PROGRAMS = fbt
fbt_SOURCES = fbt.c
fbt_LDADD = -lgd libsfb.la
