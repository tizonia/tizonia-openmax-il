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

# Correctly clean any generated headers...
BUILT_SOURCES = mpris_dbus.hpp
CLEANFILES = $(BUILT_SOURCES)

# but keep the xml description
EXTRA_DIST = $(xml_files)

DISTCLEANFILES = \
	Makefile.in

noinst_HEADERS = \
	mpris_dbus.hpp

xml_files = mpris_introspect.xml

mpris_dbus.hpp: $(xml_files)
	@TIZDBUSXML2CPP@ $^ --adaptor=$@
