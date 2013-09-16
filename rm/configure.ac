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
AC_INIT([tizonia_rm], [0.1.0], [juan.rubio@aratelia.com])
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
LT_INIT

# Checks for libraries.
DBUS_REQUIRED_VERSION=0.60
PKG_CHECK_MODULES(dbus, [dbus-1 >= $DBUS_REQUIRED_VERSION],,
	AC_MSG_ERROR([You need the DBus libraries (version 0.6 or better)]
	[http://www.freedesktop.org/wiki/Software_2fdbus]))
PKG_CHECK_MODULES([DBUS], [dbus-c++-1 >= 0.6.0-pre1])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.

# DBus
AS_AC_EXPAND(DATADIR, $datadir)
DBUS_SERVICES_DIR="$DATADIR/dbus-1/services"
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR",
                                      [Where services dir for DBUS is])

AC_CONFIG_FILES([Makefile dbus/Makefile])
AC_CONFIG_SUBDIRS([libtizrmproxy tizrmd])
AC_OUTPUT
