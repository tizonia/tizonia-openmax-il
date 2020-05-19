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
AC_INIT([tizfw], [0.22.0], [juan.rubio@aratelia.com])
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

# Define location of plugin directory
AS_AC_EXPAND(PLUGINDIR, ${libdir}/tizonia0-plugins12)
AC_DEFINE_UNQUOTED(PLUGINDIR, "$PLUGINDIR",
  [Directory where Tizonia plugins are located])
AC_MSG_NOTICE([Using $PLUGINDIR as the components install location])
# Define plugin directory configure-time variable
AC_SUBST([plugindir], ['${libdir}/tizonia0-plugins12'])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([strerror strndup])

AC_CONFIG_FILES([Makefile
                 src/Makefile])

# End the configure script.
AC_OUTPUT
