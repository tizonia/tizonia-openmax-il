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

TESTS = check_tizrmproxy

EXTRA_DIST = db_acquire_and_destroy_no_release.sql3 \
	db_acquire_and_release.sql3 \
	db_busy_resource_management.after.sql3 \
	db_busy_resource_management.before.sql3 \
	db_resource_preemption.after.sql3 \
	db_resource_preemption.before.sql3 \
	db_wait_cancel_wait.after.sql3 \
	db_wait_cancel_wait.before.sql3 \
	updatedb.sh

check_PROGRAMS = check_tizrmproxy

check_tizrmproxy_SOURCES = check_tizrmproxy.c

check_tizrmproxy_CFLAGS = \
	-I$(top_srcdir)/../../include/omxil-1.2 \
	-I$(top_srcdir)/../../libtizplatform/src \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	-I$(top_srcdir)/../dbus \
	-I$(top_srcdir)/src
	@CHECK_CFLAGS@

check_tizrmproxy_LDADD = \
	$(top_builddir)/../../libtizplatform/src/libtizplatform.la \
	@TIZPLATFORM_LIBS@ \
	$(top_builddir)/src/libtizrmproxy.la \
	@CHECK_LIBS@

