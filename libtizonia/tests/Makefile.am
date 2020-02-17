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

TESTS = check_tizonia

BUILT_SOURCES = check_tizonia.h

EXTRA_DIST = \
	tizonia.conf \
	tizonia.conf.in \
	check_tizonia.h.in \
	check_tizonia.h

CLEANFILES = check_tizonia.h tizonia.conf

check_PROGRAMS = check_tizonia

check_tizonia_SOURCES = check_tizonia.c

check_tizonia_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	-I$(top_srcdir)/src/ \
	@CHECK_CFLAGS@

check_tizonia_LDADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZCORE_LIBS@ \
	$(top_builddir)/src/libtizonia.la \
	@CHECK_LIBS@

do_subst = sed -e 's,[@]abs_top_builddir[@],$(abs_top_builddir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]libdir[@],$(libdir),g' \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

check_tizonia.h: check_tizonia.h.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

tizonia.conf: tizonia.conf.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

all-local: tizonia.conf

clean-local: clean-local-check-tizonia
distclean-local: clean-local-check-tizonia
.PHONY: clean-local-check-tizonia
clean-local-check-tizonia:
	-rm -f core tizrm.db
