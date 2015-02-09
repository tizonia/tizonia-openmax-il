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

#SUBDIRS = http-parser

AM_CFLAGS = $(WARN_CFLAGS)

lib_LTLIBRARIES = libtizplatform.la

libtizplatform_includedir = $(includedir)/tizonia/platform

libtizplatform_include_HEADERS = \
	utarray/utarray.h \
	avl/avl.h \
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
	tizshuffle.h

libtizplatform_la_SOURCES = \
	http-parser/http_parser.c \
	avl/avl.c \
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
	tizshuffle.c

libtizplatform_la_CFLAGS = \
	-I$(top_srcdir)/../include/omxil-1.2 \
	$(AM_CFLAGS) \
	@LOG4C_CFLAGS@

libtizplatform_la_LIBADD = \
	-lpthread \
	-lev \
	@LOG4C_LIBS@ \
	@uuid_LIBS@


# End of Makefile.am
