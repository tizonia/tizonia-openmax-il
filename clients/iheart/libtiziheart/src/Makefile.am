# Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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

lib_LTLIBRARIES = libtiziheart.la

libtiziheart_includedir = $(includedir)/tizonia

noinst_HEADERS =

libtiziheart_include_HEADERS = \
	tiziheart.hpp \
	tiziheart_c.h

libtiziheart_la_SOURCES = \
	tiziheart.cpp \
	tiziheart_c.cpp

libtiziheart_la_CPPFLAGS = \
	@PYTHON_CPPFLAGS@ \
	@BOOST_CPPFLAGS@

libtiziheart_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtiziheart_la_LIBADD = \
	@BOOST_PYTHON_LIB@ \
	@PYTHON_LIBS@ \
	-lboost_python3


