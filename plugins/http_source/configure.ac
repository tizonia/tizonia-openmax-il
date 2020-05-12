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
AC_INIT([tizhttpsrc], [0.22.0], [juan.rubio@aratelia.com])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules -Wall -Werror])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# 'm4' is the directory where the extra autoconf macros are stored
AC_CONFIG_MACRO_DIR([m4])

################################################################################
# Set the shared versioning info, according to section 6.3 of the libtool info #
# pages. CURRENT:REVISION:AGE must be updated immediately before each release: #
#                                                                              #
#   * If the library source code has changed at all since the last             #
#     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').             #
#                                                                              #
#   * If any interfaces have been added, removed, or changed since the         #
#     last update, increment CURRENT, and set REVISION to 0.                   #
#                                                                              #
#   * If any interfaces have been added since the last public release,         #
#     then increment AGE.                                                      #
#                                                                              #
#   * If any interfaces have been removed since the last public release,       #
#     then set AGE to 0.                                                       #
#                                                                              #
################################################################################
SHARED_VERSION_INFO="0:22:0"
SHLIB_VERSION_ARG=""

AC_SUBST(SHLIB_VERSION_ARG)
AC_SUBST(SHARED_VERSION_INFO)

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

AC_CHECK_HEADERS([tizonia/OMX_Core.h tizonia/OMX_Component.h],
	[tiz_found_omx_headers=yes; break;])
AS_IF([test "x$tiz_found_omx_headers" != "xyes"],
	[AC_SUBST([TIZILHEADERS_CFLAGS], ['-I$(top_srcdir)/../../include/tizonia'])
	AC_SUBST([TIZILHEADERS_LIBS], ['not-used'])],
	[AC_MSG_NOTICE([Not substituting TIZILHEADERS cflags and libs with local paths])])
