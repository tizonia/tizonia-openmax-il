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

libtizhttpsrcdir = $(plugindir)

libtizhttpsrc_LTLIBRARIES = libtizhttpsrc.la

noinst_HEADERS = \
	httpsrc.h \
	httpsrcport.h \
	httpsrcport_decls.h \
	httpsrcprc.h \
	httpsrcprc_decls.h \
	gmusicprc.h \
	gmusicprc_decls.h \
	gmusiccfgport.h \
	gmusiccfgport_decls.h \
	scloudprc.h \
	scloudprc_decls.h \
	scloudcfgport.h \
	scloudcfgport_decls.h \
	youtubeprc.h \
	youtubeprc_decls.h \
	youtubecfgport.h \
	youtubecfgport_decls.h \
	plexprc.h \
	plexprc_decls.h \
	plexcfgport.h \
	plexcfgport_decls.h \
	tuneinprc.h \
	tuneinprc_decls.h \
	tuneincfgport.h \
	tuneincfgport_decls.h \
	iheartprc.h \
	iheartprc_decls.h \
	iheartcfgport.h \
	iheartcfgport_decls.h

libtizhttpsrc_la_SOURCES = \
	httpsrc.c \
	httpsrcport.c \
	httpsrcprc.c \
	gmusicprc.c \
	gmusiccfgport.c \
	scloudprc.c \
	scloudcfgport.c \
	youtubeprc.c \
	youtubecfgport.c \
	plexprc.c \
	plexcfgport.c \
	tuneinprc.c \
	tuneincfgport.c \
	iheartprc.c \
	iheartcfgport.c

libtizhttpsrc_la_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZONIA_CFLAGS@ \
	@TIZGMUSIC_CFLAGS@ \
	@TIZSOUNDCLOUD_CFLAGS@ \
	@TIZYOUTUBE_CFLAGS@ \
	@TIZPLEX_CFLAGS@ \
	@TIZTUNEIN_CFLAGS@ \
	@TIZIHEART_CFLAGS@

libtizhttpsrc_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizhttpsrc_la_LIBADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZONIA_LIBS@ \
	@TIZGMUSIC_LIBS@ \
	@TIZSOUNDCLOUD_LIBS@ \
	@TIZYOUTUBE_LIBS@ \
	@TIZPLEX_LIBS@ \
	@TIZTUNEIN_LIBS@ \
	@TIZIHEART_LIBS@
