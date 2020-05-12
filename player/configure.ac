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
AC_INIT([tizonia-player], [0.22.0], [juan.rubio@aratelia.com])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules subdir-objects -Wextra -Werror])
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
AC_LANG([C++])
AM_PATH_PYTHON([3.5])
AX_PYTHON_DEVEL([>= '3.5'])
AX_BOOST_BASE([1.54],, [AC_MSG_ERROR([tizonia needs Boost 1.54])])
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_THREAD
AX_BOOST_CHRONO
AX_BOOST_PYTHON

PKG_CHECK_MODULES([TAGLIB], [taglib >= 1.7.0])
PKG_CHECK_MODULES([LIBMEDIAINFO], [libmediainfo >= 0.7.65])

AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([tizonia/dbus-c++/dbus.h],
	[tiz_found_dbuscplusplus_headers=yes; break;])
AC_LANG_POP([C++])
AS_IF([test "x$tiz_found_dbuscplusplus_headers" != "xyes"],
	[AC_SUBST([TIZDBUSCPLUSPLUS_CFLAGS], ['-I$(top_srcdir)/../3rdparty/dbus-cplusplus/include'])
	AC_SUBST([TIZDBUSCPLUSPLUS_LIBS], ['$(top_builddir)/../3rdparty/dbus-cplusplus/tizonia/libtizdbus-c++.la'])
	AC_SUBST([TIZDBUSXML2CPP], ['$(top_builddir)/../3rdparty/dbus-cplusplus/tools/tizonia-dbusxx-xml2cpp'])],
	[AC_MSG_NOTICE([Not substituting TIZDBUSCPLUSPLUS cflags and libs with local paths])])
AS_IF([test "x$tiz_found_dbuscplusplus_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZDBUSCPLUSPLUS], [libtizdbus-c++ >= 0.11.0])
        AC_SUBST([TIZDBUSXML2CPP], ['tizonia-dbusxx-xml2cpp'])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZDBUSCPLUSPLUS cflags and libs])])

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

AC_CHECK_LIB([tizcore], [OMX_Init],
	[tiz_found_core_lib=yes; break;])
AS_IF([test "x$tiz_found_core_lib" != "xyes"],
	[AC_SUBST([TIZCORE_CFLAGS], ['not-used'])
	AC_SUBST([TIZCORE_LIBS], ['$(top_builddir)/../libtizcore/tizonia/libtizcore.la'])],
	[AC_MSG_NOTICE([Not substituting TIZCORE cflags and libs with local paths])])
AS_IF([test "x$tiz_found_core_lib" == "xyes"],
	[PKG_CHECK_MODULES([TIZCORE], [libtizcore >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZCORE cflags and libs])])

AC_CHECK_HEADERS([tizonia/tizrmproxytypes.h],
	[tiz_found_rmproxy_headers=yes; break;])
AS_IF([test "x$tiz_found_rmproxy_headers" != "xyes"],
	[AC_SUBST([TIZRMPROXY_CFLAGS], ['-I$(top_srcdir)/../rm/libtizrmproxy/tizonia'])
	AC_SUBST([TIZRMPROXY_LIBS], ['$(top_builddir)/../rm/libtizrmproxy/tizonia/libtizrmproxy.la'])],
	[AC_MSG_NOTICE([Not substituting TIZRMPROXY cflags and libs with local paths])])
AS_IF([test "x$tiz_found_rmproxy_headers" == "xyes"],
	[PKG_CHECK_MODULES([TIZRMPROXY], [libtizrmproxy >= 0.1.0])],
	[AC_MSG_NOTICE([Not using pkg-config to find TIZRMPROXY cflags and libs])])

AC_ARG_WITH([bashcompletiondir],
        AS_HELP_STRING([--with-bashcompletiondir=DIR], [Bash completions directory]),
        [AS_IF([test "$withval" != ""], [
                with_bashcompletiondir=$withval
        ] , [
                with_bashcompletiondir=no
        ])],
        [AS_IF([$($PKG_CONFIG --exists bash-completion)], [
                with_bashcompletiondir=$($PKG_CONFIG --variable=completionsdir bash-completion)
        ] , [
                with_bashcompletiondir=${datadir}/bash-completion/completions
        ])])
AM_CONDITIONAL(ENABLE_BASH_COMPLETION, [test "$with_bashcompletiondir" != "no"])
AC_SUBST([bashcompletiondir], [$with_bashcompletiondir])

AC_ARG_WITH([zshcompletiondir],
        AS_HELP_STRING([--with-zshcompletiondir=DIR], [Zsh completions directory]),
        [AS_IF([test "$withval" != ""], [
                with_zshcompletiondir=$withval
        ] , [
                with_zshcompletiondir=no
        ])], [with_zshcompletiondir=${datadir}/zsh/vendor-completions])
AM_CONDITIONAL(ENABLE_ZSH_COMPLETION, [test "$with_zshcompletiondir" != "no"])
AC_SUBST([zshcompletiondir], [$with_zshcompletiondir])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
# This one was introduced in 2.69
# AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT64_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 gethostname getpass strerror])

AC_ARG_WITH(libspotify,
    AS_HELP_STRING([--with-libspotify],
        [build support for the libspotify-based OpenMAX IL plugin (default: yes)]),,
    with_libspotify=yes)

AM_CONDITIONAL(WITH_LIBSPOTIFY, test "x$with_libspotify" = xyes)

AC_CONFIG_FILES([Makefile
                tools/Makefile
                dbus/Makefile
                man/Makefile
                src/Makefile])

if test "$with_libspotify" = yes; then
      AC_DEFINE(HAVE_LIBSPOTIFY, 1, [Support for libspotify is included])
fi

# End the configure script.
AC_OUTPUT
