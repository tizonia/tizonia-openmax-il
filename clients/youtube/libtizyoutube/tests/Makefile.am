# Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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

TESTS = check_tizyoutube

check_PROGRAMS = check_tizyoutube

check_tizyoutube_SOURCES = check_tizyoutube.c

check_tizyoutube_CFLAGS = \
	-I$(top_srcdir)/src/ \
	@CHECK_CFLAGS@

check_tizyoutube_LDADD = \
	$(top_builddir)/src/libtizyoutube.la \
	@CHECK_LIBS@

