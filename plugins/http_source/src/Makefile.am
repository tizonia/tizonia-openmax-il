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

lib_LTLIBRARIES = libtizhttpsrc.la

noinst_HEADERS = \
	httpsrc.h \
	httpsrcport.h \
	httpsrcport_decls.h \
	httpsrcprc.h \
	httpsrcprc_decls.h \
	gmusicprc.h \
	gmusicprc_decls.h \
	gmusiccfgport.h \
	gmusiccfgport_decls.h \
	httpsrctrans.h

libtizhttpsrc_la_SOURCES = \
	httpsrc.c \
	httpsrcport.c \
	httpsrcprc.c \
	gmusicprc.c \
	gmusiccfgport.c \
	httpsrctrans.c

libtizhttpsrc_la_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZONIA_CFLAGS@ \
	@TIZGMUSIC_CFLAGS@ \
	@LIBCURL_CFLAGS@

libtizhttpsrc_la_LIBADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZONIA_LIBS@ \
	@TIZGMUSIC_LIBS@ \
	@LIBCURL_LIBS@
