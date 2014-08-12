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

AC_PREREQ([2.67])
AC_INIT([libtizopusfiled], [0.1.0], [juan.rubio@aratelia.com])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules -Wall -Werror])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

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
PKG_CHECK_MODULES([OPUSFILE], [opusfile >= 0.5], [HAVE_OPUSFILE=yes], [HAVE_OPUSFILE=no])

if test "x$HAVE_OPUSFILE" = "xno"; then
   AC_MSG_ERROR([Please install libopusfile version 0.5 or later.])
fi

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])

# End the configure script.
AC_OUTPUT
