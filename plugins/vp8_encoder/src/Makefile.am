# Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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

lib_LTLIBRARIES = libtizvp8e.la

noinst_HEADERS = \
	vp8eprc.h \
	vp8eprc_decls.h

libtizvp8e_la_SOURCES = \
	vp8e.c \
	vp8eprc.c

libtizvp8e_la_CFLAGS = \
	-I$(top_srcdir)/../../include/omxil-1.2 \
	-I$(top_srcdir)/../../libtizosal/src \
	-I$(top_srcdir)/../../libtizonia/src

libtizvp8e_la_LIBADD = \
	$(top_builddir)/../../libtizosal/src/libtizosal.la \
	$(top_builddir)/../../libtizonia/src/libtizonia.la


