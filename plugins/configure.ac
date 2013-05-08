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

AC_PREREQ([2.67])
AC_INIT([tizonia-plugins], [0.1], [juan.rubio@aratelia.com])
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
#PKG_CHECK_MODULES([KHROMTIZ12], [libkhromxil12])
#PKG_CHECK_MODULES([TIZ], [libtizosal])
#PKG_CHECK_MODULES([TIZCORE], [libtizcore])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.


AC_CONFIG_FILES([Makefile])

AC_CONFIG_SUBDIRS([audio_renderer
                   file_reader
                   file_writer
                   mp3_decoder
                   mp3_encoder
                   vp8_decoder
                   vp8_encoder
                   webp_decoder
                   webp_encoder
                   yuv_renderer
                   http_renderer])

# End the configure script.
AC_OUTPUT
