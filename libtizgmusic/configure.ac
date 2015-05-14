# Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
AC_INIT([libtizgmusic], [0.1.0], [juan.rubio@aratelia.com])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules -Wall -Werror])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# 'm4' is the directory where the extra autoconf macros are stored
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT

# Checks for libraries.
AC_LANG([C++])
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
AM_PATH_PYTHON([2.7])
AX_PYTHON_DEVEL([>= '2.7'])
AX_PYTHON_MODULE(gmusicapi, [libtizgmusic needs gmusicapi])
AX_BOOST_BASE([1.54],, [AC_MSG_ERROR([libtizgmusic needs Boost 1.54])])
AX_BOOST_PYTHON

# Checks for header files.
#AC_CHECK_HEADER_STDBOOL

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                src/Makefile
                tests/Makefile])

# End the configure script.
AC_OUTPUT
