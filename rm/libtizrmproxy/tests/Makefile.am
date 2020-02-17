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

TESTS = check_tizrmproxy

BUILT_SOURCES = check_tizrmproxy.h

EXTRA_DIST = \
	tizonia.conf \
	tizonia.conf.in \
	gendb.sh \
	gendb.sh.in \
	updatedb.sh \
	updatedb.sh.in \
	check_tizrmproxy.h.in \
	check_tizrmproxy.h \
	db_acquire_and_destroy_no_release.sql3 \
	db_acquire_and_release.sql3 \
	db_busy_resource_management.after.sql3 \
	db_busy_resource_management.before.sql3 \
	db_resource_preemption.after.sql3 \
	db_resource_preemption.before.sql3 \
	db_wait_cancel_wait.after.sql3 \
	db_wait_cancel_wait.before.sql3

CLEANFILES = check_tizrmproxy.h tizonia.conf gendb.sh updatedb.sh

check_PROGRAMS = check_tizrmproxy

check_tizrmproxy_SOURCES = check_tizrmproxy.c

check_tizrmproxy_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZRMD_CFLAGS@ \
	-I$(top_srcdir)/src \
	@CHECK_CFLAGS@

check_tizrmproxy_LDADD = \
	@TIZPLATFORM_LIBS@ \
	$(top_builddir)/src/libtizrmproxy.la \
	@CHECK_LIBS@

do_subst = sed -e 's,[@]abs_top_builddir[@],$(abs_top_builddir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]libdir[@],$(libdir),g' \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

check_tizrmproxy.h: check_tizrmproxy.h.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

tizonia.conf: tizonia.conf.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

gendb.sh: gendb.sh.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@
	chmod +x $@

updatedb.sh: updatedb.sh.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@
	chmod +x $@

all-local: tizonia.conf gendb.sh updatedb.sh

clean-local: clean-local-check-tizrmproxy
distclean-local: clean-local-check-tizrmproxy
.PHONY: clean-local-check-tizrmproxy
clean-local-check-tizrmproxy:
	-rm -f core tizrm.db
