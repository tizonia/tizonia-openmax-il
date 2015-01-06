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

TESTS = check_tizplatform

check_PROGRAMS = check_tizplatform

noinst_HEADERS = \
	check_mem.c \
	check_mutex.c \
	check_pqueue.c \
	check_queue.c \
	check_sem.c \
	check_vector.c \
	check_rc.c \
	check_soa.c \
	check_event.c \
	check_http_parser.c \
	check_map.c

check_tizplatform_SOURCES = check_tizplatform.c

check_tizplatform_CFLAGS = \
	-I$(top_srcdir)/src/ \
	-I$(top_srcdir)/../include/omxil-1.2 \
	@CHECK_CFLAGS@

check_tizplatform_LDADD = \
	$(top_builddir)/src/libtizplatform.la \
	@CHECK_LIBS@

# End of Makefile.am
