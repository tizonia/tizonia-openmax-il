# Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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

lib_LTLIBRARIES = libtizdeezer.la

libtizdeezer_includedir = $(includedir)/tizonia

noinst_HEADERS =

libtizdeezer_include_HEADERS = \
	tizdeezer.hpp \
	tizdeezer_c.h

libtizdeezer_la_SOURCES = \
	tizdeezer.cpp \
	tizdeezer_c.cpp

libtizdeezer_la_CPPFLAGS = \
	@PYTHON_CPPFLAGS@ \
	@BOOST_CPPFLAGS@

libtizdeezer_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizdeezer_la_LIBADD = \
	@BOOST_PYTHON_LIB@ \
	@PYTHON_LDFLAGS@ \
	-lboost_python


