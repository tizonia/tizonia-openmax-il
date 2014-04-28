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
AC_INIT([tizplatform], [0.1.0], [juan.rubio@aratelia.com])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules subdir-objects -Wall -Werror])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL
LT_INIT

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG([0.25])

AC_DEFINE([HTTP_PARSER_STRICT], [1], [Using strict http parsing.])

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([libuuid], [uuid_generate])
AC_CHECK_LIB([ev], [ev_run])
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
PKG_CHECK_MODULES([uuid], [uuid >= 2.19.0])
#PKG_CHECK_MODULES([KHROMTIZ12], [libkhromxil12])
AM_PATH_LOG4C([1.2.1])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h stddef.h stdint.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
# This one was introduced in 2.69
# AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero gettimeofday memmove memset pathconf socket strdup strerror strndup strstr strtoul])

AC_CONFIG_FILES([Makefile
                 libtizplatform.pc
                 src/Makefile
                 tests/Makefile])
AC_OUTPUT
