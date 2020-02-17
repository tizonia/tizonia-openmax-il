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

REQUIRED_SUBDIRS = \
	chromecast_renderer \
	file_reader \
	file_writer \
	flac_decoder \
	http_renderer \
	http_source \
	mp3_decoder \
	mp3_encoder \
	mp3_metadata \
	mpeg_audio_decoder \
	ogg_demuxer \
	ogg_muxer \
	opus_decoder \
	opusfile_decoder \
	pcm_decoder \
	pcm_renderer_pa \
	vorbis_decoder \
	vp8_decoder \
	webm_demuxer \
	yuv_renderer

LIBSPOTIFY_SUBDIR= \
	spotify_source

ALSA_SUBDIR= \
	pcm_renderer_alsa

AAC_SUBDIR= \
	aac_decoder

SUBDIRS = ${REQUIRED_SUBDIRS}

if WITH_LIBSPOTIFY
SUBDIRS += ${LIBSPOTIFY_SUBDIR}
endif

if WITH_ALSA
SUBDIRS += ${ALSA_SUBDIR}
endif

if WITH_AAC
SUBDIRS += ${AAC_SUBDIR}
endif
