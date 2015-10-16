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
	tizgraphfsm.hpp \
	tizgraphevt.hpp \
	tizgraphguard.hpp \
	tizgraphaction.hpp \
	tizgraphstate.hpp \
	tizgraphcmd.hpp \
	tizgraphops.hpp \
	tizgraph.hpp \
	tizgraphmgrcaps.hpp \
	tizplaybackstatus.hpp \
	tizplaybackevents.hpp \
	decoders/tizdecgraphmgr.hpp \
	decoders/tizdecgraph.hpp \
	decoders/tizmp3graph.hpp \
	decoders/tizaacgraph.hpp \
	decoders/tizopusgraph.hpp \
	decoders/tizoggopusgraph.hpp \
	decoders/tizvorbisgraph.hpp \
	decoders/tizflacgraph.hpp \
	decoders/tizoggflacgraph.hpp \
	decoders/tizpcmgraph.hpp \
	decoders/tizmpeggraph.hpp \
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
	spotify/tizspotifygraphops.hpp \
	spotify/tizspotifyconfig.hpp \
	spotify/tizspotifymgr.hpp \
	services/tizservicegraph.hpp \
	services/tizservicegraphfsm.hpp \
	services/googlemusic/tizgmusicgraph.hpp \
	services/googlemusic/tizgmusicgraphops.hpp \
	services/googlemusic/tizgmusicconfig.hpp \
	services/googlemusic/tizgmusicmgr.hpp \
	services/soundcloud/tizscloudgraph.hpp \
	services/soundcloud/tizscloudgraphops.hpp \
	services/soundcloud/tizscloudconfig.hpp \
	services/soundcloud/tizscloudmgr.hpp \
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
	tizgraphfsm.cpp \
	tizgraphops.cpp \
	tizgraphcmd.cpp \
	tizgraph.cpp \
	decoders/tizdecgraphmgr.cpp \
	decoders/tizdecgraph.cpp \
	decoders/tizmp3graph.cpp \
	decoders/tizaacgraph.cpp \
	decoders/tizopusgraph.cpp \
	decoders/tizoggopusgraph.cpp \
	decoders/tizvorbisgraph.cpp \
	decoders/tizflacgraph.cpp \
	decoders/tizoggflacgraph.cpp \
	decoders/tizpcmgraph.cpp \
	decoders/tizmpeggraph.cpp \
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
	services/tizservicegraph.cpp \
	services/tizservicegraphfsm.cpp \
	services/googlemusic/tizgmusicgraph.cpp \
	services/googlemusic/tizgmusicgraphops.cpp \
	services/googlemusic/tizgmusicmgr.cpp \
	services/soundcloud/tizscloudgraph.cpp \
	services/soundcloud/tizscloudgraphops.cpp \
	services/soundcloud/tizscloudmgr.cpp \
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
	-I$(top_srcdir)/src/services \
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
