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

lib_LTLIBRARIES = libtizhttpr.la

noinst_HEADERS = \
	httprcfgport.h \
	httprcfgport_decls.h \
	httprmp3port.h \
	httprmp3port_decls.h \
	httprsrv.h \
	httpr.h \
	httprprc.h \
	httprprc_decls.h

libtizhttpr_la_SOURCES = \
	httpr.c \
	httprcfgport.c \
	httprmp3port.c \
	httprsrv.c \
	httprprc.c

libtizhttpr_la_CFLAGS = \
	-I$(top_srcdir)/../../include/omxil-1.2 \
	-I$(top_srcdir)/../../libtizplatform/src \
	-I$(top_srcdir)/../../libtizonia/src

libtizhttpr_la_LIBADD = \
	$(top_builddir)/../../libtizplatform/src/libtizplatform.la \
	$(top_builddir)/../../libtizonia/src/libtizonia.la


