# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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

lib_LTLIBRARIES = libtizcastclient.la

libtizcastclient_includedir = $(includedir)/tizonia

noinst_HEADERS =

libtizcastclient_include_HEADERS = \
	tizcastclienttypes.h \
	tizcastclient_c.h \
	tizcastclient.hh

libtizcastclient_la_SOURCES = \
	tizcastclient.cc \
	tizcastclient_c.cc

libtizcastclient_la_CPPFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZCASTD_CFLAGS@ \
	@TIZDBUSCPLUSPLUS_CFLAGS@

libtizcastclient_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizcastclient_la_LIBADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZDBUSCPLUSPLUS_LIBS@
