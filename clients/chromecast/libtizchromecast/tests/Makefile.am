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

TESTS = check_tizchromecast

AUTOMAKE_OPTIONS = serial-tests

check_PROGRAMS = check_tizchromecast

check_tizchromecast_SOURCES = check_tizchromecast.c

check_tizchromecast_CFLAGS = \
	-I$(top_srcdir)/src/ \
	@CHECK_CFLAGS@

check_tizchromecast_LDADD = \
	$(top_builddir)/src/libtizchromecast.la \
	@CHECK_LIBS@

