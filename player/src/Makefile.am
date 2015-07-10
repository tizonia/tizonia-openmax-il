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

bin_PROGRAMS = tizonia

noinst_HEADERS = \
	tizomxutil.hpp \
	tizplayapp.hpp \
	tizprogramopts.hpp \
	tizgraphutil.hpp \
	tizgraphutil.inl \
	tizgraphcback.hpp \
	tizdaemon.hpp \
	tizprobe.hpp \
	tizplaylist.hpp \
	tizgraphfactory.hpp \
	tizgraphtypes.hpp \
	tizgraphconfig.hpp \
	tizgraphmgrcmd.hpp \
	tizgraphmgrops.hpp \
	tizgraphmgrfsm.hpp \
	tizgraphmgr.hpp \
	tizdecgraphmgr.hpp \
	tizgraphfsm.hpp \
	tizgraphevt.hpp \
	tizgraphguard.hpp \
	tizgraphaction.hpp \
	tizgraphstate.hpp \
	tizgraphcmd.hpp \
	tizgraphops.hpp \
	tizgraph.hpp \
	tizmp3graph.hpp \
	tizaacgraph.hpp \
	tizopusgraph.hpp \
	tizoggopusgraph.hpp \
	tizvorbisgraph.hpp
	tizflacgraph.hpp \
	tizoggflacgraph.hpp \
	tizpcmgraph.hpp \
	tizmpeggraph.hpp \
	httpserv/tizhttpservconfig.hpp \
	httpserv/tizhttpservgraph.hpp \
	httpserv/tizhttpservgraphfsm.hpp \
	httpserv/tizhttpservgraphops.hpp \
	httpserv/tizhttpservmgr.hpp \
	httpclnt/tizhttpclntmgr.hpp \
	httpclnt/tizhttpclntgraph.hpp \
	httpclnt/tizhttpclntgraphfsm.hpp \
	httpclnt/tizhttpclntgraphops.hpp \
	spotify/tizspotifygraph.hpp \
	spotify/tizspotifygraphfsm.hpp \
	spotify/tizspotifygraphops.hpp
	spotify/tizspotifyconfig.hpp \
	spotify/tizspotifymgr.hpp \
	gmusic/tizgmusicgraph.hpp \
	gmusic/tizgmusicgraphfsm.hpp \
	gmusic/tizgmusicgraphops.hpp
	gmusic/tizgmusicconfig.hpp \
	gmusic/tizgmusicmgr.hpp \
	tizgraphmgrcaps.hpp \
	tizplaybackstatus.hpp \
	tizplaybackevents.hpp \
	mpris/tizmpriscbacks.hpp \
	mpris/tizmprisprops.hpp \
	mpris/tizmprisif.hpp \
	mpris/tizmprismgr.hpp


tizonia_SOURCES = \
	main.cpp \
	tizplayapp.cpp \
	tizprogramopts.cpp \
	tizomxutil.cpp \
	tizgraphutil.cpp \
	tizgraphcback.cpp \
	tizdaemon.cpp \
	tizprobe.cpp \
	tizplaylist.cpp \
	tizgraphfactory.cpp \
	tizgraphmgrcmd.cpp \
	tizgraphmgrops.cpp \
	tizgraphmgrfsm.cpp \
	tizgraphmgr.cpp \
	tizdecgraphmgr.cpp \
	tizgraphfsm.cpp \
	tizgraphops.cpp \
	tizgraphcmd.cpp \
	tizgraph.cpp \
	tizmp3graph.cpp \
	tizaacgraph.cpp \
	tizopusgraph.cpp \
	tizoggopusgraph.cpp \
	tizvorbisgraph.cpp \
	tizflacgraph.cpp \
	tizoggflacgraph.cpp \
	tizpcmgraph.cpp \
	tizmpeggraph.cpp \
	httpserv/tizhttpservmgr.cpp \
	httpserv/tizhttpservgraph.cpp \
	httpserv/tizhttpservgraphfsm.cpp \
	httpserv/tizhttpservgraphops.cpp \
	httpclnt/tizhttpclntmgr.cpp \
	httpclnt/tizhttpclntgraph.cpp \
	httpclnt/tizhttpclntgraphfsm.cpp \
	httpclnt/tizhttpclntgraphops.cpp \
	spotify/tizspotifygraph.cpp \
	spotify/tizspotifygraphfsm.cpp \
	spotify/tizspotifygraphops.cpp \
	spotify/tizspotifymgr.cpp \
	gmusic/tizgmusicgraph.cpp \
	gmusic/tizgmusicgraphfsm.cpp \
	gmusic/tizgmusicgraphops.cpp \
	gmusic/tizgmusicmgr.cpp \
	tizplaybackevents.cpp \
	mpris/tizmprismgr.cpp \
	mpris/tizmprisprops.cpp \
	mpris/tizmprisif.cpp

tizonia_CPPFLAGS = \
	@BOOST_CPPFLAGS@ \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	-I$(top_srcdir)/dbus \
	-I$(top_srcdir)/src \
	-I/usr/include/taglib \
	@LIBMEDIAINFO_CFLAGS@ \
	@DBUS_CFLAGS@ \
	-D__STDC_CONSTANT_MACROS

tizonia_LDADD = \
	@BOOST_PROGRAM_OPTIONS_LIB@ \
	@BOOST_SYSTEM_LIB@ \
	@BOOST_FILESYSTEM_LIB@ \
	@BOOST_THREAD_LIB@ \
	-ltag \
	@LIBMEDIAINFO_LIBS@ \
	@DBUS_LIBS@ \
	@TIZPLATFORM_LIBS@ \
	@TIZCORE_LIBS@
