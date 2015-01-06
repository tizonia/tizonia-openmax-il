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

bin_PROGRAMS = tizrmd

noinst_HEADERS = \
	tizrmowner.hpp \
	tizrmpreemptor.hpp \
	tizrmwaiter.hpp \
	tizrmd.hpp \
	tizrmdb.hpp

tizrmd_SOURCES = \
	tizrmd.cpp \
	tizrmdb.cpp

tizrmd_CPPFLAGS = \
	-I$(top_srcdir)/../../include/omxil-1.2 \
	-I$(top_srcdir)/../../libtizplatform/src \
	-I$(top_srcdir)/../dbus \
	@DBUS_CFLAGS@ \
	@SQLITE3_CFLAGS@

tizrmd_LDADD = \
	$(top_builddir)/../../libtizplatform/src/libtizplatform.la \
	@DBUS_LIBS@ \
	@SQLITE3_LDFLAGS@


