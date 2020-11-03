AC_INIT([etick], [0.1])
AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])

if test "x$LIBROOT" = "x" ; then
	export LIBROOT=$HOME
fi
echo "use LIBROOT=$LIBROOT"

AC_PREFIX_DEFAULT(${HOME}/local)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_LANG_CPLUSPLUS
AC_PROG_CXX

#AC_PROG_RANLIB
#Не создавать Shared библиотеки
AC_DISABLE_SHARED
# Цитата (quoting):
# This macro adds many new tests to the configure script 
# so that the generated libtool script will understand 
# the characteristics of the host
AC_PROG_LIBTOOL

AC_ARG_VAR(CPP_STD_VERSION, [Set c++11 or c++14 or c++17])
AS_IF( [test x"$CPP_STD_VERSION" = x"c++17" ], [ AX_CXX_COMPILE_STDCXX(17,[noext],[mandatory]) ],
       [test x"$CPP_STD_VERSION" = x"c++14" ], [ AX_CXX_COMPILE_STDCXX(14,[noext],[mandatory]) ],
                                               [ AX_CXX_COMPILE_STDCXX(11,[noext],[mandatory]) ])
#check for oracle
MRJ_CHECK_ORACLE
#check for serverlib
MRJ_CHECK_SERVERLIB
#check for edilib
MRJ_CHECK_EDILIB

MRJ_CHECK_GLIBCXX_DEBUG
#check for boost
AX_BOOST_BASE(1.33, , [AC_MSG_ERROR([not found Boost C++ libraries])])
#AX_BOOST_DATE_TIME
#AX_BOOST_REGEX
AC_PROG_RANLIB
CHECK_TCL
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

AC_OUTPUT(Makefile src/Makefile include/Makefile include/etick/Makefile)

mkdir -p lib

echo 
echo Configuration for $PACKAGE $VERSION :
echo --------------------------------
echo 
echo Source code location ....: $srcdir
echo Host System Type ........: $host
echo CFLAGS  .................: $CFLAGS
echo CXXFLAGS  ...............: $CXXFLAGS
if test x$enable_xp_testing = xyes; then
    echo XP_TESTING enabled
else
    echo XP_TESTING disabled
fi