AS_IF([test "x$tiz_found_omx_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZILHEADERS], [tizilheaders >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZILHEADERS cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizplatform.h],
	[tiz_found_platform_headers=yes; break;])
AS_IF([test "x$tiz_found_platform_headers" != "xyes"],
	[AC_SUBST([TIZPLATFORM_CFLAGS], ['-I$(top_srcdir)/../../libtizplatform/tizonia'])
	AC_SUBST([TIZPLATFORM_LIBS], ['$(top_builddir)/../../libtizplatform/tizonia/libtizplatform.la'])],
	[AC_MSG_NOTICE([Not substituting TIZPLATFORM cflags and libs with local paths])])
AS_IF([test "x$tiz_found_platform_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZPLATFORM], [libtizplatform >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZPLATFORM cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizscheduler.h],
	[tiz_found_tizonia_headers=yes; break;])
AS_IF([test "x$tiz_found_tizonia_headers" != "xyes"],
	[AC_SUBST([TIZONIA_CFLAGS], ['-I$(top_srcdir)/../../libtizonia/tizonia'])
	AC_SUBST([TIZONIA_LIBS], ['$(top_builddir)/../../libtizonia/tizonia/libtizonia.la'])],
	[AC_MSG_NOTICE([Not substituting TIZONIA cflags and libs with local paths])])
AS_IF([test "x$tiz_found_tizonia_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZONIA], [libtizonia >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZONIA cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizgmusic_c.h],
	[tiz_found_gmusic_headers=yes; break;])
AS_IF([test "x$tiz_found_gmusic_headers" != "xyes"],
	[AC_SUBST([TIZGMUSIC_CFLAGS], ['-I$(top_srcdir)/../../clients/gmusic/libtizgmusic/tizonia'])
	AC_SUBST([TIZGMUSIC_LIBS], ['$(top_builddir)/../../clients/gmusic/libtizgmusic/tizonia/libtizgmusic.la'])],
	[AC_MSG_NOTICE([Not substituting TIZGMUSIC cflags and libs with local paths])])
AS_IF([test "x$tiz_found_gmusic_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZGMUSIC], [libtizgmusic >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZGMUSIC cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizsoundcloud_c.h],
	[tiz_found_soundcloud_headers=yes; break;])
AS_IF([test "x$tiz_found_soundcloud_headers" != "xyes"],
	[AC_SUBST([TIZSOUNDCLOUD_CFLAGS], ['-I$(top_srcdir)/../../clients/soundcloud/libtizsoundcloud/tizonia'])
	AC_SUBST([TIZSOUNDCLOUD_LIBS], ['$(top_builddir)/../../clients/soundcloud/libtizsoundcloud/tizonia/libtizsoundcloud.la'])],
	[AC_MSG_NOTICE([Not substituting TIZSOUNDCLOUD cflags and libs with local paths])])
AS_IF([test "x$tiz_found_soundcloud_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZSOUNDCLOUD], [libtizsoundcloud >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZSOUNDCLOUD cflags and libs])])

AC_CHECK_HEADERS([tizonia/tiztunein_c.h],
	[tiz_found_tunein_headers=yes; break;])
AS_IF([test "x$tiz_found_tunein_headers" != "xyes"],
	[AC_SUBST([TIZTUNEIN_CFLAGS], ['-I$(top_srcdir)/../../clients/tunein/libtiztunein/tizonia'])
	AC_SUBST([TIZTUNEIN_LIBS], ['$(top_builddir)/../../clients/tunein/libtiztunein/tizonia/libtiztunein.la'])],
	[AC_MSG_NOTICE([Not substituting TIZTUNEIN cflags and libs with local paths])])
AS_IF([test "x$tiz_found_tunein_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZTUNEIN], [libtiztunein >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZTUNEIN cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizyoutube_c.h],
	[tiz_found_youtube_headers=yes; break;])
AS_IF([test "x$tiz_found_youtube_headers" != "xyes"],
	[AC_SUBST([TIZYOUTUBE_CFLAGS], ['-I$(top_srcdir)/../../clients/youtube/libtizyoutube/tizonia'])
	AC_SUBST([TIZYOUTUBE_LIBS], ['$(top_builddir)/../../clients/youtube/libtizyoutube/tizonia/libtizyoutube.la'])],
	[AC_MSG_NOTICE([Not substituting TIZYOUTUBE cflags and libs with local paths])])
AS_IF([test "x$tiz_found_youtube_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZYOUTUBE], [libtizyoutube >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZYOUTUBE cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizplex_c.h],
	[tiz_found_plex_headers=yes; break;])
AS_IF([test "x$tiz_found_plex_headers" != "xyes"],
	[AC_SUBST([TIZPLEX_CFLAGS], ['-I$(top_srcdir)/../../clients/plex/libtizplex/tizonia'])
	AC_SUBST([TIZPLEX_LIBS], ['$(top_builddir)/../../clients/plex/libtizplex/tizonia/libtizplex.la'])],
	[AC_MSG_NOTICE([Not substituting TIZPLEX cflags and libs with local paths])])
AS_IF([test "x$tiz_found_plex_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZPLEX], [libtizplex >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZPLEX cflags and libs])])

AC_CHECK_HEADERS([tizonia/tiziheart_c.h],
	[tiz_found_iheart_headers=yes; break;])
AS_IF([test "x$tiz_found_iheart_headers" != "xyes"],
	[AC_SUBST([TIZIHEART_CFLAGS], ['-I$(top_srcdir)/../../clients/iheart/libtiziheart/tizonia'])
	AC_SUBST([TIZIHEART_LIBS], ['$(top_builddir)/../../clients/iheart/libtiziheart/tizonia/libtiziheart.la'])],
	[AC_MSG_NOTICE([Not substituting TIZIHEART cflags and libs with local paths])])
AS_IF([test "x$tiz_found_iheart_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZIHEART], [libtiziheart >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZIHEART cflags and libs])])

# Define location of plugin directory
AS_AC_EXPAND(PLUGINDIR, ${libdir}/tizonia0-plugins12)
AC_DEFINE_UNQUOTED(PLUGINDIR, "$PLUGINDIR",
  [Directory where Tizonia plugins are located])
AC_MSG_NOTICE([Using $PLUGINDIR as the components install location])
# Define plugin directory configure-time variable
AC_SUBST([plugindir], ['${libdir}/tizonia0-plugins12'])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])

# End the configure script.
AC_OUTPUT
