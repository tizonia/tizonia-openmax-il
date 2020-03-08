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

AC_PREREQ([2.68])
AC_INIT([tizrmd], [0.21.0],[juan.rubio@aratelia.com])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules -Wall -Werror])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# 'm4' is the directory where the extra autoconf macros are stored
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT

PKG_PROG_PKG_CONFIG()

AX_LIB_SQLITE3([3.7.1])

AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([tizonia/dbus-c++/dbus.h],
	[tiz_found_dbuscplusplus_headers=yes; break;])
AC_LANG_POP([C++])
AS_IF([test "x$tiz_found_dbuscplusplus_headers" != "xyes"],
	[AC_SUBST([TIZDBUSCPLUSPLUS_CFLAGS], ['-I$(top_srcdir)/../../3rdparty/dbus-cplusplus/include'])
	AC_SUBST([TIZDBUSCPLUSPLUS_LIBS], ['$(top_builddir)/../../3rdparty/dbus-cplusplus/tizonia/libtizdbus-c++.la'])
        AC_SUBST([TIZDBUSXML2CPP], ['$(top_builddir)/../../3rdparty/dbus-cplusplus/tools/tizonia-dbusxx-xml2cpp'])],
	[AC_MSG_NOTICE([Not substituting TIZDBUSCPLUSPLUS cflags and libs with local paths])])
AS_IF([test "x$tiz_found_dbuscplusplus_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZDBUSCPLUSPLUS], [libtizdbus-c++ >= 0.11.0])
        AC_SUBST([TIZDBUSXML2CPP], ['tizonia-dbusxx-xml2cpp'])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZDBUSCPLUSPLUS cflags and libs])])

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

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h])
AC_LANG_PUSH([C++])
AC_LANG_POP([C++])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_C_RESTRICT
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([strtol])
# This one was introduced in 2.69
# AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# DBus
AS_AC_EXPAND(DATADIR, $datadir)
DBUS_SERVICES_DIR="$DATADIR/dbus-1/services"
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR",
                                      [Where services dir for DBUS is])

# sqlite3
AC_CHECK_PROG(HAVE_SQLITE3, sqlite3, true, false)
if test "x$HAVE_SQLITE3" = "xfalse"; then
   AC_MSG_ERROR([Program sqlite3 not found. Please install sqlite3.])
fi

AC_CONFIG_FILES([Makefile
                tizrmd.pc
                dbus/Makefile
                data/Makefile
                src/Makefile])

# End the configure script.
AC_OUTPUT
