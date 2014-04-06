# Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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

AC_PREREQ([2.69])
AC_INIT([tizrmd],[0.1.0],[juan.rubio@aratelia.com])
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

# Checks for libraries.
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
DBUS_REQUIRED_VERSION=0.60
PKG_CHECK_MODULES(dbus, [dbus-1 >= $DBUS_REQUIRED_VERSION],,
	AC_MSG_ERROR([You need the DBus libraries (version 0.6 or better)]
	[http://www.freedesktop.org/wiki/Software_2fdbus]))
PKG_CHECK_MODULES([DBUS], [dbus-c++-1 >= 0.6.0-pre1])
AX_BOOST_BASE([1.46],, [AC_MSG_ERROR([tizrmd needs Boost 1.46])])
AX_LIB_SQLITE3([3.7.1])

#PKG_CHECK_MODULES([KHROMTIZ12], [libkhromxil12])
#PKG_CHECK_MODULES([TIZ], [libtizplatform])

# Checks for header files.
AC_CHECK_HEADERS([syslog.h unistd.h])
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([boost/foreach.hpp], [],
    [AC_MSG_ERROR(You need the Boost libraries.)])
AC_LANG_POP([C++])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_C_RESTRICT

# Checks for library functions.
AC_CHECK_FUNCS([strtol])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADER_STDBOOL
AC_PROG_RANLIB
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
                data/Makefile
                src/Makefile
                tests/Makefile])

# End the configure script.
AC_OUTPUT
