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
# along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.

libtizchromecastrnddir = $(plugindir)

libtizchromecastrnd_LTLIBRARIES = libtizchromecastrnd.la

noinst_HEADERS = \
	chromecastrnd.h \
	chromecastrndport.h \
	chromecastrndport_decls.h \
	chromecastrndprc.h \
	chromecastrndprc_decls.h \
	cc_gmusicprc.h \
	cc_gmusicprc_decls.h \
	cc_gmusiccfgport.h \
	cc_gmusiccfgport_decls.h \
	cc_scloudprc.h \
	cc_scloudprc_decls.h \
	cc_scloudcfgport.h \
	cc_scloudcfgport_decls.h \
	cc_dirbleprc.h \
	cc_dirbleprc_decls.h \
	cc_dirblecfgport.h \
	cc_dirblecfgport_decls.h \
	cc_youtubeprc.h \
	cc_youtubeprc_decls.h \
	cc_youtubecfgport.h \
	cc_youtubecfgport_decls.h

libtizchromecastrnd_la_SOURCES = \
	chromecastrnd.c \
	chromecastrndport.c \
	chromecastrndprc.c \
	cc_gmusicprc.c \
	cc_gmusiccfgport.c \
	cc_scloudprc.c \
	cc_scloudcfgport.c \
	cc_dirbleprc.c \
	cc_dirblecfgport.c \
	cc_youtubeprc.c \
	cc_youtubecfgport.c

libtizchromecastrnd_la_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZONIA_CFLAGS@ \
	@TIZGMUSIC_CFLAGS@ \
	@TIZSOUNDCLOUD_CFLAGS@ \
	@TIZDIRBLE_CFLAGS@ \
	@TIZYOUTUBE_CFLAGS@ \
	@TIZCASTCLIENT_CFLAGS@ \
	@TIZCASTD_CFLAGS@

libtizchromecastrnd_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@

libtizchromecastrnd_la_LIBADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZONIA_LIBS@ \
	@TIZGMUSIC_LIBS@ \
	@TIZSOUNDCLOUD_LIBS@ \
	@TIZDIRBLE_LIBS@ \
	@TIZYOUTUBE_LIBS@ \
	@TIZCASTCLIENT_LIBS@
