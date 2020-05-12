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

AC_PREREQ([2.67])
AC_INIT([tizonia-plugins], [0.22.0], [juan.rubio@aratelia.com])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules -Wall -Werror])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL
LT_INIT
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG()

# Checks for libraries.
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_ARG_WITH(libspotify,
    AS_HELP_STRING([--with-libspotify],
        [build the libspotify-based OpenMAX IL plugin (default: yes)]),,
    with_libspotify=yes)

AM_CONDITIONAL(WITH_LIBSPOTIFY, test "x$with_libspotify" = xyes)

AC_ARG_WITH(alsa,
    AS_HELP_STRING([--with-alsa],
        [build the ALSA-based OpenMAX IL plugin (default: yes)]),,
    with_alsa=yes)

AM_CONDITIONAL(WITH_ALSA, test "x$with_alsa" = xyes)

AC_ARG_WITH(aac,
    AS_HELP_STRING([--with-aac],
        [build the AAC-based OpenMAX IL plugin (default: yes)]),,
    with_aac=yes)

AM_CONDITIONAL(WITH_AAC, test "x$with_aac" = xyes)

AC_CONFIG_FILES([Makefile])

AC_CONFIG_SUBDIRS([chromecast_renderer
                   file_reader
                   file_writer
                   flac_decoder
                   http_renderer
                   http_source
                   mp3_decoder
                   mp3_encoder
                   mp3_metadata
                   mpeg_audio_decoder
                   ogg_demuxer
                   ogg_muxer
                   opus_decoder
                   opusfile_decoder
                   pcm_decoder
                   pcm_renderer_pa
                   vorbis_decoder
                   vp8_decoder
                   webm_demuxer
                   yuv_renderer])

if test "$with_libspotify" = yes; then
   if test -d "$srcdir/spotify_source"; then
      AC_CONFIG_SUBDIRS([spotify_source])
   fi
fi

if test "$with_alsa" = yes; then
   if test -d "$srcdir/pcm_renderer_alsa"; then
      AC_CONFIG_SUBDIRS([pcm_renderer_alsa])
   fi
fi

if test "$with_aac" = yes; then
   if test -d "$srcdir/aac_decoder"; then
      AC_CONFIG_SUBDIRS([aac_decoder])
   fi
fi

# End the configure script.
AC_OUTPUT
