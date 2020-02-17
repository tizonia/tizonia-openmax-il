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

lib_LTLIBRARIES = libtizsoundcloud.la

libtizsoundcloud_includedir = $(includedir)/tizonia

noinst_HEADERS =

libtizsoundcloud_include_HEADERS = \
	tizsoundcloud.hpp \
	tizsoundcloud_c.h

libtizsoundcloud_la_SOURCES = \
	tizsoundcloud.cpp \
	tizsoundcloud_c.cpp

libtizsoundcloud_la_CPPFLAGS = \
	@PYTHON_CPPFLAGS@ \
	@BOOST_CPPFLAGS@

libtizsoundcloud_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizsoundcloud_la_LIBADD = \
	@BOOST_PYTHON_LIB@ \
	@PYTHON_LIBS@ \
	-lboost_python3
