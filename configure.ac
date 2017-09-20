# Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
AC_INIT([tizonia], [0.9.0], [juan.rubio@aratelia.com])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules -Wall -Werror])
AC_CONFIG_SRCDIR([include/omxil-1.2/OMX_Core.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
PKG_PROG_PKG_CONFIG([0.25])

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_ARG_ENABLE(player,
    AS_HELP_STRING([--enable-player],
        [build the command-line player program (default: enabled)]),,
    enable_player=yes)

AM_CONDITIONAL(ENABLE_PLAYER, test "x$enable_player" = xyes)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([include
                   clients
                   libtizplatform
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
