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

bin_PROGRAMS = tizcastd

noinst_HEADERS = \
	tizcastd.hpp \
	tizcastworker.hpp \
	tizcastmgr.hpp \
	tizcastmgrtypes.hpp \
	tizcastmgrcmd.hpp \
	tizcastmgrfsm.hpp \
	tizcastmgrops.hpp

tizcastd_SOURCES = \
	tizcastd.cpp \
	tizcastworker.cpp \
	tizcastmgr.cpp \
	tizcastmgrcmd.cpp \
	tizcastmgrfsm.cpp \
	tizcastmgrops.cpp

tizcastd_CPPFLAGS = \
	-I$(top_srcdir)/dbus \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZDBUSCPLUSPLUS_CFLAGS@ \
	@TIZCHROMECAST_CFLAGS@

tizcastd_LDADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZDBUSCPLUSPLUS_LIBS@ \
	@TIZCHROMECAST_LIBS@

