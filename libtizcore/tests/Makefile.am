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

TESTS = check_tizcore

BUILT_SOURCES = check_tizcore.h

EXTRA_DIST = \
	tizonia.conf \
	tizonia.conf.in \
	check_tizcore.h.in \
	check_tizcore.h

CLEANFILES = check_tizcore.h tizonia.conf

AUTOMAKE_OPTIONS = serial-tests

check_PROGRAMS = check_tizcore

check_tizcore_SOURCES = check_tizcore.c

check_tizcore_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	-I$(top_srcdir)/src \
	@CHECK_CFLAGS@

check_tizcore_LDADD = \
	$(top_builddir)/src/libtizcore.la \
	@TIZPLATFORM_LIBS@ \
	@CHECK_LIBS@

do_subst = sed -e 's,[@]abs_top_builddir[@],$(abs_top_builddir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]libdir[@],$(libdir),g' \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

check_tizcore.h: check_tizcore.h.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

tizonia.conf: tizonia.conf.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

all-local: tizonia.conf

clean-local: clean-local-check-tizcore
distclean-local: clean-local-check-tizcore
.PHONY: clean-local-check-tizcore
clean-local-check-tizcore:
	-rm -f core tizrm.db
