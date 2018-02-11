# Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
	-I$(top_srcdir)/dbus \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZDBUSCPLUSPLUS_CFLAGS@ \
	@SQLITE3_CFLAGS@

tizrmd_LDADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZDBUSCPLUSPLUS_LIBS@ \
	@SQLITE3_LDFLAGS@


