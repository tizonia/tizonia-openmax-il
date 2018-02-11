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

TESTS = check_tizcastclient

BUILT_SOURCES = check_tizcastclient.h

EXTRA_DIST = \
	check_tizcastclient.h.in \
	check_tizcastclient.h

CLEANFILES = check_tizcastclient.h

check_PROGRAMS = check_tizcastclient

check_tizcastclient_SOURCES = check_tizcastclient.c

check_tizcastclient_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZCASTD_CFLAGS@ \
	-I$(top_srcdir)/src \
	@CHECK_CFLAGS@

check_tizcastclient_LDADD = \
	@TIZPLATFORM_LIBS@ \
	$(top_builddir)/src/libtizcastclient.la \
	@CHECK_LIBS@

do_subst = sed -e 's,[@]abs_top_builddir[@],$(abs_top_builddir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]libdir[@],$(libdir),g' \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

check_tizcastclient.h: check_tizcastclient.h.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

clean-local: clean-local-check-tizcastclient
distclean-local: clean-local-check-tizcastclient
.PHONY: clean-local-check-tizcastclient
clean-local-check-tizcastclient:
	-rm -f core
