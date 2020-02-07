# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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

libtizmpg123ddir = $(plugindir)

libtizmpg123d_LTLIBRARIES = libtizmpg123d.la

noinst_HEADERS = \
	mpg123d.h \
	mpg123dprc.h \
	mpg123dprc_decls.h

libtizmpg123d_la_SOURCES = \
	mpg123d.c \
	mpg123dprc.c

libtizmpg123d_la_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZONIA_CFLAGS@ \
	@LIBMPG123_CFLAGS@

libtizmpg123d_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizmpg123d_la_LIBADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZONIA_LIBS@ \
	@LIBMPG123_LIBS@


