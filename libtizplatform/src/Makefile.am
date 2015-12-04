# Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
#
# This file is part of Tizonia
#
# Tizonia is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
# more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.

AM_CFLAGS = $(WARN_CFLAGS)

BUILT_SOURCES = tizplatform_config.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = tizplatform_config.h.in $(BUILT_SOURCES)

lib_LTLIBRARIES = libtizplatform.la

libtizplatform_includedir = $(includedir)/tizonia

libtizplatform_include_HEADERS = \
	utarray/utarray.h \
	avl/avl.h \
	ev/ev.h \
	ev/ev_vars.h \
	ev/ev_wrap.h \
	ev/ev_epoll.c \
	http-parser/http_parser.h \
	tizplatform.h \
	tizmacros.h \
	tizplatform_internal.h \
	tizlog.h \
	tizomxutils.h \
	tizmem.h \
	tizpqueue.h \
	tizqueue.h \
	tizsync.h \
	tizbuffer.h \
	tizvector.h \
	tizthread.h \
	tizuuid.h \
	tizrc.h \
	tizsoa.h \
	tizev.h \
	tizmap.h \
	tizhttp.h \
	tizlimits.h \
	tizprintf.h \
	tizshufflelst.h

libtizplatform_la_SOURCES = \
	http-parser/http_parser.c \
	avl/avl.c \
	ev/ev.c \
	tizplatform.c \
	tizlog.c \
	tizomxutils.c \
	tizmem.c \
	tizsync.c \
	tizqueue.c \
	tizpqueue.c \
	tizbuffer.c \
	tizvector.c \
	tizthread.c \
	tizuuid.c \
	tizrc.c \
	tizsoa.c \
	tizev.c \
	tizmap.c \
	tizhttp.c \
	tizlimits.c \
	tizprintf.c \
	tizshufflelst.c

libtizplatform_la_CFLAGS = \
	$(AM_CFLAGS) \
	@TIZILHEADERS_CFLAGS@ \
	@LOG4C_CFLAGS@

libtizplatform_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizplatform_la_LIBADD = \
	-lpthread \
	@LOG4C_LIBS@ \
	@UUID_LIBS@

do_subst = sed -e 's,[@]abs_top_builddir[@],$(abs_top_builddir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]libdir[@],$(libdir),g' \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

tizplatform_config.h: tizplatform_config.h.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@
