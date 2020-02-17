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

lib_LTLIBRARIES = libtizonia.la

AUTOMAKE_OPTIONS = serial-tests

libtizonia_includedir = $(includedir)/tizonia

libtizonia_include_HEADERS = \
	tizapi_decls.h \
	tizapi.h \
	tizaudioport_decls.h \
	tizaudioport.h \
	tizbinaryport_decls.h \
	tizbinaryport.h \
	tizconfigport_decls.h \
	tizconfigport.h \
	tizexecuting.h \
	tizexecutingtoidle.h \
	tizfsm_decls.h \
	tizfsm.h \
	tizidle.h \
	tizidletoloaded.h \
	tizidletoexecuting.h \
	tizkernel_decls.h \
	tizkernel.h \
	tizloaded.h \
	tizloadedtoidle.h \
	tizobjsys.h \
	tizobject_decls.h \
	tizobject.h \
	tizotherport_decls.h \
	tizotherport.h \
	tizimageport_decls.h \
	tizimageport.h \
	tizvideoport_decls.h \
	tizvideoport.h \
	tizpause.h \
	tizpausetoidle.h \
	tizpcmport_decls.h \
	tizpcmport.h \
	tizport_decls.h \
	tizport.h \
	tizport-macros.h \
	tizprc_decls.h \
	tizprc_internal.h \
	tizprc.h \
	tizfilterprc_decls.h \
	tizfilterprc.h \
	tizscheduler.h \
	tizservant_decls.h \
	tizservant.h \
	tizstate_decls.h \
	tizstate.h \
	tizutils.h \
	tizwaitforresources.h \
	tizmp2port_decls.h \
	tizmp2port.h \
	tizmp3port_decls.h \
	tizmp3port.h \
	tizaacport_decls.h \
	tizaacport.h \
	tizvorbisport_decls.h \
	tizvorbisport.h \
	tizopusport_decls.h \
	tizopusport.h \
	tizflacport_decls.h \
	tizflacport.h \
	tizvp8port_decls.h \
	tizvp8port.h \
	tizavcport_decls.h \
	tizavcport.h \
	tizivrport_decls.h \
	tizivrport.h \
	tizdemuxerport_decls.h \
	tizdemuxerport.h \
	tizmuxerport_decls.h \
	tizmuxerport.h \
	tizwebmport_decls.h \
	tizwebmport.h \
	tizmp4port_decls.h \
	tizmp4port.h \
	tizoggport_decls.h \
	tizoggport.h \
	tizuricfgport.h \
	tizuricfgport_decls.h \
	tizdemuxercfgport.h \
	tizdemuxercfgport_decls.h \
	tizkernel_helpers.inl \
	tizkernel_dispatch.inl \
	tizkernel_internal.h

libtizonia_la_SOURCES = \
	tizscheduler.c \
	tizobjsys.c \
	tizobject.c \
	tizapi.c \
	tizservant.c \
	tizfsm.c \
	tizstate.c \
	tizloaded.c \
	tizloadedtoidle.c \
	tizwaitforresources.c \
	tizidle.c \
	tizidletoloaded.c \
	tizidletoexecuting.c \
	tizexecuting.c \
	tizexecutingtoidle.c \
	tizpause.c \
	tizpausetoidle.c \
	tizkernel.c \
	tizport.c \
	tizconfigport.c \
	tizaudioport.c \
	tizimageport.c \
	tizvideoport.c \
	tizotherport.c \
	tizbinaryport.c \
	tizpcmport.c \
	tizprc.c \
	tizfilterprc.c \
	tizutils.c \
	tizmp2port.c \
	tizmp3port.c \
	tizaacport.c \
	tizvorbisport.c \
	tizopusport.c \
	tizflacport.c \
	tizvp8port.c \
	tizavcport.c \
	tizivrport.c \
	tizdemuxerport.c \
	tizmuxerport.c \
	tizwebmport.c \
	tizmp4port.c \
	tizoggport.c \
	tizuricfgport.c \
	tizdemuxercfgport.c

libtizonia_la_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZRMPROXY_CFLAGS@ \
	@TIZRMD_CFLAGS@ \
	-I$(top_srcdir)/src

libtizonia_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizonia_la_LIBADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZRMPROXY_LIBS@
