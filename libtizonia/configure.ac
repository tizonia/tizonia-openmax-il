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
AC_INIT([tizonia], [0.21.0], [juan.rubio@aratelia.com])
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
SHARED_VERSION_INFO="0:21:0"
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
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_CHECK_HEADERS([tizonia/OMX_Core.h tizonia/OMX_Component.h],
	[tiz_found_omx_headers=yes; break;])
AS_IF([test "x$tiz_found_omx_headers" != "xyes"],
	[AC_SUBST([TIZILHEADERS_CFLAGS], ['-I$(top_srcdir)/../include/tizonia'])
	AC_SUBST([TIZILHEADERS_LIBS], ['not-used'])],
	[AC_MSG_NOTICE([Not substituting TIZILHEADERS cflags and libs with local paths])])
AS_IF([test "x$tiz_found_omx_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZILHEADERS], [tizilheaders >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZILHEADERS cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizplatform.h],
	[tiz_found_platform_headers=yes; break;])
AS_IF([test "x$tiz_found_platform_headers" != "xyes"],
	[AC_SUBST([TIZPLATFORM_CFLAGS], ['-I$(top_srcdir)/../libtizplatform/tizonia'])
	AC_SUBST([TIZPLATFORM_LIBS], ['$(top_builddir)/../libtizplatform/tizonia/libtizplatform.la'])],
	[AC_MSG_NOTICE([Not substituting TIZPLATFORM cflags and libs with local paths])])
AS_IF([test "x$tiz_found_platform_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZPLATFORM], [libtizplatform >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZPLATFORM cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizrmproxytypes.h],
	[tiz_found_rmproxy_headers=yes; break;])
AS_IF([test "x$tiz_found_rmproxy_headers" != "xyes"],
	[AC_SUBST([TIZRMPROXY_CFLAGS], ['-I$(top_srcdir)/../rm/libtizrmproxy/tizonia'])
	AC_SUBST([TIZRMPROXY_LIBS], ['$(top_builddir)/../rm/libtizrmproxy/tizonia/libtizrmproxy.la'])],
	[AC_MSG_NOTICE([Not substituting TIZRMPROXY cflags and libs with local paths])])
AS_IF([test "x$tiz_found_rmproxy_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZRMPROXY], [libtizrmproxy >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZRMPROXY cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizrmtypes.h],
	[tiz_found_rmd_headers=yes; break;])
AS_IF([test "x$tiz_found_rmd_headers" != "xyes"],
	[AC_SUBST([TIZRMD_CFLAGS], ['-I$(top_srcdir)/../rm/tizrmd/tizonia'])
	AC_SUBST([TIZRMD_LIBS], ['not-used'])],
	[AC_MSG_NOTICE([Not substituting TIZRMD cflags and libs with local paths])])
AS_IF([test "x$tiz_found_rmd_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZRMD], [tizrmd >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZRMD cflags and libs])])

AC_CHECK_LIB([tizcore], [OMX_Init],
	[tiz_found_core_lib=yes; break;])
AS_IF([test "x$tiz_found_core_lib" != "xyes"],
	[AC_SUBST([TIZCORE_CFLAGS], ['not-used'])
	AC_SUBST([TIZCORE_LIBS], ['$(top_builddir)/../libtizcore/tizonia/libtizcore.la'])],
	[AC_MSG_NOTICE([Not substituting TIZCORE cflags and libs with local paths])])
AS_IF([test "x$tiz_found_core_lib" == "xyes"],
	[PKG_CHECK_MODULES([TIZCORE], [libtizcore >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZCORE cflags and libs])])

# Define location of plugin directory
AS_AC_EXPAND(PLUGINDIR, ${libdir}/tizonia0-plugins12)
AC_DEFINE_UNQUOTED(PLUGINDIR, "$PLUGINDIR",
  [Directory where Tizonia plugins are located])
AC_MSG_NOTICE([Using $PLUGINDIR as the components install location])
# Define plugin directory configure-time variable
AC_SUBST([plugindir], ['${libdir}/tizonia0-plugins12'])

#---------------------------------------------------------------------------
# test suite
#---------------------------------------------------------------------------
AC_ARG_ENABLE(test,
	AS_HELP_STRING([--enable-test],
		[build the test programs (default: disabled)]),,
	enable_test=no)

AM_CONDITIONAL(ENABLE_TEST, test "x$enable_test" = xyes)

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

    Blocking ETB/FTB: ............ ${blocking_etb_ftb}
    Blocking OMX_SendCommand: ... .${blocking_sendcommand}

  Installation paths:

    libraries: ................... ${LIBDIR}
    plugins: ..................... ${PLUGINDIR}
    tizonia: ..................... ${BINDIR}
    Header files: ................ ${INCLUDEDIR}/libtizonia
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
