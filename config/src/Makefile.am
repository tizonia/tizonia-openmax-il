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

tizonia_config = tizonia.conf
log4c_config = log4crc

tizoniaconfdir = $(sysconfdir)/xdg/tizonia
tizoniaconf_DATA = $(tizonia_config) $(log4c_config)

EXTRA_DIST = $(tizonia_config) $(sqlite_databases) tizonia.conf.in log4crc.in

do_subst = sed -e 's,[@]localstatedir[@],$(localstatedir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]libdir[@],$(libdir),g' \
	-e 's,[@]plugindir[@],$(plugindir),g' \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

tizonia.conf: tizonia.conf.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

log4crc: log4crc.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

CLEANFILES = $(tizonia_config) $(log4c_config)
