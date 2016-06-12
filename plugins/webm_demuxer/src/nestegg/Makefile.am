AUTOMAKE_OPTIONS = foreign 1.11 no-dist-gzip dist-xz subdir-objects
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(top_srcdir)/halloc
AM_CFLAGS = -ansi -pedantic -Wall -Wextra -Wno-long-long -Wno-unused-parameter -O2 -g

SUBDIRS = docs

EXTRA_DIST = \
	AUTHORS README LICENSE \
	nestegg-uninstalled.pc.in \
	m4/as-ac-expand.m4 \
	m4/pkg.m4 \
	halloc/src/halloc.c \
	halloc/halloc.h \
	halloc/src/align.h \
	halloc/src/hlist.h \
	halloc/src/macros.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = nestegg.pc

nesteggincludedir = $(includedir)/nestegg
nestegginclude_HEADERS = include/nestegg/nestegg.h

lib_LTLIBRARIES = src/libnestegg.la

src_libnestegg_la_SOURCES = \
	src/nestegg.c \
	halloc/src/halloc.c \
	halloc/halloc.h \
	halloc/src/align.h \
	halloc/src/hlist.h \
	halloc/src/macros.h

src_libnestegg_la_LDFLAGS = -export-symbols-regex '^nestegg_' -no-undefined

check_PROGRAMS = test/test test/regress

test_test_SOURCES = test/test.c
test_test_LDADD = src/libnestegg.la

test_regress_SOURCES = test/regress.c
test_regress_LDADD = src/libnestegg.la

TESTS = test/regress.test

dist-hook:
	find $(distdir) -type d -name '.git' | xargs rm -rf

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
