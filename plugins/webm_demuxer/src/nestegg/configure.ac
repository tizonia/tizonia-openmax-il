dnl ------------------------------------------------
dnl Initialization and Versioning
dnl ------------------------------------------------

AC_INIT(libnestegg,[0.1git])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_MACRO_DIR([m4])

AM_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([src/nestegg.c])
AM_INIT_AUTOMAKE

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Library versioning
dnl CURRENT, REVISION, AGE
dnl - library source changed -> increment REVISION
dnl - interfaces added/removed/changed -> increment CURRENT, REVISION = 0
dnl - interfaces added -> increment AGE
dnl - interfaces removed -> AGE = 0

NESTEGG_CURRENT=0
NESTEGG_REVISION=0
NESTEGG_AGE=1
AC_SUBST(NESTEGG_CURRENT)
AC_SUBST(NESTEGG_REVISION)
AC_SUBST(NESTEGG_AGE)


dnl --------------------------------------------------  
dnl Check for programs
dnl --------------------------------------------------  

dnl save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
cflags_save="$CFLAGS"
AC_PROG_CC
AC_PROG_CPP
CFLAGS="$cflags_save"

AM_PROG_CC_C_O
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl Check for doxygen
AC_ARG_ENABLE([doc],
	AS_HELP_STRING([--enable-doc], [Build API documentation]),
	[ac_enable_doc=$enableval], [ac_enable_doc=auto])

if test "x$ac_enable_doc" != "xno"; then
	AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)

	if test "x$HAVE_DOXYGEN" = "xfalse" -a "x$ac_enable_doc" = "xyes"; then
		AC_MSG_ERROR([*** API documentation explicitly requested but Doxygen not found])
	fi
else
	HAVE_DOXYGEN=false
fi
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
        AC_MSG_WARN([*** doxygen not found, API documentation will not be built])
fi

# Test whenever ld supports -version-script
AC_PROG_LD
AC_PROG_LD_GNU
AC_MSG_CHECKING([how to control symbol export])

dnl --------------------------------------------------
dnl Do substitutions
dnl --------------------------------------------------

AC_SUBST(DEBUG)
AC_SUBST(PROFILE)

AC_OUTPUT([
  Makefile 
  docs/Makefile
  docs/Doxyfile
  nestegg.pc
  nestegg-uninstalled.pc
])

AS_AC_EXPAND(LIBDIR, ${libdir})
AS_AC_EXPAND(INCLUDEDIR, ${includedir})
AS_AC_EXPAND(BINDIR, ${bindir})
AS_AC_EXPAND(DOCDIR, ${docdir})

if test $HAVE_DOXYGEN = "false"; then
  doc_build="no"
else
  doc_build="yes"
fi
