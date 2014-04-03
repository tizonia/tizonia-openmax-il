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
AC_INIT([tizonia], [0.1.0], [juan.rubio@aratelia.com])
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
#PKG_CHECK_MODULES([TIZ], [libtizplatform])
#PKG_CHECK_MODULES([ILRMPROXY], [libtizrmproxy])
#PKG_CHECK_MODULES([TIZCORE], [libtizcore])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
# TODO: Move this to libtizplatform
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


AC_OUTPUT

AS_AC_EXPAND(LIBDIR, ${libdir})
AS_AC_EXPAND(INCLUDEDIR, ${includedir})
AS_AC_EXPAND(BINDIR, ${bindir})
AS_AC_EXPAND(DOCDIR, ${docdir})

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.

  General configuration:

    Blocking ETB/FTB: ........... ${blocking_etb_ftb}

  Installation paths:

    libraries: ................... ${LIBDIR}
    tplay: ....................... ${BINDIR}
    Header files: ................ ${INCLUDEDIR}/libtizonia, ${INCLUDEDIR}/libtizplatform
    Documentation: ............... ${DOCDIR}

  Building:

    Type 'make' to compile $PACKAGE.

    Type 'make install' to install $PACKAGE.

    Type 'make check' to test $PACKAGE using the unit and functional tests
    contained in the src/tests directory.
    ${TESTS_INFO}

  Example programs will be built and installed.
------------------------------------------------------------------------
])
