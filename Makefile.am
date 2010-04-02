## Process this file with automake to produce Makefile.in
lib_LTLIBRARIES = libgtkentrymask.la
libgtkentrymask_la_SOURCES = gtkentrymask.c gtkentrymask.h

INCLUDES = @GTK_CFLAGS@
LDADD = @GTK_CFLAGS@
CLEANFILES = *~

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgtkentrymask-2.0.pc

bin_PROGRAMS = test-gtkentrymask
noinst_PROGRAMS = test-gtkentrymask
test_gtkentrymask_SOURCES = test-gtkentrymask.c
test_gtkentrymask_INCLUDES = @GTK_CFLAGS@
test_gtkentrymask_LDADD = libgtkentrymask.la @GTK_LIBS@

uninstall-local:
	rm -f $(DESTDIR)$(pkgconfigdir)/libgtkentrymask-2.0.pc
