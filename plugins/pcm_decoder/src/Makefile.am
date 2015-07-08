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

lib_LTLIBRARIES = libtizsndfiled.la

noinst_HEADERS = \
	sndfiled.h \
	sndfiledprc.h \
	sndfiledprc_decls.h

libtizsndfiled_la_SOURCES = \
	sndfiled.c \
	sndfiledprc.c

libtizsndfiled_la_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZONIA_CFLAGS@ \
	@LIBSNDFILE_CFLAGS@

libtizsndfiled_la_LIBADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZONIA_LIBS@ \
	@LIBSNDFILE_LIBS@



