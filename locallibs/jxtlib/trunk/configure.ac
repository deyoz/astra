AC_INIT([jxtlib], [0.1])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE

if test "x$LIBROOT" = "x" ; then
    export LIBROOT=$HOME
fi
echo "use LIBROOT=$LIBROOT"

AC_PREFIX_DEFAULT(${HOME}/local)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_LANG_CPLUSPLUS

AC_PROG_CXX

AC_DISABLE_SHARED

AC_PROG_LIBTOOL

AC_ARG_VAR(CPP_STD_VERSION, [Set c++11 or c++14 or c++17])
AS_IF( [test x"$CPP_STD_VERSION" = x"c++17" ], [ AX_CXX_COMPILE_STDCXX(17,[noext],[mandatory]) ],
       [test x"$CPP_STD_VERSION" = x"c++14" ], [ AX_CXX_COMPILE_STDCXX(14,[noext],[mandatory]) ],
                                               [ AX_CXX_COMPILE_STDCXX(11,[noext],[mandatory]) ])
MRJ_CHECK_ORACLE

CHECK_TCL
MRJ_CHECK_SERVERLIB
AX_BOOST_BASE(1.33, , [AC_MSG_ERROR([Boost C++ libraries not found])])
MRJ_CHECK_GLIBCXX_DEBUG
PKG_CHECK_MODULES(XML, libxml-2.0, [AC_MSG_NOTICE($XML_LIBS)], [AC_MSG_ERROR([libxml2 not found])])
PKG_CHECK_MODULES(XSLT, libxslt, [AC_MSG_NOTICE($XSLT_LIBS)], [AC_MSG_ERROR([libxslt not found])])
PKG_CHECK_MODULES(ZLIB, zlib, 
              [
                 AC_MSG_NOTICE($ZLIB_LIBS)
                 AC_DEFINE(WITH_ZLIB, , [zlib enabled])
              ], [AC_MSG_ERROR([zlib not found])])

AC_ARG_ENABLE([xp-testing],
    [  --enable-xp-testing       Enable xp testing],
    [
        [if test x$no_check != xyes; then enable_xp_testing=yes; fi]
        AM_CONDITIONAL([XP_TESTING], [test x$enable_xp_testing = xyes])
    ],\
    [AM_CONDITIONAL([XP_TESTING], [test 1 = 0])] )

if test x$enable_xp_testing = xyes; then
    AC_DEFINE([XP_TESTING],,[XP TESTING support])
    PKG_CHECK_MODULES([CHECK], [check], AC_MSG_NOTICE($CHECK_LIBS), AC_MSG_ERROR([libcheck not found]))
fi

CXXFLAGS="$CXXFLAGS $SERVERLIB_CXXFLAGS"

AC_OUTPUT(Makefile src/Makefile)

if ! test -d lib ; then
    mkdir lib
    cd lib
    ln -s ../src/libjxt.la libjxt.la
elif ! test -f lib/libjxt.la ; then
    cd lib
    ln -s ../src/libjxt.la libjxt.la
fi

echo 
echo Configuration for $PACKAGE $VERSION :
echo --------------------------------
echo 
echo Source code location ....: $srcdir
echo Host System Type ........: $host
echo CFLAGS  .................: $CFLAGS
echo CXXFLAGS  ...............: $CXXFLAGS
if test x$enable_xp_testing = xyes; then
echo XP_TESTING ..............: enabled
fi
echo Now type 'make' to build $PACKAGE $VERSION,
echo
