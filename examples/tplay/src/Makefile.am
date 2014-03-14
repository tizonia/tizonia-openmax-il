# Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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

bin_PROGRAMS = tplay

noinst_HEADERS = \
	tizomxutil.h \
	tizgraphutil.h \
	tizgraphcback.h \
	tizprobe.h \
	tizplaylist.h \
	tizgraphfactory.h \
	tizgraphtypes.h \
	tizgraphconfig.h \
	tizgraphmgrcmd.h \
	tizgraphmgrops.h \
	tizgraphmgrfsm.h \
	tizgraphmgr.h \
	tizgraphmgrfsm.h \
	tizdecgraphmgr.h \
	tizhttpservmgr.h \
	tizgraphfsm.h \
	tizgraphevt.h \
	tizgraphguard.h \
	tizgraphaction.h \
	tizgraphstate.h \
	tizgraphcmd.h \
	tizgraphops.h \
	tizgraph.h \
	tizmp3graph.h \
	tizopusgraph.h \
	tizvorbisgraph.h
	tizflacgraph.h \
	tizoggflacgraph.h \
	tizhttpservconfig.h \
	tizhttpservgraph.h \
	tizhttpservgraphfsm.h \
	tizhttpservgraphops.h

tplay_SOURCES = \
	tizplay.cc \
	tizomxutil.cc \
	tizgraphutil.cc \
	tizgraphcback.cc \
	tizprobe.cc \
	tizplaylist.cc \
	tizgraphfactory.cc \
	tizgraphmgrcmd.cc \
	tizgraphmgrops.cc \
	tizgraphmgr.cc \
	tizdecgraphmgr.cc \
	tizhttpservmgr.cc \
	tizgraphops.cc \
	tizgraphcmd.cc \
	tizgraph.cc \
	tizmp3graph.cc \
	tizopusgraph.cc \
	tizvorbisgraph.cc \
	tizflacgraph.cc \
	tizoggflacgraph.cc \
	tizhttpservgraph.cc \
	tizhttpservgraphops.cc

tplay_CPPFLAGS = \
	-I$(top_srcdir)/../../include/omxil-1.2 \
	-I$(top_srcdir)/../../libtizosal/src \
	-I/usr/include/taglib
	-D__STDC_CONSTANT_MACROS

tplay_LDADD = \
	-lboost_thread \
	-lboost_system \
	-lboost_filesystem \
	-lavformat \
	-lavcodec \
	-lavutil \
	-ltag \
	$(top_builddir)/../../libtizosal/src/libtizosal.la \
	$(top_builddir)/../../libtizcore/src/libtizcore.la
