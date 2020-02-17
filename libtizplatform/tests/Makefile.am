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

TESTS = check_tizplatform

AUTOMAKE_OPTIONS = serial-tests

BUILT_SOURCES = check_tizplatform.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = tizonia.conf check_tizplatform.h.in $(BUILT_SOURCES)

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
	-I$(top_srcdir)/src \
	@TIZILHEADERS_CFLAGS@ \
	@CHECK_CFLAGS@

check_tizplatform_LDADD = \
	$(top_builddir)/src/libtizplatform.la \
	@CHECK_LIBS@

do_subst = sed -e 's,[@]abs_top_builddir[@],$(abs_top_builddir),g'

check_tizplatform.h: check_tizplatform.h.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

clean-local: clean-local-check-tizplatform
distclean-local: clean-local-check-tizplatform
.PHONY: clean-local-check-tizplatform
clean-local-check-tizplatform:
	-rm -f core
