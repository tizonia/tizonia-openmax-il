# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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

# Correctly clean the generated headers...
BUILT_SOURCES = tizrmproxy-dbus.hh tizrmd-dbus.hh com.aratelia.tiz.rm.service
CLEANFILES = $(BUILT_SOURCES)

#but keep the xml description and service.in
EXTRA_DIST = $(service_in_files) $(xml_files)

DISTCLEANFILES = \
	$(service_DATA) \
	Makefile.in

tizrmd_dbus_includedir = $(includedir)/tizonia

tizrmd_dbus_include_HEADERS = \
	tizrmtypes.h \
	tizrmd-dbus.hh \
	tizrmproxy-dbus.hh

# Dbus service file
servicedir = $(DBUS_SERVICES_DIR)
service_in_files = com.aratelia.tiz.rm.service.in
service_DATA = $(service_in_files:.service.in=.service)

xml_files = tizrm-introspect.xml

# Rule to make the service file with bindir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@

tizrmd-dbus.hh: $(xml_files)
	@TIZDBUSXML2CPP@ $^ --adaptor=$@

tizrmproxy-dbus.hh: $(xml_files)
	@TIZDBUSXML2CPP@ $^ --proxy=$@
