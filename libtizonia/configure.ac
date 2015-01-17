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
AC_CHECK_HEADERS([limits.h stddef.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
# This one was introduced in 2.69
# AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
# TODO: Move some of these to libtizplatform
AC_FUNC_FORK
AC_CHECK_FUNCS([select strndup strstr])

AC_CONFIG_FILES([Makefile
                 libtizonia.pc
                 src/Makefile
                 test_component/Makefile
                 tests/Makefile])

# Checks for command-line options
# Additional GCC warnings option
AC_ARG_ENABLE([gcc-warnings],
[AS_HELP_STRING([--enable-gcc-warnings],
[turn on lots of GCC warnings (for developers)])],
[case $enableval in
yes|no) ;;
*) AC_MSG_ERROR([bad value $enableval for gcc-warnings option]) ;;
esac
gl_gcc_warnings=$enableval],
[gl_gcc_warnings=no]
)

if test "$gl_gcc_warnings" = yes; then
   gl_MANYWARN_ALL_GCC([warnings])
   # Set up the list of the pointless, undesired warnings.
   nw=
   nw="$nw -Wsystem-headers"       # Don't let system headers trigger warnings
   nw="$nw -Wundef"                # All compiler preprocessors support #if UNDEF
   nw="$nw -Wtraditional"          # All compilers nowadays support ANSI C
   # nw="$nw -Wconversion"           # These warnings usually don't point to mistakes.
   # nw="$nw -Wsign-conversion"      # Likewise.
   nw="$nw -Wsuggest-attribute=pure"  # Is it worth using attributes?
   nw="$nw -Wsuggest-attribute=const" # Is it worth using attributes?
   nw="$nw -Wsuggest-attribute=format" # Is it worth using attributes?
   nw="$nw -Wsuggest-attribute=noreturn" # Is it worth using attributes?
   #nw="$nw -Wsign-conversion"      # Likewise.
   # Enable all GCC warnings not in this list.
   gl_MANYWARN_COMPLEMENT([warnings], [$warnings], [$nw])
   for w in $warnings; do
     gl_WARN_ADD([$w])
     done

   gl_WARN_ADD([-fdiagnostics-show-option])
fi

AC_ARG_ENABLE([blocking-etb-ftb],
 [  --enable-blocking-etb-ftb   Enable fully conformant blocking behaviour of ETB and FTB APIs],
 [blocking_etb_ftb=${enableval}], [blocking_etb_ftb=no])

if test "x${blocking_etb_ftb}" = xyes; then
AC_DEFINE([EFB_FTB_SHOULD_BLOCK], 1, [ Blocking behaviour of ETB and FTB APIS is enabled])
fi

AC_ARG_ENABLE([blocking-sendcommand],
 [  --enable-blocking-sendcommand   Enable fully conformant blocking behaviour of SendCommand API],
 [blocking_sendcommand=${enableval}], [blocking_sendcommand=no])

if test "x${blocking_sendcommand}" = xyes; then
AC_DEFINE([SENDCOMMAND_SHOULD_BLOCK], 1, [ Blocking behaviour of SendCommand API is enabled])
fi

AX_GCC_FUNC_ATTRIBUTE(no_sanitize_address)

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
    Blocking OMX_SendCommand: ... ${blocking_sendcommand}

  Installation paths:

    libraries: ................... ${LIBDIR}
    tizonia: ......................${BINDIR}
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
