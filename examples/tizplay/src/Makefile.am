# Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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

bin_PROGRAMS = tizplay

noinst_HEADERS = \
	tizomxutil.h \
	tizprobe.h \
	tizgraphfactory.h \
	tizgraph.h \
	tizmp3graph.h

tizplay_SOURCES = \
	tizplay.cc \
	tizomxutil.cc \
	tizprobe.cc \
	tizgraphfactory.cc \
	tizgraph.cc \
	tizmp3graph.cc

tizplay_CPPFLAGS = \
	-I$(top_srcdir)/../../include/omxil-1.2 \
	-I$(top_srcdir)/../../libtizosal/src \
	-D__STDC_CONSTANT_MACROS

tizplay_LDADD = \
	-lboost_thread \
	-lboost_system \
	-lboost_filesystem \
	-lavformat \
	-lavcodec \
	$(top_builddir)/../../libtizosal/src/libtizosal.la \
	$(top_builddir)/../../libtizcore/src/libtizcore.la
