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
AC_INIT([tizonia], [0.1.0], [juan.rubio@aratelia.com])
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
#PKG_CHECK_MODULES([ILRMPROXY], [libtizrmproxy])
#PKG_CHECK_MODULES([TIZCORE], [libtizcore])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
# TODO: Move this to OSAL
AC_CHECK_FUNCS([strdup])

AC_CONFIG_FILES([Makefile
                 libtizonia.pc
                 src/Makefile
                 test_component/Makefile
                 tests/Makefile])

# Checks for command-line options
AC_ARG_ENABLE([blocking-etb-ftb],
 [  --enable-blocking-etb-ftb   Enable fully conformant blocking behaviour of ETB and FTB APIs],
 [blocking_etb_ftb=${enableval}], [blocking_etb_ftb=no])

if test "x${blocking_etb_ftb}" = xyes; then
AC_DEFINE([EFB_FTB_SHOULD_BLOCK], 1, [ Blocking behaviour of ETB and FTB APIS is enabled])
fi


# End the configure script.
AC_OUTPUT
