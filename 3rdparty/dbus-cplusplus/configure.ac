# Autojunk script for libdbus-c++

AC_PREREQ([2.68])
AC_INIT([libdbus-c++], [0.9.1pwtiz], [juan.rubio@aratelia.com])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign color-tests silent-rules -Wall -Werror])
AC_CONFIG_SRCDIR([src/error.cpp])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_CANONICAL_HOST

AC_SUBST(PACKAGE_VERSION)

# Set configuration options

AC_CHECK_FUNCS(clock_gettime, [], [
  AC_CHECK_LIB(rt, clock_gettime, [
    AC_DEFINE(HAVE_CLOCK_GETTIME, 1)
    RT_LIBS="-lrt"
  ])
])
AC_SUBST(RT_LIBS)

# Check for programs
AC_LANG_CPLUSPLUS
AC_PROG_CC
AC_PROG_CXX

CXX_FOR_BUILD=${CXX_FOR_BUILD-${CXX}}
AC_SUBST(CXX_FOR_BUILD)

LT_INIT([disable-shared])

PKG_PROG_PKG_CONFIG

# Check for dependencies

DBUS_REQUIRED_VERSION=0.60
PKG_CHECK_MODULES(dbus, [dbus-1 >= $DBUS_REQUIRED_VERSION],,
	AC_MSG_ERROR([You need the DBus libraries (version 0.6 or better)]
	[http://www.freedesktop.org/wiki/Software_2fdbus])
)
AC_SUBST(dbus_CFLAGS)
AC_SUBST(dbus_LIBS)

DBUS_API_STABLE_VERSION=1.0.0
PKG_CHECK_EXISTS([dbus-1 < $DBUS_API_STABLE_VERSION],
	[AC_DEFINE(DBUS_API_SUBJECT_TO_CHANGE, , [unstable DBus])]
)

DBUS_THREADS_INIT_DEFAULT_VERSION=0.93
PKG_CHECK_EXISTS([dbus-1 >= $DBUS_THREADS_INIT_DEFAULT_VERSION],
	[AC_DEFINE(DBUS_HAS_THREADS_INIT_DEFAULT, , [dbus_threads_init_default (needs DBus >= 0.93)])]
)

DBUS_RECURSIVE_MUTEX_VERSION=0.95
PKG_CHECK_EXISTS([dbus-1 >= $DBUS_RECURSIVE_MUTEX_VERSION],
	[AC_DEFINE(DBUS_HAS_RECURSIVE_MUTEX, , [DBus supports recursive mutexes (needs DBus >= 0.95)])]
)

AC_CHECK_LIB([expat], XML_ParserCreate_MM,
	[AC_CHECK_HEADERS(expat.h, have_expat=true, have_expat=false)],
	have_expat=false)

if ! $have_expat; then
   AC_MSG_ERROR([You need the eXpat xml parser]
   [http://expat.sourceforge.net/])
fi

xml_CFLAGS=
xml_LIBS=-lexpat

AC_SUBST(xml_CFLAGS)
AC_SUBST(xml_LIBS)

ACX_PTHREAD
if test x"$acx_pthread_ok" = xyes; then
   AC_DEFINE(HAVE_PTHREAD_H, 1, [Define to enable pthread support])
   AM_CONDITIONAL(HAVE_PTHREAD, test x"$acx_pthread_ok" = xyes)
fi

# Save processed files

AC_OUTPUT(
        Makefile
        src/Makefile
        tools/Makefile
        data/Makefile
)
