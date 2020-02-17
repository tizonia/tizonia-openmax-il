# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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

libtizwebmdemuxdir = $(plugindir)

libtizwebmdemux_LTLIBRARIES = libtizwebmdemux.la

noinst_HEADERS = \
	webmdmux.h \
	webmdmuxsrcprc.h \
	webmdmuxsrcprc_decls.h \
	webmdmuxfltprc.h \
	webmdmuxfltprc_decls.h

libtizwebmdemux_la_SOURCES = \
	nestegg/include/nestegg/nestegg.h \
	nestegg/src/nestegg.c \
	webmdmux.c \
	webmdmuxsrcprc.c \
	webmdmuxfltprc.c

libtizwebmdemux_la_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZONIA_CFLAGS@ \
	-I$(top_srcdir)/src/nestegg/include \
	-I$(top_srcdir)/src/nestegg/include/nestegg

libtizwebmdemux_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizwebmdemux_la_LIBADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZONIA_LIBS@
