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
AM_INIT_AUTOMAKE([foreign color-tests silent-rules -Wall -Werror])
AC_CONFIG_SRCDIR([include/omxil-1.2/OMX_Core.h])
AC_CONFIG_HEADERS([config.h])

# 'm4' is the directory where the extra autoconf macros are stored
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
PKG_PROG_PKG_CONFIG([0.25])

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Define location of plugin directory
AS_AC_EXPAND(PLUGINDIR, ${libdir}/tizonia0-plugins12)
AC_DEFINE_UNQUOTED(PLUGINDIR, "$PLUGINDIR",
  [Directory where Tizonia plugins are located])
AC_MSG_NOTICE([Using $PLUGINDIR as the components install location])
# Define plugin directory configure-time variable
AC_SUBST([plugindir], ['${libdir}/tizonia0-plugins12'])

AC_ARG_ENABLE([blocking-etb-ftb],
 [  --enable-blocking-etb-ftb   Enable fully conformant blocking behaviour of ETB and FTB APIs],
 [blocking_etb_ftb=${enableval}], [blocking_etb_ftb=no])

AC_ARG_ENABLE([blocking-sendcommand],
 [  --enable-blocking-sendcommand   Enable fully conformant blocking behaviour of SendCommand API],
 [blocking_sendcommand=${enableval}], [blocking_sendcommand=no])

AC_ARG_ENABLE(player,
    AS_HELP_STRING([--enable-player],
        [build the command-line player program (default: enabled)]),,
    enable_player=yes)
AM_CONDITIONAL(ENABLE_PLAYER, test "x$enable_player" = xyes)

AC_ARG_WITH(libspotify,
    AS_HELP_STRING([--with-libspotify],
        [build the libspotify-based OpenMAX IL plugin (default: yes)]),,
    with_libspotify=yes)

AC_ARG_WITH(alsa,
    AS_HELP_STRING([--with-alsa],
        [build the ALSA-based OpenMAX IL plugin (default: yes)]),,
    with_alsa=yes)

AC_ARG_WITH(aac,
    AS_HELP_STRING([--with-aac],
        [build the AAC-based OpenMAX IL plugin (default: yes)]),,
    with_aac=yes)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([3rdparty
                   include
                   clients
                   libtizplatform
                   cast
                   rm
                   libtizcore
                   libtizonia
                   plugins
                   config])

if test "$enable_player" = yes; then
   if test -d "$srcdir/player"; then
      AC_CONFIG_SUBDIRS([player])
   fi
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

    Tizonia player: .............. ${enable_player}
    libspotify plugin: ........... ${with_libspotify}
    ALSA plugin: ................. ${with_alsa}
    Blocking ETB/FTB: ............ ${blocking_etb_ftb}
    Blocking OMX_SendCommand: .... ${blocking_sendcommand}

  Installation paths:

    libraries: ................... ${LIBDIR}
    plugins: ..................... ${PLUGINDIR}
    tizonia: ..................... ${BINDIR}
    Header files: ................ ${INCLUDEDIR}/libtizonia
    Documentation: ............... ${DOCDIR}

  Building:

    Type 'make' to compile all $PACKAGE sub-projects.

    Type 'make install' to install all $PACKAGE sub-projects.

    Type 'make check' to test all $PACKAGE sub-projects using the tests
    contained in the various src/tests directories.

------------------------------------------------------------------------
])
