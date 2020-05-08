ACLOCAL_AMFLAGS = -Im4

AM_CFLAGS = -Wall -I./libfb
AM_LDFLAGS = -L/usr/lib

lib_LTLIBRARIES = libsfb.la
libsfb_la_SOURCES = libfb/sfb.c

bin_PROGRAMS = fbt
fbt_SOURCES = fbt.c font.c
fbt_LDADD = -lgd libsfb.la
