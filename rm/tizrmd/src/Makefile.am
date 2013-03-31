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

bin_PROGRAMS = tizrmd

noinst_HEADERS = \
	tizrmowner.h \
	tizrmpreemptor.h \
	tizrmwaiter.h \
        tizrmd.h \
	tizrmdb.h

tizrmd_SOURCES = \
	tizrmd.cc \
	tizrmdb.cc

tizrmd_CPPFLAGS = \
	-I$(top_srcdir)/../../include/omxil-1.2 \
	-I$(top_srcdir)/../../libtizosal/src \
	-I$(top_srcdir)/../dbus \
	@DBUS_CFLAGS@ \
	@SQLITE3_CFLAGS@

tizrmd_LDADD = \
	$(top_builddir)/../../libtizosal/src/libtizosal.la \
	@DBUS_LIBS@ \
	@SQLITE3_LDFLAGS@


