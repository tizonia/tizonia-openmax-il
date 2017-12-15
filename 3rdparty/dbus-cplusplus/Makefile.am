SUBDIRS = src tools data

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = debian

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tizdbus-c++.pc

BUILT_SOURCES = tizonia

DISTCLEANFILES = tizonia

noinst_HEADERS = tizonia

tizonia:
	ln -s $(srcdir)/src tizonia
