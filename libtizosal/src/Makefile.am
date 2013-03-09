# Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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

EXTRA_DIST = log4crc tizrc

lib_LTLIBRARIES = libtizosal.la

libtizosal_includedir = $(includedir)/libtizosal

libtizosal_include_HEADERS = \
	utarray/utarray.h \
	avl/avl.h \
	http-parser/http_parser.h \
	tizosal.h \
	tizosallog.h \
	tizosalutils.h \
	tizosalmem.h \
	tizosalpqueue.h \
	tizosalqueue.h \
	tizosalsync.h \
	tizosalvector.h \
	tizosalthread.h \
	tizosaluuid.h \
	tizosalrc.h \
	tizosalsoa.h \
	tizosalev.h \
	tizosalmap.h \
	tizosalhttp.h

libtizosal_la_SOURCES = \
	http-parser/http_parser.c \
	avl/avl.c \
	tizosal.c \
	tizosallog.c \
	tizosalutils.c \
	tizosalmem.c \
	tizosalsync.c \
	tizosalqueue.c \
	tizosalpqueue.c \
	tizosalvector.c \
	tizosalthread.c \
	tizosaluuid.c \
	tizosalrc.c \
	tizosalsoa.c \
	tizosalev.c \
	tizosalmap.c \
	tizosalhttp.c

libtizosal_la_CFLAGS = \
	-I$(top_srcdir)/../include/omxil-1.2 \
	@LOG4C_CFLAGS@ \
	@uuid_CFLAGS@

libtizosal_la_LIBADD = \
	-lpthread \
	-lev \
	@LOG4C_LIBS@ \
	@uuid_LIBS@


# End of Makefile.am
